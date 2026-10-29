#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace borrowhub::services {

enum class EventType {
    USER_REGISTERED,
    REQUEST_CREATED,
    REQUEST_ACCEPTED,
    REQUEST_REJECTED,
    REQUEST_CANCELLED,
    DEPOSIT_PAID,
    HANDOVER_CONFIRMED,
    TRANSACTION_ACTIVE,
    TRANSACTION_OVERDUE,
    RETURN_REPORTED,
    SETTLED,
    DISPUTE_RAISED,
    DISPUTE_DECIDED,
    DUE_REMINDER
};

struct DomainEvent {
    EventType type;
    int64_t targetUserId{0};
    int64_t actorId{0};
    int64_t entityId{0};
    std::string payloadJson;
};

class EventBus {
public:
    using EventHandler = std::function<void(const DomainEvent&)>;

    void subscribe(EventType type, EventHandler handler) {
        handlers_[type].push_back(std::move(handler));
    }

    void publish(const DomainEvent& event) {
        auto it = handlers_.find(event.type);
        if (it != handlers_.end()) {
            for (const auto& handler : it->second) {
                try {
                    handler(event);
                } catch (...) {
                    // Per Blueprint 10.9: "A throwing handler does not roll back the business transaction (log and continue)"
                }
            }
        }
    }

private:
    std::unordered_map<EventType, std::vector<EventHandler>> handlers_;
};

} // namespace borrowhub::services
