#pragma once

#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/util/Clock.hpp"
#include "src/services/EventBus.hpp"

#include <vector>

namespace borrowhub::services {

class NotificationService {
public:
    NotificationService(db::Database& db,
                        db::NotificationRepo& notifications,
                        const util::Clock& clock,
                        EventBus& eventBus);

    // Subscribes handlers to all relevant domain events on the EventBus
    void registerHandlers();

    // Lists notifications for user
    [[nodiscard]] std::vector<db::Notification> listNotifications(int64_t userId, bool unreadOnly = false);

    // Marks notification as read
    void markAsRead(int64_t notificationId, int64_t userId);

private:
    void createNotification(int64_t userId, std::string kind, std::string payload);

    db::Database& db_;
    db::NotificationRepo& notifications_;
    const util::Clock& clock_;
    EventBus& eventBus_;
};

} // namespace borrowhub::services
