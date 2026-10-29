#pragma once

#include "src/api/AuthMiddleware.hpp"
#include "src/api/RateLimiter.hpp"
#include "src/db/Database.hpp"
#include "src/services/AuthService.hpp"
#include "src/services/BorrowService.hpp"
#include "src/services/ChatService.hpp"
#include "src/services/DisputeService.hpp"
#include "src/services/EventBus.hpp"
#include "src/services/ItemService.hpp"
#include "src/services/NotificationService.hpp"
#include "src/services/RatingService.hpp"
#include "src/services/Scheduler.hpp"
#include "src/services/SearchService.hpp"
#include "src/services/SettlementService.hpp"
#include "src/util/Clock.hpp"
#include "src/util/Config.hpp"

#include <memory>
#include <string>

namespace httplib {
class Server;
}

namespace borrowhub::api {

class Server {
public:
    Server(util::Config config,
           const util::Clock& clock,
           db::Database& db,
           services::AuthService& auth,
           services::ItemService& items,
           services::SearchService& search,
           services::ChatService& chat,
           services::BorrowService& borrow,
           services::SettlementService& settlement,
           services::RatingService& ratings,
           services::DisputeService& disputes,
           services::NotificationService& notifications);

    ~Server();

    // Configures middleware, security headers, exception handlers, and registers routes
    void setup();

    // Starts listening on configured port (blocking call)
    void start();

    // Stops server and releases socket
    void stop();

    // Directly register routes on httplib::Server
    void registerRoutes();

private:
    util::Config config_;
    const util::Clock& clock_;
    db::Database& db_;
    services::AuthService& auth_;
    services::ItemService& items_;
    services::SearchService& search_;
    services::ChatService& chat_;
    services::BorrowService& borrow_;
    services::SettlementService& settlement_;
    services::RatingService& ratings_;
    services::DisputeService& disputes_;
    services::NotificationService& notifications_;

    RateLimiter rateLimiter_;
    AuthMiddleware authMiddleware_;
    std::unique_ptr<httplib::Server> httpServer_;
};

} // namespace borrowhub::api
