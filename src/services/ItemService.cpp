#include "src/services/ItemService.hpp"
#include "src/db/DbTransaction.hpp"
#include "src/domain/ItemFactory.hpp"
#include "src/util/Errors.hpp"
#include "src/util/Validation.hpp"

namespace borrowhub::services {

ItemService::ItemService(db::Database& db,
                         db::ItemRepo& items,
                         db::TxnRepo& txns,
                         const util::Clock& clock,
                         EventBus& eventBus)
    : db_(db),
      items_(items),
      txns_(txns),
      clock_(clock),
      eventBus_(eventBus) {}

int64_t ItemService::createItem(const domain::User& actor, const CreateItemRequest& req) {
    if (!actor.isVerified()) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Only verified students can list items");
    }
    if (actor.isBlocked()) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Blocked accounts cannot list items");
    }

    std::string title = util::Validation::trim(req.title);
    if (title.empty() || title.length() > 200) {
        throw util::AppError(util::ErrorCode::VALIDATION, "Title must be between 1 and 200 characters");
    }

    int64_t now = clock_.now();

    // Factory creates and validates minimum deposit and type-specific attributes
    auto item = domain::ItemFactory::create(
        0, actor.id(), req.type, title, req.category, req.condition,
        req.location, domain::Money::fromPaise(req.depositPaise),
        req.availFrom, req.availTo, domain::ItemStatus::AVAILABLE, now, req.attrsJson
    );

    db::DbTransaction tx(db_);
    int64_t itemId = items_.insert(*item);
    tx.commit();

    return itemId;
}

void ItemService::updateItem(const domain::User& actor, int64_t itemId, const CreateItemRequest& req) {
    auto item = items_.getById(itemId);
    if (!item) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Item not found");
    }
    if (item->ownerId() != actor.id() && actor.role() != domain::UserRole::ADMIN) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Only the owner can update this item");
    }
    if (item->status() == domain::ItemStatus::BORROWED || item->status() == domain::ItemStatus::RESERVED) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Cannot edit item while currently reserved or borrowed");
    }

    // Recreate with factory to enforce type invariants and minimum deposit
    auto updated = domain::ItemFactory::create(
        itemId, item->ownerId(), domain::itemTypeToString(item->type()),
        req.title.empty() ? item->title() : req.title,
        req.category.empty() ? item->category() : req.category,
        req.condition.empty() ? item->itemCondition() : req.condition,
        req.location.empty() ? item->location() : req.location,
        req.depositPaise > 0 ? domain::Money::fromPaise(req.depositPaise) : item->deposit(),
        req.availFrom > 0 ? req.availFrom : item->availFrom(),
        req.availTo > 0 ? req.availTo : item->availTo(),
        item->status(), item->createdAt(),
        req.attrsJson.empty() ? item->attributesJson() : req.attrsJson
    );

    db::DbTransaction tx(db_);
    items_.update(*updated);
    tx.commit();
}

void ItemService::removeItem(const domain::User& actor, int64_t itemId) {
    auto item = items_.getById(itemId);
    if (!item) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Item not found");
    }
    if (item->ownerId() != actor.id() && actor.role() != domain::UserRole::ADMIN) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Only the owner can remove this item");
    }

    if (item->status() == domain::ItemStatus::BORROWED || item->status() == domain::ItemStatus::RESERVED) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Cannot remove item while a live transaction exists");
    }

    db::DbTransaction tx(db_);
    items_.setStatus(itemId, domain::ItemStatus::REMOVED);
    tx.commit();
}

std::unique_ptr<domain::Item> ItemService::getItem(int64_t itemId) {
    auto item = items_.getById(itemId);
    if (!item || item->status() == domain::ItemStatus::REMOVED) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Item not found");
    }
    return item;
}

std::vector<std::unique_ptr<domain::Item>> ItemService::listMyItems(int64_t ownerId) {
    return items_.listByOwner(ownerId);
}

} // namespace borrowhub::services
