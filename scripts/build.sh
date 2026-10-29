#!/usr/bin/env bash
# BorrowHub Build Script
# Idempotent, exits on first error, builds release executable

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "=== BorrowHub Build Pipeline ==="
echo "Project Root: ${PROJECT_ROOT}"

BUILD_DIR="${PROJECT_ROOT}/build"
VCPKG_ROOT="${VCPKG_ROOT:-${PROJECT_ROOT}/vcpkg}"

TOOLCHAIN_ARG=""
if [ -f "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" ]; then
    TOOLCHAIN_ARG="-DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    echo "Using vcpkg toolchain: ${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
else
    echo "Warning: vcpkg not found at ${VCPKG_ROOT}. Proceeding with system libraries."
fi

echo "Configuring CMake..."
cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    ${TOOLCHAIN_ARG}

echo "Building targets..."
cmake --build "${BUILD_DIR}" --config Release -j

echo "Build completed successfully."
