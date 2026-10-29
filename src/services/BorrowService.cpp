#include "src/services/BorrowService.hpp"
#include "src/db/DbTransaction.hpp"
#include "src/util/Errors.hpp"

#include <cmath>

namespace borrowhub::services {

BorrowService::BorrowService(db::Database& db,
                             db::TxnRepo& txns,
                             db::ItemRepo& items,
                             db::ThreadRepo& threads,
                             db::LedgerRepo& ledger,
                             const util::Clock& clock,
                             EventBus& eventBus)
    : db_(db),
      txns_(txns),
      items_(items),
      threads_(threads),
      ledger_(ledger),
      clock_(clock),
      eventBus_(eventBus) {}

int64_t BorrowService::createRequest(const domain::User& user, const CreateBorrowRequest& req) {
    if (!user.canRequest()) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "User account is not eligible to request items");
    }

    auto item = items_.getById(req.itemId);
    if (!item || item->status() == domain::ItemStatus::REMOVED) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Item not found");
    }
    if (item->ownerId() == user.id()) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Cannot borrow your own item");
    }
    if (item->status() != domain::ItemStatus::AVAILABLE) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Item is currently not available for borrowing");
    }

    // Blueprint 10.9: "require thread exists for (item, user) // forces negotiate-first"
    auto thread = threads_.getByItemAndBorrower(req.itemId, user.id());
    if (!thread.has_value()) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Must open a chat thread and negotiate before requesting item");
    }

    int64_t now = clock_.now();
    if (req.dueDate <= req.startDate) {
        throw util::AppError(util::ErrorCode::VALIDATION, "Due date must be after start date");
    }
    if (req.startDate < now - 3600) { // Allow minor clock tolerance
        throw util::AppError(util::ErrorCode::VALIDATION, "Start date cannot be in the past");
    }

    // Check maximum borrow duration for this specific item type (polymorphism)
    int64_t durationSeconds = req.dueDate - req.startDate;
    int64_t durationDays = (durationSeconds + 86399) / 86400;
    if (durationDays > item->maxBorrowDays()) {
        throw util::AppError(util::ErrorCode::VALIDATION,
            "Requested duration (" + std::to_string(durationDays) + " days) exceeds maximum allowed (" +
            std::to_string(item->maxBorrowDays()) + " days) for " + std::string(item->typeName()));
    }

    // Date bounds within item availability
    if (item->availFrom() > 0 && req.startDate < item->availFrom()) {
        throw util::AppError(util::ErrorCode::VALIDATION, "Start date is before item availability window");
    }
    if (item->availTo() > 0 && req.dueDate > item->availTo()) {
        throw util::AppError(util::ErrorCode::VALIDATION, "Due date is after item availability window");
    }

    // Check if user already has an active or pending request for this item
    if (txns_.hasLiveTransactionForUserAndItem(req.itemId, user.id())) {
        throw util::AppError(util::ErrorCode::CONFLICT, "You already have an active or pending request for this item");
    }

    db::DbTransaction tx(db_);

    domain::BorrowTransaction txn(
        0, req.itemId, thread.value().id, item->ownerId(), user.id(),
        req.startDate, req.dueDate, item->deposit(), now, now,
        domain::TransactionStatus::REQUESTED
    );

    int64_t txnId = txns_.insert(txn);

    tx.commit();

    eventBus_.publish(DomainEvent{
        EventType::REQUEST_CREATED,
        item->ownerId(),
        user.id(),
        txnId,
        "{\"itemId\":" + std::to_string(req.itemId) + "}"
    });

    return txnId;
}

void BorrowService::acceptRequest(const domain::User& owner, int64_t txnId) {
    db::DbTransaction tx(db_);

    auto txn = txns_.getById(txnId);
    if (!txn) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Transaction not found");
    }

    auto item = items_.getById(txn->itemId());
    if (!item) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Item not found");
    }
    if (item->status() != domain::ItemStatus::AVAILABLE) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Item is already reserved or borrowed");
    }

    int64_t now = clock_.now();
    txn->accept(owner.id(), now);
    item->setStatus(domain::ItemStatus::RESERVED);

    // Auto-reject other pending requests for the same item
    auto pending = txns_.findRequestedFor(item->id());
    for (auto& other : pending) {
        if (other->id() != txnId) {
            other->reject(owner.id());
            txns_.update(*other);
            eventBus_.publish(DomainEvent{
                EventType::REQUEST_REJECTED,
                other->borrowerId(),
                owner.id(),
                other->id(),
                "{\"reason\":\"Item was accepted for another student\"}"
            });
        }
    }

    txns_.update(*txn);
    items_.update(*item);

    tx.commit();

    eventBus_.publish(DomainEvent{
        EventType::REQUEST_ACCEPTED,
        txn->borrowerId(),
        owner.id(),
        txnId,
        "{\"itemId\":" + std::to_string(item->id()) + "}"
    });
}

