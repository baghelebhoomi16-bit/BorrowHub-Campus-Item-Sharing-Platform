#include "src/services/NotificationService.hpp"
#include <iostream>

namespace borrowhub::services {

NotificationService::NotificationService(db::Database& db,
                                         db::NotificationRepo& notifications,
                                         const util::Clock& clock,
                                         EventBus& eventBus)
    : db_(db),
      notifications_(notifications),
      clock_(clock),
      eventBus_(eventBus) {
    registerHandlers();
}

void NotificationService::registerHandlers() {
    auto notifyHandler = [this](std::string kind) {
        return [this, kind = std::move(kind)](const DomainEvent& event) {
            if (event.targetUserId > 0) {
                createNotification(event.targetUserId, kind, event.payloadJson);
            }
        };
    };

    eventBus_.subscribe(EventType::REQUEST_CREATED, notifyHandler("REQUEST_CREATED"));
    eventBus_.subscribe(EventType::REQUEST_ACCEPTED, notifyHandler("REQUEST_ACCEPTED"));
    eventBus_.subscribe(EventType::REQUEST_REJECTED, notifyHandler("REQUEST_REJECTED"));
    eventBus_.subscribe(EventType::REQUEST_CANCELLED, notifyHandler("REQUEST_CANCELLED"));
    eventBus_.subscribe(EventType::DEPOSIT_PAID, notifyHandler("DEPOSIT_PAID"));
    eventBus_.subscribe(EventType::HANDOVER_CONFIRMED, notifyHandler("HANDOVER_CONFIRMED"));
    eventBus_.subscribe(EventType::TRANSACTION_ACTIVE, notifyHandler("TRANSACTION_ACTIVE"));
    eventBus_.subscribe(EventType::TRANSACTION_OVERDUE, notifyHandler("TRANSACTION_OVERDUE"));
    eventBus_.subscribe(EventType::RETURN_REPORTED, notifyHandler("RETURN_REPORTED"));
    eventBus_.subscribe(EventType::SETTLED, notifyHandler("SETTLED"));
    eventBus_.subscribe(EventType::DISPUTE_RAISED, notifyHandler("DISPUTE_RAISED"));
    eventBus_.subscribe(EventType::DISPUTE_DECIDED, notifyHandler("DISPUTE_DECIDED"));
    eventBus_.subscribe(EventType::DUE_REMINDER, notifyHandler("DUE_REMINDER"));
}

void NotificationService::createNotification(int64_t userId, std::string kind, std::string payload) {
    try {
        int64_t now = clock_.now();
        notifications_.insert(userId, kind, payload, now);
    } catch (const std::exception& e) {
        std::cerr << "Failed to write notification for user " << userId << ": " << e.what() << "\n";
    }
}

std::vector<db::Notification> NotificationService::listNotifications(int64_t userId, bool unreadOnly) {
    return notifications_.listForUser(userId, unreadOnly);
}

void NotificationService::markAsRead(int64_t notificationId, int64_t userId) {
    notifications_.markRead(notificationId, userId);
}

} // namespace borrowhub::services
