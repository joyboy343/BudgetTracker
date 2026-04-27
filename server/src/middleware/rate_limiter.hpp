#pragma once
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <string>

// Simple token bucket rate limiter per IP address.
// rps = max requests per second. Bursts allowed up to rps * 2.
class RateLimiter {
public:
    explicit RateLimiter(int rps = 20)
        : rps_(rps), burst_(rps * 2) {}

    // Returns true if the request should be allowed.
    bool allow(const std::string& ip) {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mutex_);

        auto& bucket = buckets_[ip];
        auto elapsed = std::chrono::duration<double>(now - bucket.last_refill).count();
        bucket.tokens = std::min(
            static_cast<double>(burst_),
            bucket.tokens + elapsed * rps_
        );
        bucket.last_refill = now;

        if (bucket.tokens >= 1.0) {
            bucket.tokens -= 1.0;
            return true;
        }
        return false;
    }

    // Stricter limiter for login/register endpoints (5 per minute)
    bool allow_auth(const std::string& ip) {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(auth_mutex_);

        auto& bucket = auth_buckets_[ip];
        auto elapsed = std::chrono::duration<double>(now - bucket.last_refill).count();
        bucket.tokens = std::min(5.0, bucket.tokens + elapsed * (5.0 / 60.0));
        bucket.last_refill = now;

        if (bucket.tokens >= 1.0) {
            bucket.tokens -= 1.0;
            return true;
        }
        return false;
    }

private:
    struct Bucket {
        double tokens = 0;
        std::chrono::steady_clock::time_point last_refill = std::chrono::steady_clock::now();
    };

    int rps_;
    int burst_;
    std::unordered_map<std::string, Bucket> buckets_;
    std::unordered_map<std::string, Bucket> auth_buckets_;
    std::mutex mutex_;
    std::mutex auth_mutex_;
};
