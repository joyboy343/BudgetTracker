#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../src/database.hpp"
#include "../src/auth.hpp"

// Use an in-memory SQLite DB for each test
static std::shared_ptr<Database> make_test_db() {
    auto db = std::make_shared<Database>(":memory:");
    db->migrate();
    return db;
}

static int make_user(Database& db, const std::string& email = "test@example.com") {
    auto id = db.create_user(email, Auth::hash_password("password123"));
    REQUIRE(id.has_value());
    return *id;
}

TEST_CASE("User creation and retrieval", "[database][users]") {
    auto db = make_test_db();

    SECTION("Create and find user by email") {
        auto id = db->create_user("alice@example.com", "hashxyz");
        REQUIRE(id.has_value());
        REQUIRE(*id > 0);

        auto user = db->get_user_by_email("alice@example.com");
        REQUIRE(user.has_value());
        REQUIRE(user->email == "alice@example.com");
        REQUIRE(user->id == *id);
    }

    SECTION("Duplicate email returns nullopt") {
        db->create_user("dup@example.com", "hash1");
        auto id2 = db->create_user("dup@example.com", "hash2");
        REQUIRE_FALSE(id2.has_value());
    }

    SECTION("Non-existent user returns nullopt") {
        auto user = db->get_user_by_email("nobody@example.com");
        REQUIRE_FALSE(user.has_value());
    }
}

TEST_CASE("Account balance", "[database][account]") {
    auto db = make_test_db();
    int uid = make_user(*db);

    SECTION("New user starts at 0") {
        REQUIRE(db->get_balance(uid) == Catch::Approx(0.0));
    }

    SECTION("Balance updates atomically with transaction") {
        db->create_transaction(uid, "2026-01-15", 500.0, "income", std::nullopt, "Salary", false);
        REQUIRE(db->get_balance(uid) == Catch::Approx(500.0));
    }

    SECTION("Expense reduces balance") {
        db->create_transaction(uid, "2026-01-15", 500.0, "income",  std::nullopt, "", false);
        db->create_transaction(uid, "2026-01-16", 100.0, "expense", std::nullopt, "", false);
        REQUIRE(db->get_balance(uid) == Catch::Approx(400.0));
    }

    SECTION("Overdraft throws BalanceError without force") {
        db->create_transaction(uid, "2026-01-15", 100.0, "income", std::nullopt, "", false);
        REQUIRE_THROWS_AS(
            db->create_transaction(uid, "2026-01-15", 200.0, "expense", std::nullopt, "", false),
            Database::BalanceError
        );
        REQUIRE(db->get_balance(uid) == Catch::Approx(100.0)); // unchanged
    }

    SECTION("Overdraft succeeds with force=true") {
        db->create_transaction(uid, "2026-01-15", 100.0, "income",  std::nullopt, "", false);
        REQUIRE_NOTHROW(
            db->create_transaction(uid, "2026-01-15", 200.0, "expense", std::nullopt, "", false, true)
        );
        REQUIRE(db->get_balance(uid) == Catch::Approx(-100.0));
    }
}

TEST_CASE("Transaction CRUD", "[database][transactions]") {
    auto db = make_test_db();
    int uid = make_user(*db);
    db->create_transaction(uid, "2026-01-01", 1000.0, "income", std::nullopt, "Initial", false);

    SECTION("Create and retrieve transaction") {
        int id = db->create_transaction(uid, "2026-01-10", 25.50, "expense", std::nullopt, "Coffee", false);
        REQUIRE(id > 0);
        auto txn = db->get_transaction(id, uid);
        REQUIRE(txn.has_value());
        REQUIRE(txn->amount == Catch::Approx(25.50));
        REQUIRE(txn->type   == "expense");
        REQUIRE(txn->note   == "Coffee");
        REQUIRE(txn->date   == "2026-01-10");
    }

    SECTION("Update transaction adjusts balance correctly") {
        int id = db->create_transaction(uid, "2026-01-10", 25.0, "expense", std::nullopt, "Old", false);
        double bal_after_create = db->get_balance(uid);

        db->update_transaction(id, uid, "2026-01-10", 50.0, "expense", std::nullopt, "New", false);
        REQUIRE(db->get_balance(uid) == Catch::Approx(bal_after_create - 25.0));

        auto txn = db->get_transaction(id, uid);
        REQUIRE(txn->amount == Catch::Approx(50.0));
        REQUIRE(txn->note   == "New");
    }

    SECTION("Delete transaction restores balance") {
        double bal_before = db->get_balance(uid);
        int id = db->create_transaction(uid, "2026-01-15", 75.0, "expense", std::nullopt, "", false);
        REQUIRE(db->get_balance(uid) == Catch::Approx(bal_before - 75.0));

        db->delete_transaction(id, uid);
        REQUIRE(db->get_balance(uid) == Catch::Approx(bal_before));
    }

    SECTION("Cannot access another user's transaction") {
        auto uid2 = db->create_user("other@test.com", "hash");
        int id = db->create_transaction(uid, "2026-01-10", 10.0, "expense", std::nullopt, "", false);
        auto txn = db->get_transaction(id, *uid2);
        REQUIRE_FALSE(txn.has_value());
    }

    SECTION("Undo last N transactions") {
        db->create_transaction(uid, "2026-01-20", 10.0, "expense", std::nullopt, "A", false);
        db->create_transaction(uid, "2026-01-21", 20.0, "expense", std::nullopt, "B", false);
        double bal = db->get_balance(uid);
        auto removed = db->undo_last_transactions(uid, 2);
        REQUIRE(removed.size() == 2);
        REQUIRE(db->get_balance(uid) == Catch::Approx(bal + 30.0));
    }
}

