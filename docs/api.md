# BorrowHub REST API Specification

All API endpoints return JSON responses. Protected endpoints require a valid session cookie `bh_session` (HttpOnly, SameSite=Strict).

---

## Error Response Format
```json
{
  "error": {
    "code": "VALIDATION | UNAUTHENTICATED | FORBIDDEN | NOT_FOUND | CONFLICT | ILLEGAL_STATE | RATE_LIMITED | INTERNAL",
    "message": "Human-readable description of error"
  }
}
```

---

## 1. Authentication & Account Lifecycle

### `POST /api/auth/register`
- **Auth**: Public
- **Request Body**:
  ```json
  {
    "email": "student@campus.edu",
    "prn": "PRN2024005",
    "password": "SecurePassword123!",
    "name": "Alex Student",
    "phone": "+919988776655",
    "department": "Computer Science",
    "year": 3,
    "division": "A"
  }
  ```
- **Response (201 Created)**:
  ```json
  {
    "id": 5,
    "email": "student@campus.edu",
    "message": "Registration successful. Please verify via verification link."
  }
  ```
- **Errors**: 400 (VALIDATION), 409 (CONFLICT: email/PRN already exists).

### `POST /api/auth/verify`
- **Auth**: Public
- **Request Body**:
  ```json
  {
    "token": "43-character-base64url-token"
  }
  ```
- **Response (200 OK)**:
  ```json
  {
    "verified": true,
    "message": "Account successfully verified."
  }
  ```
- **Errors**: 400 (VALIDATION), 404 (NOT_FOUND: expired or invalid token).

### `POST /api/auth/login`
- **Auth**: Public (Rate-limited to 5 requests / min / IP)
- **Request Body**:
  ```json
  {
    "email": "student@campus.edu",
    "password": "SecurePassword123!"
  }
  ```
- **Response (200 OK)**:
  - Header: `Set-Cookie: bh_session=<token>; HttpOnly; SameSite=Strict; Path=/; Max-Age=86400`
  ```json
  {
    "user": {
      "id": 5,
      "email": "student@campus.edu",
      "name": "Alex Student",
      "role": "STUDENT",
      "reputation": 62.0
    }
  }
  ```
- **Errors**: 401 (UNAUTHENTICATED: invalid credentials or unverified account), 403 (FORBIDDEN: account blocked), 429 (RATE_LIMITED).

### `POST /api/auth/logout`
- **Auth**: Session
- **Response (200 OK)**:
  - Header: `Set-Cookie: bh_session=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0`
  ```json
  { "message": "Logged out successfully" }
  ```

### `GET /api/auth/me`
- **Auth**: Session
- **Response (200 OK)**: Current authenticated user details and reputation.

---

## 2. Items & Search

### `GET /api/items`
- **Auth**: Session
- **Query Params**:
  - `q`: Text search across title and category
  - `category`: Category filter
  - `location`: Location filter
  - `condition`: `GOOD` | `MINOR_DAMAGE` | `MAJOR_DAMAGE`
  - `max_deposit`: Maximum deposit in paise
  - `available_on`: Epoch seconds timestamp
  - `min_rating`: Minimum owner reputation (0-100)
  - `page`: Page index (default: 1)
  - `page_size`: Results per page (default: 20, max: 50)
- **Response (200 OK)**:
  ```json
  {
    "items": [
      {
        "id": 1,
        "owner_id": 2,
        "type": "BOOK",
        "title": "Introduction to Algorithms (CLRS 3rd Ed)",
        "category": "Academics",
        "condition": "GOOD",
        "location": "Hostel 3, Room 214",
        "deposit_paise": 15000,
        "deposit_formatted": "₹150.00",
        "status": "AVAILABLE",
        "owner_reputation": 62.0,
        "attrs": { "author": "CLRS", "isbn": "9780262033848" }
      }
    ],
    "total": 1,
    "page": 1,
    "page_size": 20
  }
  ```

### `POST /api/items`
- **Auth**: Verified Student
- **Request Body**:
  ```json
  {
    "type": "ELECTRONIC",
    "title": "Raspberry Pi 4 Model B (4GB)",
    "category": "Embedded Systems",
    "condition": "GOOD",
    "location": "Tech Lab 3",
    "deposit_paise": 50000,
    "avail_from": 1730000000,
    "avail_to": 1761500000,
    "attrs": { "brand": "Raspberry Pi Foundation", "model": "4B" }
  }
  ```
- **Response (201 Created)**: Created item DTO.
- **Errors**: 400 (VALIDATION: deposit below type minimum, invalid dates).

### `PUT /api/items/{id}`
- **Auth**: Item Owner
- **Request Body**: Partial or full item fields.
- **Errors**: 403 (FORBIDDEN), 409 (ILLEGAL_STATE: item has active borrowing).

### `DELETE /api/items/{id}`
- **Auth**: Item Owner or Admin
- **Response (200 OK)**: Soft-deletes item to `REMOVED`.
- **Errors**: 409 (ILLEGAL_STATE: active transaction exists).

---

## 3. Chat System

