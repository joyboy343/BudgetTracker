#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <optional>
#include <memory>
#include "models.hpp"

class Database {
public:
    explicit Database(const std::string& db_path);
    ~Database();

    // Run all pending migrations at startup
    void migrate();

    // ── Users ─────────────────────────────────────────────────────────────
    std::optional<int>  create_user(const std::string& email, const std::string& hash);
    std::optional<User> get_user_by_email(const std::string& email);
    std::optional<User> get_user_by_id(int id);

    // ── Account ───────────────────────────────────────────────────────────
    double get_balance(int user_id);
    void   ensure_account(int user_id);

    // ── Categories ────────────────────────────────────────────────────────
    std::vector<Category>   list_categories(int user_id);
    std::optional<Category> get_category(int id, int user_id);
    int                     create_category(int user_id, const std::string& name,
                                            const std::string& color, const std::string& icon);
    bool                    update_category(int id, int user_id, const std::string& name,
                                            const std::string& color, const std::string& icon);
    bool                    delete_category(int id, int user_id);

    // ── Transactions ──────────────────────────────────────────────────────
    struct TxnFilter {
        std::string start_date;
        std::string end_date;
        std::optional<int> category_id;
        std::string type;       // "" | "expense" | "income"
        int page = 1;
        int size = 50;
    };

    struct TxnPage {
        std::vector<Transaction> items;
        int total = 0;
        int page  = 1;
        int size  = 50;
    };

    TxnPage                    list_transactions(int user_id, const TxnFilter& f);
    std::optional<Transaction> get_transaction(int id, int user_id);
    int                        create_transaction(int user_id, const std::string& date,
                                                  double amount, const std::string& type,
                                                  std::optional<int> category_id,
                                                  const std::string& note, bool recurring,
                                                  bool force = false);
    bool                       update_transaction(int id, int user_id,
                                                  const std::string& date, double amount,
                                                  const std::string& type,
                                                  std::optional<int> category_id,
                                                  const std::string& note, bool recurring);
    bool                       delete_transaction(int id, int user_id);
    std::vector<int>           undo_last_transactions(int user_id, int n);

    // ── Budgets ───────────────────────────────────────────────────────────
    std::vector<Budget>   list_budgets(int user_id, const std::string& year_month);
    std::optional<Budget> get_budget(int id, int user_id);
    int                   upsert_budget(int user_id, const std::string& year_month,
                                        int category_id, double amount);
    bool                  delete_budget(int id, int user_id);

    // ── Reports ───────────────────────────────────────────────────────────
    MonthlyReport         get_monthly_report(int user_id, const std::string& year_month);
    std::vector<MonthlyTrend> get_trend(int user_id, int months);

    // ── Error type for balance underflow ─────────────────────────────────
    struct BalanceError : std::runtime_error {
        double current_balance;
        double required;
        BalanceError(double bal, double req)
            : std::runtime_error("Insufficient balance"),
              current_balance(bal), required(req) {}
    };

private:
    sqlite3* db_ = nullptr;

    // Low-level helpers
    void exec(const std::string& sql);
    int  exec_and_get_id(const std::string& sql, const std::vector<std::string>& params);

    using RowCallback = std::function<void(sqlite3_stmt*)>;
    void query(const std::string& sql,
               const std::vector<std::string>& params,
               RowCallback cb);

    static void bind_params(sqlite3_stmt* stmt, const std::vector<std::string>& params);
    void update_balance_in_txn(sqlite3* db, int user_id, double delta);
};
