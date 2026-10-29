# Database Layer (`db/`)

## Purpose
This directory defines the relational database schema, database triggers, index definitions, and initial seed datasets for BorrowHub using SQLite 3 with Write-Ahead Logging (WAL).

## Files
- `schema.sql`: Complete DDL schema including tables, CHECK constraints, foreign keys, triggers (append-only enforcement), and partial unique indexes.
- `seed.sql`: Initial bootstrap and demo dataset containing an administrator account, verified students across engineering departments, and sample items across all three polymorphism types (`BOOK`, `ELECTRONIC`, `EQUIPMENT`).
- `readme.md`: Directory overview, schema application instructions, and reset guide.
- `implementation.md`: Design rationale for integer monetary representations, triggers, and concurrency control.

## How to Apply and Reset the Schema
### 1. Applying Schema via SQLite CLI
```bash
sqlite3 data/borrowhub.sqlite < db/schema.sql
sqlite3 data/borrowhub.sqlite < db/seed.sql
```

### 2. Applying Schema via Application Binary
```bash
./build/src/borrowhub --init-db --seed
```

### 3. Resetting the Database
```bash
./scripts/reset_db.sh --yes
```

## Dependencies
- SQLite 3.45+ with foreign key constraints enabled (`PRAGMA foreign_keys = ON;`).

## Callers
- `src/main.cpp` and `src/db/Database.cpp` load and execute these SQL files during initialization.
- Integration test suite (`tests/integration/TestApp.hpp`) executes `schema.sql` on `:memory:` SQLite instances.
