# Unit Test Suite (`tests/unit/`)

## Purpose
This directory contains fast, in-memory unit tests validating pure domain invariants, policies, cryptographic routines, state machines, and service workflows.

## Test Files
- `test_money.cpp`: Precision, negative value protections, and overflow detection in `Money`.
- `test_user.cpp`: Permissions, role checks, and student eligibility formulas (`canRequest`).
- `test_item.cpp`: Category polymorphism, type attributes, and `ItemFactory` construction.
- `test_penalty_policy.cpp`: Ceiling day division, deposit bounds, and zero-day calculations.
- `test_transaction.cpp`: Table-driven `BorrowTransaction` state machine transitions and actor guards.
- `test_reputation.cpp`: Bayesian prior math (prior mean 3.5, weight 5) and Laplace smoothed rates.
- `test_auth_service.cpp`: Registration, constant-time verification error handling, and session expiration.
- `test_borrow_service.cpp`: Pre-negotiation chat requirement, polymorphic duration boundaries, and auto-rejection.
- `test_settlement_service.cpp`: Financial settlement deductions, late penalties, and ledger invariant audits.
- `test_scheduler.cpp`: Background ticks, overdue marking, 24h deposit cancellation, and 48h auto-settlement.

## How to Run
```bash
ctest --test-dir build -R UnitTests --output-on-failure
```
