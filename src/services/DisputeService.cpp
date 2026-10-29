#include "src/services/DisputeService.hpp"
#include "src/db/DbTransaction.hpp"
#include "src/util/Errors.hpp"
#include "src/util/Validation.hpp"

#include <cassert>

namespace borrowhub::services {

DisputeService::DisputeService(db::Database& db,
                               db::DisputeRepo& disputes,
                               db::TxnRepo& txns,
                               db::ItemRepo& items,
                               db::LedgerRepo& ledger,
                               const util::Clock& clock,
                               EventBus& eventBus)
    : db_(db),
      disputes_(disputes),
      txns_(txns),
      items_(items),
      ledger_(ledger),
      clock_(clock),
      eventBus_(eventBus) {}

int64_t DisputeService::raiseDispute(const domain::User& actor, int64_t txnId, std::string_view reason) {
    if (reason.empty() || reason.length() > 2000) {
        throw util::AppError(util::ErrorCode::VALIDATION, "Dispute reason must be between 1 and 2000 characters");
    }

    db::DbTransaction tx(db_);

    auto txn = txns_.getById(txnId);
    if (!txn) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Transaction not found");
    }

    int64_t now = clock_.now();
    // BorrowTransaction validates: status == RETURNED, actor is participant, within 48h
    txn->raiseDispute(actor.id(), now);
    txns_.update(*txn);

    int64_t disputeId = disputes_.insert(txnId, actor.id(), reason, now);

    tx.commit();

    eventBus_.publish(DomainEvent{
        EventType::DISPUTE_RAISED,
        0, // Broadcast to admins
        actor.id(),
        disputeId,
        "{\"txnId\":" + std::to_string(txnId) + "}"
    });

    return disputeId;
}

void DisputeService::decideDispute(const domain::User& admin, int64_t disputeId, domain::Money ownerAward, std::string_view note) {
    if (admin.role() != domain::UserRole::ADMIN) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Only administrators can decide disputes");
    }
    std::string trimmedNote = util::Validation::trim(note);
    if (trimmedNote.empty()) {
        throw util::AppError(util::ErrorCode::VALIDATION, "Decision note cannot be empty");
    }

    db::DbTransaction tx(db_);

    auto disputeOpt = disputes_.getById(disputeId);
    if (!disputeOpt.has_value()) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Dispute not found");
    }
    auto dispute = disputeOpt.value();
    if (dispute.status != "OPEN") {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Dispute has already been decided");
    }

    auto txn = txns_.getById(dispute.txnId);
    if (!txn) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Associated transaction not found");
    }
    if (ownerAward > txn->deposit()) {
        throw util::AppError(util::ErrorCode::VALIDATION, "Owner award cannot exceed total deposit");
    }

    int64_t now = clock_.now();

    // Financial split: owner award + refund = deposit
    domain::Money refund = txn->deposit() - ownerAward;
    assert(ownerAward.paise() + refund.paise() == txn->deposit().paise());

    // Write ledger entries
    if (ownerAward.paise() > 0) {
        ledger_.insert(txn->id(), txn->borrowerId(), "DEDUCT", ownerAward, now);
        ledger_.insert(txn->id(), txn->ownerId(), "PAYOUT", ownerAward, now);
    }
    if (refund.paise() > 0) {
        ledger_.insert(txn->id(), txn->borrowerId(), "REFUND", refund, now);
    }

    // Mark dispute decided
    disputes_.decide(disputeId, admin.id(), ownerAward, trimmedNote, now);

    // Update transaction to SETTLED
    txn->setFinancials(domain::Money::fromPaise(0), ownerAward, refund);
    txn->settle();
    txns_.update(*txn);

    // Free item back to AVAILABLE
    auto item = items_.getById(txn->itemId());
    if (item) {
        item->setStatus(domain::ItemStatus::AVAILABLE);
        items_.update(*item);
    }

    tx.commit();

    eventBus_.publish(DomainEvent{
        EventType::DISPUTE_DECIDED,
        txn->borrowerId(),
        admin.id(),
        disputeId,
        "{\"awardPaise\":" + std::to_string(ownerAward.paise()) +
        ",\"refundPaise\":" + std::to_string(refund.paise()) + "}"
    });
}

std::vector<db::Dispute> DisputeService::listDisputes(const domain::User& admin) {
    if (admin.role() != domain::UserRole::ADMIN) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Admin authorization required");
    }
    return disputes_.listAll();
}

} // namespace borrowhub::services
