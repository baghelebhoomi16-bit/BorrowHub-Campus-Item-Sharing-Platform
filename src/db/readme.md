# Repository and Database Layer (`src/db/`)

## Purpose
This directory provides C++ RAII abstractions over the SQLite C API, implements transactions with strict locking (`BEGIN IMMEDIATE`), and defines repository interfaces and concrete implementations for data access.

## Files
- `Database.hpp` / `Database.cpp`: RAII wrapper around `sqlite3*` and `sqlite3_stmt*` with type-safe binding and column extraction.
- `DbTransaction.hpp`: Transaction scope guard issuing `BEGIN IMMEDIATE` and auto-rollback on failure.
- `Repositories.hpp`: Pure abstract interfaces for all domain repositories (`UserRepo`, `ItemRepo`, `ThreadRepo`, `MessageRepo`, `TxnRepo`, `LedgerRepo`, `RatingRepo`, `NotificationRepo`, `DisputeRepo`, `SessionRepo`, `VerificationTokenRepo`).
- `SqliteRepositories.cpp`: Production SQLite implementation of each repository interface using prepared statements.
- `readme.md`: Overview and guide for adding new repositories.
- `implementation.md`: Technical rationale for prepared statements, mapping rules, and concurrency control.

## How to Add a New Repository
1. Define the pure virtual interface in `src/db/Repositories.hpp`.
2. Add factory function declaration: `std::unique_ptr<NewRepo> makeNewRepo(Database& db);`.
3. Implement the class in `src/db/SqliteRepositories.cpp` using `db_.prepare(...)` and bound parameters only.
4. For unit testing, implement an in-memory mock repository inheriting from the same interface.

## Dependencies
- SQLite 3 library and headers.

## Callers
- `src/services/` (all use cases execute queries strictly through repository interfaces).
