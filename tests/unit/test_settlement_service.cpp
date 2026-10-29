#include "tests/TestCommon.hpp"
#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/domain/Electronic.hpp"
#include "src/domain/Student.hpp"
#include "src/services/BorrowService.hpp"
#include "src/services/EventBus.hpp"
#include "src/services/SettlementService.hpp"
#include "src/util/Clock.hpp"
#include "src/util/Errors.hpp"

#include <fstream>
#include <sstream>

using namespace borrowhub::services;
using namespace borrowhub::util;
using namespace borrowhub::domain;
using namespace borrowhub::db;

namespace {

void initSchema(Database& db) {
    std::ifstream file("db/schema.sql");
    std::ostringstream ss;
    ss << file.rdbuf();
    db.exec(ss.str());
}

} // namespace

TEST_CASE("SettlementService calculates accurate deductions and preserves ledger invariants", "[service][settlement]") {
    Database db(":memory:");
    initSchema(db);

    auto users = makeUserRepo(db);
    auto items = makeItemRepo(db);
    auto threads = makeThreadRepo(db);
    auto txns = makeTxnRepo(db);
    auto ledger = makeLedgerRepo(db);
    auto disputes = makeDisputeRepo(db);

    FakeClock clock(1730000000);
    EventBus eventBus;

    SettlementService settlement(db, *txns, *items, *ledger, *disputes, clock, eventBus);
    BorrowService borrow(db, *txns, *items, *threads, *ledger, clock, eventBus);

    Student owner(1, "owner@campus.edu", "hash", "Owner", "", "PRN01", "CS", 3, "A", 62.0, true, false, clock.now());
    int64_t ownerId = users->insert(owner, "PRN01", "CS", 3, "A");
    auto ownerUser = users->getById(ownerId);

    Student borrower(2, "borrower@campus.edu", "hash", "Borrower", "", "PRN02", "CS", 2, "B", 62.0, true, false, clock.now());
    int64_t borrowerId = users->insert(borrower, "PRN02", "CS", 2, "B");
    auto borrowerUser = users->getById(borrowerId);

    // List Electronic item: min deposit 500 INR (50000 paise), penalty 50 INR/day (5000 paise/day)
    Electronic elec(0, ownerId, "Arduino Mega", "Robotics", "GOOD", "Lab", Money::fromRupees(500),
                    clock.now(), clock.now() + 30 * 86400, ItemStatus::AVAILABLE, clock.now(), "Arduino", "Mega");
    int64_t itemId = items->insert(elec);
    threads->getOrCreate(itemId, borrowerId);

    int64_t startDate = clock.now() + 3600;
    int64_t dueDate = startDate + 2 * 86400; // 2 days duration

    SECTION("On-time return with GOOD condition grants full refund") {
        int64_t txnId = borrow.createRequest(*borrowerUser, CreateBorrowRequest{itemId, startDate, dueDate});
        borrow.acceptRequest(*ownerUser, txnId);
        borrow.payDeposit(*borrowerUser, txnId);
        borrow.confirmHandover(*ownerUser, txnId);
        borrow.confirmHandover(*borrowerUser, txnId);

        // Advance to due date minus 1 hour (on-time)
        clock.set(dueDate - 3600);
        borrow.reportReturn(*ownerUser, txnId, "GOOD", Money::fromPaise(0));

        SettlementResult res = settlement.settle(txnId);
        REQUIRE(res.penalty.paise() == 0);
        REQUIRE(res.claim.paise() == 0);
        REQUIRE(res.refund.paise() == 50000); // Full ₹500 refunded

        // Check ledger audit entries
        auto entries = ledger->listForTransaction(txnId);
        REQUIRE(entries.size() == 2); // 1 HOLD, 1 REFUND
        REQUIRE(entries[0].kind == "HOLD");
        REQUIRE(entries[1].kind == "REFUND");
        REQUIRE(entries[0].amount == entries[1].amount);
    }

    SECTION("2 days late on Electronic results in 100 INR penalty deduction") {
        int64_t txnId = borrow.createRequest(*borrowerUser, CreateBorrowRequest{itemId, startDate, dueDate});
        borrow.acceptRequest(*ownerUser, txnId);
        borrow.payDeposit(*borrowerUser, txnId);
        borrow.confirmHandover(*ownerUser, txnId);
        borrow.confirmHandover(*borrowerUser, txnId);

        // Advance clock to exactly 2 days late (dueDate + 2*86400)
        clock.set(dueDate + 2 * 86400);
        borrow.reportReturn(*ownerUser, txnId, "GOOD", Money::fromPaise(0));

        SettlementResult res = settlement.settle(txnId);
        // Electronic penalty is ₹50/day * 2 days = ₹100 (10000 paise)
        REQUIRE(res.penalty.paise() == 10000);
        REQUIRE(res.refund.paise() == 40000); // ₹400 refunded

        // Invariant audit: HOLD (50000) == REFUND (40000) + DEDUCT (10000)
        Money holdSum = ledger->getSumForTxnAndKind(txnId, "HOLD");
        Money refundSum = ledger->getSumForTxnAndKind(txnId, "REFUND");
        Money deductSum = ledger->getSumForTxnAndKind(txnId, "DEDUCT");
        REQUIRE(holdSum == refundSum + deductSum);
    }

    SECTION("Settling twice throws ILLEGAL_STATE") {
        int64_t txnId = borrow.createRequest(*borrowerUser, CreateBorrowRequest{itemId, startDate, dueDate});
        borrow.acceptRequest(*ownerUser, txnId);
        borrow.payDeposit(*borrowerUser, txnId);
        borrow.confirmHandover(*ownerUser, txnId);
        borrow.confirmHandover(*borrowerUser, txnId);
        clock.set(dueDate - 100);
        borrow.reportReturn(*ownerUser, txnId, "GOOD", Money::fromPaise(0));

        settlement.settle(txnId);
        // Second attempt must throw
        REQUIRE_THROWS_AS(settlement.settle(txnId), AppError);
    }
}