### `POST /api/items/{id}/threads`
- **Auth**: Student (non-owner)
- **Response (200 OK / 201 Created)**:
  ```json
  {
    "thread_id": 1,
    "item_id": 1,
    "borrower_id": 3
  }
  ```

### `GET /api/threads`
- **Auth**: Session
- **Response (200 OK)**: List of threads where user is borrower or owner.

### `GET /api/threads/{id}/messages?after_id={N}`
- **Auth**: Thread Participant
- **Response (200 OK)**:
  ```json
  {
    "messages": [
      {
        "id": 1,
        "thread_id": 1,
        "sender_id": 3,
        "sender_name": "Priya Sharma",
        "body": "Is this available?",
        "created_at": 1730001000
      }
    ]
  }
  ```

### `POST /api/threads/{id}/messages`
- **Auth**: Thread Participant
- **Request Body**:
  ```json
  { "body": "Yes, it is available from tomorrow." }
  ```
- **Response (201 Created)**: Message DTO.
- **Errors**: 400 (VALIDATION: length > 2000), 429 (RATE_LIMITED: max 20 msgs/min).

---

## 4. Borrow Lifecycle Actions

### `POST /api/requests`
- **Auth**: Verified Student (Reputation >= 30 if >= 3 txns)
- **Request Body**:
  ```json
  {
    "item_id": 1,
    "start_date": 1730100000,
    "due_date": 1730445600
  }
  ```
- **Response (201 Created)**: Transaction DTO in `REQUESTED` status.
- **Errors**: 400 (VALIDATION: duration > maxBorrowDays), 409 (CONFLICT / ILLEGAL_STATE).

### `POST /api/requests/{id}/accept`
- **Auth**: Item Owner
- **Response (200 OK)**: Transaction DTO in `ACCEPTED` status. Other pending requests auto-rejected.

### `POST /api/requests/{id}/reject`
- **Auth**: Item Owner
- **Response (200 OK)**: Transaction DTO in `REJECTED` status.

### `POST /api/requests/{id}/cancel`
- **Auth**: Owner or Borrower
- **Response (200 OK)**: Transaction DTO in `CANCELLED` status.

### `POST /api/requests/{id}/pay-deposit`
- **Auth**: Borrower
- **Response (200 OK)**: Records ledger `HOLD`.
- **Errors**: 400 (ILLEGAL_STATE: expired 24h window or not in ACCEPTED state).

### `POST /api/requests/{id}/confirm-handover`
- **Auth**: Owner or Borrower
- **Response (200 OK)**: Sets party flag; when both confirmed, status becomes `ACTIVE`.

### `POST /api/requests/{id}/report-return`
- **Auth**: Owner
- **Request Body**:
  ```json
  {
    "condition": "GOOD | MINOR_DAMAGE | MAJOR_DAMAGE | LOST",
    "claim_paise": 0
  }
  ```
- **Response (200 OK)**: Status becomes `RETURNED`.

### `POST /api/requests/{id}/accept-settlement`
- **Auth**: Borrower
- **Response (200 OK)**: Executes settlement; writes `REFUND` and `DEDUCT`/`PAYOUT`; status becomes `SETTLED`.

### `POST /api/requests/{id}/dispute`
- **Auth**: Borrower or Owner (within 48h of return report)
- **Request Body**:
  ```json
  { "reason": "Damage claim is inaccurate; item was returned in identical condition." }
  ```
- **Response (201 Created)**: Status becomes `DISPUTED`.

### `POST /api/requests/{id}/rating`
- **Auth**: Transaction Participant (Status must be `SETTLED`)
- **Request Body**:
  ```json
  {
    "stars": 5,
    "comment": "Returned on time in great condition!"
  }
  ```
- **Response (201 Created)**: Rating recorded and user reputation recomputed.

---

## 5. Notifications & Dashboard

### `GET /api/notifications`
- **Auth**: Session
- **Response (200 OK)**: Array of notification objects.

### `POST /api/notifications/{id}/read`
- **Auth**: Notification Recipient
- **Response (200 OK)**: Marks notification read.

### `GET /api/dashboard`
- **Auth**: Session
- **Response (200 OK)**: Active listings, active borrows, pending requests, unread notifications.

---

## 6. Admin Endpoints

### `GET /api/admin/disputes`
- **Auth**: Admin
- **Response (200 OK)**: Open and decided disputes with associated transaction data.

### `POST /api/admin/disputes/{id}/decide`
- **Auth**: Admin
- **Request Body**:
  ```json
  {
    "owner_award_paise": 10000,
    "decision_note": "Evidence confirms minor scratch. Awarding ₹100 to owner and refunding balance."
  }
  ```
- **Response (200 OK)**: Resolves dispute and executes settlement.

### `POST /api/admin/users/{id}/block`
- **Auth**: Admin
- **Request Body**:
  ```json
  { "blocked": true }
  ```
- **Response (200 OK)**: Updates user block status and terminates sessions.

### `DELETE /api/admin/items/{id}`
- **Auth**: Admin
- **Response (200 OK)**: Moderation removal of item.

---

## 7. System Health

### `GET /health`
- **Auth**: Public
- **Response (200 OK)**:
  ```json
  {
    "status": "UP",
    "timestamp": 1730000000
  }
  ```
