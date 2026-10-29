# Database and Repository Implementation Notes (`src/db/`)

## Design Decisions and Architectural Notes

### 1. RAII Resource Management
- Raw SQLite handles (`sqlite3*`, `sqlite3_stmt*`) are vulnerable to memory leaks and database locks if an exception unwinds the stack before `sqlite3_finalize` or `sqlite3_close` is reached.
- `Statement` and `Database` encapsulate these handles, ensuring destruction and statement finalization are guaranteed under all stack-unwinding conditions, fully passing AddressSanitizer (ASan).

### 2. Prepared Statements & Parameter Binding
- No SQL string concatenation is permitted anywhere in data access queries.
- Every user input, text filter, and ID is bound via `sqlite3_bind_*`. This renders SQL injection attempts (e.g. `'; DROP TABLE users;--`) completely harmless, as the string is interpreted strictly as data.

### 3. Immediate Transactions (`BEGIN IMMEDIATE`)
- **Problem**: Default SQLite `BEGIN` (deferred) allows multiple readers to begin simultaneously. When two readers subsequently attempt to write, one fails with `SQLITE_BUSY` deadlock.
- **Solution**: `DbTransaction` begins with `BEGIN IMMEDIATE;`. This reserves write intent immediately, serializing concurrent writes without mid-transaction deadlocks.

### 4. SQLite Error Mapping
- SQLite native numeric error codes are caught and translated into domain `AppError` exceptions:
  - `SQLITE_CONSTRAINT_UNIQUE` / `PRIMARYKEY` $\rightarrow$ `ErrorCode::CONFLICT` (409)
  - `SQLITE_CONSTRAINT_FOREIGNKEY` $\rightarrow$ `ErrorCode::VALIDATION` (400)
  - `SQLITE_CONSTRAINT_CHECK` $\rightarrow$ `ErrorCode::VALIDATION` (400)
  - `SQLITE_CONSTRAINT_TRIGGER` $\rightarrow$ `ErrorCode::ILLEGAL_STATE` (409)
  - `SQLITE_BUSY` / `LOCKED` $\rightarrow$ `ErrorCode::INTERNAL` (500)
