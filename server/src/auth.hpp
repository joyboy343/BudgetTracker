#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <optional>
#include <chrono>
#include <sstream>

struct JwtClaims {
    int         user_id   = 0;
    std::string email;
    long long   exp       = 0;
    std::string type;   // "access" | "refresh"
};

class Auth {
public:
    // ── Password hashing (PBKDF2-HMAC-SHA256) ─────────────────────────────
    static std::string hash_password(const std::string& password);
    static bool verify_password(const std::string& password, const std::string& hash);

    // ── JWT ───────────────────────────────────────────────────────────────
    static std::string create_access_token(int user_id, const std::string& email,
                                           const std::string& secret, int expiry_seconds);
    static std::string create_refresh_token(int user_id, const std::string& email,
                                            const std::string& secret, int expiry_seconds);
    static std::optional<JwtClaims> verify_token(const std::string& token,
                                                  const std::string& secret);

    // ── Helpers ───────────────────────────────────────────────────────────
    static std::string extract_bearer_token(const std::string& auth_header);
    static bool is_valid_email(const std::string& email);
    static bool is_strong_password(const std::string& password); // min 8 chars

private:
    static std::string base64url_encode(const std::string& input);
    static std::string base64url_decode(const std::string& input);
    static std::string hmac_sha256(const std::string& data, const std::string& key);
    static std::string build_jwt(const nlohmann::json& payload,
                                  const std::string& secret);
};
