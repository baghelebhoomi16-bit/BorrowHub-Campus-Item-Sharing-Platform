#pragma once

#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/domain/Money.hpp"
#include "src/domain/User.hpp"
#include "src/util/Clock.hpp"
#include "src/services/EventBus.hpp"

namespace borrowhub::services {

struct SettlementResult {
    domain::Money penalty;
    domain::Money claim;
    domain::Money refund;
    domain::Money totalDeduction;
};

class SettlementService {
public:
    SettlementService(db::Database& db,
                      db::TxnRepo& txns,
                      db::ItemRepo& items,
                      db::LedgerRepo& ledger,
                      db::DisputeRepo& disputes,
                      const util::Clock& clock,
                      EventBus& eventBus);

    // Executes deterministic financial settlement, writes immutable ledger entries, and sets item AVAILABLE
    SettlementResult settle(int64_t txnId);

    // Borrower explicitly accepts settlement before the 48-hour timeout
    SettlementResult acceptSettlement(const domain::User& borrower, int64_t txnId);

private:
    db::Database& db_;
    db::TxnRepo& txns_;
    db::ItemRepo& items_;
    db::LedgerRepo& ledger_;
    db::DisputeRepo& disputes_;
    const util::Clock& clock_;
    EventBus& eventBus_;
};

} // namespace borrowhub::services
