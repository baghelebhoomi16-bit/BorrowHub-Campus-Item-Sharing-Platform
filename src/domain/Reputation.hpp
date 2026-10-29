#pragma once

#include <vector>
#include <cstdint>

namespace borrowhub::domain {

class Reputation {
public:
    // Computes reputation score (0.0 to 100.0) based on Bayesian rating score and Laplace-smoothed rates
    [[nodiscard]] static double calculate(
        const std::vector<int>& stars,
        int onTimeCount,
        int damageFreeCount,
        int totalTransactions);
};

} // namespace borrowhub::domain
