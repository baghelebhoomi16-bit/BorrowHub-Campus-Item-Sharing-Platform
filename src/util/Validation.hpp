#pragma once

#include <string>
#include <string_view>

namespace borrowhub::util {

class Validation {
public:
    // Trims leading and trailing ASCII whitespace
    [[nodiscard]] static std::string trim(std::string_view str);

    // Checks if string contains control characters (e.g. \0, \r, \x01-\x1F except \n, \t in multi-line)
    [[nodiscard]] static bool hasControlChars(std::string_view str, bool allowNewlines = false);

    // Validates email format and optionally checks domain (e.g. .edu or campus.edu)
    [[nodiscard]] static bool isValidEmail(std::string_view email);

    // Validates PRN format (e.g. alphanumeric PRN followed by digits)
    [[nodiscard]] static bool isValidPrn(std::string_view prn);

    // Validates string length bounds
    [[nodiscard]] static bool isWithinLength(std::string_view str, size_t minLen, size_t maxLen);

    // Validates message content (1 to 2000 chars, no null bytes)
    static void validateMessageBody(std::string_view body);

    // Validates registration input
    static void validateRegistration(std::string_view email,
                                     std::string_view prn,
                                     std::string_view password,
                                     std::string_view name);
};

} // namespace borrowhub::util
