#pragma once

#include "src/db/Database.hpp"
#include "src/util/Errors.hpp"

namespace borrowhub::db {

class DbTransaction {
public:
    explicit DbTransaction(Database& db) : db_(db) {
        db_.exec("BEGIN IMMEDIATE;");
    }

    ~DbTransaction() {
        if (!committed_) {
            try {
                db_.exec("ROLLBACK;");
            } catch (...) {
                // Suppress rollback errors in destructor to avoid std::terminate
            }
        }
    }

    DbTransaction(const DbTransaction&) = delete;
    DbTransaction& operator=(const DbTransaction&) = delete;
    DbTransaction(DbTransaction&&) = delete;
    DbTransaction& operator=(DbTransaction&&) = delete;

    void commit() {
        if (committed_) {
            throw util::AppError(util::ErrorCode::INTERNAL, "Transaction already committed");
        }
        db_.exec("COMMIT;");
        committed_ = true;
    }

    void rollback() {
        if (committed_) {
            throw util::AppError(util::ErrorCode::INTERNAL, "Cannot rollback committed transaction");
        }
        db_.exec("ROLLBACK;");
        committed_ = true; // Mark as settled
    }

    [[nodiscard]] bool isCommitted() const noexcept {
        return committed_;
    }

private:
    Database& db_;
    bool committed_{false};
};

} // namespace borrowhub::db
