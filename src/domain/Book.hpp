#pragma once

#include "src/domain/Item.hpp"
#include <sstream>

namespace borrowhub::domain {

class Book : public Item {
public:
    Book(int64_t id,
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
         std::string author,
         std::string isbn)
        : Item(id, ownerId, std::move(title), std::move(category), std::move(itemCondition),
               std::move(location), deposit, availFrom, availTo, status, createdAt),
          author_(std::move(author)),
          isbn_(std::move(isbn)) {
        if (author_.empty()) {
            throw util::AppError(util::ErrorCode::VALIDATION, "Book author cannot be empty");
        }
        if (isbn_.length() != 10 && isbn_.length() != 13) {
            throw util::AppError(util::ErrorCode::VALIDATION, "ISBN must be 10 or 13 characters");
        }
        validateDeposit();
    }

    [[nodiscard]] const std::string& author() const noexcept { return author_; }
    [[nodiscard]] const std::string& isbn() const noexcept { return isbn_; }

    [[nodiscard]] ItemType type() const noexcept override { return ItemType::BOOK; }
    [[nodiscard]] std::string_view typeName() const noexcept override { return "BOOK"; }
    [[nodiscard]] int maxBorrowDays() const noexcept override { return 7; }
    [[nodiscard]] Money penaltyPerDay() const noexcept override { return Money::fromRupees(5); } // 500 paise
    [[nodiscard]] Money minDeposit() const noexcept override { return Money::fromRupees(100); } // 10000 paise

    [[nodiscard]] std::string attributesJson() const override {
        std::ostringstream oss;
        oss << "{\"author\":\"" << author_ << "\",\"isbn\":\"" << isbn_ << "\"}";
        return oss.str();
    }

private:
    std::string author_;
    std::string isbn_;
};

} // namespace borrowhub::domain
