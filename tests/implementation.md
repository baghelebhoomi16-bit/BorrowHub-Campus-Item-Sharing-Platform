# Testing Architecture & Standards (`tests/`)

## Design Principles

### 1. Test Pyramid
- **Base (Unit Tests)**: High volume of tests verifying domain invariants, state machine transitions, monetary overflows, Bayesian reputation math, and input validation without touching the database or network sockets.
- **Apex (Integration Tests)**: Multi-entity lifecycles driving complete transaction sequences from borrow request to settlement and mutual rating.

### 2. Zero Sleep Rule
- In accordance with Blueprint Section 7 & 10.13, **no test is ever allowed to call `sleep()`**.
- Real-time delays introduce non-determinism, slow CI down, and make tests flaky.
- All temporal behavior is governed by `util::FakeClock`, which is advanced deterministically and instantaneously.

### 3. Financial Invariant Verification
- A dedicated fuzzing test in `test_lifecycle.cpp` executes 1,000 randomly generated borrowing sequences (random deposits, late durations, and damage claims), verifying that in 100% of cases:
  $$\text{HOLD} = \text{REFUND} + \text{DEDUCT}$$
