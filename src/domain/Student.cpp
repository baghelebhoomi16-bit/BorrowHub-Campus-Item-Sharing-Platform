#include "src/domain/Student.hpp"

namespace borrowhub::domain {

Student::Student(int64_t id,
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
                 int totalTransactions,
                 int onTimeCount,
                 int damageFreeCount)
    : User(id, std::move(email), std::move(passwordHash), std::move(name), std::move(phone), verified, blocked, createdAt),
      prn_(std::move(prn)),
      department_(std::move(department)),
      year_(year),
      division_(std::move(division)),
      reputation_(reputation),
      totalTransactions_(totalTransactions),
      onTimeCount_(onTimeCount),
      damageFreeCount_(damageFreeCount) {}

bool Student::canRequest() const noexcept {
    if (blocked_ || !verified_) {
        return false;
    }
    // Blueprint 3.4: "Users under 30 after at least 3 transactions cannot request items."
    if (totalTransactions_ >= 3 && reputation_ < 30.0) {
        return false;
    }
    return true;
}

} // namespace borrowhub::domain
