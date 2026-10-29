#include "src/services/AuthService.hpp"
#include "src/db/DbTransaction.hpp"
#include "src/domain/Student.hpp"
#include "src/util/Errors.hpp"
#include "src/util/Token.hpp"
#include "src/util/Validation.hpp"

#include <iostream>

namespace borrowhub::services {

AuthService::AuthService(db::Database& db,
                         db::UserRepo& users,
                         db::SessionRepo& sessions,
                         db::VerificationTokenRepo& verifyTokens,
                         const util::Clock& clock,
                         const util::Config& config,
                         EventBus& eventBus)
    : db_(db),
      users_(users),
      sessions_(sessions),
      verifyTokens_(verifyTokens),
      clock_(clock),
      config_(config),
      eventBus_(eventBus) {}

int64_t AuthService::registerStudent(const RegisterRequest& req) {
    util::Validation::validateRegistration(req.email, req.prn, req.password, req.name);

    if (users_.getByEmail(req.email)) {
        throw util::AppError(util::ErrorCode::CONFLICT, "Email is already registered");
    }
    if (!req.prn.empty() && users_.getByPrn(req.prn)) {
        throw util::AppError(util::ErrorCode::CONFLICT, "PRN is already registered");
    }

    db::DbTransaction tx(db_);

    int64_t now = clock_.now();
    std::string passwordHash = util::Token::hashPassword(req.password);

    domain::Student student(
        0, req.email, passwordHash, req.name, req.phone,
        req.prn, req.department, req.year, req.division,
        62.0, false, false, now
    );

    int64_t userId = users_.insert(student, req.prn, req.department, req.year, req.division);

    // Generate email verification token (valid for 24h)
    std::string rawVerifyToken = util::Token::generate();
    std::string tokenHash = util::Token::hashSha256(rawVerifyToken);

    verifyTokens_.insert(db::VerificationToken{
        tokenHash,
        userId,
        now + 86400
    });

    tx.commit();

    // Verification link logging (per Blueprint 9 & 10.9)
    if (config_.verifyMode == "console" || config_.verifyMode == "auto") {
        std::cout << "[BORROWHUB AUTH] Email verification link for " << req.email << ": "
                  << "http://localhost:" << config_.port << "/#verify=" << rawVerifyToken << "\n";
    }

    eventBus_.publish(DomainEvent{
        EventType::USER_REGISTERED,
        userId,
        userId,
        userId,
        "{\"email\":\"" + req.email + "\"}"
    });

    return userId;
}

void AuthService::verifyAccount(std::string_view rawToken) {
    std::string tokenHash = util::Token::hashSha256(rawToken);
    auto tokenOpt = verifyTokens_.getByTokenHash(tokenHash);

    if (!tokenOpt.has_value()) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Invalid or expired verification token");
    }

    int64_t now = clock_.now();
    if (tokenOpt.value().expiresAt < now) {
        verifyTokens_.deleteByTokenHash(tokenHash);
        throw util::AppError(util::ErrorCode::VALIDATION, "Verification token has expired");
    }

    db::DbTransaction tx(db_);
    users_.setVerified(tokenOpt.value().userId, true);
    verifyTokens_.deleteByTokenHash(tokenHash);
    tx.commit();
}

LoginResult AuthService::login(std::string_view email, std::string_view password) {
    auto user = users_.getByEmail(email);
    // Generic error text for unknown email and wrong password
    const std::string genericError = "Invalid email or password";

    if (!user) {
        throw util::AppError(util::ErrorCode::UNAUTHENTICATED, genericError);
    }

    if (!util::Token::verifyPassword(password, user->passwordHash())) {
        throw util::AppError(util::ErrorCode::UNAUTHENTICATED, genericError);
    }

    if (!user->isVerified()) {
        throw util::AppError(util::ErrorCode::UNAUTHENTICATED, "Account email is not verified yet");
    }

    if (user->isBlocked()) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Account has been blocked by an administrator");
    }

    int64_t now = clock_.now();
    int64_t ttlSeconds = static_cast<int64_t>(config_.sessionTtlHours) * 3600;

    std::string rawToken = util::Token::generate();
    std::string tokenHash = util::Token::hashSha256(rawToken);

    sessions_.insert(db::Session{
        tokenHash,
        user->id(),
        now + ttlSeconds
    });

    return LoginResult{std::move(user), rawToken};
}

void AuthService::logout(std::string_view rawSessionToken) {
    if (rawSessionToken.empty()) return;
    std::string tokenHash = util::Token::hashSha256(rawSessionToken);
    sessions_.deleteByTokenHash(tokenHash);
}

std::unique_ptr<domain::User> AuthService::authenticateSession(std::string_view rawSessionToken) {
    if (rawSessionToken.empty()) {
        throw util::AppError(util::ErrorCode::UNAUTHENTICATED, "Session cookie missing");
    }

    std::string tokenHash = util::Token::hashSha256(rawSessionToken);
    auto session = sessions_.getByTokenHash(tokenHash);
    if (!session.has_value()) {
        throw util::AppError(util::ErrorCode::UNAUTHENTICATED, "Invalid session");
    }

    int64_t now = clock_.now();
    if (session.value().expiresAt < now) {
        sessions_.deleteByTokenHash(tokenHash);
        throw util::AppError(util::ErrorCode::UNAUTHENTICATED, "Session expired");
    }

    auto user = users_.getById(session.value().userId);
    if (!user) {
        throw util::AppError(util::ErrorCode::UNAUTHENTICATED, "User associated with session not found");
    }
    if (user->isBlocked()) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "User account is blocked");
    }

    return user;
}

} // namespace borrowhub::services
