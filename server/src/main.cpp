#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <memory>
#include <csignal>

#include "config.hpp"
#include "database.hpp"
#include "models.hpp"
#include "auth.hpp"
#include "middleware/auth_middleware.hpp"
#include "middleware/rate_limiter.hpp"
#include "handlers/auth_handler.hpp"
#include "handlers/transaction_handler.hpp"
#include "handlers/other_handlers.hpp"

using json = nlohmann::json;

static volatile bool g_running = true;
static void signal_handler(int) { g_running = false; }

// ── Logging helper ────────────────────────────────────────────────────────────
static void log(const std::string& level, const std::string& msg) {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    char ts[24];
    std::strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
    std::cout << "[" << ts << "] [" << level << "] " << msg << "\n";
}

// ── CORS setup ────────────────────────────────────────────────────────────────
static void setup_cors(httplib::Server& svr, const std::string& origin) {
    svr.set_pre_routing_handler([origin](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin",  origin);
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Authorization, Content-Type, Accept");
        res.set_header("Access-Control-Max-Age",       "86400");
        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });
}

int main(int argc, char* argv[]) {
    std::string config_path = "config.json";
    if (argc > 1) config_path = argv[1];

    // Load config
    Config cfg;
    try {
        cfg = Config::load(config_path);
        log("info", "Config loaded from " + config_path);
    } catch (const std::exception& e) {
        log("warn", std::string("Config load failed, using defaults: ") + e.what());
    }

    // Open database
    std::shared_ptr<Database> db;
    try {
        db = std::make_shared<Database>(cfg.db_path);
        db->migrate();
        log("info", "Database ready: " + cfg.db_path);
    } catch (const std::exception& e) {
        log("error", std::string("DB init failed: ") + e.what());
        return 1;
    }

    // Build handlers
    AuthHandler        auth_h  (db, cfg);
    TransactionHandler txn_h   (db, cfg);
    CategoryHandler    cat_h   (db, cfg);
    BudgetHandler      bud_h   (db, cfg);
    AccountHandler     acc_h   (db, cfg);
    ReportHandler      rep_h   (db, cfg);

    // Rate limiter
    auto limiter = std::make_shared<RateLimiter>(cfg.rate_limit_rps);

    // ── HTTP Server ────────────────────────────────────────────────────────────
    httplib::Server svr;
    setup_cors(svr, cfg.cors_origin);

    // Request logger
    svr.set_logger([&](const httplib::Request& req, const httplib::Response& res) {
        log("info", req.method + " " + req.path + " → " + std::to_string(res.status));
    });

    // Global rate limiter
    svr.set_pre_routing_handler([&](const httplib::Request& req, httplib::Response& res) -> httplib::Server::HandlerResponse {
        // CORS preflight handled first
        res.set_header("Access-Control-Allow-Origin",  cfg.cors_origin);
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Authorization, Content-Type, Accept");
        res.set_header("Access-Control-Max-Age",       "86400");

        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }

        if (!limiter->allow(req.remote_addr)) {
            res.status = 429;
            res.set_content(make_error(429, "Too many requests").dump(), "application/json");
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // ── Auth routes ────────────────────────────────────────────────────────────
    svr.Post("/api/v1/auth/register", [&](const httplib::Request& req, httplib::Response& res) {
        if (!limiter->allow_auth(req.remote_addr)) {
            res.status = 429;
            res.set_content(make_error(429, "Too many auth attempts").dump(), "application/json");
            return;
        }
        auth_h.handle_register(req, res);
    });

    svr.Post("/api/v1/auth/login", [&](const httplib::Request& req, httplib::Response& res) {
        if (!limiter->allow_auth(req.remote_addr)) {
            res.status = 429;
            res.set_content(make_error(429, "Too many auth attempts").dump(), "application/json");
            return;
        }
        auth_h.handle_login(req, res);
    });

    svr.Post("/api/v1/auth/refresh", [&](const httplib::Request& req, httplib::Response& res) {
        auth_h.handle_refresh(req, res);
    });

    svr.Post("/api/v1/auth/logout", [&](const httplib::Request& req, httplib::Response& res) {
        auth_h.handle_logout(req, res);
    });

    svr.Get("/api/v1/auth/me", [&](const httplib::Request& req, httplib::Response& res) {
        auth_h.handle_me(req, res);
    });

    // ── Account routes ─────────────────────────────────────────────────────────
    svr.Get("/api/v1/account", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        acc_h.handle_get(req, res, uid);
    }));

    svr.Post("/api/v1/account/fund", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        acc_h.handle_fund(req, res, uid);
    }));

    // ── Category routes ────────────────────────────────────────────────────────
    svr.Get("/api/v1/categories", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        cat_h.handle_list(req, res, uid);
    }));

    svr.Post("/api/v1/categories", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        cat_h.handle_create(req, res, uid);
    }));

    svr.Put("/api/v1/categories/:id", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        cat_h.handle_update(req, res, uid);
    }));

    svr.Delete("/api/v1/categories/:id", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        cat_h.handle_delete(req, res, uid);
    }));

    // ── Transaction routes ─────────────────────────────────────────────────────
    svr.Get("/api/v1/transactions", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        txn_h.handle_list(req, res, uid);
    }));

    svr.Get("/api/v1/transactions/:id", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        txn_h.handle_get(req, res, uid);
    }));

    svr.Post("/api/v1/transactions", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        txn_h.handle_create(req, res, uid);
    }));

    svr.Put("/api/v1/transactions/:id", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        txn_h.handle_update(req, res, uid);
    }));

    svr.Delete("/api/v1/transactions/:id", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        txn_h.handle_delete(req, res, uid);
    }));

    svr.Post("/api/v1/transactions/undo", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        txn_h.handle_undo(req, res, uid);
    }));

    svr.Post("/api/v1/transactions/import", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        txn_h.handle_import_csv(req, res, uid);
    }));

    // ── Budget routes ──────────────────────────────────────────────────────────
    svr.Get("/api/v1/budgets", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        bud_h.handle_list(req, res, uid);
    }));

    svr.Post("/api/v1/budgets", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        bud_h.handle_upsert(req, res, uid);
    }));

    svr.Delete("/api/v1/budgets/:id", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        bud_h.handle_delete(req, res, uid);
    }));

    // ── Report routes ──────────────────────────────────────────────────────────
    svr.Get("/api/v1/reports/monthly", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        rep_h.handle_monthly(req, res, uid);
    }));

    svr.Get("/api/v1/reports/trend", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        rep_h.handle_trend(req, res, uid);
    }));

    svr.Get("/api/v1/reports/export", require_auth(cfg.jwt_secret, [&](auto& req, auto& res, int uid) {
        rep_h.handle_export(req, res, uid);
    }));

    // ── Health check ───────────────────────────────────────────────────────────
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    // ── Start listening ────────────────────────────────────────────────────────
    log("info", "Budget Server starting on 0.0.0.0:" + std::to_string(cfg.port));

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    if (!svr.listen("0.0.0.0", cfg.port)) {
        log("error", "Failed to bind port " + std::to_string(cfg.port));
        return 1;
    }

    return 0;
}
