#include "src/domain/Item.hpp"

namespace borrowhub::domain {

ItemType stringToItemType(std::string_view str) {
    if (str == "BOOK") return ItemType::BOOK;
    if (str == "ELECTRONIC") return ItemType::ELECTRONIC;
    if (str == "EQUIPMENT") return ItemType::EQUIPMENT;
    throw util::AppError(util::ErrorCode::VALIDATION, "Unknown item type: " + std::string(str));
}

ItemStatus stringToItemStatus(std::string_view str) {
    if (str == "AVAILABLE") return ItemStatus::AVAILABLE;
    if (str == "RESERVED") return ItemStatus::RESERVED;
    if (str == "BORROWED") return ItemStatus::BORROWED;
    if (str == "UNAVAILABLE") return ItemStatus::UNAVAILABLE;
    if (str == "REMOVED") return ItemStatus::REMOVED;
    throw util::AppError(util::ErrorCode::VALIDATION, "Unknown item status: " + std::string(str));
}

Item::Item(int64_t id,
           int64_t ownerId,
           std::string title,
           std::string category,
           std::string itemCondition,
           std::string location,
           Money deposit,
           int64_t availFrom,
           int64_t availTo,
           ItemStatus status,
           int64_t createdAt)
    : id_(id),
      ownerId_(ownerId),
      title_(std::move(title)),
      category_(std::move(category)),
      itemCondition_(std::move(itemCondition)),
      location_(std::move(location)),
      deposit_(deposit),
      availFrom_(availFrom),
      availTo_(availTo),
      status_(status),
      createdAt_(createdAt) {
    if (availTo_ > 0 && availFrom_ > availTo_) {
        throw util::AppError(util::ErrorCode::VALIDATION, "Item availability start date must be before end date");
    }
}

void Item::setDeposit(Money deposit) {
    if (deposit < minDeposit()) {
        throw util::AppError(util::ErrorCode::VALIDATION,
            "Deposit cannot be lower than minimum required for " + std::string(typeName()));
    }
    deposit_ = deposit;
}

void Item::setAvailability(int64_t from, int64_t to) {
    if (to > 0 && from > to) {
        throw util::AppError(util::ErrorCode::VALIDATION, "Invalid availability date range");
    }
    availFrom_ = from;
    availTo_ = to;
}

void Item::validateDeposit() const {
    if (deposit_ < minDeposit()) {
        throw util::AppError(util::ErrorCode::VALIDATION,
            "Deposit of " + deposit_.format() + " is below minimum required (" +
            minDeposit().format() + ") for " + std::string(typeName()));
    }
}

} // namespace borrowhub::domain
