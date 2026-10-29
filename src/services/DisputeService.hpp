#pragma once

#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/domain/Money.hpp"
#include "src/domain/User.hpp"
#include "src/util/Clock.hpp"
#include "src/services/EventBus.hpp"

#include <vector>

namespace borrowhub::services {

class DisputeService {
public:
    DisputeService(db::Database& db,
                   db::DisputeRepo& disputes,
                   db::TxnRepo& txns,
                   db::ItemRepo& items,
                   db::LedgerRepo& ledger,
                   const util::Clock& clock,
                   EventBus& eventBus);

    // Borrower or owner raises dispute within 48h of return report
    int64_t raiseDispute(const domain::User& actor, int64_t txnId, std::string_view reason);

    // Admin resolves dispute with explicit owner award split and decision note
    void decideDispute(const domain::User& admin, int64_t disputeId, domain::Money ownerAward, std::string_view note);

    // Lists disputes for admin dashboard
    [[nodiscard]] std::vector<db::Dispute> listDisputes(const domain::User& admin);

private:
    db::Database& db_;
    db::DisputeRepo& disputes_;
    db::TxnRepo& txns_;
    db::ItemRepo& items_;
    db::LedgerRepo& ledger_;
    const util::Clock& clock_;
    EventBus& eventBus_;
};

} // namespace borrowhub::services
