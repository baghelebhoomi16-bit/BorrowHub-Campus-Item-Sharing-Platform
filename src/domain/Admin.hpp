#pragma once

#include "src/domain/User.hpp"

namespace borrowhub::domain {

class Admin : public User {
public:
    Admin(int64_t id,
          std::string email,
          std::string passwordHash,
          std::string name,
          std::string phone,
          int64_t createdAt);

    [[nodiscard]] UserRole role() const noexcept override { return UserRole::ADMIN; }

    // Blueprint 3.1: "Admin: moderation and dispute resolution. Cannot borrow."
    [[nodiscard]] bool canRequest() const noexcept override { return false; }
};

} // namespace borrowhub::domain
