#pragma once

#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/services/EventBus.hpp"
#include "src/services/SettlementService.hpp"
#include "src/util/Clock.hpp"

#include <atomic>
#include <thread>

namespace borrowhub::services {

class Scheduler {
public:
    Scheduler(db::Database& db,
              db::TxnRepo& txns,
              db::ItemRepo& items,
              db::SessionRepo& sessions,
              db::VerificationTokenRepo& verifyTokens,
              SettlementService& settlement,
              const util::Clock& clock,
              EventBus& eventBus);

    ~Scheduler();

    // Executes a single idempotent maintenance tick
    void tick(int64_t now);

    // Starts background worker thread running tick every intervalSeconds
    void start(int intervalSeconds = 60);

    // Stops background worker thread cleanly
    void stop();

private:
    db::Database& db_;
    db::TxnRepo& txns_;
    db::ItemRepo& items_;
    db::SessionRepo& sessions_;
    db::VerificationTokenRepo& verifyTokens_;
    SettlementService& settlement_;
    const util::Clock& clock_;
    EventBus& eventBus_;

    std::atomic<bool> running_{false};
    std::thread workerThread_;
};

} // namespace borrowhub::services
