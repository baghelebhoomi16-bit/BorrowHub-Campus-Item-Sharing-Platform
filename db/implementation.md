# Database Design Decisions and Implementation Notes (`db/`)

## Design Decisions and Rationale

### 1. Integer Money (`paise`) vs Floating Point
- **Problem**: IEEE 754 floating point arithmetic introduces binary representation errors (e.g. `0.1 + 0.2 != 0.3`). In financial systems, cumulative rounding errors break audit invariants.
- **Decision**: All financial balances, deposits, late penalties, damage deductions, and refunds are strictly stored as 64-bit signed integers in unit **paise** (`1 INR = 100 paise`).
- **Invariant**: At settlement, `deposit == refund_paise + deduction_paise`. Using integers ensures exact mathematical balance across all ledger entries.

### 2. Double-Booking Prevention: Partial Unique Index
- **Problem**: High-concurrency environments may suffer from race conditions where two simultaneous requests for the same item both pass domain availability checks and get accepted.
- **Decision**: A partial unique index is enforced in SQLite:
  ```sql
  CREATE UNIQUE INDEX ux_one_live_txn_per_item ON transactions(item_id)
    WHERE status IN ('ACCEPTED', 'ACTIVE', 'OVERDUE');
  ```
- **Benefit**: Even if an application bug or thread interleaving allows concurrent acceptance, SQLite guarantees that only one transaction can ever be in a live state for a given item. Any conflicting insert or update immediately fails with a `SQLITE_CONSTRAINT_UNIQUE` error, which maps to `AppError(CONFLICT)`.

### 3. Append-Only Ledger and Chat Messages
- **Problem**: Modifying or deleting ledger rows destroys financial auditability. Deleting chat messages destroys dispute resolution evidence.
- **Decision**: Explicit SQLite `BEFORE UPDATE` and `BEFORE DELETE` triggers abort any modification:
  ```sql
  CREATE TRIGGER messages_no_update BEFORE UPDATE ON messages BEGIN SELECT RAISE(ABORT, 'append-only'); END;
  CREATE TRIGGER ledger_no_update BEFORE UPDATE ON ledger_entries BEGIN SELECT RAISE(ABORT, 'append-only'); END;
  ```

### 4. CHECK Constraints & Data Integrity
- Enforced at the engine level:
  - `status IN ('REQUESTED', 'ACCEPTED', 'ACTIVE', 'OVERDUE', 'RETURNED', 'DISPUTED', 'SETTLED', 'REJECTED', 'CANCELLED')`
  - `type IN ('BOOK', 'ELECTRONIC', 'EQUIPMENT')`
  - `role IN ('STUDENT', 'ADMIN')`
  - `stars BETWEEN 1 AND 5`
  - `deposit_paise >= 0` and `amount_paise > 0`
  - `due_date > start_date`

### 5. Migration Policy
- Migrations are versioned numbered scripts (`V1__schema.sql`, `V2__...`).
- Once a migration is committed and applied in production/CI, it is immutable. Schema changes must be applied via forward-only additive scripts.
