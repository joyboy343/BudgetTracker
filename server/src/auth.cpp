#include "auth.hpp"
#include <nlohmann/json.hpp>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>
#include <regex>
#include <chrono>

using json = nlohmann::json;

// ── Base64URL ─────────────────────────────────────────────────────────────────

std::string Auth::base64url_encode(const std::string& input) {
    BIO* b64  = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input.data(), static_cast<int>(input.size()));
    BIO_flush(b64);
    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64);
    // Convert standard base64 → base64url
    for (auto& c : result) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    // Remove padding
    while (!result.empty() && result.back() == '=') result.pop_back();
    return result;
}

std::string Auth::base64url_decode(const std::string& input) {
    std::string s = input;
    for (auto& c : s) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    // Add padding
    while (s.size() % 4 != 0) s += '=';

    BIO* b64  = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new_mem_buf(s.data(), static_cast<int>(s.size()));
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    std::string result(s.size(), '\0');
    int len = BIO_read(b64, result.data(), static_cast<int>(result.size()));
    BIO_free_all(b64);
    if (len < 0) return "";
    result.resize(len);
    return result;
}

// ── HMAC-SHA256 ───────────────────────────────────────────────────────────────

std::string Auth::hmac_sha256(const std::string& data, const std::string& key) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  digest_len = 0;
    HMAC(EVP_sha256(),
         key.data(),   static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(data.data()),
         static_cast<int>(data.size()),
         digest, &digest_len);
    return std::string(reinterpret_cast<char*>(digest), digest_len);
}

// ── JWT ───────────────────────────────────────────────────────────────────────

std::string Auth::build_jwt(const json& payload, const std::string& secret) {
    json header = {{"alg", "HS256"}, {"typ", "JWT"}};
    std::string h = base64url_encode(header.dump());
    std::string p = base64url_encode(payload.dump());
    std::string signing_input = h + "." + p;
    std::string sig = base64url_encode(hmac_sha256(signing_input, secret));
    return signing_input + "." + sig;
}

std::string Auth::create_access_token(int user_id, const std::string& email,
                                       const std::string& secret, int expiry_seconds) {
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::seconds(expiry_seconds);
    long long exp_ts = std::chrono::duration_cast<std::chrono::seconds>(
                           exp.time_since_epoch()).count();
    json payload = {
        {"sub",     std::to_string(user_id)},
        {"email",   email},
        {"type",    "access"},
        {"exp",     exp_ts},
        {"iat",     std::chrono::duration_cast<std::chrono::seconds>(
                        now.time_since_epoch()).count()}
    };
    return build_jwt(payload, secret);
}

std::string Auth::create_refresh_token(int user_id, const std::string& email,
                                        const std::string& secret, int expiry_seconds) {
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::seconds(expiry_seconds);
    long long exp_ts = std::chrono::duration_cast<std::chrono::seconds>(
                           exp.time_since_epoch()).count();
    json payload = {
        {"sub",  std::to_string(user_id)},
        {"email", email},
        {"type", "refresh"},
        {"exp",  exp_ts},
        {"iat",  std::chrono::duration_cast<std::chrono::seconds>(
                     now.time_since_epoch()).count()}
    };
    return build_jwt(payload, secret);
}

std::optional<JwtClaims> Auth::verify_token(const std::string& token,
                                              const std::string& secret) {
    // Split into 3 parts
    auto dot1 = token.find('.');
    if (dot1 == std::string::npos) return std::nullopt;
    auto dot2 = token.find('.', dot1 + 1);
    if (dot2 == std::string::npos) return std::nullopt;

    std::string header_b64  = token.substr(0, dot1);
    std::string payload_b64 = token.substr(dot1 + 1, dot2 - dot1 - 1);
    std::string sig_b64     = token.substr(dot2 + 1);

    // Verify signature
    std::string signing_input = header_b64 + "." + payload_b64;
    std::string expected_sig  = base64url_encode(hmac_sha256(signing_input, secret));
    if (expected_sig != sig_b64) return std::nullopt;

    // Decode payload
    std::string payload_str = base64url_decode(payload_b64);
    json payload;
    try {
        payload = json::parse(payload_str);
    } catch (...) {
        return std::nullopt;
    }

    // Check expiry
    long long exp = payload.value("exp", 0LL);
    auto now_ts = std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::system_clock::now().time_since_epoch()).count();
    if (exp < now_ts) return std::nullopt;

    JwtClaims claims;
    claims.user_id = std::stoi(payload.value("sub", "0"));
    claims.email   = payload.value("email", "");
    claims.exp     = exp;
    claims.type    = payload.value("type", "access");
    return claims;
}

std::string Auth::extract_bearer_token(const std::string& auth_header) {
    if (auth_header.rfind("Bearer ", 0) == 0) {
        return auth_header.substr(7);
    }
    return "";
}

// ── Password hashing (PBKDF2-HMAC-SHA256) ────────────────────────────────────

std::string Auth::hash_password(const std::string& password) {
    // Generate 16-byte random salt
    unsigned char salt[16];
    RAND_bytes(salt, sizeof(salt));

    // Derive 32-byte key using PBKDF2
    unsigned char key[32];
    PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()),
                       salt, sizeof(salt),
                       100000, EVP_sha256(),
                       sizeof(key), key);

    // Encode as hex: salt_hex:key_hex
    auto to_hex = [](const unsigned char* data, size_t len) {
        std::ostringstream oss;
        for (size_t i = 0; i < len; ++i)
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
        return oss.str();
    };

    return to_hex(salt, sizeof(salt)) + ":" + to_hex(key, sizeof(key));
}

bool Auth::verify_password(const std::string& password, const std::string& hash) {
    auto colon = hash.find(':');
    if (colon == std::string::npos) return false;

    std::string salt_hex = hash.substr(0, colon);
    std::string key_hex  = hash.substr(colon + 1);

    if (salt_hex.size() != 32 || key_hex.size() != 64) return false;

    auto from_hex = [](const std::string& hex, unsigned char* out, size_t expected) {
        if (hex.size() != expected * 2) return false;
        for (size_t i = 0; i < expected; ++i) {
            out[i] = static_cast<unsigned char>(std::stoi(hex.substr(i * 2, 2), nullptr, 16));
        }
        return true;
    };

    unsigned char salt[16], stored_key[32];
    if (!from_hex(salt_hex, salt, 16))       return false;
    if (!from_hex(key_hex, stored_key, 32))  return false;

    unsigned char derived_key[32];
    PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()),
                       salt, 16,
                       100000, EVP_sha256(),
                       32, derived_key);

    // Constant-time compare
    return CRYPTO_memcmp(derived_key, stored_key, 32) == 0;
}

// ── Validators ────────────────────────────────────────────────────────────────

bool Auth::is_valid_email(const std::string& email) {
    static const std::regex re(R"([a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, re);
}

bool Auth::is_strong_password(const std::string& password) {
    return password.size() >= 8;
}
