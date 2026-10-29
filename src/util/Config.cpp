#include "src/util/Config.hpp"
#include "src/util/Errors.hpp"

#include <cstdlib>
#include <stdexcept>

namespace borrowhub::util {

Config Config::loadFromEnv() {
    Config cfg;

    if (const char* portStr = std::getenv("BH_PORT")) {
        try {
            int p = std::stoi(portStr);
            cfg.port = static_cast<uint16_t>(p);
        } catch (...) {
            throw AppError(ErrorCode::VALIDATION, "Invalid BH_PORT environment variable");
        }
    }

    if (const char* dbPath = std::getenv("BH_DB_PATH")) {
        cfg.dbPath = dbPath;
    }

    if (const char* mode = std::getenv("BH_VERIFY_MODE")) {
        cfg.verifyMode = mode;
    }

    if (const char* ttlStr = std::getenv("BH_SESSION_TTL_HOURS")) {
        try {
            cfg.sessionTtlHours = std::stoi(ttlStr);
        } catch (...) {
            throw AppError(ErrorCode::VALIDATION, "Invalid BH_SESSION_TTL_HOURS");
        }
    }

    if (const char* log = std::getenv("BH_LOG_LEVEL")) {
        cfg.logLevel = log;
    }

    if (const char* staticDir = std::getenv("BH_STATIC_DIR")) {
        cfg.staticDir = staticDir;
    }

    cfg.validate();
    return cfg;
}

void Config::validate() const {
    if (port == 0) {
        throw AppError(ErrorCode::VALIDATION, "Port must be in range 1-65535");
    }
    if (sessionTtlHours <= 0) {
        throw AppError(ErrorCode::VALIDATION, "Session TTL hours must be greater than 0");
    }
    if (dbPath.empty()) {
        throw AppError(ErrorCode::VALIDATION, "Database path cannot be empty");
    }
}

} // namespace borrowhub::util
