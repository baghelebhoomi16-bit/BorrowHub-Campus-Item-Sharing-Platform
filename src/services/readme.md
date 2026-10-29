# Service Layer (`src/services/`)

## Purpose
The service layer implements application use cases, coordinates database transactions, enforces permissions, computes financial settlements, and dispatches events via the `EventBus`.

## Services and Events Published
- `AuthService`: Manages user registration, Argon2id verification, session tokens, and authentication cookies. Emits `USER_REGISTERED`.
- `ItemService`: Handles item listing CRUD and owner authorization. Enforces type-specific minimum deposits.
- `SearchService`: Executes filtered, paginated catalog queries with relevance and reputation sorting.
- `ChatService`: Manages private negotiation threads and enforces velocity limits (20 messages/minute).
- `BorrowService`: Drives borrowing workflows and dual-confirmation handovers. Emits `REQUEST_CREATED`, `REQUEST_ACCEPTED`, `REQUEST_REJECTED`, `REQUEST_CANCELLED`, `DEPOSIT_PAID`, `HANDOVER_CONFIRMED`, `TRANSACTION_ACTIVE`, and `RETURN_REPORTED`.
- `SettlementService`: Deterministically distributes held deposits according to policy penalties and damage claims. Emits `SETTLED`.
- `RatingService`: Records mutual participant star reviews and updates Bayesian reputation scores.
- `DisputeService`: Freezes funds, records dispute claims, and executes admin financial decisions. Emits `DISPUTE_RAISED` and `DISPUTE_DECIDED`.
- `NotificationService`: Subscribes to `EventBus` domain events and writes notifications to recipient inboxes.
- `Scheduler`: Autonomous maintenance runner marking overdue borrowings, cancelling expired 24h reservations, and auto-settling undisputed 48h returns. Emits `TRANSACTION_OVERDUE` and `DUE_REMINDER`.
- `EventBus.hpp`: In-process publish/subscribe message bus.

## How to Build and Run
Compiled into `borrowhub_core` library and tested through unit and integration suites.

## Dependencies
- `src/domain/`, `src/db/`, `src/util/`.
