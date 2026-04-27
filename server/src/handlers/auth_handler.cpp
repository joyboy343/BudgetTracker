#include "auth_handler.hpp"
#include "../auth.hpp"
#include "../models.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

AuthHandler::AuthHandler(std::shared_ptr<Database> db, const Config& cfg)
    : db_(db), cfg_(cfg) {}

void AuthHandler::handle_register(const httplib::Request& req, httplib::Response& res) {
    json body;
    try { body = json::parse(req.body); }
    catch (...) {
        res.status = 400;
        res.set_content(make_error(400, "Invalid JSON").dump(), "application/json");
        return;
    }

    std::string email    = body.value("email", "");
    std::string password = body.value("password", "");

    if (!Auth::is_valid_email(email)) {
        res.status = 400;
        res.set_content(make_error(400, "Invalid email address").dump(), "application/json");
        return;
    }
    if (!Auth::is_strong_password(password)) {
        res.status = 400;
        res.set_content(make_error(400, "Password must be at least 8 characters").dump(), "application/json");
        return;
    }

    std::string hash = Auth::hash_password(password);
    auto user_id = db_->create_user(email, hash);
    if (!user_id) {
        res.status = 409;
        res.set_content(make_error(409, "Email already registered").dump(), "application/json");
        return;
    }

    std::string access  = Auth::create_access_token(*user_id, email, cfg_.jwt_secret, cfg_.jwt_expiry_seconds);
    std::string refresh = Auth::create_refresh_token(*user_id, email, cfg_.jwt_secret, cfg_.refresh_expiry_seconds);

    json resp = {
        {"user_id", *user_id},
        {"email",   email},
        {"access_token",  access},
        {"refresh_token", refresh},
        {"expires_in",    cfg_.jwt_expiry_seconds}
    };
    res.status = 201;
    res.set_content(resp.dump(), "application/json");
}

void AuthHandler::handle_login(const httplib::Request& req, httplib::Response& res) {
    json body;
    try { body = json::parse(req.body); }
    catch (...) {
        res.status = 400;
        res.set_content(make_error(400, "Invalid JSON").dump(), "application/json");
        return;
    }

    std::string email    = body.value("email", "");
    std::string password = body.value("password", "");

    auto user = db_->get_user_by_email(email);
    if (!user || !Auth::verify_password(password, user->password_hash)) {
        res.status = 401;
        res.set_content(make_error(401, "Invalid email or password").dump(), "application/json");
        return;
    }

    std::string access  = Auth::create_access_token(user->id, user->email, cfg_.jwt_secret, cfg_.jwt_expiry_seconds);
    std::string refresh = Auth::create_refresh_token(user->id, user->email, cfg_.jwt_secret, cfg_.refresh_expiry_seconds);

    json resp = {
        {"user_id", user->id},
        {"email",   user->email},
        {"access_token",  access},
        {"refresh_token", refresh},
        {"expires_in",    cfg_.jwt_expiry_seconds}
    };
    res.set_content(resp.dump(), "application/json");
}

void AuthHandler::handle_refresh(const httplib::Request& req, httplib::Response& res) {
    json body;
    try { body = json::parse(req.body); }
    catch (...) {
        res.status = 400;
        res.set_content(make_error(400, "Invalid JSON").dump(), "application/json");
        return;
    }

    std::string refresh_token = body.value("refresh_token", "");
    auto claims = Auth::verify_token(refresh_token, cfg_.jwt_secret);
    if (!claims || claims->type != "refresh") {
        res.status = 401;
        res.set_content(make_error(401, "Invalid or expired refresh token").dump(), "application/json");
        return;
    }

    std::string new_access = Auth::create_access_token(
        claims->user_id, claims->email, cfg_.jwt_secret, cfg_.jwt_expiry_seconds);
    std::string new_refresh = Auth::create_refresh_token(
        claims->user_id, claims->email, cfg_.jwt_secret, cfg_.refresh_expiry_seconds);

    json resp = {
        {"access_token",  new_access},
        {"refresh_token", new_refresh},
        {"expires_in",    cfg_.jwt_expiry_seconds}
    };
    res.set_content(resp.dump(), "application/json");
}

void AuthHandler::handle_logout(const httplib::Request& /*req*/, httplib::Response& res) {
    // With stateless JWTs, logout is client-side; server acknowledges
    res.set_content(make_ok("Logged out").dump(), "application/json");
}

void AuthHandler::handle_me(const httplib::Request& req, httplib::Response& res) {
    std::string auth   = req.get_header_value("Authorization");
    std::string token  = Auth::extract_bearer_token(auth);
    auto claims = Auth::verify_token(token, cfg_.jwt_secret);
    if (!claims) {
        res.status = 401;
        res.set_content(make_error(401, "Unauthorized").dump(), "application/json");
        return;
    }
    auto user = db_->get_user_by_id(claims->user_id);
    if (!user) {
        res.status = 404;
        res.set_content(make_error(404, "User not found").dump(), "application/json");
        return;
    }
    json resp = {{"id", user->id}, {"email", user->email}, {"created_at", user->created_at}};
    res.set_content(resp.dump(), "application/json");
}
