#pragma once

#include <string>
#include <string_view>

namespace borrowhub::util {

class Token {
public:
    // Generates 32 cryptographically secure random bytes, Base64url encoded (43 chars)
    [[nodiscard]] static std::string generate();

    // Deterministic SHA-256 hex digest of input
    [[nodiscard]] static std::string hashSha256(std::string_view input);

    // Password hashing using Argon2id (or secure standard equivalent)
    [[nodiscard]] static std::string hashPassword(std::string_view password);

    // Verifies password against hashed string
    [[nodiscard]] static bool verifyPassword(std::string_view password, std::string_view hash);
};

} // namespace borrowhub::util
