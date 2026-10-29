#include "src/domain/BorrowTransaction.hpp"

namespace borrowhub::domain {

TransactionStatus stringToTransactionStatus(std::string_view str) {
    if (str == "REQUESTED") return TransactionStatus::REQUESTED;
    if (str == "ACCEPTED") return TransactionStatus::ACCEPTED;
    if (str == "ACTIVE") return TransactionStatus::ACTIVE;
    if (str == "OVERDUE") return TransactionStatus::OVERDUE;
    if (str == "RETURNED") return TransactionStatus::RETURNED;
    if (str == "DISPUTED") return TransactionStatus::DISPUTED;
    if (str == "SETTLED") return TransactionStatus::SETTLED;
    if (str == "REJECTED") return TransactionStatus::REJECTED;
    if (str == "CANCELLED") return TransactionStatus::CANCELLED;
    throw util::AppError(util::ErrorCode::VALIDATION, "Unknown transaction status: " + std::string(str));
}

BorrowTransaction::BorrowTransaction(int64_t id,
                                     int64_t itemId,
                                     std::optional<int64_t> threadId,
                                     int64_t ownerId,
                                     int64_t borrowerId,
                                     int64_t startDate,
                                     int64_t dueDate,
                                     Money deposit,
                                     int64_t createdAt,
                                     int64_t updatedAt,
                                     TransactionStatus status,
                                     std::optional<int64_t> returnedAt,
                                     std::string conditionReport,
                                     Money claim,
                                     Money penalty,
                                     std::optional<Money> refund,
                                     bool ownerConfirmed,
                                     bool borrowerConfirmed,
                                     bool depositHeld,
                                     int64_t acceptedAt)
    : id_(id),
      itemId_(itemId),
      threadId_(threadId),
      ownerId_(ownerId),
      borrowerId_(borrowerId),
      startDate_(startDate),
      dueDate_(dueDate),
      deposit_(deposit),
      createdAt_(createdAt),
      updatedAt_(updatedAt),
      status_(status),
      returnedAt_(returnedAt),
      conditionReport_(std::move(conditionReport)),
      claim_(claim),
      penalty_(penalty),
      refund_(refund),
      ownerConfirmed_(ownerConfirmed),
      borrowerConfirmed_(borrowerConfirmed),
      depositHeld_(depositHeld),
      acceptedAt_(acceptedAt) {
    if (dueDate_ <= startDate_) {
        throw util::AppError(util::ErrorCode::VALIDATION, "Transaction due date must be after start date");
    }
}

void BorrowTransaction::accept(int64_t actorId, int64_t now) {
    if (actorId != ownerId_) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Only the item owner can accept a borrow request");
    }
    if (status_ != TransactionStatus::REQUESTED) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Only REQUESTED transactions can be accepted");
    }
    status_ = TransactionStatus::ACCEPTED;
    acceptedAt_ = now;
    updatedAt_ = now;
}

void BorrowTransaction::reject(int64_t actorId) {
    if (actorId != ownerId_) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Only the item owner can reject a borrow request");
    }
    if (status_ != TransactionStatus::REQUESTED) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Only REQUESTED transactions can be rejected");
    }
    status_ = TransactionStatus::REJECTED;
}

void BorrowTransaction::cancel(int64_t actorId, int64_t now) {
    if (actorId != ownerId_ && actorId != borrowerId_) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Only transaction participants can cancel");
    }
    if (status_ != TransactionStatus::REQUESTED && status_ != TransactionStatus::ACCEPTED) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Cannot cancel transaction in current status");
    }
    status_ = TransactionStatus::CANCELLED;
    updatedAt_ = now;
}

void BorrowTransaction::payDeposit(int64_t actorId, int64_t now) {
    if (actorId != borrowerId_) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Only the borrower can pay the deposit");
    }
    if (status_ != TransactionStatus::ACCEPTED) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Deposit can only be paid in ACCEPTED status");
    }
    // Blueprint 3.4: "ACCEPTED requests without a deposit within 24 hours expire to CANCELLED"
    if (acceptedAt_ > 0 && (now - acceptedAt_) > 86400) {
        status_ = TransactionStatus::CANCELLED;
        updatedAt_ = now;
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "24-hour deposit payment window expired");
    }
    depositHeld_ = true;
    updatedAt_ = now;
}

void BorrowTransaction::confirmHandover(int64_t actorId) {
    if (!depositHeld_) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Deposit must be held before confirming handover");
    }
    if (actorId != ownerId_ && actorId != borrowerId_) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Only transaction participants can confirm handover");
    }
    if (status_ != TransactionStatus::ACCEPTED) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Handover confirmation requires ACCEPTED status");
    }

    if (actorId == ownerId_) {
        ownerConfirmed_ = true;
    } else if (actorId == borrowerId_) {
        borrowerConfirmed_ = true;
    }

    // Both parties must confirm handover for status to become ACTIVE
    if (ownerConfirmed_ && borrowerConfirmed_) {
        status_ = TransactionStatus::ACTIVE;
    }
}

void BorrowTransaction::markOverdue(int64_t now) {
    if (status_ != TransactionStatus::ACTIVE) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Only ACTIVE transactions can become OVERDUE");
    }
    if (now <= dueDate_) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Due date has not passed yet");
    }
    status_ = TransactionStatus::OVERDUE;
    updatedAt_ = now;
}

void BorrowTransaction::reportReturn(int64_t actorId, int64_t now, std::string condition, Money claim) {
    if (actorId != ownerId_) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Only the owner can report item return");
    }
    if (status_ != TransactionStatus::ACTIVE && status_ != TransactionStatus::OVERDUE) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Return can only be reported for ACTIVE or OVERDUE transactions");
    }
    if (claim > deposit_) {
        throw util::AppError(util::ErrorCode::VALIDATION, "Damage claim cannot exceed security deposit");
    }

    conditionReport_ = std::move(condition);
    claim_ = claim;
    returnedAt_ = now;
    status_ = TransactionStatus::RETURNED;
    updatedAt_ = now;
}

void BorrowTransaction::raiseDispute(int64_t actorId, int64_t now) {
    if (actorId != ownerId_ && actorId != borrowerId_) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Only transaction participants can raise a dispute");
    }
    if (status_ != TransactionStatus::RETURNED) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Dispute can only be raised on RETURNED transactions");
    }
    if (!returnedAt_.has_value() || (now - returnedAt_.value()) > 172800) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Dispute window (48 hours) has expired");
    }

    status_ = TransactionStatus::DISPUTED;
    updatedAt_ = now;
}

void BorrowTransaction::settle() {
    if (status_ != TransactionStatus::RETURNED && status_ != TransactionStatus::DISPUTED) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Only RETURNED or DISPUTED transactions can be settled");
    }
    status_ = TransactionStatus::SETTLED;
}

void BorrowTransaction::setFinancials(Money penalty, Money claim, Money refund) {
    penalty_ = penalty;
    claim_ = claim;
    refund_ = refund;
}

} // namespace borrowhub::domain
