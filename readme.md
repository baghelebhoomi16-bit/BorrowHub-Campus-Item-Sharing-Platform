# BorrowHub: Campus Item-Sharing Platform

> **Version**: 1.0 (29 Oct 2026)  
> **Tech Stack**: C++17, SQLite 3 (WAL), REST API (`cpp-httplib`), Vanilla Web Chat & SPA.

---

## 1. Project Pitch
Temporary item sharing on campus is untracked (informal WhatsApp messages), unsecured (no deposit, no accountability), and undiscoverable (nobody knows who owns a graphing calculator or robotics kit).

**BorrowHub** is a campus-scoped platform where verified students list idle items and other students borrow them for fixed periods. Each transaction passes through a private per-item negotiation chat, simulated security deposit hold, tracked state machine lifecycle, rule-based settlement, and mutual ratings.

---

## 2. Feature Roadmap (MUST / SHOULD / COULD)

| ID | Requirement | Priority | Status |
| :--- | :--- | :---: | :---: |
| **FR-01** | Register with college email, PRN, department, year, division. Email & PRN unique. | **MUST** | Done |
| **FR-02** | Login & logout with expiring session tokens (`HttpOnly`, `SameSite=Strict`). | **MUST** | Done |
| **FR-03** | Owner creates, edits, removes items with type-specific attributes and condition. | **MUST** | Done |
| **FR-04** | Search & filter by text, category, location, condition, max deposit, min rating. | **MUST** | Done |
| **FR-05** | One chat thread per (item, borrower). Messages append-only with server timestamps. | **MUST** | Done |
| **FR-06** | Borrow request from thread with start and end dates. Type max days enforced. | **MUST** | Done |
| **FR-07** | Owner accepts or rejects. Accepting reserves item and auto-rejects other requests. | **MUST** | Done |
| **FR-08** | Borrower pays simulated deposit recorded as immutable ledger `HOLD` entry. | **MUST** | Done |
| **FR-09** | Handover confirmed by both parties; status becomes `ACTIVE`. | **MUST** | Done |
| **FR-10** | Owner confirms return with condition report (`GOOD`, `DAMAGE`, `LOST`). | **MUST** | Done |
| **FR-11** | Deterministic settlement: refund, damage deduction, late penalty capped at deposit. | **MUST** | Done |
| **FR-12** | Overdue detection background worker job (`Scheduler`). | **SHOULD** | Done |
| **FR-13** | Mutual ratings after settlement with Bayesian reputation updates. | **SHOULD** | Done |
| **FR-14** | In-app notification feed on each state change. | **SHOULD** | Done |
| **FR-15** | Dispute raised within 48h freezes funds; decided by campus administrator. | **SHOULD** | Done |
| **FR-16** | Admin console: block users, moderate listings, view disputes, audit ledger. | **SHOULD** | Done |
| **FR-17** | Dashboard counts (active listings, active borrowings, overdue items). | **COULD** | Done |
| **FR-18** | Email notifications, condition photos, QR handover. | **COULD** | Roadmap |

---

## 3. System Architecture
```
Browser (web/: HTML + JS)          Console Admin Tool (src/cli)
            |  HTTP / JSON                       |
            v                                    v
+-------------------------------------------------------------+
| API layer (src/api): routing, auth middleware, DTO <-> JSON |
+-------------------------------------------------------------+
| Service layer (src/services): use cases, DB transactions,   |
| authorization, policies, scheduler, event publisher         |
+-------------------------------------------------------------+
| Domain layer (src/domain): User, Item hierarchy, Money,     |
| BorrowTransaction state machine, Policies. No I/O.          |
+-------------------------------------------------------------+
| Repository layer (src/db): interfaces + SQLite impl         |
+-------------------------------------------------------------+
| SQLite (WAL mode), schema in db/schema.sql                  |
+-------------------------------------------------------------+
```

