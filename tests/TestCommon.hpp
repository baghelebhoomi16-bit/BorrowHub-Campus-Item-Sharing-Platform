#pragma once

#if __has_include(<catch2/catch_test_macros.hpp>) && !defined(BH_STANDALONE_TEST_RUNNER)
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#else

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cstdlib>

namespace borrowhub::test {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

inline std::vector<TestCase>& getRegistry() {
    static std::vector<TestCase> registry;
    return registry;
}

inline bool registerTest(const std::string& name, std::function<void()> func) {
    getRegistry().push_back({name, func});
    return true;
}

} // namespace borrowhub::test

#define BH_CONCAT_INNER(a, b) a##b
#define BH_CONCAT(a, b) BH_CONCAT_INNER(a, b)

#define TEST_CASE(name, ...) \
    void BH_CONCAT(TestFunc_, __LINE__)(); \
    static bool BH_CONCAT(TestReg_, __LINE__) = ::borrowhub::test::registerTest(name, BH_CONCAT(TestFunc_, __LINE__)); \
    void BH_CONCAT(TestFunc_, __LINE__)()

#define SECTION(name)

#define REQUIRE(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "FAILED REQUIRE: " #expr << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            std::exit(1); \
        } \
    } while(0)

#define CHECK(expr) REQUIRE(expr)

#define REQUIRE_THROWS_AS(expr, exceptionType) \
    do { \
        bool caught = false; \
        try { \
            expr; \
        } catch (const exceptionType&) { \
            caught = true; \
        } catch (...) {} \
        if (!caught) { \
            std::cerr << "FAILED REQUIRE_THROWS_AS(" #expr ", " #exceptionType ") at " << __FILE__ << ":" << __LINE__ << "\n"; \
            std::exit(1); \
        } \
    } while(0)

#define REQUIRE_NOTHROW(expr) \
    do { \
        try { \
            expr; \
        } catch (const std::exception& e) { \
            std::cerr << "FAILED REQUIRE_NOTHROW(" #expr "): caught " << e.what() << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            std::exit(1); \
        } catch (...) { \
            std::cerr << "FAILED REQUIRE_NOTHROW(" #expr "): caught unknown exception at " << __FILE__ << ":" << __LINE__ << "\n"; \
            std::exit(1); \
        } \
    } while(0)

#endif
