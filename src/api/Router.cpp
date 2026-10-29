#include "src/api/Server.hpp"
#include "src/api/JsonMapper.hpp"
#include "src/util/Errors.hpp"

#if __has_include(<httplib.h>)
#include <httplib.h>
#endif

#if __has_include(<nlohmann/json.hpp>)
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace borrowhub::api {

namespace {

#if __has_include(<nlohmann/json.hpp>)
json parseJsonBody(const std::string& body) {
    try {
        return json::parse(body.empty() ? "{}" : body);
    } catch (...) {
        throw util::AppError(util::ErrorCode::VALIDATION, "Invalid JSON payload in request body");
    }
}
#endif

int64_t parsePathId(const std::string& param) {
    try {
        return std::stoll(param);
    } catch (...) {
        throw util::AppError(util::ErrorCode::VALIDATION, "Invalid numeric ID parameter in URL path");
    }
}

} // namespace

void Server::registerRoutes() {
    auto& server = *httpServer_;

    // ==========================================
    // 1. System Health
    // ==========================================
    server.Get("/health", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"UP\",\"timestamp\":" + std::to_string(clock_.now()) + "}", "application/json");
    });

    // ==========================================
    // 2. Authentication & Account Lifecycle
    // ==========================================
    server.Post("/api/auth/register", [this](const httplib::Request& req, httplib::Response& res) {
#if __has_include(<nlohmann/json.hpp>)
        auto j = parseJsonBody(req.body);
        services::RegisterRequest regReq;
        regReq.email = j.value("email", "");
        regReq.prn = j.value("prn", "");
        regReq.password = j.value("password", "");
        regReq.name = j.value("name", "");
        regReq.phone = j.value("phone", "");
        regReq.department = j.value("department", "");
        regReq.year = j.value("year", 1);
        regReq.division = j.value("division", "");

        int64_t userId = auth_.registerStudent(regReq);
        res.status = 201;
        res.set_content("{\"id\":" + std::to_string(userId) + ",\"email\":\"" + JsonMapper::escape(regReq.email) +
                        "\",\"message\":\"Registration successful. Please verify via verification link.\"}", "application/json");
#else
        res.set_content("{\"message\":\"Registered\"}", "application/json");
#endif
    });

    server.Post("/api/auth/verify", [this](const httplib::Request& req, httplib::Response& res) {
#if __has_include(<nlohmann/json.hpp>)
        auto j = parseJsonBody(req.body);
        std::string token = j.value("token", "");
        auth_.verifyAccount(token);
        res.set_content("{\"verified\":true,\"message\":\"Account successfully verified.\"}", "application/json");
#endif
    });

    server.Post("/api/auth/login", [this](const httplib::Request& req, httplib::Response& res) {
        // Blueprint 2.2: Rate limit login attempts to 5 per minute per IP
        std::string clientIp = req.remote_addr.empty() ? "127.0.0.1" : req.remote_addr;
        if (!rateLimiter_.checkAndRecord("login:" + clientIp, 5, 60)) {
            throw util::AppError(util::ErrorCode::RATE_LIMITED, "Too many login attempts. Please try again in 1 minute.");
        }

#if __has_include(<nlohmann/json.hpp>)
        auto j = parseJsonBody(req.body);
        std::string email = j.value("email", "");
        std::string password = j.value("password", "");

        auto result = auth_.login(email, password);

        // Set HttpOnly, SameSite=Strict session cookie per blueprint
        std::string cookieHeader = "bh_session=" + result.rawSessionToken +
                                   "; HttpOnly; SameSite=Strict; Path=/; Max-Age=" +
                                   std::to_string(config_.sessionTtlHours * 3600);
        res.set_header("Set-Cookie", cookieHeader.c_str());

        res.set_content("{\"user\":" + JsonMapper::toJson(*result.user, true) + "}", "application/json");
#endif
    });

    server.Post("/api/auth/logout", [this](const httplib::Request& req, httplib::Response& res) {
        std::string cookieHeader = req.get_header_value("Cookie");
        std::string token = AuthMiddleware::extractSessionCookie(cookieHeader);
        auth_.logout(token);

        res.set_header("Set-Cookie", "bh_session=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0");
        res.set_content("{\"message\":\"Logged out successfully\"}", "application/json");
    });

    server.Get("/api/auth/me", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        res.set_content("{\"user\":" + JsonMapper::toJson(*user, true) + "}", "application/json");
    });

    // ==========================================
    // 3. Item Management & Search
    // ==========================================
    server.Get("/api/items", [this](const httplib::Request& req, httplib::Response& res) {
        authMiddleware_.authenticate(req.get_header_value("Cookie"));

        db::ItemFilter filter;
        if (req.has_param("q")) filter.query = req.get_param_value("q");
        if (req.has_param("category")) filter.category = req.get_param_value("category");
        if (req.has_param("location")) filter.location = req.get_param_value("location");
        if (req.has_param("condition")) filter.condition = req.get_param_value("condition");
        if (req.has_param("max_deposit")) {
            try { filter.maxDepositPaise = std::stoll(req.get_param_value("max_deposit")); } catch (...) {}
        }
        if (req.has_param("available_on")) {
            try { filter.availableOnDate = std::stoll(req.get_param_value("available_on")); } catch (...) {}
        }
        if (req.has_param("min_rating")) {
            try { filter.minOwnerRating = std::stod(req.get_param_value("min_rating")); } catch (...) {}
        }
        if (req.has_param("page")) {
            try { filter.page = std::stoi(req.get_param_value("page")); } catch (...) {}
        }
        if (req.has_param("page_size")) {
            try { filter.pageSize = std::stoi(req.get_param_value("page_size")); } catch (...) {}
        }

        auto result = search_.search(filter);
        res.set_content(JsonMapper::toJson(result.items, result.totalCount, filter.page, filter.pageSize), "application/json");
    });

    server.Post("/api/items", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
#if __has_include(<nlohmann/json.hpp>)
        auto j = parseJsonBody(req.body);
        services::CreateItemRequest itemReq;
        itemReq.type = j.value("type", "");
        itemReq.title = j.value("title", "");
        itemReq.category = j.value("category", "");
        itemReq.condition = j.value("condition", "GOOD");
        itemReq.location = j.value("location", "");
        itemReq.depositPaise = j.value("deposit_paise", 0LL);
        itemReq.availFrom = j.value("avail_from", 0LL);
        itemReq.availTo = j.value("avail_to", 0LL);
        if (j.contains("attrs")) {
            itemReq.attrsJson = j["attrs"].dump();
        }

        int64_t itemId = items_.createItem(*user, itemReq);
        auto item = items_.getItem(itemId);
        res.status = 201;
        res.set_content(JsonMapper::toJson(*item), "application/json");
#endif
    });

    server.Put("/api/items/:id", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t itemId = parsePathId(req.path_params.at("id"));
#if __has_include(<nlohmann/json.hpp>)
        auto j = parseJsonBody(req.body);
        services::CreateItemRequest itemReq;
        itemReq.title = j.value("title", "");
        itemReq.category = j.value("category", "");
        itemReq.condition = j.value("condition", "");
        itemReq.location = j.value("location", "");
        itemReq.depositPaise = j.value("deposit_paise", 0LL);
        itemReq.availFrom = j.value("avail_from", 0LL);
        itemReq.availTo = j.value("avail_to", 0LL);
        if (j.contains("attrs")) itemReq.attrsJson = j["attrs"].dump();

        items_.updateItem(*user, itemId, itemReq);
        auto item = items_.getItem(itemId);
        res.set_content(JsonMapper::toJson(*item), "application/json");
#endif
    });

    server.Delete("/api/items/:id", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t itemId = parsePathId(req.path_params.at("id"));
        items_.removeItem(*user, itemId);
        res.set_content("{\"message\":\"Item removed successfully\"}", "application/json");
    });

    // ==========================================
    // 4. Chat System
    // ==========================================
    server.Post("/api/items/:id/threads", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t itemId = parsePathId(req.path_params.at("id"));
        int64_t threadId = chat_.getOrCreateThread(*user, itemId);
        res.status = 201;
        res.set_content("{\"thread_id\":" + std::to_string(threadId) + ",\"item_id\":" + std::to_string(itemId) + ",\"borrower_id\":" + std::to_string(user->id()) + "}", "application/json");
    });

    server.Get("/api/threads", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        auto threads = chat_.listThreads(*user);
        res.set_content(JsonMapper::toJson(threads), "application/json");
    });

    server.Get("/api/threads/:id/messages", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t threadId = parsePathId(req.path_params.at("id"));
        int64_t afterId = 0;
        if (req.has_param("after_id")) {
            try { afterId = std::stoll(req.get_param_value("after_id")); } catch (...) {}
        }
        auto msgs = chat_.pollMessages(*user, threadId, afterId);
        res.set_content(JsonMapper::toJson(msgs), "application/json");
    });

    server.Post("/api/threads/:id/messages", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t threadId = parsePathId(req.path_params.at("id"));
#if __has_include(<nlohmann/json.hpp>)
        auto j = parseJsonBody(req.body);
        std::string body = j.value("body", "");
        int64_t msgId = chat_.sendMessage(*user, threadId, body);
        res.status = 201;
        res.set_content("{\"id\":" + std::to_string(msgId) + ",\"message\":\"Message sent\"}", "application/json");
#endif
    });

    // ==========================================
    // 5. Borrow Lifecycle Actions
    // ==========================================
    server.Post("/api/requests", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
#if __has_include(<nlohmann/json.hpp>)
        auto j = parseJsonBody(req.body);
        services::CreateBorrowRequest reqDto;
        reqDto.itemId = j.value("item_id", 0LL);
        reqDto.startDate = j.value("start_date", 0LL);
        reqDto.dueDate = j.value("due_date", 0LL);

        int64_t txnId = borrow_.createRequest(*user, reqDto);
        auto txn = borrow_.getTransaction(txnId);
        res.status = 201;
        res.set_content(JsonMapper::toJson(*txn, user->id()), "application/json");
#endif
    });

    server.Post("/api/requests/:id/accept", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t txnId = parsePathId(req.path_params.at("id"));
        borrow_.acceptRequest(*user, txnId);
        auto txn = borrow_.getTransaction(txnId);
        res.set_content(JsonMapper::toJson(*txn, user->id()), "application/json");
    });

    server.Post("/api/requests/:id/reject", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t txnId = parsePathId(req.path_params.at("id"));
        borrow_.rejectRequest(*user, txnId);
        auto txn = borrow_.getTransaction(txnId);
        res.set_content(JsonMapper::toJson(*txn, user->id()), "application/json");
    });

    server.Post("/api/requests/:id/cancel", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t txnId = parsePathId(req.path_params.at("id"));
        borrow_.cancelRequest(*user, txnId);
        auto txn = borrow_.getTransaction(txnId);
        res.set_content(JsonMapper::toJson(*txn, user->id()), "application/json");
    });

    server.Post("/api/requests/:id/pay-deposit", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t txnId = parsePathId(req.path_params.at("id"));
        borrow_.payDeposit(*user, txnId);
        auto txn = borrow_.getTransaction(txnId);
        res.set_content(JsonMapper::toJson(*txn, user->id()), "application/json");
    });

    server.Post("/api/requests/:id/confirm-handover", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t txnId = parsePathId(req.path_params.at("id"));
        borrow_.confirmHandover(*user, txnId);
        auto txn = borrow_.getTransaction(txnId);
        res.set_content(JsonMapper::toJson(*txn, user->id()), "application/json");
    });

    server.Post("/api/requests/:id/report-return", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t txnId = parsePathId(req.path_params.at("id"));
#if __has_include(<nlohmann/json.hpp>)
        auto j = parseJsonBody(req.body);
        std::string cond = j.value("condition", "GOOD");
        int64_t claim = j.value("claim_paise", 0LL);
        borrow_.reportReturn(*user, txnId, cond, domain::Money::fromPaise(claim));
        auto txn = borrow_.getTransaction(txnId);
        res.set_content(JsonMapper::toJson(*txn, user->id()), "application/json");
#endif
    });

    server.Post("/api/requests/:id/accept-settlement", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t txnId = parsePathId(req.path_params.at("id"));
        auto result = settlement_.acceptSettlement(*user, txnId);
        res.set_content("{\"refundPaise\":" + std::to_string(result.refund.paise()) +
                        ",\"penaltyPaise\":" + std::to_string(result.penalty.paise()) +
                        ",\"claimPaise\":" + std::to_string(result.claim.paise()) + "}", "application/json");
    });

    server.Post("/api/requests/:id/dispute", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t txnId = parsePathId(req.path_params.at("id"));
#if __has_include(<nlohmann/json.hpp>)
        auto j = parseJsonBody(req.body);
        std::string reason = j.value("reason", "");
        int64_t disputeId = disputes_.raiseDispute(*user, txnId, reason);
        res.status = 201;
        res.set_content("{\"dispute_id\":" + std::to_string(disputeId) + ",\"message\":\"Dispute raised\"}", "application/json");
#endif
    });

    server.Post("/api/requests/:id/rating", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t txnId = parsePathId(req.path_params.at("id"));
#if __has_include(<nlohmann/json.hpp>)
        auto j = parseJsonBody(req.body);
        int stars = j.value("stars", 5);
        std::string comment = j.value("comment", "");
        int64_t ratingId = ratings_.submitRating(*user, txnId, stars, comment);
        res.status = 201;
        res.set_content("{\"rating_id\":" + std::to_string(ratingId) + ",\"message\":\"Rating submitted\"}", "application/json");
#endif
    });

    // ==========================================
    // 6. Notifications & Dashboard
    // ==========================================
    server.Get("/api/notifications", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        bool unreadOnly = req.has_param("unread") && req.get_param_value("unread") == "true";
        auto list = notifications_.listNotifications(user->id(), unreadOnly);
        res.set_content(JsonMapper::toJson(list), "application/json");
    });

    server.Post("/api/notifications/:id/read", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        int64_t notifId = parsePathId(req.path_params.at("id"));
        notifications_.markAsRead(notifId, user->id());
        res.set_content("{\"markedRead\":true}", "application/json");
    });

    server.Get("/api/dashboard", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        auto myItems = items_.listMyItems(user->id());
        auto txns = borrow_.listUserTransactions(user->id());
        auto notifs = notifications_.listNotifications(user->id(), true);

        int activeBorrowings = 0;
        int activeListings = 0;
        for (const auto& item : myItems) {
            if (item->status() == domain::ItemStatus::AVAILABLE || item->status() == domain::ItemStatus::BORROWED) {
                activeListings++;
            }
        }
        for (const auto& t : txns) {
            if (t->status() == domain::TransactionStatus::ACTIVE || t->status() == domain::TransactionStatus::OVERDUE) {
                activeBorrowings++;
            }
        }

        std::ostringstream oss;
        oss << "{"
            << "\"activeListingsCount\":" << activeListings << ","
            << "\"activeBorrowingsCount\":" << activeBorrowings << ","
            << "\"unreadNotificationsCount\":" << notifs.size() << ","
            << "\"transactions\":" << JsonMapper::toJson(txns, user->id())
            << "}";
        res.set_content(oss.str(), "application/json");
    });

    // ==========================================
    // 7. Admin Endpoints
    // ==========================================
    server.Get("/api/admin/disputes", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        AuthMiddleware::requireAdmin(*user);
        auto list = disputes_.listDisputes(*user);
        res.set_content(JsonMapper::toJson(list), "application/json");
    });

    server.Post("/api/admin/disputes/:id/decide", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        AuthMiddleware::requireAdmin(*user);
        int64_t disputeId = parsePathId(req.path_params.at("id"));
#if __has_include(<nlohmann/json.hpp>)
        auto j = parseJsonBody(req.body);
        int64_t awardPaise = j.value("owner_award_paise", 0LL);
        std::string note = j.value("decision_note", "");
        disputes_.decideDispute(*user, disputeId, domain::Money::fromPaise(awardPaise), note);
        res.set_content("{\"message\":\"Dispute resolved successfully\"}", "application/json");
#endif
    });

    server.Post("/api/admin/users/:id/block", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        AuthMiddleware::requireAdmin(*user);
        int64_t targetUserId = parsePathId(req.path_params.at("id"));
#if __has_include(<nlohmann/json.hpp>)
        auto j = parseJsonBody(req.body);
        bool blocked = j.value("blocked", true);
        db_.prepare("UPDATE users SET blocked = ? WHERE id = ?;");
        auto stmt = db_.prepare("UPDATE users SET blocked = ? WHERE id = ?;");
        stmt.bind(1, blocked ? 1 : 0);
        stmt.bind(2, targetUserId);
        stmt.step();
        res.set_content("{\"userId\":" + std::to_string(targetUserId) + ",\"blocked\":" + (blocked ? "true" : "false") + "}", "application/json");
#endif
    });

    server.Delete("/api/admin/items/:id", [this](const httplib::Request& req, httplib::Response& res) {
        auto user = authMiddleware_.authenticate(req.get_header_value("Cookie"));
        AuthMiddleware::requireAdmin(*user);
        int64_t itemId = parsePathId(req.path_params.at("id"));
        items_.removeItem(*user, itemId);
        res.set_content("{\"message\":\"Item removed by administrator\"}", "application/json");
    });
}

} // namespace borrowhub::api
