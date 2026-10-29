#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace borrowhub::domain {

enum class UserRole {
    STUDENT,
    ADMIN
};

[[nodiscard]] constexpr std::string_view userRoleToString(UserRole role) noexcept {
    switch (role) {
        case UserRole::STUDENT: return "STUDENT";
        case UserRole::ADMIN: return "ADMIN";
    }
    return "STUDENT";
}

class User {
public:
    User(int64_t id,
         std::string email,
         std::string passwordHash,
         std::string name,
         std::string phone,
         bool verified,
         bool blocked,
         int64_t createdAt);

    virtual ~User() = default;

    [[nodiscard]] int64_t id() const noexcept { return id_; }
    [[nodiscard]] const std::string& email() const noexcept { return email_; }
    [[nodiscard]] const std::string& passwordHash() const noexcept { return passwordHash_; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::string& phone() const noexcept { return phone_; }
    [[nodiscard]] bool isVerified() const noexcept { return verified_; }
    [[nodiscard]] bool isBlocked() const noexcept { return blocked_; }
    [[nodiscard]] int64_t createdAt() const noexcept { return createdAt_; }

    virtual void setVerified(bool v) noexcept { verified_ = v; }
    virtual void setBlocked(bool b) noexcept { blocked_ = b; }
    virtual void setPhone(std::string phone) { phone_ = std::move(phone); }

    [[nodiscard]] virtual UserRole role() const noexcept = 0;
    [[nodiscard]] virtual bool canRequest() const noexcept = 0;

protected:
    int64_t id_{0};
    std::string email_;
    std::string passwordHash_;
    std::string name_;
    std::string phone_;
    bool verified_{false};
    bool blocked_{false};
    int64_t createdAt_{0};
};

} // namespace borrowhub::domain
