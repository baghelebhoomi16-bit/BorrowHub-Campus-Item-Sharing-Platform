#pragma once

#include "src/util/Clock.hpp"

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

namespace borrowhub::api {

class RateLimiter {
public:
    explicit RateLimiter(const util::Clock& clock);

    // Checks if action is allowed under sliding window; records attempt if allowed
    [[nodiscard]] bool checkAndRecord(const std::string& key, int maxRequests, int64_t windowSeconds);

    // Resets counters for key
    void reset(const std::string& key);

private:
    const util::Clock& clock_;
    std::mutex mutex_;
    std::unordered_map<std::string, std::deque<int64_t>> accessLogs_;
};

} // namespace borrowhub::api
