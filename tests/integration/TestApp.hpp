#pragma once

#include "src/api/Server.hpp"
#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/domain/Book.hpp"
#include "src/domain/Electronic.hpp"
#include "src/domain/Equipment.hpp"
#include "src/domain/Student.hpp"
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

#include <fstream>
#include <memory>
#include <sstream>

namespace borrowhub::test {

class TestApp {
public:
    TestApp()
        : clock_(1730000000),
          db_(":memory:") {

        // Apply schema
        std::ifstream file("db/schema.sql");
        std::ostringstream ss;
        ss << file.rdbuf();
        db_.exec(ss.str());

        config_.port = 0; // Random port in tests
        config_.sessionTtlHours = 24;
        config_.verifyMode = "console";

        users_ = db::makeUserRepo(db_);
        sessions_ = db::makeSessionRepo(db_);
        verifyTokens_ = db::makeVerificationTokenRepo(db_);
        items_ = db::makeItemRepo(db_);
        threads_ = db::makeThreadRepo(db_);
        messages_ = db::makeMessageRepo(db_);
        txns_ = db::makeTxnRepo(db_);
        ledger_ = db::makeLedgerRepo(db_);
        ratings_ = db::makeRatingRepo(db_);
        notifs_ = db::makeNotificationRepo(db_);
        disputes_ = db::makeDisputeRepo(db_);

        auth_ = std::make_unique<services::AuthService>(db_, *users_, *sessions_, *verifyTokens_, clock_, config_, eventBus_);
        itemsService_ = std::make_unique<services::ItemService>(db_, *items_, *txns_, clock_, eventBus_);
        search_ = std::make_unique<services::SearchService>(*items_);
        chat_ = std::make_unique<services::ChatService>(db_, *threads_, *messages_, *items_, clock_);
        settlement_ = std::make_unique<services::SettlementService>(db_, *txns_, *items_, *ledger_, *disputes_, clock_, eventBus_);
        borrow_ = std::make_unique<services::BorrowService>(db_, *txns_, *items_, *threads_, *ledger_, clock_, eventBus_);
        ratingsService_ = std::make_unique<services::RatingService>(db_, *ratings_, *txns_, *users_, clock_, eventBus_);
        disputesService_ = std::make_unique<services::DisputeService>(db_, *disputes_, *txns_, *items_, *ledger_, clock_, eventBus_);
        notifService_ = std::make_unique<services::NotificationService>(db_, *notifs_, clock_, eventBus_);
        scheduler_ = std::make_unique<services::Scheduler>(db_, *txns_, *items_, *sessions_, *verifyTokens_, *settlement_, clock_, eventBus_);
    }

    // Helpers for quick fixture creation
    int64_t registerVerifiedStudent(std::string email, std::string prn, std::string name = "Student") {
        services::RegisterRequest req{
            std::move(email),
            std::move(prn),
            "Password123!",
            std::move(name),
            "+919876543210",
            "Computer Science",
            2,
            "A"
        };
        int64_t userId = auth_->registerStudent(req);
        users_->setVerified(userId, true);
        return userId;
    }

    int64_t listBook(int64_t ownerId, std::string title = "Textbook", int64_t depositRupees = 100) {
        domain::Book book(0, ownerId, std::move(title), "Academics", "GOOD", "Hostel",
                          domain::Money::fromRupees(depositRupees), clock_.now(), clock_.now() + 30 * 86400,
                          domain::ItemStatus::AVAILABLE, clock_.now(), "Author", "1234567890");
        return items_->insert(book);
    }

    int64_t listElectronic(int64_t ownerId, std::string title = "Arduino", int64_t depositRupees = 500) {
        domain::Electronic elec(0, ownerId, std::move(title), "Robotics", "GOOD", "Lab",
                                domain::Money::fromRupees(depositRupees), clock_.now(), clock_.now() + 30 * 86400,
                                domain::ItemStatus::AVAILABLE, clock_.now(), "Brand", "Model");
        return items_->insert(elec);
    }

    util::FakeClock clock_;
    util::Config config_;
    db::Database db_;
    services::EventBus eventBus_;

    std::unique_ptr<db::UserRepo> users_;
    std::unique_ptr<db::SessionRepo> sessions_;
    std::unique_ptr<db::VerificationTokenRepo> verifyTokens_;
    std::unique_ptr<db::ItemRepo> items_;
    std::unique_ptr<db::ThreadRepo> threads_;
    std::unique_ptr<db::MessageRepo> messages_;
    std::unique_ptr<db::TxnRepo> txns_;
    std::unique_ptr<db::LedgerRepo> ledger_;
    std::unique_ptr<db::RatingRepo> ratings_;
    std::unique_ptr<db::NotificationRepo> notifs_;
    std::unique_ptr<db::DisputeRepo> disputes_;

    std::unique_ptr<services::AuthService> auth_;
    std::unique_ptr<services::ItemService> itemsService_;
    std::unique_ptr<services::SearchService> search_;
    std::unique_ptr<services::ChatService> chat_;
    std::unique_ptr<services::BorrowService> borrow_;
    std::unique_ptr<services::SettlementService> settlement_;
    std::unique_ptr<services::RatingService> ratingsService_;
    std::unique_ptr<services::DisputeService> disputesService_;
    std::unique_ptr<services::NotificationService> notifService_;
    std::unique_ptr<services::Scheduler> scheduler_;
};

} // namespace borrowhub::test
