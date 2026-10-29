#include "tests/TestCommon.hpp"
#include "src/domain/Student.hpp"
#include "src/domain/Admin.hpp"

using namespace borrowhub::domain;

TEST_CASE("Student borrowing eligibility adheres to reputation and verification rules", "[domain][user]") {
    SECTION("Unverified student cannot request") {
        Student s(1, "s@campus.edu", "hash", "Student", "", "PRN01", "CS", 2, "A", 62.0, false, false, 100);
        REQUIRE(!s.canRequest());
    }

    SECTION("Blocked student cannot request") {
        Student s(2, "s@campus.edu", "hash", "Student", "", "PRN02", "CS", 2, "A", 62.0, true, true, 100);
        REQUIRE(!s.canRequest());
    }

    SECTION("Student with low reputation (< 30) after >= 3 transactions is prohibited") {
        Student s(3, "s@campus.edu", "hash", "Student", "", "PRN03", "CS", 2, "A", 29.0, true, false, 100, 3, 1, 1);
        REQUIRE(!s.canRequest());
    }

    SECTION("Student with low reputation (< 30) but only 2 transactions is allowed grace period") {
        Student s(4, "s@campus.edu", "hash", "Student", "", "PRN04", "CS", 2, "A", 29.0, true, false, 100, 2, 1, 1);
        REQUIRE(s.canRequest());
    }

    SECTION("Normal verified student can request") {
        Student s(5, "s@campus.edu", "hash", "Student", "", "PRN05", "CS", 2, "A", 62.0, true, false, 100);
        REQUIRE(s.canRequest());
    }
}

TEST_CASE("Admin role attributes and permissions", "[domain][user]") {
    Admin a(10, "admin@campus.edu", "hash", "Admin", "", 100);
    REQUIRE(a.role() == UserRole::ADMIN);
    // Admins moderate and resolve disputes; they cannot borrow items
    REQUIRE(!a.canRequest());
}
