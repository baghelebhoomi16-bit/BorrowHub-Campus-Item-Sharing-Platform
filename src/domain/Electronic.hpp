#pragma once

#include "src/domain/Item.hpp"
#include <sstream>

namespace borrowhub::domain {

class Electronic : public Item {
public:
    Electronic(int64_t id,
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
               std::string brand,
               std::string model)
        : Item(id, ownerId, std::move(title), std::move(category), std::move(itemCondition),
               std::move(location), deposit, availFrom, availTo, status, createdAt),
          brand_(std::move(brand)),
          model_(std::move(model)) {
        if (brand_.empty()) {
            throw util::AppError(util::ErrorCode::VALIDATION, "Electronic brand cannot be empty");
        }
        if (model_.empty()) {
            throw util::AppError(util::ErrorCode::VALIDATION, "Electronic model cannot be empty");
        }
        validateDeposit();
    }

    [[nodiscard]] const std::string& brand() const noexcept { return brand_; }
    [[nodiscard]] const std::string& model() const noexcept { return model_; }

    [[nodiscard]] ItemType type() const noexcept override { return ItemType::ELECTRONIC; }
    [[nodiscard]] std::string_view typeName() const noexcept override { return "ELECTRONIC"; }
    [[nodiscard]] int maxBorrowDays() const noexcept override { return 3; }
    [[nodiscard]] Money penaltyPerDay() const noexcept override { return Money::fromRupees(50); } // 5000 paise
    [[nodiscard]] Money minDeposit() const noexcept override { return Money::fromRupees(500); } // 50000 paise

    [[nodiscard]] std::string attributesJson() const override {
        std::ostringstream oss;
        oss << "{\"brand\":\"" << brand_ << "\",\"model\":\"" << model_ << "\"}";
        return oss.str();
    }

private:
    std::string brand_;
    std::string model_;
};

} // namespace borrowhub::domain
