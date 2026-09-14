#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$root"

clean=false
targets=()
for arg in "$@"; do
    case "$arg" in
        --clean|-c) clean=true ;;
        *) targets+=("$arg") ;;
    esac
done

if [[ ${#targets[@]} -eq 0 ]]; then
    targets=(main_dataset_runner)
fi

if [[ ! -f build/CMakeCache.txt || "$clean" == true ]]; then
    tools/run_guarded.sh -m 6 -n wood-build -- cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
fi
tools/run_guarded.sh -m 6 -n wood-build -- cmake --build build --config Release --parallel 4 --target "${targets[@]}"

for target in "${targets[@]}"; do
    executable="build/$target"
    if [[ -f "build/Release/$target.exe" ]]; then
        executable="build/Release/$target.exe"
    elif [[ -f "build/$target.exe" ]]; then
        executable="build/$target.exe"
    fi
    tools/run_guarded.sh -n wood-solver -- "$executable"
done

# ═══════════════════════════════════════════════════════════════════════════
# Usage: bash bash/cpp.sh [--clean] [target ...]
# ═══════════════════════════════════════════════════════════════════════════
