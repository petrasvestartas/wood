#!/usr/bin/env bash
# Build and run every wood executable.
# Usage:
#   ./bash/cpp.sh              # Build all + run every main_wood_*
#   ./bash/cpp.sh main_wood_fold_1  # Build all + run only the listed exe
#   ./bash/cpp.sh --clean      # Force cmake reconfigure
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/lib/common.sh"

REPO_ROOT="$(dirname "$SCRIPT_DIR")"
CPP_DIR="$REPO_ROOT"
FORCE_CLEAN=false
TARGETS=()

for arg in "$@"; do
    case $arg in
        --clean|-c) FORCE_CLEAN=true ;;
        *)          TARGETS+=("$arg") ;;
    esac
done

cd "$CPP_DIR"

PLATFORM=$(detect_platform)
JOBS=$(get_jobs)

if [[ ! -d "build" ]] || [[ "$FORCE_CLEAN" == "true" ]]; then
    log_lang "cpp" "Configuring CMake..."
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -vE "^-- |^MSBuild|Completed '|Performing|No .* step|absl|abseil" || true
fi

log_lang "cpp" "Building..."
if [[ "$PLATFORM" == "windows" ]]; then
    cmake --build build --config Release --parallel "${JOBS}" 2>&1 | grep -vE "\.vcxproj ->|\.lib$|\.exe$|absl|abseil|Generating Code\.\.\.|^\s*$" | awk '!seen[$0]++' || true
    BIN_DIR="build/session_cpp/Release"
    EXE_EXT=".exe"
else
    cmake --build build --config Release -- -j"${JOBS}"
    BIN_DIR="build/session_cpp"
    EXE_EXT=""
fi

if [[ ${#TARGETS[@]} -eq 0 ]]; then
    while IFS= read -r f; do
        name=$(basename "$f" "$EXE_EXT")
        TARGETS+=("$name")
    done < <(ls "$BIN_DIR"/main_wood_*"$EXE_EXT" 2>/dev/null | sort)
fi

for t in "${TARGETS[@]}"; do
    EXE="$BIN_DIR/${t}${EXE_EXT}"
    if [[ ! -x "$EXE" && ! -f "$EXE" ]]; then
        log_lang "cpp" "Skipping ${t}: ${EXE} not found"
        continue
    fi
    log_lang "cpp" "Running ${t}..."
    if ! "$EXE"; then
        log_lang "cpp" "${t} failed (exit=$?), continuing"
    fi
done
