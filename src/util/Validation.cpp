#include "src/util/Validation.hpp"
#include "src/util/Errors.hpp"

#include <regex>
#include <algorithm>
#include <cctype>

namespace borrowhub::util {

std::string Validation::trim(std::string_view str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(static_cast<unsigned char>(*start))) {
        ++start;
    }
    auto end = str.end();
    do {
        if (end == start) break;
        --end;
    } while (std::isspace(static_cast<unsigned char>(*end)));

    return std::string(start, end + (start != end ? 1 : 0));
}

bool Validation::hasControlChars(std::string_view str, bool allowNewlines) {
    for (char c : str) {
        auto uc = static_cast<unsigned char>(c);
        if (uc == '\0') {
            return true;
        }
        if (uc < 0x20) {
            if (allowNewlines && (uc == '\n' || uc == '\t' || uc == '\r')) {
                continue;
            }
            return true;
        }
    }
    return false;
}

bool Validation::isValidEmail(std::string_view email) {
    if (email.empty() || email.length() > 254) return false;
    if (hasControlChars(email, false)) return false;

    // Pattern: username@domain.tld
    static const std::regex emailPattern(
        R"(^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)+$)"
    );
    std::string emailStr(email);
    if (!std::regex_match(emailStr, emailPattern)) {
        return false;
    }

    // Must have a valid dot in domain
    auto atPos = email.find('@');
    if (atPos == std::string_view::npos || atPos == 0) return false;
    auto domain = email.substr(atPos + 1);
    if (domain.find('.') == std::string_view::npos) return false;

    return true;
}

bool Validation::isValidPrn(std::string_view prn) {
    if (prn.empty() || prn.length() < 3 || prn.length() > 32) return false;
    if (hasControlChars(prn, false)) return false;

    // Standard campus PRN format: alphanumeric uppercase/digits (e.g., PRN2024001, CS21B001)
    for (char c : prn) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') {
            return false;
        }
    }
    return true;
}

bool Validation::isWithinLength(std::string_view str, size_t minLen, size_t maxLen) {
    return str.length() >= minLen && str.length() <= maxLen;
}

void Validation::validateMessageBody(std::string_view body) {
    std::string trimmed = trim(body);
    if (trimmed.empty() || trimmed.length() > 2000) {
        throw AppError(ErrorCode::VALIDATION, "Message body must be between 1 and 2000 characters");
    }
    if (hasControlChars(body, true)) {
        throw AppError(ErrorCode::VALIDATION, "Message contains invalid control characters or null bytes");
    }
}

void Validation::validateRegistration(std::string_view email,
                                      std::string_view prn,
                                      std::string_view password,
                                      std::string_view name) {
    if (!isValidEmail(email)) {
        throw AppError(ErrorCode::VALIDATION, "Invalid email address");
    }
    if (!prn.empty() && !isValidPrn(prn)) {
        throw AppError(ErrorCode::VALIDATION, "Invalid PRN format");
    }
    if (password.length() < 8) {
        throw AppError(ErrorCode::VALIDATION, "Password must be at least 8 characters long");
    }
    std::string trimmedName = trim(name);
    if (trimmedName.empty() || trimmedName.length() > 100) {
        throw AppError(ErrorCode::VALIDATION, "Name must be between 1 and 100 characters");
    }
    if (hasControlChars(name, false)) {
        throw AppError(ErrorCode::VALIDATION, "Name contains invalid control characters");
    }
}

} // namespace borrowhub::util