void BorrowService::rejectRequest(const domain::User& owner, int64_t txnId) {
    db::DbTransaction tx(db_);

    auto txn = txns_.getById(txnId);
    if (!txn) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Transaction not found");
    }

    txn->reject(owner.id());
    txns_.update(*txn);

    tx.commit();

    eventBus_.publish(DomainEvent{
        EventType::REQUEST_REJECTED,
        txn->borrowerId(),
        owner.id(),
        txnId,
        "{}"
    });
}

void BorrowService::cancelRequest(const domain::User& actor, int64_t txnId) {
    db::DbTransaction tx(db_);

    auto txn = txns_.getById(txnId);
    if (!txn) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Transaction not found");
    }

    int64_t now = clock_.now();
    txn->cancel(actor.id(), now);
    txns_.update(*txn);

    // If item was RESERVED by this transaction, release back to AVAILABLE
    auto item = items_.getById(txn->itemId());
    if (item && item->status() == domain::ItemStatus::RESERVED) {
        item->setStatus(domain::ItemStatus::AVAILABLE);
        items_.update(*item);
    }

    tx.commit();

    int64_t otherId = (actor.id() == txn->ownerId()) ? txn->borrowerId() : txn->ownerId();
    eventBus_.publish(DomainEvent{
        EventType::REQUEST_CANCELLED,
        otherId,
        actor.id(),
        txnId,
        "{}"
    });
}

void BorrowService::payDeposit(const domain::User& borrower, int64_t txnId) {
    db::DbTransaction tx(db_);

    auto txn = txns_.getById(txnId);
    if (!txn) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Transaction not found");
    }

    int64_t now = clock_.now();
    txn->payDeposit(borrower.id(), now);

    // Record ledger HOLD entry
    ledger_.insert(txnId, borrower.id(), "HOLD", txn->deposit(), now);

    txns_.update(*txn);
    tx.commit();

    eventBus_.publish(DomainEvent{
        EventType::DEPOSIT_PAID,
        txn->ownerId(),
        borrower.id(),
        txnId,
        "{\"amountPaise\":" + std::to_string(txn->deposit().paise()) + "}"
    });
}

void BorrowService::confirmHandover(const domain::User& actor, int64_t txnId) {
    db::DbTransaction tx(db_);

    auto txn = txns_.getById(txnId);
    if (!txn) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Transaction not found");
    }

    auto item = items_.getById(txn->itemId());
    if (!item) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Item not found");
    }

    txn->confirmHandover(actor.id());

    if (txn->status() == domain::TransactionStatus::ACTIVE) {
        item->setStatus(domain::ItemStatus::BORROWED);
        items_.update(*item);
    }

    txns_.update(*txn);
    tx.commit();

    int64_t otherId = (actor.id() == txn->ownerId()) ? txn->borrowerId() : txn->ownerId();
    eventBus_.publish(DomainEvent{
        EventType::HANDOVER_CONFIRMED,
        otherId,
        actor.id(),
        txnId,
        "{\"active\":" + std::string(txn->status() == domain::TransactionStatus::ACTIVE ? "true" : "false") + "}"
    });

    if (txn->status() == domain::TransactionStatus::ACTIVE) {
        eventBus_.publish(DomainEvent{
            EventType::TRANSACTION_ACTIVE,
            txn->borrowerId(),
            actor.id(),
            txnId,
            "{}"
        });
    }
}

void BorrowService::reportReturn(const domain::User& owner, int64_t txnId, std::string condition, domain::Money claim) {
    db::DbTransaction tx(db_);

    auto txn = txns_.getById(txnId);
    if (!txn) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Transaction not found");
    }

    int64_t now = clock_.now();
    txn->reportReturn(owner.id(), now, std::move(condition), claim);
    txns_.update(*txn);

    tx.commit();

    eventBus_.publish(DomainEvent{
        EventType::RETURN_REPORTED,
        txn->borrowerId(),
        owner.id(),
        txnId,
        "{\"claimPaise\":" + std::to_string(claim.paise()) + "}"
    });
}

std::unique_ptr<domain::BorrowTransaction> BorrowService::getTransaction(int64_t txnId) {
    auto txn = txns_.getById(txnId);
    if (!txn) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Transaction not found");
    }
    return txn;
}

std::vector<std::unique_ptr<domain::BorrowTransaction>> BorrowService::listUserTransactions(int64_t userId) {
    return txns_.listForUser(userId);
}

} // namespace borrowhub::services
