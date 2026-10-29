#pragma once

#include <chrono>
#include <cstdint>
#include <memory>

namespace borrowhub::util {

class Clock {
public:
    virtual ~Clock() = default;
    [[nodiscard]] virtual int64_t now() const = 0;
};

class SystemClock : public Clock {
public:
    [[nodiscard]] int64_t now() const override {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
};

class FakeClock : public Clock {
public:
    explicit FakeClock(int64_t initialTime = 0) : currentTime_(initialTime) {}

    [[nodiscard]] int64_t now() const override {
        return currentTime_;
    }

    void set(int64_t timestamp) {
        currentTime_ = timestamp;
    }

    void advance(int64_t seconds) {
        currentTime_ += seconds;
    }

private:
    int64_t currentTime_{0};
};

} // namespace borrowhub::util
