#include "src/domain/Admin.hpp"

namespace borrowhub::domain {

Admin::Admin(int64_t id,
             std::string email,
             std::string passwordHash,
             std::string name,
             std::string phone,
             int64_t createdAt)
    : User(id, std::move(email), std::move(passwordHash), std::move(name), std::move(phone), true, false, createdAt) {}

} // namespace borrowhub::domain
