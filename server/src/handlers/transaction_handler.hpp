#pragma once
#include <httplib.h>
#include <memory>
#include "../database.hpp"
#include "../config.hpp"

class TransactionHandler {
public:
    TransactionHandler(std::shared_ptr<Database> db, const Config& cfg);
    void handle_list  (const httplib::Request&, httplib::Response&, int user_id);
    void handle_get   (const httplib::Request&, httplib::Response&, int user_id);
    void handle_create(const httplib::Request&, httplib::Response&, int user_id);
    void handle_update(const httplib::Request&, httplib::Response&, int user_id);
    void handle_delete(const httplib::Request&, httplib::Response&, int user_id);
    void handle_undo  (const httplib::Request&, httplib::Response&, int user_id);
    void handle_import_csv(const httplib::Request&, httplib::Response&, int user_id);

private:
    std::shared_ptr<Database> db_;
    Config cfg_;
    bool validate_date(const std::string& date);
};