### State Machine Lifecycle
```
REQUESTED --reject--> REJECTED
REQUESTED --accept--> ACCEPTED --24h no deposit / cancel--> CANCELLED
ACCEPTED --deposit paid + both confirm handover--> ACTIVE
ACTIVE --now > due--> OVERDUE
ACTIVE | OVERDUE --owner return report--> RETURNED
RETURNED --48h timeout / borrower accepts--> SETTLED
RETURNED --borrower disputes--> DISPUTED --admin decision--> SETTLED
```

---

## 4. Folder Map
```
borrowhub/
├── CMakeLists.txt              # Root build configuration
├── vcpkg.json                  # Manifest pinning external C++ dependencies
├── .clang-format               # C++17 formatting rules
├── .clang-tidy                 # Static code analysis configuration
├── .gitignore                  # Git exclusions for builds and databases
├── readme.md                   # Project pitch, roadmap, execution guide
├── implementation.md           # Engineering policies and release checklist
├── docs/                       # Specifications (api.md, er-diagram.md, dispute-policy.md)
├── db/                         # DDL schema (schema.sql) and seed data (seed.sql)
├── scripts/                    # Shell scripts (build, run, backup, reset_db, check_api_docs)
├── src/                        # C++17 source tree
│   ├── util/                   # Clock, Config, Errors, Token, Validation
│   ├── domain/                 # Money, User, Item hierarchy, Policies, State Machine
│   ├── db/                     # SQLite RAII wrapper, Transactions, Repositories
│   ├── services/               # Use cases, EventBus, Scheduler
│   ├── api/                    # Server, Router, AuthMiddleware, RateLimiter, JsonMapper
│   └── cli/                    # Campus admin command line tool (borrowhub_admin)
├── web/                        # Responsive single-page web app and polling chat
└── tests/                      # Automated test pyramid
    ├── unit/                   # Fast domain and service tests
    └── integration/            # Full lifecycle and API security tests
```

---

## 5. Execution Guide

### Prerequisites
- CMake 3.25+
- C++17 compliant compiler (GCC 13+, Clang 16+, or MSVC 2022)
- Git & vcpkg package manager
- Modern web browser

### Step-by-Step Build and Run
1. **Clone Repository**:
   ```bash
   git clone <repo-url> borrowhub && cd borrowhub
   ```
2. **Setup vcpkg**:
   ```bash
   git clone https://github.com/microsoft/vcpkg.git vcpkg
   ./vcpkg/bootstrap-vcpkg.sh  # Windows: .\vcpkg\bootstrap-vcpkg.bat
   export VCPKG_ROOT=$PWD/vcpkg # Windows PowerShell: $env:VCPKG_ROOT="$PWD\vcpkg"
   ```
3. **Configure & Build**:
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
   cmake --build build -j
   ```
4. **Execute Tests**:
   ```bash
   ctest --test-dir build --output-on-failure
   ```
5. **Initialize Database & Seed Data**:
   ```bash
   ./build/src/borrowhub --init-db --seed
   ```
6. **Start Server**:
   ```bash
   BH_PORT=8080 BH_DB_PATH=data/borrowhub.sqlite BH_VERIFY_MODE=console ./build/src/borrowhub
   ```
7. **Open Web Application**:
   Navigate to `http://localhost:8080` in two separate browser windows (or one normal and one incognito window) to test live item sharing and chat.

### Environment Variables
| Variable | Default | Purpose |
| :--- | :--- | :--- |
| `BH_PORT` | `8080` | Port for the HTTP web server |
| `BH_DB_PATH` | `data/borrowhub.sqlite` | Filepath to SQLite database |
| `BH_VERIFY_MODE` | `console` | `console` prints verify links to stdout; `auto` auto-verifies |
| `BH_SESSION_TTL_HOURS` | `24` | Session cookie validity window |
| `BH_LOG_LEVEL` | `info` | Logging verbosity |
