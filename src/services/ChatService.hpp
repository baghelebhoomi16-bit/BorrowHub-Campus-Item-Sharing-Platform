#pragma once

#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/domain/User.hpp"
#include "src/util/Clock.hpp"

#include <vector>

namespace borrowhub::services {

class ChatService {
public:
    ChatService(db::Database& db,
                db::ThreadRepo& threads,
                db::MessageRepo& messages,
                db::ItemRepo& items,
                const util::Clock& clock);

    // Creates or gets existing chat thread for (itemId, borrower)
    int64_t getOrCreateThread(const domain::User& user, int64_t itemId);

    // Sends message into thread; verifies participant status and rate limit (20 msgs/min)
    int64_t sendMessage(const domain::User& user, int64_t threadId, std::string_view body);

    // Polls messages after given afterId (limit 100, participant authorized)
    [[nodiscard]] std::vector<db::Message> pollMessages(const domain::User& user, int64_t threadId, int64_t afterId);

    // Lists threads for user
    [[nodiscard]] std::vector<db::ChatThread> listThreads(const domain::User& user);

private:
    void requireParticipant(const domain::User& user, int64_t threadId);

    db::Database& db_;
    db::ThreadRepo& threads_;
    db::MessageRepo& messages_;
    db::ItemRepo& items_;
    const util::Clock& clock_;
};

} // namespace borrowhub::services
