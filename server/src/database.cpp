#include "database.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

// ── Helpers
// ───────────────────────────────────────────────────────────────────

static std::string now_iso8601() {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#ifdef _WIN32
  gmtime_s(&tm, &t);
#else
  gmtime_r(&t, &tm);
#endif
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return buf;
}

static std::string opt_int_str(std::optional<int> v) {
  return v ? std::to_string(*v) : "";
}

// ── Constructor / Destructor
// ──────────────────────────────────────────────────

Database::Database(const std::string &db_path) {
  int rc = sqlite3_open(db_path.c_str(), &db_);
  if (rc != SQLITE_OK) {
    throw std::runtime_error(std::string("Cannot open DB: ") +
                             sqlite3_errmsg(db_));
  }
  exec("PRAGMA journal_mode=WAL;");
  exec("PRAGMA foreign_keys=ON;");
  exec("PRAGMA synchronous=NORMAL;");
}

Database::~Database() {
  if (db_)
    sqlite3_close(db_);
}

// ── Low-level helpers
// ─────────────────────────────────────────────────────────

void Database::exec(const std::string &sql) {
  char *err = nullptr;
  int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
  if (rc != SQLITE_OK) {
    std::string msg = err ? err : "unknown error";
    sqlite3_free(err);
    throw std::runtime_error("SQL error: " + msg + "\nSQL: " + sql);
  }
}

void Database::bind_params(sqlite3_stmt *stmt,
                           const std::vector<std::string> &params) {
  for (int i = 0; i < static_cast<int>(params.size()); ++i) {
    const auto &p = params[i];
    if (p.empty() && p != "0") {
      sqlite3_bind_null(stmt, i + 1);
    } else {
      sqlite3_bind_text(stmt, i + 1, p.c_str(), -1, SQLITE_TRANSIENT);
    }
  }
}

void Database::query(const std::string &sql,
                     const std::vector<std::string> &params, RowCallback cb) {
  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
    throw std::runtime_error(std::string("Prepare failed: ") +
                             sqlite3_errmsg(db_));

  bind_params(stmt, params);

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    cb(stmt);
  }
  sqlite3_finalize(stmt);
  if (rc != SQLITE_DONE)
    throw std::runtime_error(std::string("Query step failed: ") +
                             sqlite3_errmsg(db_));
}

int Database::exec_and_get_id(const std::string &sql,
                              const std::vector<std::string> &params) {
  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
    throw std::runtime_error(std::string("Prepare failed: ") +
                             sqlite3_errmsg(db_));

  bind_params(stmt, params);
  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE)
    throw std::runtime_error(std::string("Exec failed: ") +
                             sqlite3_errmsg(db_));

  return static_cast<int>(sqlite3_last_insert_rowid(db_));
}

// ── Migrations
// ────────────────────────────────────────────────────────────────

