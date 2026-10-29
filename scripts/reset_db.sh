#!/usr/bin/env bash
# BorrowHub Database Reset Script
# Refuses execution unless explicitly confirmed with --yes

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

CONFIRM="${1:-}"
if [ "${CONFIRM}" != "--yes" ]; then
    echo "Refusing to reset database without explicit confirmation."
    echo "Usage: ./scripts/reset_db.sh --yes"
    exit 1
fi

DB_PATH="${BH_DB_PATH:-${PROJECT_ROOT}/data/borrowhub.sqlite}"

echo "Resetting BorrowHub database at ${DB_PATH}..."
rm -f "${DB_PATH}" "${DB_PATH}-wal" "${DB_PATH}-shm"

mkdir -p "$(dirname "${DB_PATH}")"

echo "Applying schema..."
sqlite3 "${DB_PATH}" < "${PROJECT_ROOT}/db/schema.sql"

echo "Applying seed dataset..."
sqlite3 "${DB_PATH}" < "${PROJECT_ROOT}/db/seed.sql"

echo "Database reset and re-seeded successfully."
