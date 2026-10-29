# Entity-Relationship (ER) Diagram

The following Mermaid entity-relationship diagram illustrates the SQLite database schema implemented in `db/schema.sql`.

```mermaid
erDiagram
    users ||--o{ items : "owns"
    users ||--o{ sessions : "authenticates"
    users ||--o{ verification_tokens : "verifies"
    users ||--o{ chat_threads : "participates_as_borrower"
    users ||--o{ messages : "sends"
    users ||--o{ transactions : "owns_or_borrows"
    users ||--o{ ledger_entries : "holds_funds"
    users ||--o{ ratings : "rates_or_receives"
    users ||--o{ notifications : "receives"
    users ||--o{ disputes : "raises_or_administers"

    items ||--o{ chat_threads : "subject_of"
    items ||--o{ transactions : "lent_in"

    chat_threads ||--o{ messages : "contains"
    chat_threads ||--o{ transactions : "initiates"

    transactions ||--o{ ledger_entries : "audits_financials"
    transactions ||--o{ ratings : "evaluated_in"
    transactions ||--o| disputes : "contested_in"

    users {
        int id PK
        string email UK
        string prn UK
        string password_hash
        string name
        string phone
        string department
        int year
        string division
        string role
        int verified
        int blocked
        real reputation
        int created_at
    }

    sessions {
        string token_hash PK
        int user_id FK
        int expires_at
    }

    verification_tokens {
        string token_hash PK
        int user_id FK
        int expires_at
    }

    items {
        int id PK
        int owner_id FK
        string type
        string title
        string category
        string item_condition
        string location
        int deposit_paise
        int avail_from
        int avail_to
        string attrs_json
        string status
        int created_at
    }

    chat_threads {
        int id PK
        int item_id FK
        int borrower_id FK
    }

    messages {
        int id PK
        int thread_id FK
        int sender_id FK
        string body
        int created_at
    }

    transactions {
        int id PK
        int item_id FK
        int thread_id FK
        int owner_id FK
        int borrower_id FK
        int start_date
        int due_date
        int returned_at
        int deposit_paise
        string condition_report
        int claim_paise
        int penalty_paise
        int refund_paise
        string status
        int owner_confirmed
        int borrower_confirmed
        int created_at
        int updated_at
    }

    ledger_entries {
        int id PK
        int txn_id FK
        int user_id FK
        string kind
        int amount_paise
        int created_at
    }

    ratings {
        int id PK
        int txn_id FK
        int rater_id FK
        int ratee_id FK
        int stars
        string comment
    }

    notifications {
        int id PK
        int user_id FK
        string kind
        string payload_json
        int is_read
        int created_at
    }

    disputes {
        int id PK
        int txn_id FK
        int raised_by FK
        string reason
        string status
        int admin_id FK
        int owner_award_paise
        string decision_note
        int created_at
        int decided_at
    }
```

## Key Schema Constraints & Guarantees
1. **Uniqueness**:
   - `users.email` and `users.prn` are unique.
   - `chat_threads(item_id, borrower_id)` is unique (1 thread per borrower per item).
   - `ratings(txn_id, rater_id)` is unique (one rating per participant per transaction).
   - `ux_one_live_txn_per_item`: partial unique index on `transactions(item_id)` where `status IN ('ACCEPTED', 'ACTIVE', 'OVERDUE')`.
2. **Append-Only Triggers**:
   - Updates and deletions on `messages` and `ledger_entries` are blocked via SQLite triggers.