void Database::migrate() {
  exec(R"(
        CREATE TABLE IF NOT EXISTS schema_migrations (
            version INTEGER PRIMARY KEY,
            applied_at TEXT NOT NULL
        );
    )");

  auto already_applied = [&](int v) {
    bool found = false;
    query("SELECT 1 FROM schema_migrations WHERE version=?",
          {std::to_string(v)}, [&](sqlite3_stmt *) { found = true; });
    return found;
  };

  auto apply = [&](int v, const std::string &sql) {
    if (already_applied(v))
      return;
    exec("BEGIN;");
    try {
      exec(sql);
      exec("INSERT INTO schema_migrations(version,applied_at) VALUES(" +
           std::to_string(v) + ",'" + now_iso8601() + "');");
      exec("COMMIT;");
      std::cout << "[DB] Applied migration v" << v << "\n";
    } catch (...) {
      exec("ROLLBACK;");
      throw;
    }
  };

  apply(1, R"(
        CREATE TABLE IF NOT EXISTS users (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            email        TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            created_at   TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS categories (
            id      INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER,
            name    TEXT NOT NULL,
            color   TEXT NOT NULL DEFAULT '#6366f1',
            icon    TEXT NOT NULL DEFAULT 'tag',
            UNIQUE(user_id, name),
            FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
        );
        CREATE TABLE IF NOT EXISTS budgets (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id     INTEGER NOT NULL,
            year_month  TEXT NOT NULL,
            category_id INTEGER NOT NULL,
            amount      REAL NOT NULL,
            FOREIGN KEY(user_id)     REFERENCES users(id)       ON DELETE CASCADE,
            FOREIGN KEY(category_id) REFERENCES categories(id)  ON DELETE CASCADE,
            UNIQUE(user_id, year_month, category_id)
        );
        CREATE TABLE IF NOT EXISTS transactions (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id     INTEGER NOT NULL,
            date        TEXT NOT NULL,
            amount      REAL NOT NULL,
            type        TEXT NOT NULL CHECK(type IN ('expense','income')),
            category_id INTEGER,
            note        TEXT DEFAULT '',
            recurring   INTEGER DEFAULT 0,
            created_at  TEXT NOT NULL,
            FOREIGN KEY(user_id)     REFERENCES users(id)       ON DELETE CASCADE,
            FOREIGN KEY(category_id) REFERENCES categories(id)  ON DELETE SET NULL
        );
        CREATE TABLE IF NOT EXISTS accounts (
            user_id INTEGER PRIMARY KEY,
            balance REAL NOT NULL DEFAULT 0,
            FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
        );
        CREATE TABLE IF NOT EXISTS refresh_tokens (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id    INTEGER NOT NULL,
            token_hash TEXT NOT NULL,
            expires_at TEXT NOT NULL,
            FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
        );
        CREATE INDEX IF NOT EXISTS idx_txn_user_date ON transactions(user_id, date);
        CREATE INDEX IF NOT EXISTS idx_txn_category  ON transactions(category_id);
        CREATE INDEX IF NOT EXISTS idx_budget_user   ON budgets(user_id, year_month);
    )");
}

// ── Users
// ─────────────────────────────────────────────────────────────────────

std::optional<int> Database::create_user(const std::string &email,
                                         const std::string &hash) {
  try {
    int id = exec_and_get_id(
        "INSERT INTO users(email,password_hash,created_at) VALUES(?,?,?)",
        {email, hash, now_iso8601()});
    ensure_account(id);
    return id;
  } catch (...) {
    return std::nullopt; // email already exists
  }
}

std::optional<User> Database::get_user_by_email(const std::string &email) {
  std::optional<User> result;
  query("SELECT id,email,password_hash,created_at FROM users WHERE email=?",
        {email}, [&](sqlite3_stmt *s) {
          User u;
          u.id = sqlite3_column_int(s, 0);
          u.email = reinterpret_cast<const char *>(sqlite3_column_text(s, 1));
          u.password_hash =
              reinterpret_cast<const char *>(sqlite3_column_text(s, 2));
          u.created_at =
              reinterpret_cast<const char *>(sqlite3_column_text(s, 3));
          result = u;
        });
  return result;
}

std::optional<User> Database::get_user_by_id(int id) {
  std::optional<User> result;
  query("SELECT id,email,password_hash,created_at FROM users WHERE id=?",
        {std::to_string(id)}, [&](sqlite3_stmt *s) {
          User u;
          u.id = sqlite3_column_int(s, 0);
          u.email = reinterpret_cast<const char *>(sqlite3_column_text(s, 1));
          u.password_hash =
              reinterpret_cast<const char *>(sqlite3_column_text(s, 2));
          u.created_at =
              reinterpret_cast<const char *>(sqlite3_column_text(s, 3));
          result = u;
        });
  return result;
}

// ── Account
// ───────────────────────────────────────────────────────────────────

void Database::ensure_account(int user_id) {
  exec("INSERT OR IGNORE INTO accounts(user_id,balance) VALUES(" +
       std::to_string(user_id) + ",0);");
}

double Database::get_balance(int user_id) {
  double bal = 0;
  query("SELECT balance FROM accounts WHERE user_id=?",
        {std::to_string(user_id)},
        [&](sqlite3_stmt *s) { bal = sqlite3_column_double(s, 0); });
  return bal;
}

void Database::update_balance_in_txn(sqlite3 * /*db*/, int user_id,
                                     double delta) {
  // Called inside a BEGIN..COMMIT block; db_ is already in use
  sqlite3_stmt *stmt = nullptr;
  std::string sql = "UPDATE accounts SET balance=balance+? WHERE user_id=?";
  sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
  sqlite3_bind_double(stmt, 1, delta);
  sqlite3_bind_int(stmt, 2, user_id);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
}

// ── Categories
// ────────────────────────────────────────────────────────────────

std::vector<Category> Database::list_categories(int user_id) {
  std::vector<Category> cats;
  // Return global (user_id IS NULL) + user-specific categories
  query("SELECT id,user_id,name,color,icon FROM categories "
        "WHERE user_id IS NULL OR user_id=? ORDER BY name",
        {std::to_string(user_id)}, [&](sqlite3_stmt *s) {
          Category c;
          c.id = sqlite3_column_int(s, 0);
          if (sqlite3_column_type(s, 1) != SQLITE_NULL)
            c.user_id = sqlite3_column_int(s, 1);
          c.name = reinterpret_cast<const char *>(sqlite3_column_text(s, 2));
          c.color = reinterpret_cast<const char *>(sqlite3_column_text(s, 3));
          c.icon = reinterpret_cast<const char *>(sqlite3_column_text(s, 4));
          cats.push_back(c);
        });
  return cats;
}

std::optional<Category> Database::get_category(int id, int user_id) {
  std::optional<Category> result;
  query("SELECT id,user_id,name,color,icon FROM categories "
        "WHERE id=? AND (user_id IS NULL OR user_id=?)",
        {std::to_string(id), std::to_string(user_id)}, [&](sqlite3_stmt *s) {
          Category c;
          c.id = sqlite3_column_int(s, 0);
          if (sqlite3_column_type(s, 1) != SQLITE_NULL)
            c.user_id = sqlite3_column_int(s, 1);
          c.name = reinterpret_cast<const char *>(sqlite3_column_text(s, 2));
          c.color = reinterpret_cast<const char *>(sqlite3_column_text(s, 3));
          c.icon = reinterpret_cast<const char *>(sqlite3_column_text(s, 4));
          result = c;
        });
  return result;
}

int Database::create_category(int user_id, const std::string &name,
                              const std::string &color,
                              const std::string &icon) {
  return exec_and_get_id(
      "INSERT INTO categories(user_id,name,color,icon) VALUES(?,?,?,?)",
      {std::to_string(user_id), name, color, icon});
}

bool Database::update_category(int id, int user_id, const std::string &name,
                               const std::string &color,
                               const std::string &icon) {
  sqlite3_stmt *stmt = nullptr;
  sqlite3_prepare_v2(
      db_,
      "UPDATE categories SET name=?,color=?,icon=? WHERE id=? AND user_id=?",
      -1, &stmt, nullptr);
  sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, color.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, icon.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 4, id);
  sqlite3_bind_int(stmt, 5, user_id);
  sqlite3_step(stmt);
  int changes = sqlite3_changes(db_);
  sqlite3_finalize(stmt);
  return changes > 0;
}

