# Web Frontend Implementation Notes (`web/`)

## Design Decisions and Security Standards

### 1. Hash-Based SPA Architecture
- **Problem**: HTML5 `pushState` routing requires backend server fallbacks (rewriting all 404s to `index.html`), which complicates static file serving in embedded C++ HTTP servers.
- **Solution**: The application uses standard hash routing (`#browse`, `#item/1`, `#my-requests`). The browser never requests non-existent subpaths from the server, allowing straightforward static file serving directly from the `web/` folder.

### 2. Fetch Wrapper & Session Credentials
- All HTTP calls pass through `ApiClient.request()`.
- Requests include `credentials: 'same-origin'`, which instructs the browser to attach the `bh_session` cookie to every API invocation automatically.
- Any `401 Unauthorized` response immediately redirects the user to `#login`.

### 3. Strict XSS Prevention: Zero `innerHTML` for User Data
- **The Golden Rule**: User-generated content (item titles, conditions, chat messages, user names, comments, dispute notes) is **NEVER** concatenated into HTML strings or inserted via `innerHTML`.
- Dynamic content is inserted exclusively using `element.textContent = ...` or `document.createTextNode(...)`.
- Testing with injection payloads such as `<img src=x onerror=alert(1)>` proves the payload renders strictly as benign literal text without executing script tags.

### 4. Action Button Gating by State
- Buttons are rendered conditionally based on the transaction's current status and actor ID:
  - Deposit payment is only rendered when `status === 'ACCEPTED'` and `!depositHeld`.
  - Handover confirmation is only rendered when `depositHeld` and user has not confirmed yet.
  - Return reporting is only rendered when `status === 'ACTIVE'` or `status === 'OVERDUE'`.
  - Ratings are only rendered when `status === 'SETTLED'`.
