#pragma once

#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/domain/Item.hpp"
#include "src/domain/User.hpp"
#include "src/util/Clock.hpp"
#include "src/services/EventBus.hpp"

#include <memory>
#include <string>
#include <vector>

namespace borrowhub::services {

struct CreateItemRequest {
    std::string type;
    std::string title;
    std::string category;
    std::string condition;
    std::string location;
    int64_t depositPaise{0};
    int64_t availFrom{0};
    int64_t availTo{0};
    std::string attrsJson;
};

class ItemService {
public:
    ItemService(db::Database& db,
                db::ItemRepo& items,
                db::TxnRepo& txns,
                const util::Clock& clock,
                EventBus& eventBus);

    // Creates an item listing; verifies owner eligibility
    int64_t createItem(const domain::User& actor, const CreateItemRequest& req);

    // Updates an existing listing; owner only
    void updateItem(const domain::User& actor, int64_t itemId, const CreateItemRequest& req);

    // Soft-deletes item to REMOVED status; blocked if active borrowing exists
    void removeItem(const domain::User& actor, int64_t itemId);

    // Retrieves item details
    [[nodiscard]] std::unique_ptr<domain::Item> getItem(int64_t itemId);

    // Lists items owned by user
    [[nodiscard]] std::vector<std::unique_ptr<domain::Item>> listMyItems(int64_t ownerId);

private:
    db::Database& db_;
    db::ItemRepo& items_;
    db::TxnRepo& txns_;
    const util::Clock& clock_;
    EventBus& eventBus_;
};

} // namespace borrowhub::services
