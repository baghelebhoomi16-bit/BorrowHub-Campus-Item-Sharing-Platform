#include "src/api/Server.hpp"
#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
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

#include <csignal>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace {
std::atomic<bool> g_shutdownRequested{false};
borrowhub::api::Server* g_serverInstance{nullptr};
borrowhub::services::Scheduler* g_schedulerInstance{nullptr};

void signalHandler(int signum) {
    std::cout << "\n[BORROWHUB] Received signal " << signum << ". Initiating graceful shutdown...\n";
    g_shutdownRequested.store(true);
    if (g_serverInstance) {
        g_serverInstance->stop();
    }
    if (g_schedulerInstance) {
        g_schedulerInstance->stop();
    }
}

std::string readFileContents(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

} // namespace

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    bool initDb = false;
    bool seedDb = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--init-db") initDb = true;
        if (arg == "--seed") seedDb = true;
    }

    try {
        borrowhub::util::Config config = borrowhub::util::Config::loadFromEnv();
        borrowhub::util::SystemClock clock;

        std::cout << "========================================\n"
                  << "  BorrowHub Campus Item-Sharing Server  \n"
                  << "  Version 1.0 (29 Oct 2026)             \n"
                  << "========================================\n"
                  << "Database: " << config.dbPath << "\n"
                  << "Port:     " << config.port << "\n"
                  << "Verify:   " << config.verifyMode << "\n\n";

        borrowhub::db::Database db(config.dbPath);

        if (initDb) {
            std::cout << "[BORROWHUB DB] Applying database schema (db/schema.sql)...\n";
            try {
                std::string schemaSql = readFileContents("db/schema.sql");
                db.exec(schemaSql);
                std::cout << "[BORROWHUB DB] Schema applied successfully.\n";
            } catch (const std::exception& e) {
                std::cerr << "[BORROWHUB DB WARNING] Schema execution notice: " << e.what() << "\n";
            }
        }

        if (seedDb) {
            std::cout << "[BORROWHUB DB] Applying seed data (db/seed.sql)...\n";
            try {
                std::string seedSql = readFileContents("db/seed.sql");
                db.exec(seedSql);
                std::cout << "[BORROWHUB DB] Seed data applied successfully.\n";
            } catch (const std::exception& e) {
                std::cerr << "[BORROWHUB DB WARNING] Seed execution notice: " << e.what() << "\n";
            }
        }

        // Initialize repositories
        auto users = borrowhub::db::makeUserRepo(db);
        auto sessions = borrowhub::db::makeSessionRepo(db);
        auto verifyTokens = borrowhub::db::makeVerificationTokenRepo(db);
        auto items = borrowhub::db::makeItemRepo(db);
        auto threads = borrowhub::db::makeThreadRepo(db);
        auto messages = borrowhub::db::makeMessageRepo(db);
        auto txns = borrowhub::db::makeTxnRepo(db);
        auto ledger = borrowhub::db::makeLedgerRepo(db);
        auto ratings = borrowhub::db::makeRatingRepo(db);
        auto notifs = borrowhub::db::makeNotificationRepo(db);
        auto disputes = borrowhub::db::makeDisputeRepo(db);

        // Initialize event bus & services
        borrowhub::services::EventBus eventBus;

        borrowhub::services::AuthService auth(db, *users, *sessions, *verifyTokens, clock, config, eventBus);
        borrowhub::services::ItemService itemService(db, *items, *txns, clock, eventBus);
        borrowhub::services::SearchService searchService(*items);
        borrowhub::services::ChatService chatService(db, *threads, *messages, *items, clock);
        borrowhub::services::BorrowService borrowService(db, *txns, *items, *threads, *ledger, clock, eventBus);
        borrowhub::services::SettlementService settlementService(db, *txns, *items, *ledger, *disputes, clock, eventBus);
        borrowhub::services::RatingService ratingService(db, *ratings, *txns, *users, clock, eventBus);
        borrowhub::services::DisputeService disputeService(db, *disputes, *txns, *items, *ledger, clock, eventBus);
        borrowhub::services::NotificationService notificationService(db, *notifs, clock, eventBus);

        // Initialize background scheduler
        borrowhub::services::Scheduler scheduler(db, *txns, *items, *sessions, *verifyTokens, settlementService, clock, eventBus);
        g_schedulerInstance = &scheduler;
        scheduler.start(60); // 60s tick interval

        // Initialize HTTP server
        borrowhub::api::Server server(
            config, clock, db, auth, itemService, searchService, chatService,
            borrowService, settlementService, ratingService, disputeService, notificationService
        );
        g_serverInstance = &server;

        // Block on listen until SIGINT or SIGTERM
        server.start();

        // Graceful exit
        scheduler.stop();
        std::cout << "[BORROWHUB] Server shutdown completed cleanly.\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal startup error: " << e.what() << "\n";
        return 1;
    }
}
