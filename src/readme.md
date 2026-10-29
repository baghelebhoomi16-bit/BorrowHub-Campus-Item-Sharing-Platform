# Application Source Root (`src/`)

## Purpose
This directory contains the layered modular monolith implementation of BorrowHub in modern C++17.

## Layer Diagram & Dependency Rule
```
+-------------------------------------------------------------+
| Browser / Web Client (web/)      Console Admin Tool (cli/)  |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
| API Layer (src/api)                                         |
| - Routing, AuthMiddleware, RateLimiter, JsonMapper          |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
| Service Layer (src/services)                                |
| - Use cases, DB Transactions, EventBus, Scheduler           |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
| Domain Layer (src/domain)                                   |
| - Money, User, Item hierarchy, State Machine, Policies      |
| - ZERO I/O or network dependencies                          |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
| Database Layer (src/db)                                     |
| - SQLite RAII wrapper, DbTransaction, Repository Interfaces |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
| SQLite Engine (WAL mode, schema in db/schema.sql)           |
+-------------------------------------------------------------+
```

### Dependency Invariant:
**Arrows point downward only.**
The domain layer has zero knowledge of SQLite, HTTP, or JSON formatting. This separation enables fast in-memory unit testing without mocking network sockets or setting up databases.

## Directories
- `util/`: Cross-cutting helpers (injected Clock, Token, Validation, Errors, Config).
- `domain/`: Business entities, policies, and BorrowTransaction state machine.
- `db/`: SQLite wrappers, transaction scopes, and repository implementations.
- `services/`: Transactional use cases and decoupled event publishing.
- `api/`: HTTP routing, HttpOnly session cookies, sliding rate limits, and JSON mapping.
- `cli/`: Campus administrative console tool.
