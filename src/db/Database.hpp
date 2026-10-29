#pragma once

#include "src/util/Errors.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

struct sqlite3;
struct sqlite3_stmt;

namespace borrowhub::db {

class Statement {
public:
    explicit Statement(sqlite3_stmt* stmt);
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;

    // Parameter binding (1-based index)
    void bind(int index, int val);
    void bind(int index, int64_t val);
    void bind(int index, double val);
    void bind(int index, std::string_view val);
    void bindNull(int index);

    // Step execution: returns true if a row is available, false if finished
    bool step();

    // Column extraction (0-based index)
    [[nodiscard]] int columnInt(int col) const;
    [[nodiscard]] int64_t columnInt64(int col) const;
    [[nodiscard]] double columnDouble(int col) const;
    [[nodiscard]] std::string columnText(int col) const;
    [[nodiscard]] bool columnIsNull(int col) const;

    void reset();

private:
    sqlite3_stmt* stmt_{nullptr};
};

class Database {
public:
    explicit Database(const std::string& path, bool readOnly = false);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&& other) noexcept;
    Database& operator=(Database&& other) noexcept;

    // Executes immediate non-parameterized SQL (e.g. pragmas, DDL scripts)
    void exec(std::string_view sql);

    // Prepares an SQL statement for execution
    [[nodiscard]] Statement prepare(std::string_view sql);

    // Returns the rowid of the most recent successful INSERT
    [[nodiscard]] int64_t lastInsertRowid() const noexcept;

    // Applies standard pragmas: WAL mode, foreign keys, busy timeout
    void applyPragmas();

    // Directly access raw pointer when necessary
    [[nodiscard]] sqlite3* handle() const noexcept { return db_; }

private:
    void checkSqliteError(int rc, std::string_view context);

    sqlite3* db_{nullptr};
    std::string path_;
};

} // namespace borrowhub::db
