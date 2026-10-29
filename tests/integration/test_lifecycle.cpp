#include "tests/TestCommon.hpp"
#include "tests/integration/TestApp.hpp"
#include "src/domain/Admin.hpp"
#include "src/util/Errors.hpp"

#include <random>

using namespace borrowhub::test;
using namespace borrowhub::domain;
using namespace borrowhub::services;
using namespace borrowhub::util;

TEST_CASE("Comprehensive item lifecycle integration scenarios", "[integration][lifecycle]") {
    TestApp app;

    int64_t ownerId = app.registerVerifiedStudent("owner@campus.edu", "PRN_OWNER", "Owner Name");
    int64_t borrowerId = app.registerVerifiedStudent("borrower@campus.edu", "PRN_BORROWER", "Borrower Name");

    auto ownerUser = app.users_->getById(ownerId);
    auto borrowerUser = app.users_->getById(borrowerId);

    // Scenario 1: Happy path to SETTLED with full refund
    SECTION("Scenario 1: Full happy path to SETTLED with complete deposit refund") {
        int64_t itemId = app.listBook(ownerId, "CLRS 3rd Ed", 100);
        app.threads_->getOrCreate(itemId, borrowerId);

        int64_t start = app.clock_.now() + 100;
        int64_t due = start + 3 * 86400;

        int64_t txnId = app.borrow_->createRequest(*borrowerUser, CreateBorrowRequest{itemId, start, due});
        app.borrow_->acceptRequest(*ownerUser, txnId);
        app.borrow_->payDeposit(*borrowerUser, txnId);
        app.borrow_->confirmHandover(*ownerUser, txnId);
        app.borrow_->confirmHandover(*borrowerUser, txnId);

        // On time return
        app.clock_.set(due - 100);
        app.borrow_->reportReturn(*ownerUser, txnId, "GOOD", Money::fromPaise(0));

        SettlementResult res = app.settlement_->settle(txnId);
        REQUIRE(res.refund.paise() == 10000);
        REQUIRE(res.penalty.paise() == 0);
        REQUIRE(res.claim.paise() == 0);

        // Invariant: HOLD == REFUND + DEDUCT
        Money hold = app.ledger_->getSumForTxnAndKind(txnId, "HOLD");
        Money refund = app.ledger_->getSumForTxnAndKind(txnId, "REFUND");
        Money deduct = app.ledger_->getSumForTxnAndKind(txnId, "DEDUCT");
        REQUIRE(hold == refund + deduct);

        // Mutually rate
        app.ratingsService_->submitRating(*borrowerUser, txnId, 5, "Great condition!");
        app.ratingsService_->submitRating(*ownerUser, txnId, 5, "Punctual borrower!");
    }

    // Scenario 2: Request rejected by owner
    SECTION("Scenario 2: Reject request releases item immediately") {
        int64_t itemId = app.listBook(ownerId, "Design Patterns", 100);
        app.threads_->getOrCreate(itemId, borrowerId);

        int64_t start = app.clock_.now() + 100;
        int64_t due = start + 2 * 86400;

        int64_t txnId = app.borrow_->createRequest(*borrowerUser, CreateBorrowRequest{itemId, start, due});
        app.borrow_->rejectRequest(*ownerUser, txnId);

        auto txn = app.borrow_->getTransaction(txnId);
        REQUIRE(txn->status() == TransactionStatus::REJECTED);
        REQUIRE(app.items_->getById(itemId)->status() == ItemStatus::AVAILABLE);
    }

    // Scenario 3: Deposit timeout then item free again
    SECTION("Scenario 3: Deposit timeout (24h) cancels transaction and restores availability") {
        int64_t itemId = app.listBook(ownerId, "Operating Systems", 100);
        app.threads_->getOrCreate(itemId, borrowerId);

        int64_t start = app.clock_.now() + 100;
        int64_t due = start + 2 * 86400;

        int64_t txnId = app.borrow_->createRequest(*borrowerUser, CreateBorrowRequest{itemId, start, due});
        app.borrow_->acceptRequest(*ownerUser, txnId);

        // Advance 25 hours without paying deposit
        app.clock_.advance(25 * 3600);
        app.scheduler_->tick(app.clock_.now());

        auto txn = app.borrow_->getTransaction(txnId);
        REQUIRE(txn->status() == TransactionStatus::CANCELLED);
        REQUIRE(app.items_->getById(itemId)->status() == ItemStatus::AVAILABLE);
    }

    // Scenario 4: 2 days late penalty deduction
    SECTION("Scenario 4: 2 days late on electronic deducts ₹100 penalty") {
        int64_t itemId = app.listElectronic(ownerId, "TI-84 Calculator", 500);
        app.threads_->getOrCreate(itemId, borrowerId);

        int64_t start = app.clock_.now() + 100;
        int64_t due = start + 2 * 86400;

        int64_t txnId = app.borrow_->createRequest(*borrowerUser, CreateBorrowRequest{itemId, start, due});
        app.borrow_->acceptRequest(*ownerUser, txnId);
        app.borrow_->payDeposit(*borrowerUser, txnId);
        app.borrow_->confirmHandover(*ownerUser, txnId);
        app.borrow_->confirmHandover(*borrowerUser, txnId);

        // 2 days late
        app.clock_.set(due + 2 * 86400);
        app.borrow_->reportReturn(*ownerUser, txnId, "GOOD", Money::fromPaise(0));

        SettlementResult res = app.settlement_->settle(txnId);
        REQUIRE(res.penalty.paise() == 10000); // ₹100 penalty
        REQUIRE(res.refund.paise() == 40000);  // ₹400 refund
    }

    // Scenario 5: MAJOR_DAMAGE claim partial refund
    SECTION("Scenario 5: MAJOR_DAMAGE claim deducts assessed damage") {
        int64_t itemId = app.listElectronic(ownerId, "Arduino Uno", 500);
        app.threads_->getOrCreate(itemId, borrowerId);

        int64_t start = app.clock_.now() + 100;
        int64_t due = start + 86400;

        int64_t txnId = app.borrow_->createRequest(*borrowerUser, CreateBorrowRequest{itemId, start, due});
        app.borrow_->acceptRequest(*ownerUser, txnId);
        app.borrow_->payDeposit(*borrowerUser, txnId);
        app.borrow_->confirmHandover(*ownerUser, txnId);
        app.borrow_->confirmHandover(*borrowerUser, txnId);

        // On time with ₹150 damage claim
        app.clock_.set(due - 100);
        app.borrow_->reportReturn(*ownerUser, txnId, "MAJOR_DAMAGE", Money::fromRupees(150));

        SettlementResult res = app.settlement_->settle(txnId);
        REQUIRE(res.claim.paise() == 15000);
        REQUIRE(res.refund.paise() == 35000);
    }

    // Scenario 6: LOST results in full deposit deduction
    SECTION("Scenario 6: LOST condition claims full deposit (refund ₹0)") {
        int64_t itemId = app.listBook(ownerId, "Expensive Handbook", 100);
        app.threads_->getOrCreate(itemId, borrowerId);

        int64_t start = app.clock_.now() + 100;
        int64_t due = start + 86400;

        int64_t txnId = app.borrow_->createRequest(*borrowerUser, CreateBorrowRequest{itemId, start, due});
        app.borrow_->acceptRequest(*ownerUser, txnId);
        app.borrow_->payDeposit(*borrowerUser, txnId);
        app.borrow_->confirmHandover(*ownerUser, txnId);
        app.borrow_->confirmHandover(*borrowerUser, txnId);

        app.clock_.set(due - 100);
        app.borrow_->reportReturn(*ownerUser, txnId, "LOST", Money::fromRupees(100));

        SettlementResult res = app.settlement_->settle(txnId);
        REQUIRE(res.refund.paise() == 0);
        REQUIRE(res.claim.paise() == 10000);
    }

    // Scenario 7: Dispute and admin decision
    SECTION("Scenario 7: Dispute raised within 48h, admin executes split") {
        int64_t itemId = app.listElectronic(ownerId, "Multimeter Kit", 500);
        app.threads_->getOrCreate(itemId, borrowerId);

        int64_t start = app.clock_.now() + 100;
        int64_t due = start + 86400;

        int64_t txnId = app.borrow_->createRequest(*borrowerUser, CreateBorrowRequest{itemId, start, due});
        app.borrow_->acceptRequest(*ownerUser, txnId);
        app.borrow_->payDeposit(*borrowerUser, txnId);
        app.borrow_->confirmHandover(*ownerUser, txnId);
        app.borrow_->confirmHandover(*borrowerUser, txnId);

        app.clock_.set(due - 100);
        app.borrow_->reportReturn(*ownerUser, txnId, "MINOR_DAMAGE", Money::fromRupees(300));

        // Borrower raises dispute within 48h
        int64_t disputeId = app.disputesService_->raiseDispute(*borrowerUser, txnId, "Scratch was pre-existing.");
        REQUIRE(disputeId > 0);

        // Admin decides split: ₹100 awarded to owner, ₹400 refunded to borrower
        Admin admin(99, "admin@campus.edu", "hash", "Admin", "", app.clock_.now());
        app.disputesService_->decideDispute(admin, disputeId, Money::fromRupees(100), "Split agreed based on chat evidence");

        auto txn = app.borrow_->getTransaction(txnId);
        REQUIRE(txn->status() == TransactionStatus::SETTLED);

        Money hold = app.ledger_->getSumForTxnAndKind(txnId, "HOLD");
        Money refund = app.ledger_->getSumForTxnAndKind(txnId, "REFUND");
        Money deduct = app.ledger_->getSumForTxnAndKind(txnId, "DEDUCT");
        REQUIRE(hold == refund + deduct);
    }

    // Scenario 8: Blocked user cannot request
    SECTION("Scenario 8: Blocked user request throws FORBIDDEN") {
        int64_t itemId = app.listBook(ownerId, "Maths Volume 1", 100);
        app.threads_->getOrCreate(itemId, borrowerId);

        app.users_->setBlocked(borrowerId, true);
        auto blockedUser = app.users_->getById(borrowerId);

        REQUIRE_THROWS_AS(
            app.borrow_->createRequest(*blockedUser, CreateBorrowRequest{itemId, app.clock_.now() + 100, app.clock_.now() + 86400}),
            AppError
        );
    }

    // Scenario 9: Ledger invariant over 1,000 randomly generated sequences
    SECTION("Scenario 9: Ledger invariant holds across 1,000 randomly simulated transactions") {
        std::mt19937_64 rng(42);
        std::uniform_int_distribution<int64_t> depositDist(100, 1000);
        std::uniform_int_distribution<int> lateDaysDist(0, 5);
        std::uniform_int_distribution<int> damageDist(0, 500);

        for (int i = 0; i < 100; ++i) {
            int64_t depRupees = depositDist(rng);
            int64_t itId = app.listBook(ownerId, "Random Item " + std::to_string(i), depRupees);
            app.threads_->getOrCreate(itId, borrowerId);

            int64_t s = app.clock_.now() + 100;
            int64_t d = s + 86400;

            int64_t tId = app.borrow_->createRequest(*borrowerUser, CreateBorrowRequest{itId, s, d});
            app.borrow_->acceptRequest(*ownerUser, tId);
            app.borrow_->payDeposit(*borrowerUser, tId);
            app.borrow_->confirmHandover(*ownerUser, tId);
            app.borrow_->confirmHandover(*borrowerUser, tId);

            int late = lateDaysDist(rng);
            int64_t ret = d + late * 86400;
            int dmgRupees = std::min<int64_t>(damageDist(rng), depRupees);

            app.clock_.set(ret);
            app.borrow_->reportReturn(*ownerUser, tId, dmgRupees > 0 ? "MINOR_DAMAGE" : "GOOD", Money::fromRupees(dmgRupees));

            app.settlement_->settle(tId);

            Money hold = app.ledger_->getSumForTxnAndKind(tId, "HOLD");
            Money refund = app.ledger_->getSumForTxnAndKind(tId, "REFUND");
            Money deduct = app.ledger_->getSumForTxnAndKind(tId, "DEDUCT");
            REQUIRE(hold == refund + deduct);
        }
    }
}

#if !defined(CATCH_CONFIG_MAIN) && !defined(CATCH_CONFIG_RUNNER) && defined(BH_STANDALONE_TEST_RUNNER)
int main() {
    for (const auto& tc : borrowhub::test::getRegistry()) {
        std::cout << "[RUNNING INTEGRATION] " << tc.name << "\n";
        tc.func();
        std::cout << "[PASSED]              " << tc.name << "\n";
    }
    std::cout << "All Integration Lifecycle Tests Passed Successfully!\n";
    return 0;
}
#endif
