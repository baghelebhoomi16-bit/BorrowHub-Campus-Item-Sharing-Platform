#pragma once

#include <exception>
#include <string>
#include <string_view>

namespace borrowhub::util {

enum class ErrorCode {
    VALIDATION,
    UNAUTHENTICATED,
    FORBIDDEN,
    NOT_FOUND,
    CONFLICT,
    ILLEGAL_STATE,
    RATE_LIMITED,
    INTERNAL
};

[[nodiscard]] constexpr std::string_view errorCodeToString(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::VALIDATION: return "VALIDATION";
        case ErrorCode::UNAUTHENTICATED: return "UNAUTHENTICATED";
        case ErrorCode::FORBIDDEN: return "FORBIDDEN";
        case ErrorCode::NOT_FOUND: return "NOT_FOUND";
        case ErrorCode::CONFLICT: return "CONFLICT";
        case ErrorCode::ILLEGAL_STATE: return "ILLEGAL_STATE";
        case ErrorCode::RATE_LIMITED: return "RATE_LIMITED";
        case ErrorCode::INTERNAL: return "INTERNAL";
    }
    return "INTERNAL";
}

[[nodiscard]] constexpr int errorCodeToHttpStatus(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::VALIDATION: return 400;
        case ErrorCode::UNAUTHENTICATED: return 401;
        case ErrorCode::FORBIDDEN: return 403;
        case ErrorCode::NOT_FOUND: return 404;
        case ErrorCode::CONFLICT: return 409;
        case ErrorCode::ILLEGAL_STATE: return 409;
        case ErrorCode::RATE_LIMITED: return 429;
        case ErrorCode::INTERNAL: return 500;
    }
    return 500;
}

class AppError : public std::exception {
public:
    AppError(ErrorCode code, std::string message)
        : code_(code), message_(std::move(message)) {}

    [[nodiscard]] const char* what() const noexcept override {
        return message_.c_str();
    }

    [[nodiscard]] ErrorCode code() const noexcept {
        return code_;
    }

    [[nodiscard]] const std::string& message() const noexcept {
        return message_;
    }

    [[nodiscard]] int httpStatus() const noexcept {
        return errorCodeToHttpStatus(code_);
    }

    [[nodiscard]] std::string_view codeString() const noexcept {
        return errorCodeToString(code_);
    }

private:
    ErrorCode code_;
    std::string message_;
};

} // namespace borrowhub::util
