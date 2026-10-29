#include "src/domain/PenaltyPolicy.hpp"
#include <algorithm>
#include <cmath>

namespace borrowhub::domain {

Money PenaltyPolicy::calculatePenalty(
    int64_t dueAt,
    int64_t returnedAt,
    Money perDayRate,
    Money cap) {

    if (returnedAt <= dueAt || perDayRate.paise() == 0) {
        return Money::fromPaise(0);
    }

    int64_t diffSeconds = returnedAt - dueAt;
    // Ceil division by 86400 (seconds per day):
    // ceil(diffSeconds / 86400) = (diffSeconds + 86399) / 86400
    int64_t daysLate = (diffSeconds + 86399) / 86400;

    int64_t penaltyPaise = daysLate * perDayRate.paise();
    int64_t cappedPaise = std::min(penaltyPaise, cap.paise());

    return Money::fromPaise(cappedPaise);
}

} // namespace borrowhub::domain
