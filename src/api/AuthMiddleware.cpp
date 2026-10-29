#include "src/api/AuthMiddleware.hpp"
#include "src/util/Errors.hpp"

namespace borrowhub::api {

AuthMiddleware::AuthMiddleware(services::AuthService& authService)
    : authService_(authService) {}

std::string AuthMiddleware::extractSessionCookie(std::string_view cookieHeader) {
    if (cookieHeader.empty()) return "";

    std::string key = "bh_session=";
    auto pos = cookieHeader.find(key);
    if (pos == std::string_view::npos) return "";

    auto start = pos + key.length();
    auto end = cookieHeader.find(';', start);
    if (end == std::string_view::npos) {
        return std::string(cookieHeader.substr(start));
    }
    return std::string(cookieHeader.substr(start, end - start));
}

std::unique_ptr<domain::User> AuthMiddleware::authenticate(std::string_view cookieHeader) {
    std::string token = extractSessionCookie(cookieHeader);
    if (token.empty()) {
        throw util::AppError(util::ErrorCode::UNAUTHENTICATED, "Authentication required: missing bh_session cookie");
    }
    return authService_.authenticateSession(token);
}

bool AuthMiddleware::isPublicRoute(std::string_view path) {
    if (path == "/health" ||
        path == "/api/auth/register" ||
        path == "/api/auth/verify" ||
        path == "/api/auth/login") {
        return true;
    }
    // Static assets
    if (path == "/" || path == "/index.html" || path == "/app.js" ||
        path == "/chat.js" || path == "/styles.css" || path.rfind("/favicon", 0) == 0) {
        return true;
    }
    return false;
}

void AuthMiddleware::requireAdmin(const domain::User& user) {
    if (user.role() != domain::UserRole::ADMIN) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Forbidden: Administrative access required");
    }
}

} // namespace borrowhub::api
