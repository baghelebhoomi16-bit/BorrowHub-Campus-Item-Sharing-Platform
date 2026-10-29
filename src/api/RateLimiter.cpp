#include "src/api/RateLimiter.hpp"

namespace borrowhub::api {

RateLimiter::RateLimiter(const util::Clock& clock) : clock_(clock) {}

bool RateLimiter::checkAndRecord(const std::string& key, int maxRequests, int64_t windowSeconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    int64_t now = clock_.now();
    int64_t windowStart = now - windowSeconds;

    auto& log = accessLogs_[key];

    // Remove timestamps outside the sliding window
    while (!log.empty() && log.front() < windowStart) {
        log.pop_front();
    }

    if (static_cast<int>(log.size()) >= maxRequests) {
        return false; // Rate limit exceeded
    }

    log.push_back(now);
    return true;
}

void RateLimiter::reset(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    accessLogs_.erase(key);
}

} // namespace borrowhub::api
