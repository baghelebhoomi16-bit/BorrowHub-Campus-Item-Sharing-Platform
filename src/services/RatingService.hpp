#pragma once

#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/domain/User.hpp"
#include "src/util/Clock.hpp"
#include "src/services/EventBus.hpp"

namespace borrowhub::services {

class RatingService {
public:
    RatingService(db::Database& db,
                  db::RatingRepo& ratings,
                  db::TxnRepo& txns,
                  db::UserRepo& users,
                  const util::Clock& clock,
                  EventBus& eventBus);

    // Submits star rating (1-5) and comment post-settlement; recalculates reputation
    int64_t submitRating(const domain::User& rater, int64_t txnId, int stars, std::string_view comment);

    // Recomputes reputation score for user from all historical ratings and transactions
    double recalculateUserReputation(int64_t userId);

private:
    db::Database& db_;
    db::RatingRepo& ratings_;
    db::TxnRepo& txns_;
    db::UserRepo& users_;
    const util::Clock& clock_;
    EventBus& eventBus_;
};

} // namespace borrowhub::services
