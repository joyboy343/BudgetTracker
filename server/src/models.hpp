#pragma once
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

// ── User ──────────────────────────────────────────────────────────────────────
struct User {
    int         id         = 0;
    std::string email;
    std::string password_hash;
    std::string created_at;
};

// ── Category ──────────────────────────────────────────────────────────────────
struct Category {
    int                  id      = 0;
    std::optional<int>   user_id;
    std::string          name;
    std::string          color;  // hex color for UI
    std::string          icon;   // lucide icon name

    nlohmann::json to_json() const {
        nlohmann::json j;
        j["id"]   = id;
        j["name"] = name;
        j["color"] = color;
        j["icon"]  = icon;
        if (user_id) j["user_id"] = *user_id;
        else         j["user_id"] = nullptr;
        return j;
    }
};

// ── Transaction ───────────────────────────────────────────────────────────────
struct Transaction {
    int                  id          = 0;
    int                  user_id     = 0;
    std::string          date;          // YYYY-MM-DD
    double               amount      = 0.0;
    std::string          type;          // "expense" | "income"
    std::optional<int>   category_id;
    std::string          category_name; // joined from categories
    std::string          category_color;
    std::string          note;
    bool                 recurring   = false;
    std::string          created_at;

    nlohmann::json to_json() const {
        nlohmann::json j;
        j["id"]             = id;
        j["date"]           = date;
        j["amount"]         = amount;
        j["type"]           = type;
        j["note"]           = note;
        j["recurring"]      = recurring;
        j["created_at"]     = created_at;
        j["category_name"]  = category_name;
        j["category_color"] = category_color;
        if (category_id) j["category_id"] = *category_id;
        else             j["category_id"] = nullptr;
        return j;
    }
};

// ── Budget ────────────────────────────────────────────────────────────────────
struct Budget {
    int         id          = 0;
    int         user_id     = 0;
    std::string year_month;    // YYYY-MM
    int         category_id = 0;
    std::string category_name;
    std::string category_color;
    double      amount      = 0.0;

    nlohmann::json to_json() const {
        nlohmann::json j;
        j["id"]             = id;
        j["year_month"]     = year_month;
        j["category_id"]    = category_id;
        j["category_name"]  = category_name;
        j["category_color"] = category_color;
        j["amount"]         = amount;
        return j;
    }
};

// ── Account ───────────────────────────────────────────────────────────────────
struct Account {
    int    user_id = 0;
    double balance = 0.0;
};

// ── Report ────────────────────────────────────────────────────────────────────
struct CategorySummary {
    int         category_id  = 0;
    std::string category     ;
    std::string color        ;
    double      budget       = 0.0;
    double      spent        = 0.0;
    double      income       = 0.0;
    std::string status       ;    // "ok" | "over" | "no_budget"
    double      over_by      = 0.0;
    double      remaining    = 0.0;
};

struct MonthlyReport {
    std::string                  year_month;
    double                       total_income    = 0.0;
    double                       total_expense   = 0.0;
    double                       net_change      = 0.0;
    double                       ending_balance  = 0.0;
    std::vector<CategorySummary> by_category;
    std::vector<Transaction>     top_expenses;
};

struct MonthlyTrend {
    std::string year_month;
    double      income  = 0.0;
    double      expense = 0.0;
};

// ── Error response helper ─────────────────────────────────────────────────────
inline nlohmann::json make_error(int code, const std::string& msg,
                                  const nlohmann::json& details = nullptr) {
    nlohmann::json j;
    j["error"]   = msg;
    j["code"]    = code;
    if (!details.is_null()) j["details"] = details;
    return j;
}

inline nlohmann::json make_ok(const std::string& msg = "ok") {
    return {{"message", msg}};
}
