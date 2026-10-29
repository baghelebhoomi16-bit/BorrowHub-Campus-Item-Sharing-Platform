#include "tests/TestCommon.hpp"
#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/domain/Book.hpp"
#include "src/domain/Student.hpp"
#include "src/services/BorrowService.hpp"
#include "src/services/EventBus.hpp"
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

TEST_CASE("BorrowService validates chat negotiation requirement and borrow limits", "[service][borrow]") {
    Database db(":memory:");
    initSchema(db);

    auto users = makeUserRepo(db);
    auto items = makeItemRepo(db);
    auto threads = makeThreadRepo(db);
    auto messages = makeMessageRepo(db);
    auto txns = makeTxnRepo(db);
    auto ledger = makeLedgerRepo(db);

    FakeClock clock(1730000000);
    EventBus eventBus;

    BorrowService borrow(db, *txns, *items, *threads, *ledger, clock, eventBus);

    // Setup Owner and Borrower
    Student owner(1, "owner@campus.edu", "hash", "Owner", "", "PRN01", "CS", 3, "A", 62.0, true, false, clock.now());
    int64_t ownerId = users->insert(owner, "PRN01", "CS", 3, "A");

    Student borrower(2, "borrower@campus.edu", "hash", "Borrower", "", "PRN02", "CS", 2, "B", 62.0, true, false, clock.now());
    int64_t borrowerId = users->insert(borrower, "PRN02", "CS", 2, "B");
    auto borrowerUser = users->getById(borrowerId);
    auto ownerUser = users->getById(ownerId);

    // List Book item (7 max days)
    Book book(0, ownerId, "Algorithms CLRS", "Academics", "GOOD", "Hostel", Money::fromRupees(150),
              clock.now(), clock.now() + 30 * 86400, ItemStatus::AVAILABLE, clock.now(), "CLRS", "1234567890");
    int64_t itemId = items->insert(book);

    SECTION("Request without negotiation thread is rejected") {
        CreateBorrowRequest req{itemId, clock.now() + 86400, clock.now() + 3 * 86400};
        REQUIRE_THROWS_AS(borrow.createRequest(*borrowerUser, req), AppError);
    }

    SECTION("Request with duration exceeding item type maxBorrowDays (7 days for Book) is rejected") {
        threads->getOrCreate(itemId, borrowerId);
        // 8 days requested
        CreateBorrowRequest req{itemId, clock.now() + 86400, clock.now() + 9 * 86400};
        REQUIRE_THROWS_AS(borrow.createRequest(*borrowerUser, req), AppError);
    }

    SECTION("Owner cannot borrow own item") {
        threads->getOrCreate(itemId, ownerId);
        CreateBorrowRequest req{itemId, clock.now() + 86400, clock.now() + 3 * 86400};
        REQUIRE_THROWS_AS(borrow.createRequest(*ownerUser, req), AppError);
    }

    SECTION("Accepting one request auto-rejects other pending requests on same item") {
        threads->getOrCreate(itemId, borrowerId);

        // Second borrower
        Student borrower2(3, "borrower2@campus.edu", "hash", "Borrower 2", "", "PRN03", "CS", 2, "B", 62.0, true, false, clock.now());
        int64_t b2Id = users->insert(borrower2, "PRN03", "CS", 2, "B");
        auto b2User = users->getById(b2Id);
        threads->getOrCreate(itemId, b2Id);

        CreateBorrowRequest req1{itemId, clock.now() + 86400, clock.now() + 3 * 86400};
        CreateBorrowRequest req2{itemId, clock.now() + 86400, clock.now() + 4 * 86400};

        int64_t txn1 = borrow.createRequest(*borrowerUser, req1);
        int64_t txn2 = borrow.createRequest(*b2User, req2);

        // Owner accepts txn1
        borrow.acceptRequest(*ownerUser, txn1);

        auto t1 = borrow.getTransaction(txn1);
        auto t2 = borrow.getTransaction(txn2);
        auto currentItem = items->getById(itemId);

        REQUIRE(t1->status() == TransactionStatus::ACCEPTED);
        REQUIRE(t2->status() == TransactionStatus::REJECTED); // Auto-rejected
        REQUIRE(currentItem->status() == ItemStatus::RESERVED);
    }
}
