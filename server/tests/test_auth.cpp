#include <catch2/catch_test_macros.hpp>
#include "../src/auth.hpp"

TEST_CASE("Password hashing and verification", "[auth]") {
    SECTION("Valid password round-trips correctly") {
        std::string pw = "SecurePass123";
        std::string hash = Auth::hash_password(pw);
        REQUIRE(!hash.empty());
        REQUIRE(Auth::verify_password(pw, hash));
    }

    SECTION("Wrong password fails verification") {
        std::string pw   = "SecurePass123";
        std::string hash = Auth::hash_password(pw);
        REQUIRE_FALSE(Auth::verify_password("WrongPassword", hash));
    }

    SECTION("Two hashes of same password differ (salt)") {
        std::string pw = "SamePassword1";
        REQUIRE(Auth::hash_password(pw) != Auth::hash_password(pw));
    }
}

TEST_CASE("JWT creation and verification", "[auth]") {
    const std::string secret = "test-secret-key-32-chars-long!!!";

    SECTION("Access token round-trips") {
        auto token  = Auth::create_access_token(42, "user@example.com", secret, 900);
        REQUIRE(!token.empty());
        auto claims = Auth::verify_token(token, secret);
        REQUIRE(claims.has_value());
        REQUIRE(claims->user_id == 42);
        REQUIRE(claims->email   == "user@example.com");
        REQUIRE(claims->type    == "access");
    }

    SECTION("Refresh token has correct type") {
        auto token  = Auth::create_refresh_token(7, "r@test.com", secret, 3600);
        auto claims = Auth::verify_token(token, secret);
        REQUIRE(claims.has_value());
        REQUIRE(claims->type == "refresh");
        REQUIRE(claims->user_id == 7);
    }

    SECTION("Wrong secret fails verification") {
        auto token  = Auth::create_access_token(1, "x@y.com", secret, 900);
        auto claims = Auth::verify_token(token, "wrong-secret");
        REQUIRE_FALSE(claims.has_value());
    }

    SECTION("Expired token fails") {
        auto token  = Auth::create_access_token(1, "x@y.com", secret, -1); // already expired
        auto claims = Auth::verify_token(token, secret);
        REQUIRE_FALSE(claims.has_value());
    }

    SECTION("Tampered payload fails") {
        auto token = Auth::create_access_token(1, "x@y.com", secret, 900);
        std::string tampered = token;
        tampered[10] ^= 0x01; // flip a bit
        auto claims = Auth::verify_token(tampered, secret);
        REQUIRE_FALSE(claims.has_value());
    }
}

TEST_CASE("Email validation", "[auth]") {
    REQUIRE( Auth::is_valid_email("user@example.com"));
    REQUIRE( Auth::is_valid_email("a.b+c@x.co.uk"));
    REQUIRE_FALSE(Auth::is_valid_email("not-an-email"));
    REQUIRE_FALSE(Auth::is_valid_email("@nodomain.com"));
    REQUIRE_FALSE(Auth::is_valid_email("noatsign.com"));
}

TEST_CASE("Password strength check", "[auth]") {
    REQUIRE( Auth::is_strong_password("12345678"));
    REQUIRE( Auth::is_strong_password("abcdefghij"));
    REQUIRE_FALSE(Auth::is_strong_password("short"));
    REQUIRE_FALSE(Auth::is_strong_password("1234567"));
}

TEST_CASE("Bearer token extraction", "[auth]") {
    REQUIRE(Auth::extract_bearer_token("Bearer abc123") == "abc123");
    REQUIRE(Auth::extract_bearer_token("Basic xyz").empty());
    REQUIRE(Auth::extract_bearer_token("").empty());
}
