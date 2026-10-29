#include "src/domain/Reputation.hpp"
#include <algorithm>
#include <numeric>

namespace borrowhub::domain {

double Reputation::calculate(
    const std::vector<int>& stars,
    int onTimeCount,
    int damageFreeCount,
    int totalTransactions) {

    // 1. Bayesian average for ratings with prior mean 3.5 and weight 5
    // R = (5 * 3.5 + sum(stars)) / (5 + len(stars))
    double starSum = std::accumulate(stars.begin(), stars.end(), 0.0);
    double r = (5.0 * 3.5 + starSum) / (5.0 + static_cast<double>(stars.size()));
    double ratingScore = (r / 5.0) * 100.0;

    // 2. Objective rate metrics using Laplace smoothing: (successes + 1) / (total + 2)
    double onTimeRate = static_cast<double>(onTimeCount + 1) / static_cast<double>(totalTransactions + 2);
    double dfRate = static_cast<double>(damageFreeCount + 1) / static_cast<double>(totalTransactions + 2);

    // 3. Composite score formula: 0.6 * rating + 0.3 * onTime + 0.1 * damageFree
    double score = 0.6 * ratingScore + 0.3 * (onTimeRate * 100.0) + 0.1 * (dfRate * 100.0);

    // Bound between 0.0 and 100.0
    return std::clamp(score, 0.0, 100.0);
}

} // namespace borrowhub::domain
