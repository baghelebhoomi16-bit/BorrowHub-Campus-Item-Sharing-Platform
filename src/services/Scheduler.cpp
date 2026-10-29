#include "src/services/Scheduler.hpp"
#include "src/db/DbTransaction.hpp"

#include <chrono>
#include <iostream>

namespace borrowhub::services {

Scheduler::Scheduler(db::Database& db,
                     db::TxnRepo& txns,
                     db::ItemRepo& items,
                     db::SessionRepo& sessions,
                     db::VerificationTokenRepo& verifyTokens,
                     SettlementService& settlement,
                     const util::Clock& clock,
                     EventBus& eventBus)
    : db_(db),
      txns_(txns),
      items_(items),
      sessions_(sessions),
      verifyTokens_(verifyTokens),
      settlement_(settlement),
      clock_(clock),
      eventBus_(eventBus) {}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::tick(int64_t now) {
    // 1. ACTIVE with due < now -> markOverdue, notify both
    auto overdueList = txns_.findOverdue(now);
    for (auto& txn : overdueList) {
        try {
            db::DbTransaction tx(db_);
            txn->markOverdue(now);
            txns_.update(*txn);
            tx.commit();

            eventBus_.publish(DomainEvent{
                EventType::TRANSACTION_OVERDUE,
                txn->borrowerId(),
                0,
                txn->id(),
                "{\"dueDate\":" + std::to_string(txn->dueDate()) + "}"
            });
            eventBus_.publish(DomainEvent{
                EventType::TRANSACTION_OVERDUE,
                txn->ownerId(),
                0,
                txn->id(),
                "{\"dueDate\":" + std::to_string(txn->dueDate()) + "}"
            });
        } catch (const std::exception& e) {
            std::cerr << "[SCHEDULER] Failed to mark txn " << txn->id() << " overdue: " << e.what() << "\n";
        }
    }

    // 2. ACCEPTED with acceptedAt + 24h < now -> CANCELLED, item -> AVAILABLE
    int64_t staleAcceptedThreshold = now - 86400;
    auto staleAccepted = txns_.findStaleAccepted(staleAcceptedThreshold);
    for (auto& txn : staleAccepted) {
        try {
            db::DbTransaction tx(db_);
            txn->cancel(txn->borrowerId(), now);
            txns_.update(*txn);

            auto item = items_.getById(txn->itemId());
            if (item && item->status() == domain::ItemStatus::RESERVED) {
                item->setStatus(domain::ItemStatus::AVAILABLE);
                items_.update(*item);
            }
            tx.commit();

            eventBus_.publish(DomainEvent{
                EventType::REQUEST_CANCELLED,
                txn->ownerId(),
                0,
                txn->id(),
                "{\"reason\":\"Deposit not paid within 24 hours\"}"
            });
        } catch (const std::exception& e) {
            std::cerr << "[SCHEDULER] Failed to expire accepted txn " << txn->id() << ": " << e.what() << "\n";
        }
    }

    // 3. RETURNED with returnedAt + 48h < now and no open dispute -> SettlementService.settle
    int64_t autoSettleThreshold = now - 172800;
    auto autoSettleable = txns_.findAutoSettleable(autoSettleThreshold);
    for (auto& txn : autoSettleable) {
        try {
            settlement_.settle(txn->id());
        } catch (const std::exception& e) {
            std::cerr << "[SCHEDULER] Failed to auto-settle txn " << txn->id() << ": " << e.what() << "\n";
        }
    }

    // 4. ACTIVE with due - now < 24h and not yet overdue -> reminder notification
    auto dueSoon = txns_.findDueWithin(86400, now);
    for (auto& txn : dueSoon) {
        eventBus_.publish(DomainEvent{
            EventType::DUE_REMINDER,
            txn->borrowerId(),
            0,
            txn->id(),
            "{\"hoursLeft\":" + std::to_string((txn->dueDate() - now) / 3600) + "}"
        });
    }

    // 5. Delete expired sessions and verification tokens
    try {
        db::DbTransaction tx(db_);
        sessions_.deleteExpired(now);
        verifyTokens_.deleteExpired(now);
        tx.commit();
    } catch (...) {}
}

void Scheduler::start(int intervalSeconds) {
    if (running_.exchange(true)) return;

    workerThread_ = std::thread([this, intervalSeconds]() {
        while (running_.load()) {
            try {
                tick(clock_.now());
            } catch (const std::exception& e) {
                std::cerr << "[SCHEDULER WORKER] Tick exception: " << e.what() << "\n";
            }

            // Sleep in small increments for clean shutdown
            for (int i = 0; i < intervalSeconds * 10 && running_.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    });
}

void Scheduler::stop() {
    if (running_.exchange(false)) {
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }
}

} // namespace borrowhub::services
