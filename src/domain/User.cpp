#include "src/domain/User.hpp"

namespace borrowhub::domain {

User::User(int64_t id,
           std::string email,
           std::string passwordHash,
           std::string name,
           std::string phone,
           bool verified,
           bool blocked,
           int64_t createdAt)
    : id_(id),
      email_(std::move(email)),
      passwordHash_(std::move(passwordHash)),
      name_(std::move(name)),
      phone_(std::move(phone)),
      verified_(verified),
      blocked_(blocked),
      createdAt_(createdAt) {}

} // namespace borrowhub::domain
