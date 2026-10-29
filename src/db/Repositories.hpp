#pragma once

#include "src/domain/BorrowTransaction.hpp"
#include "src/domain/Item.hpp"
#include "src/domain/Money.hpp"
#include "src/domain/Student.hpp"
#include "src/domain/User.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace borrowhub::db {

// --- Data Transfer & Entity Structures ---

struct Session {
    std::string tokenHash;
    int64_t userId{0};
    int64_t expiresAt{0};
};

struct VerificationToken {
    std::string tokenHash;
    int64_t userId{0};
    int64_t expiresAt{0};
};

struct ChatThread {
    int64_t id{0};
    int64_t itemId{0};
    int64_t borrowerId{0};
};

struct Message {
    int64_t id{0};
    int64_t threadId{0};
    int64_t senderId{0};
    std::string senderName;
    std::string body;
    int64_t createdAt{0};
};

struct LedgerEntry {
    int64_t id{0};
    int64_t txnId{0};
    int64_t userId{0};
    std::string kind; // "HOLD", "REFUND", "DEDUCT", "PAYOUT"
    domain::Money amount;
    int64_t createdAt{0};
};

struct Rating {
    int64_t id{0};
    int64_t txnId{0};
    int64_t raterId{0};
    int64_t rateeId{0};
    int stars{5};
    std::string comment;
};

struct Notification {
    int64_t id{0};
    int64_t userId{0};
    std::string kind;
    std::string payloadJson;
    bool isRead{false};
    int64_t createdAt{0};
};

struct Dispute {
    int64_t id{0};
    int64_t txnId{0};
    int64_t raisedBy{0};
    std::string reason;
    std::string status; // "OPEN", "DECIDED"
    std::optional<int64_t> adminId;
    std::optional<domain::Money> ownerAward;
    std::string decisionNote;
    int64_t createdAt{0};
    std::optional<int64_t> decidedAt;
};

struct ItemFilter {
    std::optional<std::string> query;
    std::optional<std::string> category;
    std::optional<std::string> location;
    std::optional<std::string> condition;
    std::optional<int64_t> maxDepositPaise;
    std::optional<int64_t> availableOnDate;
    std::optional<double> minOwnerRating;
    int page{1};
    int pageSize{20};
};

struct ItemSearchResult {
    std::vector<std::unique_ptr<domain::Item>> items;
    int totalCount{0};
};

// --- Repository Interfaces ---

class UserRepo {
public:
    virtual ~UserRepo() = default;
    [[nodiscard]] virtual std::unique_ptr<domain::User> getById(int64_t id) = 0;
    [[nodiscard]] virtual std::unique_ptr<domain::User> getByEmail(std::string_view email) = 0;
    [[nodiscard]] virtual std::unique_ptr<domain::User> getByPrn(std::string_view prn) = 0;
    virtual int64_t insert(const domain::User& user, std::string_view prn, std::string_view department, int year, std::string_view division) = 0;
    virtual void update(const domain::User& user) = 0;
    virtual void setVerified(int64_t userId, bool verified) = 0;
    virtual void setBlocked(int64_t userId, bool blocked) = 0;
    virtual void updateReputation(int64_t userId, double reputation) = 0;
    virtual void updateCounters(int64_t userId, int totalTxns, int onTime, int damageFree) = 0;
};

class SessionRepo {
public:
    virtual ~SessionRepo() = default;
    virtual void insert(const Session& session) = 0;
    [[nodiscard]] virtual std::optional<Session> getByTokenHash(std::string_view tokenHash) = 0;
    virtual void deleteByTokenHash(std::string_view tokenHash) = 0;
    virtual void deleteExpired(int64_t now) = 0;
};

class VerificationTokenRepo {
public:
    virtual ~VerificationTokenRepo() = default;
    virtual void insert(const VerificationToken& token) = 0;
    [[nodiscard]] virtual std::optional<VerificationToken> getByTokenHash(std::string_view tokenHash) = 0;
    virtual void deleteByTokenHash(std::string_view tokenHash) = 0;
    virtual void deleteExpired(int64_t now) = 0;
};

class ItemRepo {
public:
    virtual ~ItemRepo() = default;
    [[nodiscard]] virtual std::unique_ptr<domain::Item> getById(int64_t id) = 0;
    virtual int64_t insert(const domain::Item& item) = 0;
    virtual void update(const domain::Item& item) = 0;
    virtual void setStatus(int64_t itemId, domain::ItemStatus status) = 0;
    [[nodiscard]] virtual ItemSearchResult search(const ItemFilter& filter) = 0;
    [[nodiscard]] virtual std::vector<std::unique_ptr<domain::Item>> listByOwner(int64_t ownerId) = 0;
};

class ThreadRepo {
public:
    virtual ~ThreadRepo() = default;
    [[nodiscard]] virtual std::optional<ChatThread> getById(int64_t id) = 0;
    [[nodiscard]] virtual std::optional<ChatThread> getByItemAndBorrower(int64_t itemId, int64_t borrowerId) = 0;
    virtual int64_t getOrCreate(int64_t itemId, int64_t borrowerId) = 0;
    [[nodiscard]] virtual std::vector<ChatThread> listForUser(int64_t userId) = 0;
};

