#pragma once

#include "src/domain/Money.hpp"
#include "src/util/Errors.hpp"

#include <string>
#include <string_view>
#include <cstdint>
#include <optional>

namespace borrowhub::domain {

enum class TransactionStatus {
    REQUESTED,
    ACCEPTED,
    ACTIVE,
    OVERDUE,
    RETURNED,
    DISPUTED,
    SETTLED,
    REJECTED,
    CANCELLED
};

[[nodiscard]] constexpr std::string_view transactionStatusToString(TransactionStatus status) noexcept {
    switch (status) {
        case TransactionStatus::REQUESTED: return "REQUESTED";
        case TransactionStatus::ACCEPTED: return "ACCEPTED";
        case TransactionStatus::ACTIVE: return "ACTIVE";
        case TransactionStatus::OVERDUE: return "OVERDUE";
        case TransactionStatus::RETURNED: return "RETURNED";
        case TransactionStatus::DISPUTED: return "DISPUTED";
        case TransactionStatus::SETTLED: return "SETTLED";
        case TransactionStatus::REJECTED: return "REJECTED";
        case TransactionStatus::CANCELLED: return "CANCELLED";
    }
    return "REQUESTED";
}

TransactionStatus stringToTransactionStatus(std::string_view str);

class BorrowTransaction {
public:
    BorrowTransaction(int64_t id,
                      int64_t itemId,
                      std::optional<int64_t> threadId,
                      int64_t ownerId,
                      int64_t borrowerId,
                      int64_t startDate,
                      int64_t dueDate,
                      Money deposit,
                      int64_t createdAt,
                      int64_t updatedAt,
                      TransactionStatus status = TransactionStatus::REQUESTED,
                      std::optional<int64_t> returnedAt = std::nullopt,
                      std::string conditionReport = "",
                      Money claim = Money::fromPaise(0),
                      Money penalty = Money::fromPaise(0),
                      std::optional<Money> refund = std::nullopt,
                      bool ownerConfirmed = false,
                      bool borrowerConfirmed = false,
                      bool depositHeld = false,
                      int64_t acceptedAt = 0);

    // Getters
    [[nodiscard]] int64_t id() const noexcept { return id_; }
    [[nodiscard]] int64_t itemId() const noexcept { return itemId_; }
    [[nodiscard]] std::optional<int64_t> threadId() const noexcept { return threadId_; }
    [[nodiscard]] int64_t ownerId() const noexcept { return ownerId_; }
    [[nodiscard]] int64_t borrowerId() const noexcept { return borrowerId_; }
    [[nodiscard]] int64_t startDate() const noexcept { return startDate_; }
    [[nodiscard]] int64_t dueDate() const noexcept { return dueDate_; }
    [[nodiscard]] std::optional<int64_t> returnedAt() const noexcept { return returnedAt_; }
    [[nodiscard]] Money deposit() const noexcept { return deposit_; }
    [[nodiscard]] const std::string& conditionReport() const noexcept { return conditionReport_; }
    [[nodiscard]] Money claim() const noexcept { return claim_; }
    [[nodiscard]] Money penalty() const noexcept { return penalty_; }
    [[nodiscard]] std::optional<Money> refund() const noexcept { return refund_; }
    [[nodiscard]] TransactionStatus status() const noexcept { return status_; }
    [[nodiscard]] bool isOwnerConfirmed() const noexcept { return ownerConfirmed_; }
    [[nodiscard]] bool isBorrowerConfirmed() const noexcept { return borrowerConfirmed_; }
    [[nodiscard]] bool isDepositHeld() const noexcept { return depositHeld_; }
    [[nodiscard]] int64_t acceptedAt() const noexcept { return acceptedAt_; }
    [[nodiscard]] int64_t createdAt() const noexcept { return createdAt_; }
    [[nodiscard]] int64_t updatedAt() const noexcept { return updatedAt_; }

    // State machine operations
    void accept(int64_t actorId, int64_t now);
    void reject(int64_t actorId);
    void cancel(int64_t actorId, int64_t now);
    void payDeposit(int64_t actorId, int64_t now);
    void confirmHandover(int64_t actorId);
    void markOverdue(int64_t now);
    void reportReturn(int64_t actorId, int64_t now, std::string condition, Money claim);
    void raiseDispute(int64_t actorId, int64_t now);
    void settle();

    // Financial calculations update
    void setFinancials(Money penalty, Money claim, Money refund);

private:
    int64_t id_{0};
    int64_t itemId_{0};
    std::optional<int64_t> threadId_;
    int64_t ownerId_{0};
    int64_t borrowerId_{0};
    int64_t startDate_{0};
    int64_t dueDate_{0};
    Money deposit_;
    int64_t createdAt_{0};
    int64_t updatedAt_{0};

    TransactionStatus status_{TransactionStatus::REQUESTED};
    std::optional<int64_t> returnedAt_;
    std::string conditionReport_;
    Money claim_{Money::fromPaise(0)};
    Money penalty_{Money::fromPaise(0)};
    std::optional<Money> refund_;

    bool ownerConfirmed_{false};
    bool borrowerConfirmed_{false};
    bool depositHeld_{false};
    int64_t acceptedAt_{0};
};

} // namespace borrowhub::domain
