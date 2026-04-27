#include "other_handlers.hpp"
#include "../models.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

// ── Category Handler ──────────────────────────────────────────────────────────

CategoryHandler::CategoryHandler(std::shared_ptr<Database> db, const Config& cfg)
    : db_(db), cfg_(cfg) {}

void CategoryHandler::handle_list(const httplib::Request&,
                                   httplib::Response& res, int user_id) {
    auto cats = db_->list_categories(user_id);
    json arr = json::array();
    for (const auto& c : cats) arr.push_back(c.to_json());
    res.set_content(arr.dump(), "application/json");
}

void CategoryHandler::handle_create(const httplib::Request& req,
                                     httplib::Response& res, int user_id) {
    json body;
    try { body = json::parse(req.body); }
    catch (...) {
        res.status = 400;
        res.set_content(make_error(400, "Invalid JSON").dump(), "application/json");
        return;
    }

    std::string name  = body.value("name",  "");
    std::string color = body.value("color", "#6366f1");
    std::string icon  = body.value("icon",  "tag");

    if (name.empty() || name.size() > 64) {
        res.status = 400;
        res.set_content(make_error(400, "Name must be 1–64 characters").dump(), "application/json");
        return;
    }

    try {
        int id = db_->create_category(user_id, name, color, icon);
        auto cat = db_->get_category(id, user_id);
        res.status = 201;
        res.set_content(cat->to_json().dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 409;
        res.set_content(make_error(409, "Category name already exists").dump(), "application/json");
    }
}

void CategoryHandler::handle_update(const httplib::Request& req,
                                     httplib::Response& res, int user_id) {
    int id = std::stoi(req.path_params.at("id"));
    json body;
    try { body = json::parse(req.body); }
    catch (...) {
        res.status = 400;
        res.set_content(make_error(400, "Invalid JSON").dump(), "application/json");
        return;
    }

    auto existing = db_->get_category(id, user_id);
    if (!existing) {
        res.status = 404;
        res.set_content(make_error(404, "Category not found").dump(), "application/json");
        return;
    }

    std::string name  = body.value("name",  existing->name);
    std::string color = body.value("color", existing->color);
    std::string icon  = body.value("icon",  existing->icon);

    bool ok = db_->update_category(id, user_id, name, color, icon);
    if (!ok) {
        res.status = 404;
        res.set_content(make_error(404, "Category not found or not editable").dump(), "application/json");
        return;
    }
    auto updated = db_->get_category(id, user_id);
    res.set_content(updated->to_json().dump(), "application/json");
}

void CategoryHandler::handle_delete(const httplib::Request& req,
                                     httplib::Response& res, int user_id) {
    int id = std::stoi(req.path_params.at("id"));
    bool ok = db_->delete_category(id, user_id);
    if (!ok) {
        res.status = 404;
        res.set_content(make_error(404, "Category not found or not deletable").dump(), "application/json");
        return;
    }
    res.set_content(make_ok("Category deleted").dump(), "application/json");
}

// ── Budget Handler ────────────────────────────────────────────────────────────

BudgetHandler::BudgetHandler(std::shared_ptr<Database> db, const Config& cfg)
    : db_(db), cfg_(cfg) {}

void BudgetHandler::handle_list(const httplib::Request& req,
                                 httplib::Response& res, int user_id) {
    std::string ym = req.get_param_value("year_month");
    auto budgets = db_->list_budgets(user_id, ym);
    json arr = json::array();
    for (const auto& b : budgets) arr.push_back(b.to_json());
    res.set_content(arr.dump(), "application/json");
}

void BudgetHandler::handle_upsert(const httplib::Request& req,
                                   httplib::Response& res, int user_id) {
    json body;
    try { body = json::parse(req.body); }
    catch (...) {
        res.status = 400;
        res.set_content(make_error(400, "Invalid JSON").dump(), "application/json");
        return;
    }

    std::string ym  = body.value("year_month",  "");
    int cat_id      = body.value("category_id", 0);
    double amount   = body.value("amount",      0.0);

    // Validate YYYY-MM
    if (ym.size() != 7 || ym[4] != '-') {
        res.status = 400;
        res.set_content(make_error(400, "year_month must be YYYY-MM").dump(), "application/json");
        return;
    }
    if (cat_id <= 0) {
        res.status = 400;
        res.set_content(make_error(400, "Valid category_id required").dump(), "application/json");
        return;
    }
    if (amount <= 0) {
        res.status = 400;
        res.set_content(make_error(400, "Amount must be positive").dump(), "application/json");
        return;
    }

    try {
        int id = db_->upsert_budget(user_id, ym, cat_id, amount);
        auto budgets = db_->list_budgets(user_id, ym);
        for (const auto& b : budgets) {
            if (b.id == id || (b.year_month == ym && b.category_id == cat_id)) {
                res.status = 201;
                res.set_content(b.to_json().dump(), "application/json");
                return;
            }
        }
        res.status = 201;
        res.set_content(make_ok("Budget set").dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(make_error(400, e.what()).dump(), "application/json");
    }
}

void BudgetHandler::handle_delete(const httplib::Request& req,
                                   httplib::Response& res, int user_id) {
    int id = std::stoi(req.path_params.at("id"));
    bool ok = db_->delete_budget(id, user_id);
    if (!ok) {
        res.status = 404;
        res.set_content(make_error(404, "Budget not found").dump(), "application/json");
        return;
    }
    res.set_content(make_ok("Budget deleted").dump(), "application/json");
}

// ── Account Handler ───────────────────────────────────────────────────────────

AccountHandler::AccountHandler(std::shared_ptr<Database> db, const Config& cfg)
    : db_(db), cfg_(cfg) {}

void AccountHandler::handle_get(const httplib::Request&,
                                  httplib::Response& res, int user_id) {
    double balance = db_->get_balance(user_id);
    json resp = {{"balance", balance}};
    res.set_content(resp.dump(), "application/json");
}

void AccountHandler::handle_fund(const httplib::Request& req,
                                  httplib::Response& res, int user_id) {
    json body;
    try { body = json::parse(req.body); }
    catch (...) {
        res.status = 400;
        res.set_content(make_error(400, "Invalid JSON").dump(), "application/json");
        return;
    }

    double amount  = body.value("amount", 0.0);
    std::string note = body.value("note", "Account funding");

    if (amount <= 0) {
        res.status = 400;
        res.set_content(make_error(400, "Amount must be positive").dump(), "application/json");
        return;
    }

    // Use today's date
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char date[16];
    std::strftime(date, sizeof(date), "%Y-%m-%d", &tm);

    try {
        db_->create_transaction(user_id, date, amount, "income", std::nullopt, note, false, true);
        double new_balance = db_->get_balance(user_id);
        json resp = {{"balance", new_balance}};
        res.set_content(resp.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(make_error(400, e.what()).dump(), "application/json");
    }
}

// ── Report Handler ────────────────────────────────────────────────────────────

ReportHandler::ReportHandler(std::shared_ptr<Database> db, const Config& cfg)
    : db_(db), cfg_(cfg) {}

void ReportHandler::handle_monthly(const httplib::Request& req,
                                    httplib::Response& res, int user_id) {
    std::string ym = req.get_param_value("year_month");
    if (ym.empty() || ym.size() != 7) {
        // Default to current month
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        gmtime_r(&t, &tm);
        char buf[8];
        std::strftime(buf, sizeof(buf), "%Y-%m", &tm);
        ym = buf;
    }

    auto report = db_->get_monthly_report(user_id, ym);

    json by_cat = json::array();
    for (const auto& cs : report.by_category) {
        json c = {
            {"category_id", cs.category_id},
            {"category",    cs.category},
            {"color",       cs.color},
            {"budget",      cs.budget},
            {"spent",       cs.spent},
            {"income",      cs.income},
            {"status",      cs.status},
            {"over_by",     cs.over_by},
            {"remaining",   cs.remaining}
        };
        by_cat.push_back(c);
    }

    json top = json::array();
    for (const auto& t : report.top_expenses) top.push_back(t.to_json());

    json resp = {
        {"year_month",     report.year_month},
        {"total_income",   report.total_income},
        {"total_expense",  report.total_expense},
        {"net_change",     report.net_change},
        {"ending_balance", report.ending_balance},
        {"by_category",    by_cat},
        {"top_expenses",   top}
    };
    res.set_content(resp.dump(), "application/json");
}

void ReportHandler::handle_trend(const httplib::Request& req,
                                  httplib::Response& res, int user_id) {
    std::string m = req.get_param_value("months");
    int months = m.empty() ? 6 : std::min(24, std::max(1, std::stoi(m)));
    auto trend = db_->get_trend(user_id, months);

    json arr = json::array();
    for (const auto& t : trend) {
        arr.push_back({{"year_month", t.year_month},
                       {"income",     t.income},
                       {"expense",    t.expense}});
    }
    res.set_content(arr.dump(), "application/json");
}

void ReportHandler::handle_export(const httplib::Request& req,
                                   httplib::Response& res, int user_id) {
    std::string ym = req.get_param_value("year_month");
    std::string fmt = req.get_param_value("format");

    Database::TxnFilter f;
    if (!ym.empty()) {
        f.start_date = ym + "-01";
        f.end_date   = ym + "-31";
    }
    f.size = 10000;

    auto result = db_->list_transactions(user_id, f);

    if (fmt == "csv" || fmt.empty()) {
        std::ostringstream csv;
        csv << "id,date,amount,type,category,note,recurring,created_at\n";
        for (const auto& t : result.items) {
            csv << t.id << ","
                << t.date << ","
                << t.amount << ","
                << t.type << ","
                << "\"" << t.category_name << "\","
                << "\"" << t.note << "\","
                << (t.recurring ? "true" : "false") << ","
                << t.created_at << "\n";
        }
        std::string filename = "transactions";
        if (!ym.empty()) filename += "_" + ym;
        filename += ".csv";

        res.set_header("Content-Disposition", "attachment; filename=\"" + filename + "\"");
        res.set_content(csv.str(), "text/csv");
    } else {
        res.status = 400;
        res.set_content(make_error(400, "Unsupported format. Use format=csv").dump(), "application/json");
    }
}
