# Application Implementation & Runtime Architecture (`src/`)

## Design Decisions

### 1. Application Startup Order
1. Install signal traps for `SIGINT` and `SIGTERM`.
2. Load configuration from environment variables with range validation (`util::Config`).
3. Instantiate monotonic `util::SystemClock`.
4. Open SQLite connection with pragmas (`foreign_keys = ON; journal_mode = WAL; busy_timeout = 5000;`).
5. Execute `db/schema.sql` and `db/seed.sql` if `--init-db` or `--seed` flags are provided.
6. Instantiate repositories and `EventBus`.
7. Instantiate service layer, automatically subscribing `NotificationService` to events.
8. Start the background `Scheduler` thread (60-second polling cadence).
9. Initialize `Server`, configure payload bounds (16 KB), attach security headers, and register routes.
10. Block on `server.start()` until signal arrival initiates clean thread join and DB closure.

### 2. Threading Model
- **HTTP Worker Pool**: `cpp-httplib` dispatches incoming requests across an internal thread pool. Concurrent read transactions proceed simultaneously under SQLite WAL mode.
- **Write Serialization**: Write operations request `BEGIN IMMEDIATE;` locks, guaranteeing single-writer isolation without deadlocks.
- **Scheduler Worker**: A single background thread executes maintenance ticks (`overdue`, `deposit cancellation`, `auto-settlement`) every 60 seconds without blocking API handlers.

### 3. Error Handling Hierarchy
- Low-level failures (constraint violations, invalid regex) throw `util::AppError(code, message)`.
- Services pass `AppError` exceptions up to the API layer.
- `Server::setup()` catches `AppError` at the root boundary, mapping the error code directly to its standard HTTP status code and rendering a structured JSON payload:
  ```json
  { "error": { "code": "VALIDATION", "message": "..." } }
  ```
- Unhandled standard exceptions return HTTP 500 with a generic message, preventing sensitive stack traces from leaking to clients.