bool Database::delete_category(int id, int user_id) {
  sqlite3_stmt *stmt = nullptr;
  sqlite3_prepare_v2(db_, "DELETE FROM categories WHERE id=? AND user_id=?", -1,
                     &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, id);
  sqlite3_bind_int(stmt, 2, user_id);
  sqlite3_step(stmt);
  int changes = sqlite3_changes(db_);
  sqlite3_finalize(stmt);
  return changes > 0;
}

// ── Transactions
// ──────────────────────────────────────────────────────────────

static Transaction row_to_txn(sqlite3_stmt *s) {
  Transaction t;
  t.id = sqlite3_column_int(s, 0);
  t.user_id = sqlite3_column_int(s, 1);
  t.date = reinterpret_cast<const char *>(sqlite3_column_text(s, 2));
  t.amount = sqlite3_column_double(s, 3);
  t.type = reinterpret_cast<const char *>(sqlite3_column_text(s, 4));
  if (sqlite3_column_type(s, 5) != SQLITE_NULL)
    t.category_id = sqlite3_column_int(s, 5);
  t.note = sqlite3_column_type(s, 6) != SQLITE_NULL
               ? reinterpret_cast<const char *>(sqlite3_column_text(s, 6))
               : "";
  t.recurring = sqlite3_column_int(s, 7) != 0;
  t.created_at = reinterpret_cast<const char *>(sqlite3_column_text(s, 8));
  t.category_name =
      sqlite3_column_type(s, 9) != SQLITE_NULL
          ? reinterpret_cast<const char *>(sqlite3_column_text(s, 9))
          : "";
  t.category_color =
      sqlite3_column_type(s, 10) != SQLITE_NULL
          ? reinterpret_cast<const char *>(sqlite3_column_text(s, 10))
          : "#94a3b8";
  return t;
}

