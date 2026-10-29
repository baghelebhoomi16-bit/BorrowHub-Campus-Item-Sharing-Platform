#pragma once

#include "src/domain/Item.hpp"
#include <sstream>

namespace borrowhub::domain {

class Equipment : public Item {
public:
    Equipment(int64_t id,
              int64_t ownerId,
              std::string title,
              std::string category,
              std::string itemCondition,
              std::string location,
              Money deposit,
              int64_t availFrom,
              int64_t availTo,
              ItemStatus status,
              int64_t createdAt,
              std::string equipmentType,
              std::string brand)
        : Item(id, ownerId, std::move(title), std::move(category), std::move(itemCondition),
               std::move(location), deposit, availFrom, availTo, status, createdAt),
          equipmentType_(std::move(equipmentType)),
          brand_(std::move(brand)) {
        if (equipmentType_.empty()) {
            throw util::AppError(util::ErrorCode::VALIDATION, "Equipment type specification cannot be empty");
        }
        validateDeposit();
    }

    [[nodiscard]] const std::string& equipmentType() const noexcept { return equipmentType_; }
    [[nodiscard]] const std::string& brand() const noexcept { return brand_; }

    [[nodiscard]] ItemType type() const noexcept override { return ItemType::EQUIPMENT; }
    [[nodiscard]] std::string_view typeName() const noexcept override { return "EQUIPMENT"; }
    [[nodiscard]] int maxBorrowDays() const noexcept override { return 5; }
    [[nodiscard]] Money penaltyPerDay() const noexcept override { return Money::fromRupees(20); } // 2000 paise
    [[nodiscard]] Money minDeposit() const noexcept override { return Money::fromRupees(200); } // 20000 paise

    [[nodiscard]] std::string attributesJson() const override {
        std::ostringstream oss;
        oss << "{\"equipmentType\":\"" << equipmentType_ << "\",\"brand\":\"" << brand_ << "\"}";
        return oss.str();
    }

private:
    std::string equipmentType_;
    std::string brand_;
};

} // namespace borrowhub::domain
