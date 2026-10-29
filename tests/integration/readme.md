# Integration Test Suite (`tests/integration/`)

## Purpose
This directory contains end-to-end multi-service tests validating complete borrowing lifecycles, race condition defenses, financial conservation invariants, and API security.

## Files
- `TestApp.hpp`: Lightweight test fixture provisioning an in-memory SQLite database instance with loaded schema, pre-configured services, and student registration helpers. Boots in under 50 ms.
- `test_lifecycle.cpp`: Drives the service graph through all 10 core scenarios specified in Blueprint 10.13.
- `test_api.cpp`: Exercises REST security policies, stranger access restrictions, rate limiting, and SQL/XSS injection resilience.
- `readme.md`: Integration test overview.
- `implementation.md`: Fixture lifecycle and fuzzing design.