Database::TxnPage Database::list_transactions(int user_id, const TxnFilter &f) {
  TxnPage page;
  page.page = f.page;
  page.size = f.size;

  std::string where = "WHERE t.user_id=?";
  std::vector<std::string> params = {std::to_string(user_id)};

  if (!f.start_date.empty()) {
    where += " AND t.date>=?";
    params.push_back(f.start_date);
  }
  if (!f.end_date.empty()) {
    where += " AND t.date<=?";
    params.push_back(f.end_date);
  }
  if (f.category_id) {
    where += " AND t.category_id=?";
    params.push_back(std::to_string(*f.category_id));
  }
  if (!f.type.empty()) {
    where += " AND t.type=?";
    params.push_back(f.type);
  }

  // Count total
  query("SELECT COUNT(*) FROM transactions t " + where, params,
        [&](sqlite3_stmt *s) { page.total = sqlite3_column_int(s, 0); });

  // Fetch page
  int offset = (f.page - 1) * f.size;
  params.push_back(std::to_string(f.size));
  params.push_back(std::to_string(offset));

  std::string sql = R"(
        SELECT t.id,t.user_id,t.date,t.amount,t.type,t.category_id,
               t.note,t.recurring,t.created_at,
               c.name,c.color
        FROM transactions t
        LEFT JOIN categories c ON c.id=t.category_id
    )" + where + " ORDER BY t.date DESC, t.id DESC LIMIT ? OFFSET ?";

  query(sql, params,
        [&](sqlite3_stmt *s) { page.items.push_back(row_to_txn(s)); });
  return page;
}

std::optional<Transaction> Database::get_transaction(int id, int user_id) {
  std::optional<Transaction> result;
  query(R"(SELECT t.id,t.user_id,t.date,t.amount,t.type,t.category_id,
                    t.note,t.recurring,t.created_at,c.name,c.color
             FROM transactions t
             LEFT JOIN categories c ON c.id=t.category_id
             WHERE t.id=? AND t.user_id=?)",
        {std::to_string(id), std::to_string(user_id)},
        [&](sqlite3_stmt *s) { result = row_to_txn(s); });
  return result;
}

int Database::create_transaction(int user_id, const std::string &date,
                                 double amount, const std::string &type,
                                 std::optional<int> category_id,
                                 const std::string &note, bool recurring,
                                 bool force) {
  if (amount <= 0)
    throw std::invalid_argument("Amount must be positive");
  if (type != "expense" && type != "income")
    throw std::invalid_argument("Type must be 'expense' or 'income'");

  double delta = (type == "income") ? amount : -amount;

  // Check balance for expenses
  if (!force && delta < 0) {
    double bal = get_balance(user_id);
    if (bal + delta < 0)
      throw BalanceError(bal, amount);
  }

  exec("BEGIN;");
  try {
    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db_,
                       "INSERT INTO "
                       "transactions(user_id,date,amount,type,category_id,note,"
                       "recurring,created_at) "
                       "VALUES(?,?,?,?,?,?,?,?)",
                       -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, amount);
    sqlite3_bind_text(stmt, 4, type.c_str(), -1, SQLITE_TRANSIENT);
    if (category_id)
      sqlite3_bind_int(stmt, 5, *category_id);
    else
      sqlite3_bind_null(stmt, 5);
    sqlite3_bind_text(stmt, 6, note.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, recurring ? 1 : 0);
    sqlite3_bind_text(stmt, 8, now_iso8601().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    int new_id = static_cast<int>(sqlite3_last_insert_rowid(db_));
    sqlite3_finalize(stmt);

    update_balance_in_txn(db_, user_id, delta);
    exec("COMMIT;");
    return new_id;
  } catch (...) {
    exec("ROLLBACK;");
    throw;
  }
}

