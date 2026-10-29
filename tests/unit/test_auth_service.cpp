#include "tests/TestCommon.hpp"
#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/services/AuthService.hpp"
#include "src/services/EventBus.hpp"
#include "src/util/Clock.hpp"
#include "src/util/Config.hpp"
#include "src/util/Errors.hpp"

#include <fstream>
#include <sstream>

using namespace borrowhub::services;
using namespace borrowhub::util;
using namespace borrowhub::db;

namespace {

void initSchema(Database& db) {
    std::ifstream file("db/schema.sql");
    std::ostringstream ss;
    ss << file.rdbuf();
    db.exec(ss.str());
}

} // namespace

TEST_CASE("AuthService registration, verification, and login flows", "[service][auth]") {
    Database db(":memory:");
    initSchema(db);

    auto users = makeUserRepo(db);
    auto sessions = makeSessionRepo(db);
    auto verifyTokens = makeVerificationTokenRepo(db);

    FakeClock clock(1730000000);
    Config config;
    config.verifyMode = "console";
    config.sessionTtlHours = 24;
    EventBus eventBus;

    AuthService auth(db, *users, *sessions, *verifyTokens, clock, config, eventBus);

    RegisterRequest req{
        "test@campus.edu",
        "PRN9999",
        "StrongPassword123!",
        "Test Student",
        "+919876543210",
        "Computer Science",
        3,
        "A"
    };

    SECTION("Registration succeeds and prevents duplicates") {
        int64_t id = auth.registerStudent(req);
        REQUIRE(id > 0);

        // Duplicate email throws CONFLICT
        REQUIRE_THROWS_AS(auth.registerStudent(req), AppError);

        // Weak password (< 8 chars) throws VALIDATION
        auto weakReq = req;
        weakReq.email = "other@campus.edu";
        weakReq.prn = "PRN8888";
        weakReq.password = "123";
        REQUIRE_THROWS_AS(auth.registerStudent(weakReq), AppError);
    }

    SECTION("Unverified user cannot log in") {
        auth.registerStudent(req);
        REQUIRE_THROWS_AS(auth.login(req.email, req.password), AppError);
    }

    SECTION("Identical error text for wrong password and non-existent email") {
        auth.registerStudent(req);
        // Direct verify for test
        users->setVerified(1, true);

        std::string errUnknown;
        try {
            auth.login("nonexistent@campus.edu", req.password);
        } catch (const AppError& e) {
            errUnknown = e.message();
        }

        std::string errWrongPwd;
        try {
            auth.login(req.email, "WrongPassword!");
        } catch (const AppError& e) {
            errWrongPwd = e.message();
        }

        REQUIRE(errUnknown == "Invalid email or password");
        REQUIRE(errWrongPwd == "Invalid email or password");
    }

    SECTION("Verified user logs in, receives session, and expired session is rejected") {
        auth.registerStudent(req);
        users->setVerified(1, true);

        auto loginRes = auth.login(req.email, req.password);
        REQUIRE(!loginRes.rawSessionToken.empty());
        REQUIRE(loginRes.user->email() == req.email);

        // Authenticate session works
        auto user = auth.authenticateSession(loginRes.rawSessionToken);
        REQUIRE(user != nullptr);

        // Advance FakeClock by 25 hours (past 24h TTL)
        clock.advance(25 * 3600);
        REQUIRE_THROWS_AS(auth.authenticateSession(loginRes.rawSessionToken), AppError);
    }
}
