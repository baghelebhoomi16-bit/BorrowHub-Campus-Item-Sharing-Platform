# Documentation Design Decisions and Synchronization (`docs/`)

## Design Decisions and Synchronization Policy

### 1. Documentation as Code
API specifications in `docs/api.md` serve as the formal contract between frontend components (`web/app.js`, `web/chat.js`), external clients, and the backend routing layer (`src/api/Router.cpp`).
To prevent documentation rot:
- Route registrations in `src/api/Router.cpp` are checked against `docs/api.md` using the automated shell script `scripts/check_api_docs.sh`.
- Any addition or deletion of a REST endpoint without a corresponding update in `docs/api.md` fails the validation check.

### 2. Formats and Conventions
- HTTP verbs follow REST semantics:
  - `GET` for idempotent read operations.
  - `POST` for state-changing commands and resource creation.
  - `PUT` for complete/partial resource modifications.
  - `DELETE` for soft deletion / moderation actions.
- Monetary amounts are documented in integer paise, with optional UI display strings (`deposit_formatted`).

### 3. Limitations
- Automated documentation generators (such as OpenAPI/Swagger) often require heavy annotations or third-party macros that add build complexity in C++.
- Maintaining clean markdown specifications with CI verification ensures documentation is easy to read, diff, and audit without runtime overhead.
