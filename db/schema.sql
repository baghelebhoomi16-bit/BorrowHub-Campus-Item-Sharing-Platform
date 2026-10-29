-- BorrowHub SQLite Database Schema
-- Version: 1.0 (29 Oct 2026)
-- Conforms strictly to Master Technical Blueprint Section 10.3

PRAGMA foreign_keys = ON;

-- Users table
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    email TEXT UNIQUE NOT NULL,
    prn TEXT UNIQUE,
    password_hash TEXT NOT NULL,
    name TEXT NOT NULL,
    phone TEXT,
    department TEXT,
    year INTEGER CHECK(year IS NULL OR year BETWEEN 1 AND 4),
    division TEXT,
    role TEXT NOT NULL CHECK(role IN ('STUDENT', 'ADMIN')),
    verified INTEGER NOT NULL DEFAULT 0,
    blocked INTEGER NOT NULL DEFAULT 0,
    reputation REAL NOT NULL DEFAULT 50.0,
    created_at INTEGER NOT NULL
);

-- Sessions table
CREATE TABLE IF NOT EXISTS sessions (
    token_hash TEXT PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    expires_at INTEGER NOT NULL
);

-- Email Verification Tokens
CREATE TABLE IF NOT EXISTS verification_tokens (
    token_hash TEXT PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    expires_at INTEGER NOT NULL
);

-- Items table
CREATE TABLE IF NOT EXISTS items (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    owner_id INTEGER NOT NULL REFERENCES users(id),
    type TEXT NOT NULL CHECK(type IN ('BOOK', 'ELECTRONIC', 'EQUIPMENT')),
    title TEXT NOT NULL,
    category TEXT NOT NULL,
    item_condition TEXT NOT NULL,
    location TEXT NOT NULL,
    deposit_paise INTEGER NOT NULL CHECK(deposit_paise >= 0),
    avail_from INTEGER,
    avail_to INTEGER,
    attrs_json TEXT NOT NULL DEFAULT '{}',
    status TEXT NOT NULL CHECK(status IN ('AVAILABLE', 'RESERVED', 'BORROWED', 'UNAVAILABLE', 'REMOVED')),
    created_at INTEGER NOT NULL
);

-- Chat Threads: exactly one thread per (item, borrower)
CREATE TABLE IF NOT EXISTS chat_threads (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    item_id INTEGER NOT NULL REFERENCES items(id),
    borrower_id INTEGER NOT NULL REFERENCES users(id),
    UNIQUE(item_id, borrower_id)
);

-- Messages: append-only conversation log
CREATE TABLE IF NOT EXISTS messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    thread_id INTEGER NOT NULL REFERENCES chat_threads(id),
    sender_id INTEGER NOT NULL REFERENCES users(id),
    body TEXT NOT NULL CHECK(length(body) BETWEEN 1 AND 2000),
    created_at INTEGER NOT NULL
);

-- Borrow Transactions: tracks full item lifecycle
CREATE TABLE IF NOT EXISTS transactions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    item_id INTEGER NOT NULL REFERENCES items(id),
    thread_id INTEGER REFERENCES chat_threads(id),
    owner_id INTEGER NOT NULL REFERENCES users(id),
    borrower_id INTEGER NOT NULL REFERENCES users(id),
    start_date INTEGER NOT NULL,
    due_date INTEGER NOT NULL,
    returned_at INTEGER,
    deposit_paise INTEGER NOT NULL,
    condition_report TEXT,
    claim_paise INTEGER NOT NULL DEFAULT 0,
    penalty_paise INTEGER NOT NULL DEFAULT 0,
    refund_paise INTEGER,
    status TEXT NOT NULL CHECK(status IN ('REQUESTED', 'ACCEPTED', 'ACTIVE', 'OVERDUE', 'RETURNED', 'DISPUTED', 'SETTLED', 'REJECTED', 'CANCELLED')),
    owner_confirmed INTEGER NOT NULL DEFAULT 0,
    borrower_confirmed INTEGER NOT NULL DEFAULT 0,
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL,
    CHECK(due_date > start_date)
);

-- Partial unique index: prevents double-booking at database level
CREATE UNIQUE INDEX IF NOT EXISTS ux_one_live_txn_per_item ON transactions(item_id)
    WHERE status IN ('ACCEPTED', 'ACTIVE', 'OVERDUE');

-- Ledger entries: financial audit trail
CREATE TABLE IF NOT EXISTS ledger_entries (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    txn_id INTEGER NOT NULL REFERENCES transactions(id),
    user_id INTEGER NOT NULL REFERENCES users(id),
    kind TEXT NOT NULL CHECK(kind IN ('HOLD', 'REFUND', 'DEDUCT', 'PAYOUT')),
    amount_paise INTEGER NOT NULL CHECK(amount_paise > 0),
    created_at INTEGER NOT NULL
);

-- Ratings: mutual reviews post settlement
CREATE TABLE IF NOT EXISTS ratings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    txn_id INTEGER NOT NULL REFERENCES transactions(id),
    rater_id INTEGER NOT NULL REFERENCES users(id),
    ratee_id INTEGER NOT NULL REFERENCES users(id),
    stars INTEGER NOT NULL CHECK(stars BETWEEN 1 AND 5),
    comment TEXT,
    UNIQUE(txn_id, rater_id)
);

-- In-app notifications
CREATE TABLE IF NOT EXISTS notifications (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL REFERENCES users(id),
    kind TEXT NOT NULL,
    payload_json TEXT NOT NULL,
    is_read INTEGER NOT NULL DEFAULT 0,
    created_at INTEGER NOT NULL
);

-- Disputes
CREATE TABLE IF NOT EXISTS disputes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    txn_id INTEGER NOT NULL UNIQUE REFERENCES transactions(id),
    raised_by INTEGER NOT NULL REFERENCES users(id),
    reason TEXT NOT NULL,
    status TEXT NOT NULL CHECK(status IN ('OPEN', 'DECIDED')),
    admin_id INTEGER REFERENCES users(id),
    owner_award_paise INTEGER,
    decision_note TEXT,
    created_at INTEGER NOT NULL,
    decided_at INTEGER
);

-- Triggers for Append-Only Guarantees
CREATE TRIGGER IF NOT EXISTS messages_no_update BEFORE UPDATE ON messages
BEGIN
    SELECT RAISE(ABORT, 'append-only');
END;

CREATE TRIGGER IF NOT EXISTS messages_no_delete BEFORE DELETE ON messages
BEGIN
    SELECT RAISE(ABORT, 'append-only');
END;

CREATE TRIGGER IF NOT EXISTS ledger_no_update BEFORE UPDATE ON ledger_entries
BEGIN
    SELECT RAISE(ABORT, 'append-only');
END;

CREATE TRIGGER IF NOT EXISTS ledger_no_delete BEFORE DELETE ON ledger_entries
BEGIN
    SELECT RAISE(ABORT, 'append-only');
END;

-- Query performance indexes
CREATE INDEX IF NOT EXISTS idx_items_status_category ON items(status, category);
CREATE INDEX IF NOT EXISTS idx_messages_thread_id ON messages(thread_id, id);
CREATE INDEX IF NOT EXISTS idx_transactions_borrower_id ON transactions(borrower_id);
CREATE INDEX IF NOT EXISTS idx_transactions_owner_id ON transactions(owner_id);
CREATE INDEX IF NOT EXISTS idx_notifications_user_id ON notifications(user_id, is_read);
