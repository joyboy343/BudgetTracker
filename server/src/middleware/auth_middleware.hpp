#pragma once
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <functional>
#include "../auth.hpp"
#include "../models.hpp"

using AuthenticatedHandler = std::function<void(const httplib::Request&, httplib::Response&, int user_id)>;

inline auto require_auth(const std::string& jwt_secret, AuthenticatedHandler handler) {
    return [jwt_secret, handler](const httplib::Request& req, httplib::Response& res) {
        std::string auth_header = req.get_header_value("Authorization");
        if (auth_header.empty()) {
            res.status = 401;
            res.set_content(make_error(401, "Missing Authorization header").dump(), "application/json");
            return;
        }

        std::string token = Auth::extract_bearer_token(auth_header);
        if (token.empty()) {
            res.status = 401;
            res.set_content(make_error(401, "Invalid Authorization format").dump(), "application/json");
            return;
        }

        auto claims = Auth::verify_token(token, jwt_secret);
        if (!claims) {
            res.status = 401;
            res.set_content(make_error(401, "Token is invalid or expired").dump(), "application/json");
            return;
        }

        if (claims->type != "access") {
            res.status = 401;
            res.set_content(make_error(401, "Must use access token").dump(), "application/json");
            return;
        }

        handler(req, res, claims->user_id);
    };
}