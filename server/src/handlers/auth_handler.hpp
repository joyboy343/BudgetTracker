#pragma once
#include <httplib.h>
#include <memory>
#include "../database.hpp"
#include "../config.hpp"

class AuthHandler {
public:
    AuthHandler(std::shared_ptr<Database> db, const Config& cfg);
    void handle_register(const httplib::Request& req, httplib::Response& res);
    void handle_login   (const httplib::Request& req, httplib::Response& res);
    void handle_refresh (const httplib::Request& req, httplib::Response& res);
    void handle_logout  (const httplib::Request& req, httplib::Response& res);
    void handle_me      (const httplib::Request& req, httplib::Response& res);

private:
    std::shared_ptr<Database> db_;
    Config cfg_;
};
