#!/usr/bin/env bash
# BorrowHub Server Runner Script
# Exports defaults, safely initializes DB if absent, starts server

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

export BH_PORT="${BH_PORT:-8080}"
export BH_DB_PATH="${BH_DB_PATH:-${PROJECT_ROOT}/data/borrowhub.sqlite}"
export BH_VERIFY_MODE="${BH_VERIFY_MODE:-console}"
export BH_SESSION_TTL_HOURS="${BH_SESSION_TTL_HOURS:-24}"
export BH_LOG_LEVEL="${BH_LOG_LEVEL:-info}"

DATA_DIR="$(dirname "${BH_DB_PATH}")"
mkdir -p "${DATA_DIR}"

SERVER_BIN="${PROJECT_ROOT}/build/src/borrowhub"
if [ ! -f "${SERVER_BIN}" ]; then
    SERVER_BIN="${PROJECT_ROOT}/build/src/Release/borrowhub.exe"
fi
if [ ! -f "${SERVER_BIN}" ]; then
    SERVER_BIN="${PROJECT_ROOT}/build/src/Release/borrowhub"
fi

if [ ! -f "${SERVER_BIN}" ]; then
    echo "Error: Server executable not found. Please run ./scripts/build.sh first."
    exit 1
fi

INIT_FLAGS=""
if [ ! -f "${BH_DB_PATH}" ]; then
    echo "Database absent at ${BH_DB_PATH}. Applying initial schema and seed..."
    INIT_FLAGS="--init-db --seed"
else
    echo "Found existing database at ${BH_DB_PATH}. Preserving data."
fi

echo "Starting BorrowHub on port ${BH_PORT} with database ${BH_DB_PATH}..."
exec "${SERVER_BIN}" ${INIT_FLAGS}