class MessageRepo {
public:
    virtual ~MessageRepo() = default;
    virtual int64_t insert(int64_t threadId, int64_t senderId, std::string_view body, int64_t createdAt) = 0;
    [[nodiscard]] virtual std::vector<Message> listForThreadAfter(int64_t threadId, int64_t afterId, int limit) = 0;
    [[nodiscard]] virtual int countMessagesInWindow(int64_t senderId, int64_t windowSeconds, int64_t now) = 0;
};

class TxnRepo {
public:
    virtual ~TxnRepo() = default;
    [[nodiscard]] virtual std::unique_ptr<domain::BorrowTransaction> getById(int64_t id) = 0;
    virtual int64_t insert(const domain::BorrowTransaction& txn) = 0;
    virtual void update(const domain::BorrowTransaction& txn) = 0;
    [[nodiscard]] virtual std::vector<std::unique_ptr<domain::BorrowTransaction>> listForUser(int64_t userId) = 0;
    [[nodiscard]] virtual std::vector<std::unique_ptr<domain::BorrowTransaction>> findRequestedFor(int64_t itemId) = 0;
    [[nodiscard]] virtual std::vector<std::unique_ptr<domain::BorrowTransaction>> findOverdue(int64_t now) = 0;
    [[nodiscard]] virtual std::vector<std::unique_ptr<domain::BorrowTransaction>> findStaleAccepted(int64_t thresholdTime) = 0;
    [[nodiscard]] virtual std::vector<std::unique_ptr<domain::BorrowTransaction>> findAutoSettleable(int64_t thresholdTime) = 0;
    [[nodiscard]] virtual std::vector<std::unique_ptr<domain::BorrowTransaction>> findDueWithin(int64_t windowSeconds, int64_t now) = 0;
    [[nodiscard]] virtual bool hasLiveTransactionForUserAndItem(int64_t itemId, int64_t borrowerId) = 0;
};

class LedgerRepo {
public:
    virtual ~LedgerRepo() = default;
    virtual int64_t insert(int64_t txnId, int64_t userId, std::string_view kind, domain::Money amount, int64_t createdAt) = 0;
    [[nodiscard]] virtual std::vector<LedgerEntry> listForTransaction(int64_t txnId) = 0;
    [[nodiscard]] virtual domain::Money getSumForTxnAndKind(int64_t txnId, std::string_view kind) = 0;
};

class RatingRepo {
public:
    virtual ~RatingRepo() = default;
    virtual int64_t insert(int64_t txnId, int64_t raterId, int64_t rateeId, int stars, std::string_view comment) = 0;
    [[nodiscard]] virtual bool hasRated(int64_t txnId, int64_t raterId) = 0;
    [[nodiscard]] virtual std::vector<int> getStarsForUser(int64_t userId) = 0;
};

class NotificationRepo {
public:
    virtual ~NotificationRepo() = default;
    virtual int64_t insert(int64_t userId, std::string_view kind, std::string_view payloadJson, int64_t createdAt) = 0;
    [[nodiscard]] virtual std::vector<Notification> listForUser(int64_t userId, bool unreadOnly) = 0;
    virtual void markRead(int64_t notificationId, int64_t userId) = 0;
};

class DisputeRepo {
public:
    virtual ~DisputeRepo() = default;
    virtual int64_t insert(int64_t txnId, int64_t raisedBy, std::string_view reason, int64_t createdAt) = 0;
    [[nodiscard]] virtual std::optional<Dispute> getById(int64_t id) = 0;
    [[nodiscard]] virtual std::optional<Dispute> getByTxnId(int64_t txnId) = 0;
    [[nodiscard]] virtual std::vector<Dispute> listAll() = 0;
    virtual void decide(int64_t disputeId, int64_t adminId, domain::Money ownerAward, std::string_view note, int64_t now) = 0;
};

// Factory functions
std::unique_ptr<UserRepo> makeUserRepo(class Database& db);
std::unique_ptr<SessionRepo> makeSessionRepo(class Database& db);
std::unique_ptr<VerificationTokenRepo> makeVerificationTokenRepo(class Database& db);
std::unique_ptr<ItemRepo> makeItemRepo(class Database& db);
std::unique_ptr<ThreadRepo> makeThreadRepo(class Database& db);
std::unique_ptr<MessageRepo> makeMessageRepo(class Database& db);
std::unique_ptr<TxnRepo> makeTxnRepo(class Database& db);
std::unique_ptr<LedgerRepo> makeLedgerRepo(class Database& db);
std::unique_ptr<RatingRepo> makeRatingRepo(class Database& db);
std::unique_ptr<NotificationRepo> makeNotificationRepo(class Database& db);
std::unique_ptr<DisputeRepo> makeDisputeRepo(class Database& db);

} // namespace borrowhub::db
