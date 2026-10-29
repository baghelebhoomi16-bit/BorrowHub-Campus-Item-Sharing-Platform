cmake_minimum_required(VERSION 3.25)

project(BorrowHub
    VERSION 1.0.0
    DESCRIPTION "Campus Item-Sharing Platform with Auditable Chat and Ledger Settlement"
    LANGUAGES C CXX
)

# Enforce standard C++17 with no compiler extensions
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clang-tidy & IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build Options
option(BH_COVERAGE "Enable code coverage reporting via gcov/lcov" OFF)
option(BH_SANITIZE "Enable AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan)" OFF)

if (BH_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling code coverage flags (--coverage)")
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)
    else()
        message(WARNING "BH_COVERAGE is only supported on GCC or Clang")
    endif()
endif()

if (BH_SANITIZE)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling ASan and UBSan flags")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address,undefined)
    elseif(MSVC)
        message(STATUS "Enabling MSVC AddressSanitizer")
        add_compile_options(/fsanitize=address)
    endif()
endif()

# Find dependencies (installed via vcpkg manifest mode)
find_package(sqlite3 CONFIG QUIET)
if (NOT TARGET SQLite::SQLite3)
    find_package(SQLite3 REQUIRED)
endif()

find_package(httplib CONFIG QUIET)
find_package(nlohmann_json CONFIG QUIET)
find_package(libsodium CONFIG QUIET)
find_package(spdlog CONFIG QUIET)
find_package(Catch2 3 QUIET)

# Include main source tree
add_subdirectory(src)

# Enable testing
enable_testing()
add_subdirectory(tests)
