#pragma once
#include <httplib.h>
#include <memory>
#include "../database.hpp"
#include "../config.hpp"

class CategoryHandler {
public:
    CategoryHandler(std::shared_ptr<Database> db, const Config& cfg);
    void handle_list  (const httplib::Request&, httplib::Response&, int user_id);
    void handle_create(const httplib::Request&, httplib::Response&, int user_id);
    void handle_update(const httplib::Request&, httplib::Response&, int user_id);
    void handle_delete(const httplib::Request&, httplib::Response&, int user_id);
private:
    std::shared_ptr<Database> db_;
    Config cfg_;
};

class BudgetHandler {
public:
    BudgetHandler(std::shared_ptr<Database> db, const Config& cfg);
    void handle_list  (const httplib::Request&, httplib::Response&, int user_id);
    void handle_upsert(const httplib::Request&, httplib::Response&, int user_id);
    void handle_delete(const httplib::Request&, httplib::Response&, int user_id);
private:
    std::shared_ptr<Database> db_;
    Config cfg_;
};

class AccountHandler {
public:
    AccountHandler(std::shared_ptr<Database> db, const Config& cfg);
    void handle_get (const httplib::Request&, httplib::Response&, int user_id);
    void handle_fund(const httplib::Request&, httplib::Response&, int user_id);
private:
    std::shared_ptr<Database> db_;
    Config cfg_;
};

class ReportHandler {
public:
    ReportHandler(std::shared_ptr<Database> db, const Config& cfg);
    void handle_monthly(const httplib::Request&, httplib::Response&, int user_id);
    void handle_trend  (const httplib::Request&, httplib::Response&, int user_id);
    void handle_export (const httplib::Request&, httplib::Response&, int user_id);
private:
    std::shared_ptr<Database> db_;
    Config cfg_;
};
