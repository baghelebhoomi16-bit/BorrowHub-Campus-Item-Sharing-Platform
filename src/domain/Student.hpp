#pragma once

#include "src/domain/User.hpp"

namespace borrowhub::domain {

class Student : public User {
public:
    Student(int64_t id,
            std::string email,
            std::string passwordHash,
            std::string name,
            std::string phone,
            std::string prn,
            std::string department,
            int year,
            std::string division,
            double reputation,
            bool verified,
            bool blocked,
            int64_t createdAt,
            int totalTransactions = 0,
            int onTimeCount = 0,
            int damageFreeCount = 0);

    [[nodiscard]] const std::string& prn() const noexcept { return prn_; }
    [[nodiscard]] const std::string& department() const noexcept { return department_; }
    [[nodiscard]] int year() const noexcept { return year_; }
    [[nodiscard]] const std::string& division() const noexcept { return division_; }
    [[nodiscard]] double reputation() const noexcept { return reputation_; }
    [[nodiscard]] int totalTransactions() const noexcept { return totalTransactions_; }
    [[nodiscard]] int onTimeCount() const noexcept { return onTimeCount_; }
    [[nodiscard]] int damageFreeCount() const noexcept { return damageFreeCount_; }

    void setReputation(double rep) noexcept { reputation_ = rep; }
    void incrementTransactions() noexcept { totalTransactions_++; }
    void incrementOnTime() noexcept { onTimeCount_++; }
    void incrementDamageFree() noexcept { damageFreeCount_++; }

    [[nodiscard]] UserRole role() const noexcept override { return UserRole::STUDENT; }

    // Business rule: Verified, not blocked, and cannot request if reputation < 30 after at least 3 transactions
    [[nodiscard]] bool canRequest() const noexcept override;

private:
    std::string prn_;
    std::string department_;
    int year_{1};
    std::string division_;
    double reputation_{50.0};
    int totalTransactions_{0};
    int onTimeCount_{0};
    int damageFreeCount_{0};
};

} // namespace borrowhub::domain
