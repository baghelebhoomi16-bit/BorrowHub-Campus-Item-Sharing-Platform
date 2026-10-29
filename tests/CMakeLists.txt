# tests/CMakeLists.txt

find_package(Catch2 3 QUIET)

set(UNIT_TEST_SOURCES
    unit/test_money.cpp
    unit/test_user.cpp
    unit/test_item.cpp
    unit/test_penalty_policy.cpp
    unit/test_transaction.cpp
    unit/test_reputation.cpp
    unit/test_auth_service.cpp
    unit/test_borrow_service.cpp
    unit/test_settlement_service.cpp
    unit/test_scheduler.cpp
)

set(INTEGRATION_TEST_SOURCES
    integration/test_lifecycle.cpp
    integration/test_api.cpp
)

if (TARGET Catch2::Catch2WithMain)
    add_executable(borrowhub_unit_tests ${UNIT_TEST_SOURCES})
    target_link_libraries(borrowhub_unit_tests PRIVATE borrowhub_core Catch2::Catch2WithMain)
    target_include_directories(borrowhub_unit_tests PRIVATE ${CMAKE_SOURCE_DIR})

    add_executable(borrowhub_integration_tests ${INTEGRATION_TEST_SOURCES})
    target_link_libraries(borrowhub_integration_tests PRIVATE borrowhub_core Catch2::Catch2WithMain)
    target_include_directories(borrowhub_integration_tests PRIVATE ${CMAKE_SOURCE_DIR})

    include(CTest)
    include(Catch)
    catch_discover_tests(borrowhub_unit_tests)
    catch_discover_tests(borrowhub_integration_tests)
else()
    # Self-contained standalone Catch2 runner fallback if Catch2 is not installed via vcpkg
    add_executable(borrowhub_unit_tests ${UNIT_TEST_SOURCES})
    target_link_libraries(borrowhub_unit_tests PRIVATE borrowhub_core)
    target_include_directories(borrowhub_unit_tests PRIVATE ${CMAKE_SOURCE_DIR})
    target_compile_definitions(borrowhub_unit_tests PRIVATE BH_STANDALONE_TEST_RUNNER=1)

    add_executable(borrowhub_integration_tests ${INTEGRATION_TEST_SOURCES})
    target_link_libraries(borrowhub_integration_tests PRIVATE borrowhub_core)
    target_include_directories(borrowhub_integration_tests PRIVATE ${CMAKE_SOURCE_DIR})
    target_compile_definitions(borrowhub_integration_tests PRIVATE BH_STANDALONE_TEST_RUNNER=1)

    add_test(NAME UnitTests COMMAND borrowhub_unit_tests)
    add_test(NAME IntegrationTests COMMAND borrowhub_integration_tests)
endif()
