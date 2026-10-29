# Test Suite Root (`tests/`)

## Purpose
This directory houses the comprehensive automated test suite for BorrowHub, structured as a clean testing pyramid with unit tests isolating domain rules and integration tests driving complete end-to-end borrowing workflows.

## Directory Structure
- `unit/`: Fast, isolated tests for domain entities, policies, mathematical formulas, and service business logic.
- `integration/`: Multi-entity workflow tests using in-memory SQLite fixtures (`TestApp`) and API security tests.
- `TestCommon.hpp`: Cross-platform testing macros supporting Catch2 v3 and standalone execution.

## How to Run Tests

### 1. Run All Tests via CTest
```bash
ctest --test-dir build --output-on-failure
```

### 2. Run Only Unit Tests
```bash
./build/tests/borrowhub_unit_tests
```

### 3. Run Only Integration Tests
```bash
./build/tests/borrowhub_integration_tests
```

### 4. Run With Sanitizers Enabled
```bash
cmake -B build-san -S . -DCMAKE_BUILD_TYPE=Debug -DBH_SANITIZE=ON
cmake --build build-san -j
ctest --test-dir build-san --output-on-failure
```
