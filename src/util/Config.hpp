#pragma once

#include <string>
#include <cstdint>

namespace borrowhub::util {

struct Config {
    uint16_t port{8080};
    std::string dbPath{"data/borrowhub.sqlite"};
    std::string verifyMode{"console"}; // "console" or "auto" or "disabled"
    int sessionTtlHours{24};
    std::string logLevel{"info"};
    std::string staticDir{"web"};

    static Config loadFromEnv();
    void validate() const;
};

} // namespace borrowhub::util
