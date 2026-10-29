#include "tests/TestCommon.hpp"
#include "src/domain/Money.hpp"
#include "src/util/Errors.hpp"

#include <limits>

using namespace borrowhub::domain;
using namespace borrowhub::util;

TEST_CASE("Money represents integer paise accurately without float loss", "[domain][money]") {
    Money m = Money::fromPaise(150);
    REQUIRE(m.paise() == 150);
    REQUIRE(m.format() == "INR 1.50");

    Money m2 = Money::fromRupees(15);
    REQUIRE(m2.paise() == 1500);
    REQUIRE(m2.format() == "INR 15.00");
}

TEST_CASE("Money prevents negative values and arithmetic underflow", "[domain][money]") {
    REQUIRE_THROWS_AS(Money::fromPaise(-1), AppError);
    REQUIRE_THROWS_AS(Money::fromRupees(-10), AppError);

    Money a = Money::fromPaise(100);
    Money b = Money::fromPaise(200);
    REQUIRE_THROWS_AS(a - b, AppError);
}

TEST_CASE("Money detects addition overflow", "[domain][money]") {
    Money maxNear = Money::fromPaise(std::numeric_limits<int64_t>::max() - 10);
    Money extra = Money::fromPaise(20);
    REQUIRE_THROWS_AS(maxNear + extra, AppError);
}

TEST_CASE("Money comparison operators work correctly", "[domain][money]") {
    Money low = Money::fromPaise(100);
    Money high = Money::fromPaise(200);
    Money equal = Money::fromPaise(100);

    REQUIRE(low < high);
    REQUIRE(high > low);
    REQUIRE(low <= high);
    REQUIRE(low <= equal);
    REQUIRE(low == equal);
    REQUIRE(low != high);
}

#if !defined(CATCH_CONFIG_MAIN) && !defined(CATCH_CONFIG_RUNNER) && defined(BH_STANDALONE_TEST_RUNNER)
int main() {
    for (const auto& tc : borrowhub::test::getRegistry()) {
        std::cout << "[RUNNING] " << tc.name << "\n";
        tc.func();
        std::cout << "[PASSED]  " << tc.name << "\n";
    }
    std::cout << "All Unit Tests Passed Successfully!\n";
    return 0;
}
#endif
