#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../src/database.hpp"
#include "../src/auth.hpp"

static std::shared_ptr<Database> make_test_db() {
    auto db = std::make_shared<Database>(":memory:");
    db->migrate();
    return db;
}

TEST_CASE("Monthly report calculation", "[database][reports]") {
    auto db = make_test_db();
    auto uid_opt = db->create_user("report@test.com", Auth::hash_password("pass1234"));
    REQUIRE(uid_opt.has_value());
    int uid = *uid_opt;

    // Setup: 1000 income, categories, transactions
    int food_id   = db->create_category(uid, "Food",      "#ef4444", "utensils");
    int travel_id = db->create_category(uid, "Travel",    "#3b82f6", "plane");

    // March 2026 transactions
    db->create_transaction(uid, "2026-03-01", 3000.0, "income",  std::nullopt, "Salary",   false);
    db->create_transaction(uid, "2026-03-05", 150.0,  "expense", food_id,      "Groceries",false);
    db->create_transaction(uid, "2026-03-10", 85.0,   "expense", food_id,      "Dinner",   false);
    db->create_transaction(uid, "2026-03-15", 500.0,  "expense", travel_id,    "Train",    false);

    // Set budgets
    db->upsert_budget(uid, "2026-03", food_id,   300.0);
    db->upsert_budget(uid, "2026-03", travel_id, 400.0);

    auto report = db->get_monthly_report(uid, "2026-03");

    SECTION("Total income and expense correct") {
        REQUIRE(report.total_income  == Catch::Approx(3000.0));
        REQUIRE(report.total_expense == Catch::Approx(735.0));
        REQUIRE(report.net_change    == Catch::Approx(2265.0));
    }

    SECTION("Per-category totals correct") {
        bool found_food = false, found_travel = false;
        for (const auto& cs : report.by_category) {
            if (cs.category_id == food_id) {
                found_food = true;
                REQUIRE(cs.spent    == Catch::Approx(235.0));
                REQUIRE(cs.budget   == Catch::Approx(300.0));
                REQUIRE(cs.status   == "ok");
                REQUIRE(cs.remaining == Catch::Approx(65.0));
            }
            if (cs.category_id == travel_id) {
                found_travel = true;
                REQUIRE(cs.spent  == Catch::Approx(500.0));
                REQUIRE(cs.budget == Catch::Approx(400.0));
                REQUIRE(cs.status == "over");
                REQUIRE(cs.over_by == Catch::Approx(100.0));
            }
        }
        REQUIRE(found_food);
        REQUIRE(found_travel);
    }

    SECTION("Top expenses populated") {
        REQUIRE(!report.top_expenses.empty());
        // Largest expense should be first
        REQUIRE(report.top_expenses[0].amount == Catch::Approx(500.0));
    }

    SECTION("Empty month returns zeros") {
        auto r = db->get_monthly_report(uid, "2025-01");
        REQUIRE(r.total_income  == Catch::Approx(0.0));
        REQUIRE(r.total_expense == Catch::Approx(0.0));
        REQUIRE(r.by_category.empty());
    }
}

TEST_CASE("Trend calculation", "[database][reports]") {
    auto db = make_test_db();
    auto uid_opt = db->create_user("trend@test.com", Auth::hash_password("pass1234"));
    int uid = *uid_opt;

    db->create_transaction(uid, "2026-01-10", 1000.0, "income",  std::nullopt, "", false);
    db->create_transaction(uid, "2026-01-15", 200.0,  "expense", std::nullopt, "", false);
    db->create_transaction(uid, "2026-02-10", 1200.0, "income",  std::nullopt, "", false);
    db->create_transaction(uid, "2026-02-20", 350.0,  "expense", std::nullopt, "", false);

    auto trend = db->get_trend(uid, 6);

    SECTION("Returns correct number of months with data") {
        REQUIRE(trend.size() == 2);
        REQUIRE(trend[0].year_month == "2026-01");
        REQUIRE(trend[0].income    == Catch::Approx(1000.0));
        REQUIRE(trend[0].expense   == Catch::Approx(200.0));
        REQUIRE(trend[1].year_month == "2026-02");
        REQUIRE(trend[1].income    == Catch::Approx(1200.0));
    }
}
