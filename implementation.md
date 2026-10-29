# Engineering Standards & Implementation Guide (`BorrowHub`)

## 1. Dependency Management Policy
- All external third-party C++ libraries are pinned and declared strictly through `vcpkg.json` (vcpkg manifest mode).
- Git submodules or unversioned vendor drops are prohibited to ensure 100% reproducible builds across developer workstations and CI runners.

## 2. Coding Standards
- **Standard**: Modern C++17 (`-std=c++17`, with extensions disabled).
- **Style Enforcement**:
  - Formatting is enforced via `.clang-format` based on Google C++ Style with 4-space indentation.
  - Static analysis is enforced via `.clang-tidy`.
- **Memory Safety**:
  - Zero raw `new` and `delete` in business logic; all resource allocation uses smart pointers (`std::unique_ptr`, `std::make_unique`) or RAII containers.
  - SQLite statements and database handles are finalized and closed in destructors.
- **Error Handling**:
  - Exceptions derived from `util::AppError` communicate domain, validation, and authorization violations.
  - Services open immediate transactions (`BEGIN IMMEDIATE`) and automatically rollback upon any uncaught exception.

## 3. Branching & Commit Conventions
- **Trunk-Based Development**: Main branch is `main`.
- **Branch Naming**:
  - `feat/<feature-name>` (e.g. `feat/dispute-settlement`)
  - `fix/<bug-name>` (e.g. `fix/penalty-cap-overflow`)
  - `test/<test-name>` (e.g. `test/ledger-fuzzing`)
- **Conventional Commits**:
  - Format: `<type>(<scope>): <subject>`
  - Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`.

## 4. Continuous Integration (CI) Pipeline
Configured via `.github/workflows/ci.yml`:
1. **Matrix Build**: Linux (`ubuntu-latest`) and Windows (`windows-latest`) Release targets.
2. **Automated Testing**: Runs `ctest` with `--output-on-failure`.
3. **Sanitizers**: Compiles with AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) on Linux.
4. **Linting**: Runs `clang-tidy` and checks API documentation synchronization via `scripts/check_api_docs.sh`.

## 5. Release Checklist
Prior to cutting a release:
- [ ] `scripts/check_api_docs.sh` passes with zero undocumented routes.
- [ ] `ctest` passes 100% of unit and integration test cases.
- [ ] Sanitizers report zero memory leaks or undefined behavior.
- [ ] Live backup integrity verified (`scripts/backup.sh`).
- [ ] Web frontend verified for XSS immunity and responsiveness at 360px viewport.
