#pragma once

#include "src/domain/User.hpp"
#include "src/services/AuthService.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace borrowhub::api {

class AuthMiddleware {
public:
    explicit AuthMiddleware(services::AuthService& authService);

    // Extracts session cookie from HTTP Cookie header
    [[nodiscard]] static std::string extractSessionCookie(std::string_view cookieHeader);

    // Resolves authenticated user from session cookie string
    [[nodiscard]] std::unique_ptr<domain::User> authenticate(std::string_view cookieHeader);

    // Checks if route path is public (unauthenticated)
    [[nodiscard]] static bool isPublicRoute(std::string_view path);

    // Requires admin privilege on user, throws FORBIDDEN (403) if student
    static void requireAdmin(const domain::User& user);

private:
    services::AuthService& authService_;
};

} // namespace borrowhub::api
