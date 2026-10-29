#pragma once

#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/domain/BorrowTransaction.hpp"
#include "src/domain/User.hpp"
#include "src/util/Clock.hpp"
#include "src/services/EventBus.hpp"

#include <memory>
#include <vector>

namespace borrowhub::services {

struct CreateBorrowRequest {
    int64_t itemId{0};
    int64_t startDate{0};
    int64_t dueDate{0};
};

class BorrowService {
public:
    BorrowService(db::Database& db,
                  db::TxnRepo& txns,
                  db::ItemRepo& items,
                  db::ThreadRepo& threads,
                  db::LedgerRepo& ledger,
                  const util::Clock& clock,
                  EventBus& eventBus);

    // Creates a new borrow request
    int64_t createRequest(const domain::User& user, const CreateBorrowRequest& req);

    // Owner accepts request; auto-rejects other pending requests for the same item
    void acceptRequest(const domain::User& owner, int64_t txnId);

    // Owner rejects request
    void rejectRequest(const domain::User& owner, int64_t txnId);

    // Either party cancels before activation
    void cancelRequest(const domain::User& actor, int64_t txnId);

    // Borrower pays simulated security deposit (recorded as HOLD in ledger)
    void payDeposit(const domain::User& borrower, int64_t txnId);

    // Confirms physical handover; status becomes ACTIVE when both confirm
    void confirmHandover(const domain::User& actor, int64_t txnId);

    // Owner reports item return and optional damage claim
    void reportReturn(const domain::User& owner, int64_t txnId, std::string condition, domain::Money claim);

    // Retrieves transaction by ID
    [[nodiscard]] std::unique_ptr<domain::BorrowTransaction> getTransaction(int64_t txnId);

    // Lists transactions for user
    [[nodiscard]] std::vector<std::unique_ptr<domain::BorrowTransaction>> listUserTransactions(int64_t userId);

private:
    db::Database& db_;
    db::TxnRepo& txns_;
    db::ItemRepo& items_;
    db::ThreadRepo& threads_;
    db::LedgerRepo& ledger_;
    const util::Clock& clock_;
    EventBus& eventBus_;
};

} // namespace borrowhub::services
