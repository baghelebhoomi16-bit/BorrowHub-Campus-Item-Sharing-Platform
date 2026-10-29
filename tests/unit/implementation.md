# Unit Testing Principles (`tests/unit/`)

## Design Principles

### 1. In-Memory Isolation & Zero Sleeping
- In accordance with Blueprint Section 7 & 10.13, **no test may ever call `sleep()`**.
- All time-sensitive workflows (24h deposit expiry, 48h dispute windows, overdue marking) inject `util::FakeClock`.
- Time is advanced deterministically and instantly via `clock.advance(seconds)` or `clock.set(timestamp)`.

### 2. Fast Execution
- Unit tests run against `:memory:` SQLite instances or domain objects directly without file system I/O.
- The entire unit suite executes in under 200 milliseconds.

### 3. Comprehensive Negative Testing
- Tests explicitly verify that unauthorized actors receive `AppError(FORBIDDEN)`, illegal state transitions receive `AppError(ILLEGAL_STATE)`, and duplicate identities throw `AppError(CONFLICT)`.
