#pragma once

#include "src/domain/Money.hpp"
#include <cstdint>

namespace borrowhub::domain {

class PenaltyPolicy {
public:
    // Calculates late penalty in paise
    // If returnedAt <= dueAt, penalty is 0
    // Otherwise, days = ceil((returnedAt - dueAt) / 86400.0)
    // Result is capped at cap
    [[nodiscard]] static Money calculatePenalty(
        int64_t dueAt,
        int64_t returnedAt,
        Money perDayRate,
        Money cap);
};

} // namespace borrowhub::domain
