# Utility Components (`src/util/`)

## Purpose
This directory houses foundational, reusable cross-cutting components including time abstractions, error taxonomies, input validation routines, configuration management, and cryptographic helpers.

## Files
- `Clock.hpp`: Time provider interface (`Clock`), monotonic `SystemClock`, and controllable `FakeClock` for tests.
- `Errors.hpp`: Standardized `ErrorCode` enum, `AppError` exception class, and HTTP status code mappings.
- `Config.hpp` / `Config.cpp`: Environment variable parsing, defaults, and boundary validation.
- `Validation.hpp` / `Validation.cpp`: Sanitization, string trimming, email regex, PRN checking, and length bounds.
- `Token.hpp` / `Token.cpp`: Cryptographically secure token generation, SHA-256 hashing, and Argon2id password verification.
- `readme.md`: Summary catalog and dependencies.
- `implementation.md`: Design rationale and security considerations.

## How to Build and Run
Built as part of `borrowhub_core` via CMake:
```bash
cmake --build build --target borrowhub_core
```

## Dependencies
- Standard C++17 library.
- Optional: `libsodium` (for hardware/kernel accelerated Argon2id and CSPRNG).

## Callers
- Domain, Service, API, and CLI layers.
