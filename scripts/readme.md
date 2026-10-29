# Automation Scripts (`scripts/`)

## Purpose
This directory contains shell and operational scripts for building, running, backing up, resetting, and validating the BorrowHub codebase.

## Files
- `build.sh`: Configures and compiles release targets using CMake and vcpkg.
- `run.sh`: Boots the BorrowHub server with default environment variables and automatic database bootstrap.
- `backup.sh`: Executes non-blocking SQLite hot backup (`.backup`) and maintains a 7-day rolling snapshot archive.
- `reset_db.sh`: Re-initializes the database with schema and seed data when invoked with `--yes`.
- `check_api_docs.sh`: Static analysis script verifying all router endpoints are documented in `docs/api.md`.
- `readme.md`: Summary catalog and command usage examples.
- `implementation.md`: Design standards, error handling principles, and idempotency guarantees.

## Usage Examples
```bash
# Build binary
./scripts/build.sh

# Run server with defaults
./scripts/run.sh

# Run hot backup
./scripts/backup.sh

# Reset local development database
./scripts/reset_db.sh --yes

# Validate API documentation coverage
./scripts/check_api_docs.sh
```

## Dependencies
- Bash shell (POSIX compatible, Git Bash / WSL on Windows), CMake, SQLite3 CLI.

## Callers
- Developers, continuous integration pipelines (`.github/workflows/ci.yml`), and local operators.
