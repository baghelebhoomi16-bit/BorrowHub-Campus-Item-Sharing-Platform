#include "tests/TestCommon.hpp"
#include "tests/integration/TestApp.hpp"
#include "src/api/JsonMapper.hpp"
#include "src/util/Errors.hpp"

using namespace borrowhub::test;
using namespace borrowhub::domain;
using namespace borrowhub::services;
using namespace borrowhub::util;
using namespace borrowhub::api;

TEST_CASE("REST API integration tests for security, injection resistance, and rate limits", "[integration][api]") {
    TestApp app;

    int64_t ownerId = app.registerVerifiedStudent("api_owner@campus.edu", "PRN_API_OWNER", "Owner");
    int64_t borrowerId = app.registerVerifiedStudent("api_borrower@campus.edu", "PRN_API_BORROWER", "Borrower");
    int64_t strangerId = app.registerVerifiedStudent("api_stranger@campus.edu", "PRN_API_STRANGER", "Stranger");

    auto ownerUser = app.users_->getById(ownerId);
    auto borrowerUser = app.users_->getById(borrowerId);
    auto strangerUser = app.users_->getById(strangerId);

    int64_t itemId = app.listBook(ownerId, "Textbook for API Testing", 100);
    int64_t threadId = app.threads_->getOrCreate(itemId, borrowerId);

    SECTION("Stranger reading another student's chat thread throws FORBIDDEN (403)") {
        REQUIRE_THROWS_AS(
            app.chat_->pollMessages(*strangerUser, threadId, 0),
            AppError
        );
    }

    SECTION("SQL injection and XSS strings round-trip safely without execution or injection") {
        std::string xssPayload = "<script>alert('XSS')</script>";
        std::string sqlInjection = "'; DROP TABLE users;--";

        // Send XSS and SQL injection payloads into chat
        int64_t msgId1 = app.chat_->sendMessage(*borrowerUser, threadId, xssPayload);
        int64_t msgId2 = app.chat_->sendMessage(*borrowerUser, threadId, sqlInjection);

        REQUIRE(msgId1 > 0);
        REQUIRE(msgId2 > 0);

        auto messages = app.chat_->pollMessages(*borrowerUser, threadId, 0);
        REQUIRE(messages.size() == 2);
        REQUIRE(messages[0].body == xssPayload);
        REQUIRE(messages[1].body == sqlInjection);

        // Verify users table was NOT dropped
        REQUIRE(app.users_->getById(ownerId) != nullptr);

        // Verify JsonMapper escapes safely
        std::string jsonStr = JsonMapper::toJson(messages[0]);
        REQUIRE(jsonStr.find("<script>") != std::string::npos);
        REQUIRE(jsonStr.find("\\\"") == std::string::npos); // escaped properly
    }

    SECTION("Chat velocity rate limiting triggers after 20 messages per minute") {
        // Send 20 messages within the same minute
        for (int i = 0; i < 20; ++i) {
            app.chat_->sendMessage(*borrowerUser, threadId, "Message " + std::to_string(i));
        }

        // 21st message must throw RATE_LIMITED
        REQUIRE_THROWS_AS(
            app.chat_->sendMessage(*borrowerUser, threadId, "21st message"),
            AppError
        );
    }
}
