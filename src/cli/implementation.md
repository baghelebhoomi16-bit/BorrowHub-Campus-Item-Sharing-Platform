# Admin CLI Implementation Notes (`src/cli/`)

## Design Decisions

### 1. Reusing Domain and Database Repositories
- The administrative CLI links directly against the identical `borrowhub_core` library used by the web REST server.
- No database logic is duplicated; changes made via CLI invoke the same repository routines, maintaining uniform data integrity and trigger enforcement.

### 2. Secure Terminal Password Entry
- Passwords entered via `create-admin` disable console echo (using Win32 `SetConsoleMode` or POSIX `termios`), ensuring passwords are never visible on the screen or in shell history.

### 3. Exit Code Convention
- `0`: Operation succeeded.
- `1`: Operation failed (business constraint, validation error).
- `2`: Bad invocation or unknown command (triggers usage text).
