#pragma once
#include <string>
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

struct Config {
    int         port          = 8080;
    std::string db_path       = "budget.db";
    std::string jwt_secret    = "change-me-in-production-use-long-random-string";
    int         jwt_expiry_seconds      = 900;        // 15 minutes for access token
    int         refresh_expiry_seconds  = 604800;     // 7 days for refresh token
    std::string cors_origin   = "http://localhost:5173";
    int         rate_limit_rps = 20;   // requests per second per IP
    bool        demo_mode     = false; // CSV fallback mode
    std::string log_level     = "info";

    static Config load(const std::string& path) {
        Config cfg;
        std::ifstream f(path);
        if (!f.is_open()) {
            // Return defaults if no config file exists
            return cfg;
        }
        try {
            nlohmann::json j;
            f >> j;
            if (j.contains("port"))                     cfg.port                    = j["port"];
            if (j.contains("db_path"))                  cfg.db_path                 = j["db_path"];
            if (j.contains("jwt_secret"))               cfg.jwt_secret              = j["jwt_secret"];
            if (j.contains("jwt_expiry_seconds"))       cfg.jwt_expiry_seconds      = j["jwt_expiry_seconds"];
            if (j.contains("refresh_expiry_seconds"))   cfg.refresh_expiry_seconds  = j["refresh_expiry_seconds"];
            if (j.contains("cors_origin"))              cfg.cors_origin             = j["cors_origin"];
            if (j.contains("rate_limit_rps"))           cfg.rate_limit_rps          = j["rate_limit_rps"];
            if (j.contains("demo_mode"))                cfg.demo_mode               = j["demo_mode"];
            if (j.contains("log_level"))                cfg.log_level               = j["log_level"];
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Failed to parse config: ") + e.what());
        }
        return cfg;
    }
};
