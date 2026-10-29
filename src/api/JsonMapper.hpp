#pragma once

#include "src/db/Repositories.hpp"
#include "src/domain/BorrowTransaction.hpp"
#include "src/domain/Item.hpp"
#include "src/domain/Money.hpp"
#include "src/domain/User.hpp"

#include <string>
#include <vector>

namespace borrowhub::api {

class JsonMapper {
public:
    // User serialization: Never emits password_hash or session data
    [[nodiscard]] static std::string toJson(const domain::User& user, bool revealPhone = false);

    // Item serialization with formatted deposit
    [[nodiscard]] static std::string toJson(const domain::Item& item);

    // List of items
    [[nodiscard]] static std::string toJson(const std::vector<std::unique_ptr<domain::Item>>& items, int totalCount, int page, int pageSize);

    // Chat message serialization
    [[nodiscard]] static std::string toJson(const db::Message& msg);
    [[nodiscard]] static std::string toJson(const std::vector<db::Message>& msgs);

    // Chat thread serialization
    [[nodiscard]] static std::string toJson(const db::ChatThread& thread);
    [[nodiscard]] static std::string toJson(const std::vector<db::ChatThread>& threads);

    // Transaction serialization (phone privacy enforced based on actor relationship)
    [[nodiscard]] static std::string toJson(const domain::BorrowTransaction& txn,
                                            int64_t currentUserId = 0,
                                            const std::string& ownerPhone = "",
                                            const std::string& borrowerPhone = "");

    // List of transactions
    [[nodiscard]] static std::string toJson(const std::vector<std::unique_ptr<domain::BorrowTransaction>>& txns, int64_t currentUserId);

    // Notification serialization
    [[nodiscard]] static std::string toJson(const db::Notification& n);
    [[nodiscard]] static std::string toJson(const std::vector<db::Notification>& notifications);

    // Dispute serialization
    [[nodiscard]] static std::string toJson(const db::Dispute& d);
    [[nodiscard]] static std::string toJson(const std::vector<db::Dispute>& disputes);

    // Error response JSON
    [[nodiscard]] static std::string errorJson(std::string_view code, std::string_view message);

    // Helper: string escaping for valid JSON
    [[nodiscard]] static std::string escape(std::string_view str);
};

} // namespace borrowhub::api