bool Database::update_transaction(int id, int user_id, const std::string &date,
                                  double amount, const std::string &type,
                                  std::optional<int> category_id,
                                  const std::string &note, bool recurring) {
  auto old = get_transaction(id, user_id);
  if (!old)
    return false;
  if (amount <= 0)
    throw std::invalid_argument("Amount must be positive");
  if (type != "expense" && type != "income")
    throw std::invalid_argument("Type must be 'expense' or 'income'");

  // Compute balance delta: reverse old, apply new
  double old_delta = (old->type == "income") ? old->amount : -old->amount;
  double new_delta = (type == "income") ? amount : -amount;
  double net_delta = new_delta - old_delta;

  exec("BEGIN;");
  try {
    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(
        db_,
        "UPDATE transactions SET "
        "date=?,amount=?,type=?,category_id=?,note=?,recurring=? "
        "WHERE id=? AND user_id=?",
        -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, amount);
    sqlite3_bind_text(stmt, 3, type.c_str(), -1, SQLITE_TRANSIENT);
    if (category_id)
      sqlite3_bind_int(stmt, 4, *category_id);
    else
      sqlite3_bind_null(stmt, 4);
    sqlite3_bind_text(stmt, 5, note.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, recurring ? 1 : 0);
    sqlite3_bind_int(stmt, 7, id);
    sqlite3_bind_int(stmt, 8, user_id);
    sqlite3_step(stmt);
    int changes = sqlite3_changes(db_);
    sqlite3_finalize(stmt);

    if (changes > 0)
      update_balance_in_txn(db_, user_id, net_delta);
    exec("COMMIT;");
    return changes > 0;
  } catch (...) {
    exec("ROLLBACK;");
    throw;
  }
}

bool Database::delete_transaction(int id, int user_id) {
  auto txn = get_transaction(id, user_id);
  if (!txn)
    return false;

  double delta = (txn->type == "income") ? -txn->amount : txn->amount;

  exec("BEGIN;");
  try {
    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db_, "DELETE FROM transactions WHERE id=? AND user_id=?",
                       -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_step(stmt);
    int changes = sqlite3_changes(db_);
    sqlite3_finalize(stmt);

    if (changes > 0)
      update_balance_in_txn(db_, user_id, delta);
    exec("COMMIT;");
    return changes > 0;
  } catch (...) {
    exec("ROLLBACK;");
    throw;
  }
}

std::vector<int> Database::undo_last_transactions(int user_id, int n) {
  std::vector<int> removed_ids;
  // Get the last n transaction IDs for this user
  query("SELECT id FROM transactions WHERE user_id=? ORDER BY created_at DESC, "
        "id DESC LIMIT ?",
        {std::to_string(user_id), std::to_string(n)}, [&](sqlite3_stmt *s) {
          removed_ids.push_back(sqlite3_column_int(s, 0));
        });

  for (int id : removed_ids)
    delete_transaction(id, user_id);
  return removed_ids;
}

// ── Budgets
// ───────────────────────────────────────────────────────────────────

std::vector<Budget> Database::list_budgets(int user_id,
                                           const std::string &year_month) {
  std::vector<Budget> budgets;
  std::string sql = R"(
        SELECT b.id,b.user_id,b.year_month,b.category_id,b.amount,c.name,c.color
        FROM budgets b
        JOIN categories c ON c.id=b.category_id
        WHERE b.user_id=?
    )";
  std::vector<std::string> params = {std::to_string(user_id)};
  if (!year_month.empty()) {
    sql += " AND b.year_month=?";
    params.push_back(year_month);
  }
  sql += " ORDER BY c.name";

  query(sql, params, [&](sqlite3_stmt *s) {
    Budget b;
    b.id = sqlite3_column_int(s, 0);
    b.user_id = sqlite3_column_int(s, 1);
    b.year_month = reinterpret_cast<const char *>(sqlite3_column_text(s, 2));
    b.category_id = sqlite3_column_int(s, 3);
    b.amount = sqlite3_column_double(s, 4);
    b.category_name = reinterpret_cast<const char *>(sqlite3_column_text(s, 5));
    b.category_color =
        reinterpret_cast<const char *>(sqlite3_column_text(s, 6));
    budgets.push_back(b);
  });
  return budgets;
}

int Database::upsert_budget(int user_id, const std::string &year_month,
                            int category_id, double amount) {
  return exec_and_get_id(
      "INSERT INTO budgets(user_id,year_month,category_id,amount) "
      "VALUES(?,?,?,?) "
      "ON CONFLICT(user_id,year_month,category_id) DO UPDATE SET "
      "amount=excluded.amount",
      {std::to_string(user_id), year_month, std::to_string(category_id),
       std::to_string(amount)});
}

