# Utility Implementation Notes (`src/util/`)

## Design Decisions and Architectural Notes

### 1. Injected Clock Pattern
- **Problem**: Calling `std::chrono::system_clock::now()` directly across business services makes time-sensitive workflows (24h deposit expiry, 48h dispute windows, overdue marking) flaky and slow in automated tests.
- **Solution**: Services and state machines inject a `const Clock&`. In production, `SystemClock` is used. In unit and integration tests, `FakeClock` is advanced instantly without `sleep()`.

### 2. Error Code Taxonomy
`AppError` encapsulates standard application-level error codes:
- `VALIDATION (400)`: Ill-formed input, negative deposits, invalid email.
- `UNAUTHENTICATED (401)`: Missing or expired session cookie.
- `FORBIDDEN (403)`: Action attempted by non-participant, student accessing admin routes, blocked user.
- `NOT_FOUND (404)`: Entity ID does not exist.
- `CONFLICT (409)`: Duplicate email/PRN, overlapping transaction reservation.
- `ILLEGAL_STATE (409)`: Invalid state transition (e.g. paying deposit on `REQUESTED` item).
- `RATE_LIMITED (429)`: Login attempts or chat velocity threshold exceeded.
- `INTERNAL (500)`: Database or unexpected runtime faults.

### 3. Cryptography & Security
- `Token::generate()` produces 32-byte (256-bit) cryptographically strong random tokens, encoded as Base64url (43 alphanumeric characters).
- Session tokens and email verification tokens are stored as SHA-256 hashes in the database. Even if the database is leaked, raw session tokens cannot be recovered without a pre-image attack.
- Password hashing defaults to Argon2id via `libsodium` with secure salting.
