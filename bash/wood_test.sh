#!/usr/bin/env bash
# wood_test.sh — Build wood.exe, run tests, serve/build the Vue viewer.
#
# Usage:
#   ./bash/wood_test.sh              # build + run + start dev server
#   ./bash/wood_test.sh --no-web     # build + run only (no dev server)
#   ./bash/wood_test.sh --kill       # stop dev server (port 8770)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
WOOD_TEST_DIR="${REPO_ROOT}/cmake/wood_test"
BUILD_DIR="${REPO_ROOT}/cmake/build"

START_WEB=true
KILL_SERVER=false

for arg in "$@"; do
    case $arg in
        --no-web) START_WEB=false ;;
        --kill|-k) KILL_SERVER=true ;;
    esac
done

log() { echo "[wood_test] $*"; }

# ─── Kill mode ───────────────────────────────────────────────────────────────
if [[ "$KILL_SERVER" == "true" ]]; then
    log "Stopping dev server on port 8770..."
    # Windows (MINGW / Git Bash)
    netstat -ano 2>/dev/null | grep ":8770" | awk '{print $5}' | xargs -r taskkill //F //PID 2>/dev/null || true
    # Linux / macOS
    pkill -f "vite.*8770" 2>/dev/null || true
    exit 0
fi

# ─── 1. Build cmake ──────────────────────────────────────────────────────────
log "=== Building wood.exe ==="
cmake --build "${BUILD_DIR}" --config Release --parallel 8 || true

# Locate exe (MSVC puts it in Release/, Linux puts it directly in build/)
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
    log "ERROR: could not find wood executable in ${BUILD_DIR}"
    exit 1
fi
log "Found exe: ${EXE}"

# ─── 2. Run exe from cmake/build/ ────────────────────────────────────────────
# Run from cmake/build/ so current_path().parent_path() == cmake/
# → testData.js is written to cmake/wood_test/public/testData.js
log "=== Running wood tests ==="
(cd "${BUILD_DIR}" && "${EXE}")
log "testData.js written to: ${WOOD_TEST_DIR}/public/testData.js"

# ─── 3. Copy tree-sitter WASM files if not already present ───────────────────
WASM_DST="${WOOD_TEST_DIR}/public"
mkdir -p "${WASM_DST}"

# Only install npm deps if node_modules doesn't exist yet
if [[ ! -d "${WOOD_TEST_DIR}/node_modules" ]]; then
    log "=== Installing npm dependencies ==="
    (cd "${WOOD_TEST_DIR}" && npm install)
fi

WASM_SRC="${WOOD_TEST_DIR}/node_modules/tree-sitter-wasms/out"
if [[ -d "$WASM_SRC" ]]; then
    log "Copying tree-sitter WASM files..."
    for wasm in tree-sitter-cpp.wasm; do
        [[ -f "${WASM_SRC}/${wasm}" ]] && cp "${WASM_SRC}/${wasm}" "${WASM_DST}/" || true
    done
fi
TS_WASM="${WOOD_TEST_DIR}/node_modules/web-tree-sitter/tree-sitter.wasm"
[[ -f "$TS_WASM" ]] && cp "$TS_WASM" "${WASM_DST}/" || true

# ─── 4. CI build or local dev server ─────────────────────────────────────────
if [[ -n "${CI:-}" || -n "${GITHUB_ACTIONS:-}" ]]; then
    log "=== CI: building Vue dist ==="
    (cd "${WOOD_TEST_DIR}" && npm run build)
    log "Done"
    exit 0
fi

if [[ "$START_WEB" == "true" ]]; then
    log "=== Starting Vue dev server (http://localhost:8770/wood_test/#/tests) ==="
    (cd "${WOOD_TEST_DIR}" && npm run dev) &
    log "Re-run without --no-web to regenerate test data (browser auto-reloads via Vite HMR)"
fi

log "Done"
