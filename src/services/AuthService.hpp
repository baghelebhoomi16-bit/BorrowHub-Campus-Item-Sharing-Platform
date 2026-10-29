#pragma once

#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/util/Clock.hpp"
#include "src/util/Config.hpp"
#include "src/services/EventBus.hpp"

#include <memory>
#include <string>

namespace borrowhub::services {

struct RegisterRequest {
    std::string email;
    std::string prn;
    std::string password;
    std::string name;
    std::string phone;
    std::string department;
    int year{1};
    std::string division;
};

struct LoginResult {
    std::unique_ptr<domain::User> user;
    std::string rawSessionToken;
};

class AuthService {
public:
    AuthService(db::Database& db,
                db::UserRepo& users,
                db::SessionRepo& sessions,
                db::VerificationTokenRepo& verifyTokens,
                const util::Clock& clock,
                const util::Config& config,
                EventBus& eventBus);

    // Registers a student account, stores hashed verification token, and emits verification link
    int64_t registerStudent(const RegisterRequest& req);

    // Verifies account via raw token string
    void verifyAccount(std::string_view rawToken);

    // Logs in user, verifies password, and produces session token
    [[nodiscard]] LoginResult login(std::string_view email, std::string_view password);

    // Invalidates session
    void logout(std::string_view rawSessionToken);

    // Validates session and returns authenticated User
    [[nodiscard]] std::unique_ptr<domain::User> authenticateSession(std::string_view rawSessionToken);

private:
    db::Database& db_;
    db::UserRepo& users_;
    db::SessionRepo& sessions_;
    db::VerificationTokenRepo& verifyTokens_;
    const util::Clock& clock_;
    const util::Config& config_;
    EventBus& eventBus_;
};

} // namespace borrowhub::services
