#pragma once

#include "src/domain/Money.hpp"
#include "src/util/Errors.hpp"

#include <string>
#include <string_view>
#include <cstdint>

namespace borrowhub::domain {

enum class ItemType {
    BOOK,
    ELECTRONIC,
    EQUIPMENT
};

enum class ItemStatus {
    AVAILABLE,
    RESERVED,
    BORROWED,
    UNAVAILABLE,
    REMOVED
};

[[nodiscard]] constexpr std::string_view itemTypeToString(ItemType type) noexcept {
    switch (type) {
        case ItemType::BOOK: return "BOOK";
        case ItemType::ELECTRONIC: return "ELECTRONIC";
        case ItemType::EQUIPMENT: return "EQUIPMENT";
    }
    return "BOOK";
}

[[nodiscard]] constexpr std::string_view itemStatusToString(ItemStatus status) noexcept {
    switch (status) {
        case ItemStatus::AVAILABLE: return "AVAILABLE";
        case ItemStatus::RESERVED: return "RESERVED";
        case ItemStatus::BORROWED: return "BORROWED";
        case ItemStatus::UNAVAILABLE: return "UNAVAILABLE";
        case ItemStatus::REMOVED: return "REMOVED";
    }
    return "AVAILABLE";
}

ItemType stringToItemType(std::string_view str);
ItemStatus stringToItemStatus(std::string_view str);

class Item {
public:
    Item(int64_t id,
         int64_t ownerId,
         std::string title,
         std::string category,
         std::string itemCondition,
         std::string location,
         Money deposit,
         int64_t availFrom,
         int64_t availTo,
         ItemStatus status,
         int64_t createdAt);

    virtual ~Item() = default;

    [[nodiscard]] int64_t id() const noexcept { return id_; }
    [[nodiscard]] int64_t ownerId() const noexcept { return ownerId_; }
    [[nodiscard]] const std::string& title() const noexcept { return title_; }
    [[nodiscard]] const std::string& category() const noexcept { return category_; }
    [[nodiscard]] const std::string& itemCondition() const noexcept { return itemCondition_; }
    [[nodiscard]] const std::string& location() const noexcept { return location_; }
    [[nodiscard]] Money deposit() const noexcept { return deposit_; }
    [[nodiscard]] int64_t availFrom() const noexcept { return availFrom_; }
    [[nodiscard]] int64_t availTo() const noexcept { return availTo_; }
    [[nodiscard]] ItemStatus status() const noexcept { return status_; }
    [[nodiscard]] int64_t createdAt() const noexcept { return createdAt_; }

    void setTitle(std::string title) { title_ = std::move(title); }
    void setCategory(std::string category) { category_ = std::move(category); }
    void setItemCondition(std::string cond) { itemCondition_ = std::move(cond); }
    void setLocation(std::string loc) { location_ = std::move(loc); }
    void setDeposit(Money deposit);
    void setAvailability(int64_t from, int64_t to);
    void setStatus(ItemStatus status) noexcept { status_ = status; }

    // Pure virtual polymorphic methods required by rubric
    [[nodiscard]] virtual ItemType type() const noexcept = 0;
    [[nodiscard]] virtual std::string_view typeName() const noexcept = 0;
    [[nodiscard]] virtual int maxBorrowDays() const noexcept = 0;
    [[nodiscard]] virtual Money penaltyPerDay() const noexcept = 0;
    [[nodiscard]] virtual Money minDeposit() const noexcept = 0;
    [[nodiscard]] virtual std::string attributesJson() const = 0;

    // Checks that deposit satisfies the subclass minimum deposit rule
    void validateDeposit() const;

protected:
    int64_t id_{0};
    int64_t ownerId_{0};
    std::string title_;
    std::string category_;
    std::string itemCondition_;
    std::string location_;
    Money deposit_;
    int64_t availFrom_{0};
    int64_t availTo_{0};
    ItemStatus status_{ItemStatus::AVAILABLE};
    int64_t createdAt_{0};
};

} // namespace borrowhub::domain
