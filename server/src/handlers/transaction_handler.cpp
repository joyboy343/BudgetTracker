#include "transaction_handler.hpp"
#include "../models.hpp"
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>
#include <iostream>

using json = nlohmann::json;

TransactionHandler::TransactionHandler(std::shared_ptr<Database> db, const Config& cfg)
    : db_(db), cfg_(cfg) {}

bool TransactionHandler::validate_date(const std::string& date) {
    static const std::regex re(R"(\d{4}-\d{2}-\d{2})");
    return std::regex_match(date, re);
}

void TransactionHandler::handle_list(const httplib::Request& req,
                                      httplib::Response& res, int user_id) {
    Database::TxnFilter f;
    f.start_date = req.get_param_value("start");
    f.end_date   = req.get_param_value("end");
    std::string cat = req.get_param_value("category");
    if (!cat.empty()) f.category_id = std::stoi(cat);
    f.type = req.get_param_value("type");
    std::string page = req.get_param_value("page");
    std::string size = req.get_param_value("size");
    if (!page.empty()) f.page = std::max(1, std::stoi(page));
    if (!size.empty()) f.size = std::min(200, std::max(1, std::stoi(size)));

    auto result = db_->list_transactions(user_id, f);

    json items = json::array();
    for (const auto& t : result.items) items.push_back(t.to_json());

    json resp = {
        {"items",    items},
        {"total",    result.total},
        {"page",     result.page},
        {"size",     result.size},
        {"pages",    (result.total + result.size - 1) / result.size}
    };
    res.set_content(resp.dump(), "application/json");
}

void TransactionHandler::handle_get(const httplib::Request& req,
                                     httplib::Response& res, int user_id) {
    int id = std::stoi(req.path_params.at("id"));
    auto txn = db_->get_transaction(id, user_id);
    if (!txn) {
        res.status = 404;
        res.set_content(make_error(404, "Transaction not found").dump(), "application/json");
        return;
    }
    res.set_content(txn->to_json().dump(), "application/json");
}

