#include "tests/TestCommon.hpp"
#include "src/domain/Book.hpp"
#include "src/domain/Electronic.hpp"
#include "src/domain/Equipment.hpp"
#include "src/domain/ItemFactory.hpp"
#include "src/util/Errors.hpp"

using namespace borrowhub::domain;
using namespace borrowhub::util;

TEST_CASE("Polymorphic Item subclasses enforce category constraints", "[domain][item]") {
    SECTION("Book enforces 7 max days, 100 INR min deposit, and ISBN format") {
        // Deposit 50 INR (< 100 INR min deposit) must throw
        REQUIRE_THROWS_AS(
            Book(1, 2, "Algorithms", "Academics", "GOOD", "Hostel", Money::fromRupees(50), 0, 0, ItemStatus::AVAILABLE, 100, "Author", "1234567890"),
            AppError
        );

        // Invalid ISBN length must throw
        REQUIRE_THROWS_AS(
            Book(1, 2, "Algorithms", "Academics", "GOOD", "Hostel", Money::fromRupees(150), 0, 0, ItemStatus::AVAILABLE, 100, "Author", "123"),
            AppError
        );

        Book validBook(1, 2, "Algorithms", "Academics", "GOOD", "Hostel", Money::fromRupees(150), 0, 0, ItemStatus::AVAILABLE, 100, "Author", "1234567890");
        Item* itemPtr = &validBook;

        REQUIRE(itemPtr->maxBorrowDays() == 7);
        REQUIRE(itemPtr->penaltyPerDay() == Money::fromRupees(5));
        REQUIRE(itemPtr->minDeposit() == Money::fromRupees(100));
        REQUIRE(itemPtr->typeName() == "BOOK");
    }

    SECTION("Electronic enforces 3 max days and 500 INR min deposit") {
        REQUIRE_THROWS_AS(
            Electronic(2, 3, "Arduino", "Robotics", "GOOD", "Lab", Money::fromRupees(400), 0, 0, ItemStatus::AVAILABLE, 100, "Arduino", "Mega"),
            AppError
        );

        Electronic validElec(2, 3, "Arduino", "Robotics", "GOOD", "Lab", Money::fromRupees(600), 0, 0, ItemStatus::AVAILABLE, 100, "Arduino", "Mega");
        Item* itemPtr = &validElec;

        REQUIRE(itemPtr->maxBorrowDays() == 3);
        REQUIRE(itemPtr->penaltyPerDay() == Money::fromRupees(50));
        REQUIRE(itemPtr->minDeposit() == Money::fromRupees(500));
        REQUIRE(itemPtr->typeName() == "ELECTRONIC");
    }

    SECTION("Equipment enforces 5 max days and 200 INR min deposit") {
        REQUIRE_THROWS_AS(
            Equipment(3, 4, "Multimeter", "EE", "GOOD", "Lab", Money::fromRupees(150), 0, 0, ItemStatus::AVAILABLE, 100, "Multimeter", "Fluke"),
            AppError
        );

        Equipment validEquip(3, 4, "Multimeter", "EE", "GOOD", "Lab", Money::fromRupees(250), 0, 0, ItemStatus::AVAILABLE, 100, "Multimeter", "Fluke");
        Item* itemPtr = &validEquip;

        REQUIRE(itemPtr->maxBorrowDays() == 5);
        REQUIRE(itemPtr->penaltyPerDay() == Money::fromRupees(20));
        REQUIRE(itemPtr->minDeposit() == Money::fromRupees(200));
        REQUIRE(itemPtr->typeName() == "EQUIPMENT");
    }
}

TEST_CASE("ItemFactory dynamically constructs appropriate subclasses", "[domain][item]") {
    auto book = ItemFactory::create(
        1, 2, "BOOK", "Title", "Cat", "GOOD", "Loc",
        Money::fromRupees(100), 0, 0, ItemStatus::AVAILABLE, 100,
        "{\"author\":\"Author\",\"isbn\":\"1234567890\"}"
    );
    REQUIRE(book->typeName() == "BOOK");

    auto elec = ItemFactory::create(
        2, 2, "ELECTRONIC", "Title", "Cat", "GOOD", "Loc",
        Money::fromRupees(500), 0, 0, ItemStatus::AVAILABLE, 100,
        "{\"brand\":\"Brand\",\"model\":\"Model\"}"
    );
    REQUIRE(elec->typeName() == "ELECTRONIC");

    // Unknown type must throw
    REQUIRE_THROWS_AS(
        ItemFactory::create(3, 2, "UNKNOWN", "Title", "Cat", "GOOD", "Loc", Money::fromRupees(100), 0, 0, ItemStatus::AVAILABLE, 100, "{}"),
        AppError
    );
}
