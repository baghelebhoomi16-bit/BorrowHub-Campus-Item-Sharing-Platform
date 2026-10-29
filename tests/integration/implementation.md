# Integration Testing Implementation Notes (`tests/integration/`)

## Design Decisions

### 1. In-Memory SQLite Fixtures (`TestApp`)
- Rather than maintaining external persistent test databases that require cleanup scripts, `TestApp` constructs an isolated `:memory:` database with `db/schema.sql` applied on initialization.
- Each test scenario instantiates its own `TestApp`, providing complete state isolation without cross-test data pollution.

### 2. Multi-Role Scenario Verification
Integration tests cover every branch of the borrowing lifecycle:
1. Complete happy path: Request $\rightarrow$ Accept $\rightarrow$ Hold $\rightarrow$ Dual Handover $\rightarrow$ Return $\rightarrow$ Settle $\rightarrow$ Mutual Ratings.
2. Rejection and Cancellation.
3. 24-hour deposit payment expiration.
4. Late return penalties.
5. Damage deductions.
6. Lost item forfeiture.
7. 48-hour dispute raising and administrative split resolution.
8. Double-booking prevention via partial unique indexes.
9. Blocked student rejection.
10. Continuous ledger invariant assertion across randomized trials.
