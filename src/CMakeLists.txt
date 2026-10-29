# src/CMakeLists.txt

set(CORE_SOURCES
    # util
    util/Config.cpp
    util/Validation.cpp
    util/Token.cpp

    # domain
    domain/User.cpp
    domain/Student.cpp
    domain/Admin.cpp
    domain/Item.cpp
    domain/ItemFactory.cpp
    domain/PenaltyPolicy.cpp
    domain/BorrowTransaction.cpp
    domain/Reputation.cpp

    # db
    db/Database.cpp
    db/SqliteRepositories.cpp

    # services
    services/AuthService.cpp
    services/ItemService.cpp
    services/SearchService.cpp
    services/ChatService.cpp
    services/BorrowService.cpp
    services/SettlementService.cpp
    services/RatingService.cpp
    services/DisputeService.cpp
    services/NotificationService.cpp
    services/Scheduler.cpp

    # api
    api/RateLimiter.cpp
    api/JsonMapper.cpp
    api/AuthMiddleware.cpp
    api/Server.cpp
    api/Router.cpp
)

# Core static library shared across server, CLI, and test binaries
add_library(borrowhub_core STATIC ${CORE_SOURCES})

target_include_directories(borrowhub_core PUBLIC
    ${CMAKE_SOURCE_DIR}
)

# Link external dependencies
if (TARGET SQLite::SQLite3)
    target_link_libraries(borrowhub_core PUBLIC SQLite::SQLite3)
else()
    target_link_libraries(borrowhub_core PUBLIC sqlite3)
endif()

if (TARGET httplib::httplib)
    target_link_libraries(borrowhub_core PUBLIC httplib::httplib)
endif()

if (TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(borrowhub_core PUBLIC nlohmann_json::nlohmann_json)
endif()

if (TARGET libsodium::libsodium)
    target_link_libraries(borrowhub_core PUBLIC libsodium::libsodium)
endif()

if (WIN32)
    target_link_libraries(borrowhub_core PUBLIC ws2_32 crypt32 bcrypt)
endif()

# Main HTTP server executable
add_executable(borrowhub main.cpp)
target_link_libraries(borrowhub PRIVATE borrowhub_core)

# Admin CLI executable
add_executable(borrowhub_admin cli/admin_cli.cpp)
target_link_libraries(borrowhub_admin PRIVATE borrowhub_core)
