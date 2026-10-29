#include "src/db/Database.hpp"
#include <sqlite3.h>

namespace borrowhub::db {

Statement::Statement(sqlite3_stmt* stmt) : stmt_(stmt) {}

Statement::~Statement() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
        stmt_ = nullptr;
    }
}

Statement::Statement(Statement&& other) noexcept : stmt_(other.stmt_) {
    other.stmt_ = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept {
    if (this != &other) {
        if (stmt_) {
            sqlite3_finalize(stmt_);
        }
        stmt_ = other.stmt_;
        other.stmt_ = nullptr;
    }
    return *this;
}

void Statement::bind(int index, int val) {
    if (sqlite3_bind_int(stmt_, index, val) != SQLITE_OK) {
        throw util::AppError(util::ErrorCode::INTERNAL, "Failed to bind int");
    }
}

void Statement::bind(int index, int64_t val) {
    if (sqlite3_bind_int64(stmt_, index, val) != SQLITE_OK) {
        throw util::AppError(util::ErrorCode::INTERNAL, "Failed to bind int64");
    }
}

void Statement::bind(int index, double val) {
    if (sqlite3_bind_double(stmt_, index, val) != SQLITE_OK) {
        throw util::AppError(util::ErrorCode::INTERNAL, "Failed to bind double");
    }
}

void Statement::bind(int index, std::string_view val) {
    if (sqlite3_bind_text(stmt_, index, val.data(), static_cast<int>(val.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
        throw util::AppError(util::ErrorCode::INTERNAL, "Failed to bind text");
    }
}

void Statement::bindNull(int index) {
    if (sqlite3_bind_null(stmt_, index) != SQLITE_OK) {
        throw util::AppError(util::ErrorCode::INTERNAL, "Failed to bind null");
    }
}

bool Statement::step() {
    int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) {
        return true;
    }
    if (rc == SQLITE_DONE) {
        return false;
    }

    // Handle constraint violations
    if (rc == SQLITE_CONSTRAINT) {
        int extended = sqlite3_extended_errcode(sqlite3_db_handle(stmt_));
        if (extended == SQLITE_CONSTRAINT_UNIQUE || extended == SQLITE_CONSTRAINT_PRIMARYKEY) {
            throw util::AppError(util::ErrorCode::CONFLICT, "Resource already exists (unique constraint violation)");
        }
        if (extended == SQLITE_CONSTRAINT_FOREIGNKEY) {
            throw util::AppError(util::ErrorCode::VALIDATION, "Referenced resource does not exist (foreign key violation)");
        }
        if (extended == SQLITE_CONSTRAINT_CHECK) {
            throw util::AppError(util::ErrorCode::VALIDATION, "Check constraint violated");
        }
        if (extended == SQLITE_CONSTRAINT_TRIGGER) {
            throw util::AppError(util::ErrorCode::ILLEGAL_STATE, "Append-only operation violated: modifications not permitted");
        }
        throw util::AppError(util::ErrorCode::CONFLICT, "Database constraint violation");
    }

    if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
        throw util::AppError(util::ErrorCode::INTERNAL, "Database is locked");
    }

    throw util::AppError(util::ErrorCode::INTERNAL, sqlite3_errmsg(sqlite3_db_handle(stmt_)));
}

int Statement::columnInt(int col) const {
    return sqlite3_column_int(stmt_, col);
}

int64_t Statement::columnInt64(int col) const {
    return sqlite3_column_int64(stmt_, col);
}

double Statement::columnDouble(int col) const {
    return sqlite3_column_double(stmt_, col);
}

std::string Statement::columnText(int col) const {
    const unsigned char* text = sqlite3_column_text(stmt_, col);
    if (!text) return "";
    return reinterpret_cast<const char*>(text);
}

bool Statement::columnIsNull(int col) const {
    return sqlite3_column_type(stmt_, col) == SQLITE_NULL;
}

void Statement::reset() {
    sqlite3_reset(stmt_);
    sqlite3_clear_bindings(stmt_);
}

Database::Database(const std::string& path, bool readOnly) : path_(path) {
    int flags = readOnly ? SQLITE_OPEN_READONLY : (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    int rc = sqlite3_open_v2(path.c_str(), &db_, flags, nullptr);
    if (rc != SQLITE_OK) {
        std::string err = db_ ? sqlite3_errmsg(db_) : "Failed to open sqlite database";
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
        throw util::AppError(util::ErrorCode::INTERNAL, "Cannot open DB at " + path + ": " + err);
    }
    applyPragmas();
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

Database::Database(Database&& other) noexcept : db_(other.db_), path_(std::move(other.path_)) {
    other.db_ = nullptr;
}

Database& Database::operator=(Database&& other) noexcept {
    if (this != &other) {
        if (db_) {
            sqlite3_close(db_);
        }
        db_ = other.db_;
        path_ = std::move(other.path_);
        other.db_ = nullptr;
    }
    return *this;
}

void Database::applyPragmas() {
    exec("PRAGMA foreign_keys = ON;");
    exec("PRAGMA journal_mode = WAL;");
    exec("PRAGMA busy_timeout = 5000;");
    exec("PRAGMA synchronous = NORMAL;");
}

void Database::exec(std::string_view sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql.data(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : "sqlite3_exec failed";
        sqlite3_free(err);
        checkSqliteError(rc, msg);
    }
}

Statement Database::prepare(std::string_view sql) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.data(), static_cast<int>(sql.size()), &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db_);
        checkSqliteError(rc, "Prepare failed: " + err + " for query: " + std::string(sql));
    }
    return Statement(stmt);
}

int64_t Database::lastInsertRowid() const noexcept {
    return sqlite3_last_insert_rowid(db_);
}

void Database::checkSqliteError(int rc, std::string_view context) {
    if (rc == SQLITE_CONSTRAINT) {
        throw util::AppError(util::ErrorCode::CONFLICT, std::string(context));
    }
    if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
        throw util::AppError(util::ErrorCode::INTERNAL, "Database is locked");
    }
    throw util::AppError(util::ErrorCode::INTERNAL, std::string(context));
}

} // namespace borrowhub::db
