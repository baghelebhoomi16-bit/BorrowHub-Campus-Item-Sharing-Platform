#include "src/services/RatingService.hpp"
#include "src/db/DbTransaction.hpp"
#include "src/domain/Reputation.hpp"
#include "src/util/Errors.hpp"

namespace borrowhub::services {

RatingService::RatingService(db::Database& db,
                             db::RatingRepo& ratings,
                             db::TxnRepo& txns,
                             db::UserRepo& users,
                             const util::Clock& clock,
                             EventBus& eventBus)
    : db_(db),
      ratings_(ratings),
      txns_(txns),
      users_(users),
      clock_(clock),
      eventBus_(eventBus) {}

int64_t RatingService::submitRating(const domain::User& rater, int64_t txnId, int stars, std::string_view comment) {
    if (stars < 1 || stars > 5) {
        throw util::AppError(util::ErrorCode::VALIDATION, "Rating stars must be between 1 and 5");
    }

    auto txn = txns_.getById(txnId);
    if (!txn) {
        throw util::AppError(util::ErrorCode::NOT_FOUND, "Transaction not found");
    }

    if (txn->status() != domain::TransactionStatus::SETTLED) {
        throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Ratings can only be submitted after transaction is SETTLED");
    }

    if (rater.id() != txn->ownerId() && rater.id() != txn->borrowerId()) {
        throw util::AppError(util::ErrorCode::FORBIDDEN, "Only participants of this transaction can submit ratings");
    }

    if (ratings_.hasRated(txnId, rater.id())) {
        throw util::AppError(util::ErrorCode::CONFLICT, "You have already submitted a rating for this transaction");
    }

    int64_t rateeId = (rater.id() == txn->ownerId()) ? txn->borrowerId() : txn->ownerId();

    db::DbTransaction tx(db_);
    int64_t ratingId = ratings_.insert(txnId, rater.id(), rateeId, stars, comment);

    // Recalculate ratee reputation
    recalculateUserReputation(rateeId);

    tx.commit();

    return ratingId;
}

double RatingService::recalculateUserReputation(int64_t userId) {
    auto stars = ratings_.getStarsForUser(userId);

    // Fetch user transaction history stats
    auto user = users_.getById(userId);
    int total = 0, onTime = 0, damageFree = 0;
    if (user && user->role() == domain::UserRole::STUDENT) {
        auto student = dynamic_cast<const domain::Student*>(user.get());
        if (student) {
            total = student->totalTransactions();
            onTime = student->onTimeCount();
            damageFree = student->damageFreeCount();
        }
    }

    double newScore = domain::Reputation::calculate(stars, onTime, damageFree, total);
    users_.updateReputation(userId, newScore);
    return newScore;
}

} // namespace borrowhub::services
