# Scripts Implementation Principles (`scripts/`)

## Design Decisions and Operational Philosophy

### 1. Strict Error Handling
All scripts use `set -euo pipefail`:
- `-e`: Exit immediately if any command returns a non-zero exit status.
- `-u`: Treat unset variables as an error when substituting.
- `-o pipefail`: Prevent errors in a pipeline from being masked by a successful final command.

### 2. Idempotency and Safety
- `build.sh`: Can be run repeatedly; incremental CMake builds only recompile modified translation units.
- `run.sh`: Inspects database existence before passing `--init-db` or `--seed`, preventing accidental overwrites of live data during subsequent server restarts.
- `reset_db.sh`: Demands `--yes` as a protective barrier to avoid inadvertent data destruction.
- `backup.sh`: Uses SQLite's native `.backup` mechanism, safe under WAL mode, with an automated 7-snapshot rolling pruning rule.

### 3. Documentation Audit Verification
- `check_api_docs.sh` extracts route definitions from `src/api/Router.cpp` using regular expressions and matches them against `docs/api.md`. If a developer introduces a new route without updating documentation, CI will fail.