bool Database::delete_budget(int id, int user_id) {
  sqlite3_stmt *stmt = nullptr;
  sqlite3_prepare_v2(db_, "DELETE FROM budgets WHERE id=? AND user_id=?", -1,
                     &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, id);
  sqlite3_bind_int(stmt, 2, user_id);
  sqlite3_step(stmt);
  int changes = sqlite3_changes(db_);
  sqlite3_finalize(stmt);
  return changes > 0;
}

// ── Reports
// ───────────────────────────────────────────────────────────────────

MonthlyReport Database::get_monthly_report(int user_id,
                                           const std::string &year_month) {
  MonthlyReport r;
  r.year_month = year_month;

  std::string start = year_month + "-01";
  std::string end = year_month + "-31";

  // Total income / expense
  query(R"(SELECT type, SUM(amount) FROM transactions
             WHERE user_id=? AND date>=? AND date<=? GROUP BY type)",
        {std::to_string(user_id), start, end}, [&](sqlite3_stmt *s) {
          std::string type =
              reinterpret_cast<const char *>(sqlite3_column_text(s, 0));
          double amt = sqlite3_column_double(s, 1);
          if (type == "income")
            r.total_income = amt;
          if (type == "expense")
            r.total_expense = amt;
        });

  r.net_change = r.total_income - r.total_expense;
  r.ending_balance = get_balance(user_id);

  // Per-category totals
  query(R"(SELECT c.id, c.name, c.color,
                    SUM(CASE WHEN t.type='expense' THEN t.amount ELSE 0 END) as spent,
                    SUM(CASE WHEN t.type='income'  THEN t.amount ELSE 0 END) as income
             FROM transactions t
             JOIN categories c ON c.id=t.category_id
             WHERE t.user_id=? AND t.date>=? AND t.date<=?
             GROUP BY c.id, c.name, c.color
             ORDER BY spent DESC)",
        {std::to_string(user_id), start, end}, [&](sqlite3_stmt *s) {
          CategorySummary cs;
          cs.category_id = sqlite3_column_int(s, 0);
          cs.category =
              reinterpret_cast<const char *>(sqlite3_column_text(s, 1));
          cs.color = reinterpret_cast<const char *>(sqlite3_column_text(s, 2));
          cs.spent = sqlite3_column_double(s, 3);
          cs.income = sqlite3_column_double(s, 4);
          r.by_category.push_back(cs);
        });

  // Attach budgets
  auto budgets = list_budgets(user_id, year_month);
  for (auto &cs : r.by_category) {
    for (const auto &b : budgets) {
      if (b.category_id == cs.category_id) {
        cs.budget = b.amount;
        break;
      }
    }
    if (cs.budget > 0) {
      cs.over_by = std::max(0.0, cs.spent - cs.budget);
      cs.remaining = std::max(0.0, cs.budget - cs.spent);
      cs.status = cs.spent > cs.budget ? "over" : "ok";
    } else {
      cs.status = "no_budget";
    }
  }

  // Top 5 expenses
  query(R"(SELECT t.id,t.user_id,t.date,t.amount,t.type,t.category_id,
                    t.note,t.recurring,t.created_at,c.name,c.color
             FROM transactions t
             LEFT JOIN categories c ON c.id=t.category_id
             WHERE t.user_id=? AND t.date>=? AND t.date<=? AND t.type='expense'
             ORDER BY t.amount DESC LIMIT 5)",
        {std::to_string(user_id), start, end},
        [&](sqlite3_stmt *s) { r.top_expenses.push_back(row_to_txn(s)); });

  return r;
}

std::vector<MonthlyTrend> Database::get_trend(int user_id, int months) {
  std::vector<MonthlyTrend> trend;
  query(R"(SELECT substr(date,1,7) as ym,
                    SUM(CASE WHEN type='income'  THEN amount ELSE 0 END),
                    SUM(CASE WHEN type='expense' THEN amount ELSE 0 END)
             FROM transactions WHERE user_id=?
             GROUP BY ym ORDER BY ym DESC LIMIT ?)",
        {std::to_string(user_id), std::to_string(months)},
        [&](sqlite3_stmt *s) {
          MonthlyTrend t;
          t.year_month =
              reinterpret_cast<const char *>(sqlite3_column_text(s, 0));
          t.income = sqlite3_column_double(s, 1);
          t.expense = sqlite3_column_double(s, 2);
          trend.push_back(t);
        });
  // Return in chronological order
  std::reverse(trend.begin(), trend.end());
  return trend;
}
