#!/usr/bin/env bash
# BorrowHub API Documentation Verification Script
# Verifies that every route defined in src/api/Router.cpp is documented in docs/api.md

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

ROUTER_FILE="${PROJECT_ROOT}/src/api/Router.cpp"
DOCS_FILE="${PROJECT_ROOT}/docs/api.md"

if [ ! -f "${ROUTER_FILE}" ]; then
    echo "Error: Router file missing at ${ROUTER_FILE}"
    exit 1
fi
if [ ! -f "${DOCS_FILE}" ]; then
    echo "Error: Docs file missing at ${DOCS_FILE}"
    exit 1
fi

echo "Verifying API documentation coverage..."
MISSING_COUNT=0

# Extract route paths registered via server.Get/Post/Put/Delete/etc.
# Matches patterns like: server.Get("/path", ...) or server.Post("/path", ...)
ROUTES=$(grep -E 'server\.(Get|Post|Put|Delete)\("' "${ROUTER_FILE}" | sed -E 's/.*server\.(Get|Post|Put|Delete)\("([^"]+)".*/\2/' | sort -u)

while IFS= read -r route; do
    [ -z "$route" ] && continue
    # Normalize regex for path parameters like /api/items/{id} or /api/items/:id
    BASE_PATTERN=$(echo "$route" | sed -E 's/\{[^}]+\}/.*/g' | sed -E 's/:[a-zA-Z0-9_]+/.*/g')
    if ! grep -E -q "${BASE_PATTERN}" "${DOCS_FILE}"; then
        echo "FAIL: Route '${route}' is registered in Router.cpp but missing in docs/api.md"
        MISSING_COUNT=$((MISSING_COUNT + 1))
    fi
done <<< "${ROUTES}"

if [ ${MISSING_COUNT} -gt 0 ]; then
    echo "Validation FAILED: ${MISSING_COUNT} route(s) undocumented."
    exit 1
fi

echo "SUCCESS: All API routes registered in Router.cpp are documented in docs/api.md."
