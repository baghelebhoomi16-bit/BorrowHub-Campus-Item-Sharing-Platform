#include "tests/TestCommon.hpp"
#include "src/domain/BorrowTransaction.hpp"
#include "src/util/Errors.hpp"

using namespace borrowhub::domain;
using namespace borrowhub::util;

TEST_CASE("BorrowTransaction state machine enforces strict lifecycle transitions", "[domain][transaction]") {
    const int64_t ownerId = 10;
    const int64_t borrowerId = 20;
    const int64_t strangerId = 99;
    const int64_t start = 1000;
    const int64_t due = 5000;
    const Money deposit = Money::fromRupees(100);

    BorrowTransaction txn(1, 100, 1, ownerId, borrowerId, start, due, deposit, start, start);

    SECTION("Illegal actor authorizations throw FORBIDDEN") {
        REQUIRE_THROWS_AS(txn.accept(strangerId, start + 10), AppError);
        REQUIRE_THROWS_AS(txn.accept(borrowerId, start + 10), AppError);
        REQUIRE_THROWS_AS(txn.reject(borrowerId), AppError);
    }

    SECTION("Happy path from REQUESTED to SETTLED") {
        int64_t t = start + 100;
        // 1. Owner accepts
        txn.accept(ownerId, t);
        REQUIRE(txn.status() == TransactionStatus::ACCEPTED);

        // 2. Borrower pays deposit
        t += 3600; // 1 hour later
        txn.payDeposit(borrowerId, t);
        REQUIRE(txn.isDepositHeld());

        // 3. Handover requires dual confirmation
        txn.confirmHandover(borrowerId);
        REQUIRE(txn.status() == TransactionStatus::ACCEPTED); // Still accepted, waiting for owner

        txn.confirmHandover(ownerId);
        REQUIRE(txn.status() == TransactionStatus::ACTIVE);   // Both confirmed, now ACTIVE

        // 4. Overdue check before due date throws
        REQUIRE_THROWS_AS(txn.markOverdue(due - 10), AppError);

        // 5. Owner reports return on time
        t = due - 100;
        txn.reportReturn(ownerId, t, "GOOD", Money::fromPaise(0));
        REQUIRE(txn.status() == TransactionStatus::RETURNED);

        // 6. Settle
        txn.settle();
        REQUIRE(txn.status() == TransactionStatus::SETTLED);
    }

    SECTION("Deposit after 24 hours (86,400s) timeout throws and cancels") {
        txn.accept(ownerId, start);
        int64_t lateTime = start + 86401;
        REQUIRE_THROWS_AS(txn.payDeposit(borrowerId, lateTime), AppError);
    }

    SECTION("Claim above deposit is rejected") {
        txn.accept(ownerId, start);
        txn.payDeposit(borrowerId, start + 10);
        txn.confirmHandover(ownerId);
        txn.confirmHandover(borrowerId);

        REQUIRE_THROWS_AS(
            txn.reportReturn(ownerId, due, "MAJOR_DAMAGE", Money::fromRupees(200)),
            AppError
        );
    }

    SECTION("Dispute raised after 48 hours (172,800s) throws") {
        txn.accept(ownerId, start);
        txn.payDeposit(borrowerId, start + 10);
        txn.confirmHandover(ownerId);
        txn.confirmHandover(borrowerId);
        txn.reportReturn(ownerId, due, "MINOR_DAMAGE", Money::fromRupees(20));

        int64_t lateDispute = due + 172801;
        REQUIRE_THROWS_AS(txn.raiseDispute(borrowerId, lateDispute), AppError);

        // Within 48 hours succeeds
        REQUIRE_NOTHROW(txn.raiseDispute(borrowerId, due + 3600));
        REQUIRE(txn.status() == TransactionStatus::DISPUTED);
    }
}
