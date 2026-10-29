#include "tests/TestCommon.hpp"
#include "src/domain/PenaltyPolicy.hpp"

using namespace borrowhub::domain;

TEST_CASE("PenaltyPolicy enforces ceiling day calculations and deposit caps", "[domain][penalty]") {
    const int64_t due = 100000;
    const Money rate = Money::fromRupees(10); // 1000 paise per day
    const Money cap = Money::fromRupees(50);  // 5000 paise cap

    SECTION("On time return incurs 0 penalty") {
        Money p = PenaltyPolicy::calculatePenalty(due, due - 10, rate, cap);
        REQUIRE(p.paise() == 0);

        Money pExact = PenaltyPolicy::calculatePenalty(due, due, rate, cap);
        REQUIRE(pExact.paise() == 0);
    }

    SECTION("1 second late counts as 1 full day penalty") {
        Money p = PenaltyPolicy::calculatePenalty(due, due + 1, rate, cap);
        REQUIRE(p.paise() == 1000);
    }

    SECTION("Exactly 24 hours late counts as 1 day") {
        Money p = PenaltyPolicy::calculatePenalty(due, due + 86400, rate, cap);
        REQUIRE(p.paise() == 1000);
    }

    SECTION("24 hours + 1 second late counts as 2 days") {
        Money p = PenaltyPolicy::calculatePenalty(due, due + 86401, rate, cap);
        REQUIRE(p.paise() == 2000);
    }

    SECTION("Penalty is strictly capped at maximum specified deposit") {
        // 10 days late would be 10000 paise, but cap is 5000 paise
        Money p = PenaltyPolicy::calculatePenalty(due, due + (10 * 86400), rate, cap);
        REQUIRE(p.paise() == 5000);
    }

    SECTION("Rate of 0 results in 0 penalty") {
        Money zeroRate = Money::fromPaise(0);
        Money p = PenaltyPolicy::calculatePenalty(due, due + 86400, zeroRate, cap);
        REQUIRE(p.paise() == 0);
    }
}
