#!/usr/bin/env bash
# BorrowHub Safe Backup Script
# Performs live SQLite hot backup via WAL safe .backup and keeps rolling window of 7 backups

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

DB_PATH="${BH_DB_PATH:-${PROJECT_ROOT}/data/borrowhub.sqlite}"
DATA_DIR="$(dirname "${DB_PATH}")"

if [ ! -f "${DB_PATH}" ]; then
    echo "Error: Database file does not exist at ${DB_PATH}"
    exit 1
fi

TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
BACKUP_FILE="${DATA_DIR}/backup-${TIMESTAMP}.sqlite"

echo "Creating live backup: ${BACKUP_FILE}..."
sqlite3 "${DB_PATH}" ".backup '${BACKUP_FILE}'"

echo "Verifying backup integrity..."
INTEGRITY="$(sqlite3 "${BACKUP_FILE}" "PRAGMA integrity_check;")"
if [ "${INTEGRITY}" != "ok" ]; then
    echo "Error: Backup integrity check failed: ${INTEGRITY}"
    rm -f "${BACKUP_FILE}"
    exit 1
fi
echo "Integrity check passed (ok)."

# Enforce rolling retention policy: keep 7 most recent backups
echo "Enforcing rolling retention (latest 7 backups)..."
BACKUPS=($(ls -t "${DATA_DIR}"/backup-*.sqlite 2>/dev/null || true))
COUNT=${#BACKUPS[@]}
if [ ${COUNT} -gt 7 ]; then
    for ((i=7; i<COUNT; i++)); do
        echo "Removing obsolete backup: ${BACKUPS[i]}"
        rm -f "${BACKUPS[i]}"
    done
fi
echo "Backup procedure completed successfully."
