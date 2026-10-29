# Service Implementation Architecture (`src/services/`)

## Design Decisions and Transaction Pipeline

### 1. Standard Execution Pipeline
Every public service method modifying state follows a strict seven-step transactional lifecycle:
1. **Open Transaction**: Instantiate `db::DbTransaction tx(db_)` with `BEGIN IMMEDIATE`.
2. **Authorize**: Verify the authenticated caller is eligible and has permission (e.g. participant check, owner check, admin check).
3. **Load Entities**: Fetch domain entities through repository interfaces.
4. **Domain Execution**: Invoke domain logic and state transitions on `BorrowTransaction`, `Money`, or `Item`. Any illegal transition throws `AppError`.
5. **Persist State**: Write mutated models back through repository interfaces.
6. **Publish Events**: Dispatch domain events to `EventBus`.
7. **Commit**: Call `tx.commit()`.

If an unhandled exception or business error occurs at any point, the `DbTransaction` destructor automatically issues `ROLLBACK`, guaranteeing complete atomicity across multiple tables.

### 2. No Raw DB Rows
Services never expose raw SQLite rows, handles, or query cursors to the API layer. All data passes through domain models (`Item`, `BorrowTransaction`, `User`) or structured DTOs (`SettlementResult`, `LoginResult`).

### 3. Decoupled Observer Pattern (`EventBus`)
- Rather than tight coupling between `BorrowService` and `NotificationService`, actions publish discrete events (`REQUEST_CREATED`, `TRANSACTION_OVERDUE`, `SETTLED`).
- Observers subscribe independently. If an observer throws an exception, it is caught and logged, preventing background notification failures from aborting the primary transaction.
