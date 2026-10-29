#include "tests/TestCommon.hpp"
#include "src/domain/Reputation.hpp"

#include <cmath>
#include <vector>

using namespace borrowhub::domain;

TEST_CASE("Reputation formula adheres to Bayesian rating average and Laplace smoothing", "[domain][reputation]") {
    SECTION("Brand new user scores exactly 62.0") {
        std::vector<int> stars{};
        double score = Reputation::calculate(stars, 0, 0, 0);
        REQUIRE(std::abs(score - 62.0) < 0.001);
    }

    SECTION("One 5-star rating with 1 clean on-time transaction gives expected hand-computed value (~71.67)") {
        std::vector<int> stars{5};
        double score = Reputation::calculate(stars, 1, 1, 1);
        // Hand-calculated: 0.6 * 75.0 + 0.4 * (66.6667) = 45 + 26.6667 = 71.6667
        REQUIRE(std::abs(score - 71.6667) < 0.01);
    }

    SECTION("Many 1-star ratings approach lowest bound but score remains bounded in [0, 100]") {
        std::vector<int> stars(50, 1);
        double score = Reputation::calculate(stars, 0, 0, 50);
        REQUIRE(score >= 0.0);
        REQUIRE(score <= 100.0);
        // With 50 1-star ratings and 0 on-time txns, score drops below 30
        REQUIRE(score < 30.0);
    }
}