void TransactionHandler::handle_create(const httplib::Request& req,
                                        httplib::Response& res, int user_id) {
    json body;
    try { body = json::parse(req.body); }
    catch (...) {
        res.status = 400;
        res.set_content(make_error(400, "Invalid JSON").dump(), "application/json");
        return;
    }

    std::string date = body.value("date", "");
    double amount    = body.value("amount", 0.0);
    std::string type = body.value("type", "");
    std::string note = body.value("note", "");
    bool recurring   = body.value("recurring", false);
    bool force       = req.get_param_value("force") == "true";

    std::optional<int> cat_id;
    if (body.contains("category_id") && !body["category_id"].is_null())
        cat_id = body["category_id"].get<int>();

    if (!validate_date(date)) {
        res.status = 400;
        res.set_content(make_error(400, "Invalid date format. Use YYYY-MM-DD").dump(), "application/json");
        return;
    }
    if (amount <= 0) {
        res.status = 400;
        res.set_content(make_error(400, "Amount must be positive").dump(), "application/json");
        return;
    }
    if (type != "expense" && type != "income") {
        res.status = 400;
        res.set_content(make_error(400, "Type must be 'expense' or 'income'").dump(), "application/json");
        return;
    }

    try {
        int id = db_->create_transaction(user_id, date, amount, type, cat_id, note, recurring, force);
        auto txn = db_->get_transaction(id, user_id);
        res.status = 201;
        res.set_content(txn->to_json().dump(), "application/json");
    } catch (const Database::BalanceError& e) {
        res.status = 409;
        json err = make_error(409, "Insufficient balance");
        err["current_balance"] = e.current_balance;
        err["required"]        = e.required;
        err["hint"]            = "Add ?force=true to override";
        res.set_content(err.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(make_error(400, e.what()).dump(), "application/json");
    }
}

void TransactionHandler::handle_update(const httplib::Request& req,
                                        httplib::Response& res, int user_id) {
    int id = std::stoi(req.path_params.at("id"));

    json body;
    try { body = json::parse(req.body); }
    catch (...) {
        res.status = 400;
        res.set_content(make_error(400, "Invalid JSON").dump(), "application/json");
        return;
    }

    auto existing = db_->get_transaction(id, user_id);
    if (!existing) {
        res.status = 404;
        res.set_content(make_error(404, "Transaction not found").dump(), "application/json");
        return;
    }

    std::string date  = body.value("date",  existing->date);
    double amount     = body.value("amount", existing->amount);
    std::string type  = body.value("type",  existing->type);
    std::string note  = body.value("note",  existing->note);
    bool recurring    = body.value("recurring", existing->recurring);

    std::optional<int> cat_id = existing->category_id;
    if (body.contains("category_id")) {
        if (body["category_id"].is_null()) cat_id = std::nullopt;
        else cat_id = body["category_id"].get<int>();
    }

    if (!validate_date(date)) {
        res.status = 400;
        res.set_content(make_error(400, "Invalid date format").dump(), "application/json");
        return;
    }

    try {
        bool ok = db_->update_transaction(id, user_id, date, amount, type, cat_id, note, recurring);
        if (!ok) {
            res.status = 404;
            res.set_content(make_error(404, "Transaction not found").dump(), "application/json");
            return;
        }
        auto updated = db_->get_transaction(id, user_id);
        res.set_content(updated->to_json().dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(make_error(400, e.what()).dump(), "application/json");
    }
}

void TransactionHandler::handle_delete(const httplib::Request& req,
                                        httplib::Response& res, int user_id) {
    int id = std::stoi(req.path_params.at("id"));
    bool ok = db_->delete_transaction(id, user_id);
    if (!ok) {
        res.status = 404;
        res.set_content(make_error(404, "Transaction not found").dump(), "application/json");
        return;
    }
    res.set_content(make_ok("Transaction deleted").dump(), "application/json");
}

void TransactionHandler::handle_undo(const httplib::Request& req,
                                      httplib::Response& res, int user_id) {
    json body;
    try { body = json::parse(req.body); } catch (...) { body = {}; }
    int n = body.value("last_n", 1);
    if (n < 1 || n > 50) {
        res.status = 400;
        res.set_content(make_error(400, "last_n must be between 1 and 50").dump(), "application/json");
        return;
    }
    auto removed = db_->undo_last_transactions(user_id, n);
    json resp = {{"removed_ids", removed}, {"count", removed.size()}};
    res.set_content(resp.dump(), "application/json");
}

void TransactionHandler::handle_import_csv(const httplib::Request& req,
                                            httplib::Response& res, int user_id) {
    // Parse multipart or raw CSV body
    // Expected CSV: date,amount,type,category,note
    std::istringstream stream(req.body);
    std::string line;
    int imported = 0, skipped = 0;
    std::vector<std::string> errors;

    std::getline(stream, line); // skip header
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        std::istringstream row(line);
        std::string date, amt_str, type, cat_name, note;
        std::getline(row, date,     ',');
        std::getline(row, amt_str,  ',');
        std::getline(row, type,     ',');
        std::getline(row, cat_name, ',');
        std::getline(row, note,     ',');

        // Trim whitespace
        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\r\""));
            s.erase(s.find_last_not_of(" \t\r\"") + 1);
        };
        trim(date); trim(amt_str); trim(type); trim(cat_name); trim(note);

        if (!validate_date(date)) {
            errors.push_back("Invalid date: " + date);
            skipped++;
            continue;
        }

        double amount = 0;
        try { amount = std::stod(amt_str); } catch (...) {
            errors.push_back("Invalid amount: " + amt_str);
            skipped++;
            continue;
        }

        if (type != "expense" && type != "income") {
            errors.push_back("Invalid type: " + type);
            skipped++;
            continue;
        }

        try {
            db_->create_transaction(user_id, date, amount, type, std::nullopt, note, false, true);
            imported++;
        } catch (const std::exception& e) {
            errors.push_back(std::string("Row failed: ") + e.what());
            skipped++;
        }
    }

    json resp = {
        {"imported", imported},
        {"skipped",  skipped},
        {"errors",   errors}
    };
    res.set_content(resp.dump(), "application/json");
}
