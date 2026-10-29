# Admin Command Line Tool (`src/cli/`)

## Purpose
This directory contains the console utility for campus administrators, enabling emergency account creation, user moderation, dispute review, and ledger auditing.

## Commands
- `create-admin <email>`: Prompts for a hidden password, hashes using Argon2id, and provisions a verified administrator account.
- `block <userId>`: Flags student account as blocked, immediately revoking session authorization.
- `unblock <userId>`: Reinstates borrowing permissions.
- `list-disputes`: Prints all open and decided dispute claims.
- `export-ledger <txnId>`: Generates an immutable CSV ledger report and prints the mathematical invariant check (`HOLD == REFUND + DEDUCT`).

## How to Build and Run
```bash
./build/src/borrowhub_admin create-admin admin@yourcollege.edu
./build/src/borrowhub_admin block 4
./build/src/borrowhub_admin export-ledger 1
```

## Dependencies
- `borrowhub_core` library.