TEST_CASE("Transaction filtering and pagination", "[database][transactions]") {
    auto db = make_test_db();
    int uid = make_user(*db);
    db->create_transaction(uid, "2026-01-01", 2000.0, "income",  std::nullopt, "", false);

    db->create_transaction(uid, "2026-01-05", 50.0,  "expense", std::nullopt, "Jan",  false);
    db->create_transaction(uid, "2026-01-15", 80.0,  "expense", std::nullopt, "Jan",  false);
    db->create_transaction(uid, "2026-02-05", 120.0, "expense", std::nullopt, "Feb",  false);
    db->create_transaction(uid, "2026-03-05", 30.0,  "income",  std::nullopt, "Mar",  false);

    SECTION("Filter by date range") {
        Database::TxnFilter f;
        f.start_date = "2026-01-01";
        f.end_date   = "2026-01-31";
        auto result = db->list_transactions(uid, f);
        // 2 expenses + 1 income in January
        REQUIRE(result.total == 3);
    }

    SECTION("Filter by type") {
        Database::TxnFilter f;
        f.type = "expense";
        auto result = db->list_transactions(uid, f);
        REQUIRE(result.total == 3);
    }

    SECTION("Pagination") {
        Database::TxnFilter f;
        f.size = 2;
        f.page = 1;
        auto p1 = db->list_transactions(uid, f);
        REQUIRE(p1.items.size() == 2);
        REQUIRE(p1.total == 5); // all 5 transactions

        f.page = 2;
        auto p2 = db->list_transactions(uid, f);
        REQUIRE(p2.items.size() == 2);
    }
}

TEST_CASE("Categories", "[database][categories]") {
    auto db = make_test_db();
    int uid = make_user(*db);

    SECTION("Create and list categories") {
        db->create_category(uid, "Food",   "#ef4444", "utensils");
        db->create_category(uid, "Travel", "#3b82f6", "plane");
        auto cats = db->list_categories(uid);
        REQUIRE(cats.size() >= 2);
    }

    SECTION("Update category") {
        int id = db->create_category(uid, "Misc", "#666", "tag");
        bool ok = db->update_category(id, uid, "Miscellaneous", "#888", "folder");
        REQUIRE(ok);
        auto cat = db->get_category(id, uid);
        REQUIRE(cat->name == "Miscellaneous");
    }

    SECTION("Delete category") {
        int id = db->create_category(uid, "ToDelete", "#fff", "x");
        REQUIRE(db->delete_category(id, uid));
        REQUIRE_FALSE(db->get_category(id, uid).has_value());
    }
}

TEST_CASE("Budgets", "[database][budgets]") {
    auto db = make_test_db();
    int uid = make_user(*db);
    int cat_id = db->create_category(uid, "Food", "#ef4444", "utensils");

    SECTION("Create and retrieve budget") {
        db->upsert_budget(uid, "2026-01", cat_id, 300.0);
        auto budgets = db->list_budgets(uid, "2026-01");
        REQUIRE(budgets.size() == 1);
        REQUIRE(budgets[0].amount == Catch::Approx(300.0));
        REQUIRE(budgets[0].category_id == cat_id);
    }

    SECTION("Upsert updates existing budget") {
        db->upsert_budget(uid, "2026-01", cat_id, 300.0);
        db->upsert_budget(uid, "2026-01", cat_id, 450.0); // update
        auto budgets = db->list_budgets(uid, "2026-01");
        REQUIRE(budgets.size() == 1);
        REQUIRE(budgets[0].amount == Catch::Approx(450.0));
    }
}
