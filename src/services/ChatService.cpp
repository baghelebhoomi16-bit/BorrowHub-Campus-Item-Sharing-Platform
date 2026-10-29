#include "src/services/ChatService.hpp"
#include "src/db/DbTransaction.hpp"
#include "src/util/Errors.hpp"
#include "src/util/Validation.hpp"

namespace borrowhub::services {

ChatService::ChatService(db::Database& db,
                         db::ThreadRepo& threads,
                         db::MessageRepo& messages,
                         db::ItemRepo& items,
                         const util::Clock& clock)
    : db_(db),
      threads_(threads),
      messages_(messages),
      items_(items),
      clock_(clock) {}

int64_t ChatService::getOrCreateThread(const domain::User& user, int64_t itemId) {
    if (user.isBlocked()) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Blocked users cannot participate in chat");
    }

    auto item = items_.getById(itemId);
    if (!item || item->status() == domain::ItemStatus::REMOVED) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Item not found");
    }

    // Blueprint 10.9: "Owner cannot open a thread on own item"
    if (item->ownerId() == user.id()) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Item owner cannot open negotiation thread with themselves");
    }

    db::DbTransaction tx(db_);
    int64_t threadId = threads_.getOrCreate(itemId, user.id());
    tx.commit();

    return threadId;
}

int64_t ChatService::sendMessage(const domain::User& user, int64_t threadId, std::string_view body) {
    if (user.isBlocked()) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Blocked users cannot send messages");
    }

    requireParticipant(user, threadId);
    util::Validation::validateMessageBody(body);

    int64_t now = clock_.now();

    // Blueprint 10.9: "At most 20 messages per minute. 21st message in a minute is RATE_LIMITED."
    int recentCount = messages_.countMessagesInWindow(user.id(), 60, now);
    if (recentCount >= 20) {
        throw util::AppError(util::ErrorCode::RATE_LIMITED, "Chat velocity limit reached (max 20 messages per minute)");
    }

    db::DbTransaction tx(db_);
    int64_t msgId = messages_.insert(threadId, user.id(), util::Validation::trim(body), now);
    tx.commit();

    return msgId;
}

std::vector<db::Message> ChatService::pollMessages(const domain::User& user, int64_t threadId, int64_t afterId) {
    requireParticipant(user, threadId);
    return messages_.listForThreadAfter(threadId, afterId, 100);
}

std::vector<db::ChatThread> ChatService::listThreads(const domain::User& user) {
    return threads_.listForUser(user.id());
}

void ChatService::requireParticipant(const domain::User& user, int64_t threadId) {
    auto thread = threads_.getById(threadId);
    if (!thread.has_value()) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Chat thread not found");
    }

    if (thread.value().borrowerId == user.id()) {
        return; // Valid borrower
    }

    auto item = items_.getById(thread.value().itemId);
    if (item && item->ownerId() == user.id()) {
        return; // Valid owner
    }

    if (user.role() == domain::UserRole::ADMIN) {
        return; // Admin access for dispute moderation
    }

    // Stranger cannot read or send (FORBIDDEN)
    throw util::AppError(util::ErrorCode::FORBIDDEN, "Not authorized: participant access required for this chat thread");
}

} // namespace borrowhub::services
