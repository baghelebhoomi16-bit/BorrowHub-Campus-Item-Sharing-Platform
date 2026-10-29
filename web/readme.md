# Web Frontend (`web/`)

## Purpose
This directory houses the vanilla JavaScript single-page web application (SPA) for BorrowHub. It communicates with the backend via REST endpoints and real-time polling.

## Views and API Mappings
- `#browse`: Explores available campus inventory. Calls `GET /api/items`.
- `#item/:id`: Item details and mandatory pre-request negotiation chat. Calls `GET /api/items/:id`, `POST /api/items/:id/threads`, and `POST /api/requests`.
- `#my-requests`: Displays active borrowing lifecycle and actions (pay deposit, confirm handover, dispute, settle, rate). Calls `GET /api/dashboard`, `POST /api/requests/:id/*`.
- `#my-listings`: Manages owned listings, incoming borrow requests, and return condition reports. Calls `POST /api/requests/:id/accept`, `POST /api/requests/:id/report-return`.
- `#list-item`: Creates new listings enforcing category-specific minimum deposits. Calls `POST /api/items`.
- `#notifications`: In-app activity feed and state changes. Calls `GET /api/notifications` and `POST /api/notifications/:id/read`.
- `#admin`: Campus administrator dashboard for resolving open dispute claims. Calls `GET /api/admin/disputes` and `POST /api/admin/disputes/:id/decide`.
- `#login` / `#register` / `#verify`: Authentication screens. Calls `POST /api/auth/*`.

## Files
- `index.html`: Shell page with responsive navigation and semantic containers.
- `app.js`: SPA hash router, API client, and state machine action controls.
- `chat.js`: Polling chat client with visibility-based throttling and XSS-safe DOM construction.
- `styles.css`: Glassmorphic, responsive stylesheet supporting dark and light themes.
- `readme.md`: View mapping summary.
- `implementation.md`: Architectural decisions and XSS defense policy.
