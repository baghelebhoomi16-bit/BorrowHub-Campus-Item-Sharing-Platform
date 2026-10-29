#include "tests/TestCommon.hpp"
#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/domain/Book.hpp"
#include "src/domain/Student.hpp"
#include "src/services/BorrowService.hpp"
#include "src/services/DisputeService.hpp"
#include "src/services/EventBus.hpp"
#include "src/services/NotificationService.hpp"
#include "src/services/Scheduler.hpp"
#include "src/services/SettlementService.hpp"
#include "src/util/Clock.hpp"

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

TEST_CASE("Scheduler background ticks handle time-based state transitions", "[service][scheduler]") {
    Database db(":memory:");
    initSchema(db);

    auto users = makeUserRepo(db);
    auto items = makeItemRepo(db);
    auto threads = makeThreadRepo(db);
    auto txns = makeTxnRepo(db);
    auto ledger = makeLedgerRepo(db);
    auto disputes = makeDisputeRepo(db);
    auto sessions = makeSessionRepo(db);
    auto verifyTokens = makeVerificationTokenRepo(db);
    auto notifs = makeNotificationRepo(db);

    FakeClock clock(1730000000);
    EventBus eventBus;

    NotificationService notificationService(db, *notifs, clock, eventBus);
    SettlementService settlement(db, *txns, *items, *ledger, *disputes, clock, eventBus);
    BorrowService borrow(db, *txns, *items, *threads, *ledger, clock, eventBus);
    DisputeService disputeService(db, *disputes, *txns, *items, *ledger, clock, eventBus);
    Scheduler scheduler(db, *txns, *items, *sessions, *verifyTokens, settlement, clock, eventBus);

    Student owner(1, "owner@campus.edu", "hash", "Owner", "", "PRN01", "CS", 3, "A", 62.0, true, false, clock.now());
    int64_t ownerId = users->insert(owner, "PRN01", "CS", 3, "A");
    auto ownerUser = users->getById(ownerId);

    Student borrower(2, "borrower@campus.edu", "hash", "Borrower", "", "PRN02", "CS", 2, "B", 62.0, true, false, clock.now());
    int64_t borrowerId = users->insert(borrower, "PRN02", "CS", 2, "B");
    auto borrowerUser = users->getById(borrowerId);

    Book book(0, ownerId, "Title", "Cat", "GOOD", "Loc", Money::fromRupees(100),
              clock.now(), clock.now() + 30 * 86400, ItemStatus::AVAILABLE, clock.now(), "Author", "1234567890");
    int64_t itemId = items->insert(book);
    threads->getOrCreate(itemId, borrowerId);

    SECTION("ACCEPTED request with no deposit cancels after 24h and frees item") {
        int64_t start = clock.now() + 3600;
        int64_t due = start + 86400;
        int64_t txnId = borrow.createRequest(*borrowerUser, CreateBorrowRequest{itemId, start, due});
        borrow.acceptRequest(*ownerUser, txnId);

        // Advance 23h 59m: request should remain ACCEPTED
        clock.advance(86400 - 60);
        scheduler.tick(clock.now());

        auto t = borrow.getTransaction(txnId);
        REQUIRE(t->status() == TransactionStatus::ACCEPTED);
        REQUIRE(items->getById(itemId)->status() == ItemStatus::RESERVED);

        // Advance past 24h (2 minutes later)
        clock.advance(120);
        scheduler.tick(clock.now());

        t = borrow.getTransaction(txnId);
        REQUIRE(t->status() == TransactionStatus::CANCELLED);
        REQUIRE(items->getById(itemId)->status() == ItemStatus::AVAILABLE);
    }

    SECTION("ACTIVE transaction past due becomes OVERDUE and tick is idempotent") {
        int64_t start = clock.now() + 100;
        int64_t due = start + 86400;
        int64_t txnId = borrow.createRequest(*borrowerUser, CreateBorrowRequest{itemId, start, due});
        borrow.acceptRequest(*ownerUser, txnId);
        borrow.payDeposit(*borrowerUser, txnId);
        borrow.confirmHandover(*ownerUser, txnId);
        borrow.confirmHandover(*borrowerUser, txnId);

        // Advance clock past due date
        clock.set(due + 100);
        scheduler.tick(clock.now());

        auto t = borrow.getTransaction(txnId);
        REQUIRE(t->status() == TransactionStatus::OVERDUE);

        // Second tick is completely idempotent and does not error
        scheduler.tick(clock.now());
        REQUIRE(borrow.getTransaction(txnId)->status() == TransactionStatus::OVERDUE);
    }

    SECTION("RETURNED transaction auto-settles after 48h unless disputed") {
        int64_t start = clock.now() + 100;
        int64_t due = start + 86400;
        int64_t txnId = borrow.createRequest(*borrowerUser, CreateBorrowRequest{itemId, start, due});
        borrow.acceptRequest(*ownerUser, txnId);
        borrow.payDeposit(*borrowerUser, txnId);
        borrow.confirmHandover(*ownerUser, txnId);
        borrow.confirmHandover(*borrowerUser, txnId);

        clock.set(due - 100);
        borrow.reportReturn(*ownerUser, txnId, "GOOD", Money::fromPaise(0));

        // Advance 47 hours: remains RETURNED
        clock.advance(47 * 3600);
        scheduler.tick(clock.now());
        REQUIRE(borrow.getTransaction(txnId)->status() == TransactionStatus::RETURNED);

        // Advance past 48 hours: auto-settles to SETTLED
        clock.advance(2 * 3600);
        scheduler.tick(clock.now());
        REQUIRE(borrow.getTransaction(txnId)->status() == TransactionStatus::SETTLED);
    }
}
