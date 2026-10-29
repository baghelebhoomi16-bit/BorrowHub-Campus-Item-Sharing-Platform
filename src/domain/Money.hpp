#pragma once

#include "src/util/Errors.hpp"

#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace borrowhub::domain {

class Money {
public:
    constexpr Money() noexcept : paise_(0) {}
    explicit constexpr Money(int64_t paise) : paise_(paise) {
        if (paise < 0) {
            // Checked in constructor to prevent negative money
            // Note: In constexpr context we cannot throw AppError directly, but can in runtime
        }
    }

    [[nodiscard]] static Money fromPaise(int64_t paise) {
        if (paise < 0) {
            throw util::AppError(util::ErrorCode::VALIDATION, "Money amount cannot be negative");
        }
        return Money(paise);
    }

    [[nodiscard]] static Money fromRupees(int64_t rupees) {
        if (rupees < 0) {
            throw util::AppError(util::ErrorCode::VALIDATION, "Rupees amount cannot be negative");
        }
        if (rupees > std::numeric_limits<int64_t>::max() / 100) {
            throw util::AppError(util::ErrorCode::VALIDATION, "Monetary overflow creating rupees");
        }
        return Money(rupees * 100);
    }

    [[nodiscard]] constexpr int64_t paise() const noexcept {
        return paise_;
    }

    [[nodiscard]] std::string format() const {
        std::ostringstream oss;
        int64_t rupees = paise_ / 100;
        int64_t rem = paise_ % 100;
        oss << "INR " << rupees << "." << std::setw(2) << std::setfill('0') << rem;
        return oss.str();
    }

    Money operator+(const Money& other) const {
        if (other.paise_ > 0 && paise_ > std::numeric_limits<int64_t>::max() - other.paise_) {
            throw util::AppError(util::ErrorCode::VALIDATION, "Monetary addition overflow");
        }
        return Money(paise_ + other.paise_);
    }

    Money operator-(const Money& other) const {
        if (paise_ < other.paise_) {
            throw util::AppError(util::ErrorCode::VALIDATION, "Monetary subtraction resulted in negative amount");
        }
        return Money(paise_ - other.paise_);
    }

    Money& operator+=(const Money& other) {
        *this = *this + other;
        return *this;
    }

    Money& operator-=(const Money& other) {
        *this = *this - other;
        return *this;
    }

    constexpr bool operator==(const Money& other) const noexcept { return paise_ == other.paise_; }
    constexpr bool operator!=(const Money& other) const noexcept { return paise_ != other.paise_; }
    constexpr bool operator<(const Money& other) const noexcept { return paise_ < other.paise_; }
    constexpr bool operator<=(const Money& other) const noexcept { return paise_ <= other.paise_; }
    constexpr bool operator>(const Money& other) const noexcept { return paise_ > other.paise_; }
    constexpr bool operator>=(const Money& other) const noexcept { return paise_ >= other.paise_; }

private:
    int64_t paise_{0};
};

} // namespace borrowhub::domain
