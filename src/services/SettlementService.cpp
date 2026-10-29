#include "src/services/SettlementService.hpp"
#include "src/db/DbTransaction.hpp"
#include "src/domain/PenaltyPolicy.hpp"
#include "src/util/Errors.hpp"

#include <algorithm>
#include <cassert>

namespace borrowhub::services {

SettlementService::SettlementService(db::Database& db,
                                     db::TxnRepo& txns,
                                     db::ItemRepo& items,
                                     db::LedgerRepo& ledger,
                                     db::DisputeRepo& disputes,
                                     const util::Clock& clock,
                                     EventBus& eventBus)
    : db_(db),
      txns_(txns),
      items_(items),
      ledger_(ledger),
      disputes_(disputes),
      clock_(clock),
      eventBus_(eventBus) {}

SettlementResult SettlementService::settle(int64_t txnId) {
    db::DbTransaction tx(db_);

    auto txn = txns_.getById(txnId);
    if (!txn) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Transaction not found");
    }

    if (txn->status() != domain::TransactionStatus::RETURNED &&
        txn->status() != domain::TransactionStatus::DISPUTED) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Only RETURNED or DISPUTED transactions can be settled");
    }

    // Ensure no open dispute blocks settlement
    auto disputeOpt = disputes_.getByTxnId(txnId);
    if (disputeOpt.has_value() && disputeOpt.value().status == "OPEN") {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Cannot settle transaction while dispute is OPEN");
    }

    auto item = items_.getById(txn->itemId());
    if (!item) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Item not found");
    }

    int64_t returnedAt = txn->returnedAt().value_or(clock_.now());

    // 1. Calculate late penalty
    domain::Money penalty = domain::PenaltyPolicy::calculatePenalty(
        txn->dueDate(), returnedAt, item->penaltyPerDay(), txn->deposit()
    );

    // 2. Calculate damage claim (capped at remaining deposit after penalty)
    int64_t maxClaimPaise = std::max<int64_t>(0, txn->deposit().paise() - penalty.paise());
    int64_t actualClaimPaise = std::min(txn->claim().paise(), maxClaimPaise);
    domain::Money claim = domain::Money::fromPaise(actualClaimPaise);

    // If condition report was GOOD, claim is strictly 0 per Blueprint 3.4
    if (txn->conditionReport() == "GOOD") {
        claim = domain::Money::fromPaise(0);
    }

    domain::Money totalDeduction = penalty + claim;
    domain::Money refund = txn->deposit() - totalDeduction;

    // Financial invariant assertion: refund + totalDeduction == deposit
    assert(refund.paise() + totalDeduction.paise() == txn->deposit().paise());

    int64_t now = clock_.now();

    // 3. Write immutable ledger audit entries
    if (refund.paise() > 0) {
        ledger_.insert(txnId, txn->borrowerId(), "REFUND", refund, now);
    }
    if (totalDeduction.paise() > 0) {
        ledger_.insert(txnId, txn->borrowerId(), "DEDUCT", totalDeduction, now);
        ledger_.insert(txnId, txn->ownerId(), "PAYOUT", totalDeduction, now);
    }

    // 4. Update transaction status and financials
    txn->setFinancials(penalty, claim, refund);
    txn->settle();
    txns_.update(*txn);

    // 5. Item returns to AVAILABLE status
    item->setStatus(domain::ItemStatus::AVAILABLE);
    items_.update(*item);

    tx.commit();

    eventBus_.publish(DomainEvent{
        EventType::SETTLED,
        txn->borrowerId(),
        txn->ownerId(),
        txnId,
        "{\"refundPaise\":" + std::to_string(refund.paise()) +
        ",\"deductionPaise\":" + std::to_string(totalDeduction.paise()) + "}"
    });

    return SettlementResult{penalty, claim, refund, totalDeduction};
}

SettlementResult SettlementService::acceptSettlement(const domain::User& borrower, int64_t txnId) {
    auto txn = txns_.getById(txnId);
    if (!txn) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Transaction not found");
    }
    if (txn->borrowerId() != borrower.id()) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Only the borrower can accept this settlement");
    }
    return settle(txnId);
}

} // namespace borrowhub::services
