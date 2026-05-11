#!/usr/bin/env bash
# run_windows.sh — Build wood.exe and run it (writes testData.js + session.pb).
# For the full test viewer pipeline use:  ./bash/wood_test.sh
set -e
echo "run_windows.sh start!"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/cmake/build"

# Build
cmake --build "${BUILD_DIR}" --config Release --parallel 8 || true

# Locate exe (MSVC Release/ subfolder on Windows, directly in build/ on Linux)
EXE=""
for candidate in \
    "${BUILD_DIR}/Release/wood.exe" \
    "${BUILD_DIR}/wood.exe" \
    "${BUILD_DIR}/wood"; do
    if [[ -f "$candidate" ]]; then
        EXE="$candidate"
        break
    fi
done

if [[ -z "$EXE" ]]; then
    echo "ERROR: wood executable not found in ${BUILD_DIR}"
    exit 1
fi

# Run from cmake/build/ (current_path().parent_path() == cmake/)
echo "Running: ${EXE}"
(cd "${BUILD_DIR}" && "${EXE}")

echo "run_windows.sh end!"
