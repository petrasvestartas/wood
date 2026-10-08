#!/usr/bin/env bash
# Build the wood docs site (Doxygen) and serve it on localhost, ending with the URL to open.
#
#   .claude/hooks/build_docs.sh              build, serve on http://127.0.0.1:8000 (reused if already up), print the URLs
#   .claude/hooks/build_docs.sh --clean      delete build/docs first, so removed pages go too
#   .claude/hooks/build_docs.sh --watch      build, serve, and rebuild on every change under docs/, src/, examples/; Ctrl+C stops
#   .claude/hooks/build_docs.sh --no-serve   build only
#   .claude/hooks/build_docs.sh --port 8080  another port
#
# As a Claude Code hook (UserPromptSubmit, wired in .claude/settings.json with --hook) the prompt
# `builddocs` builds and serves, `builddocs clean` cleans first; every other prompt passes through
# untouched and the output goes to Claude as context. --watch blocks, so it is for a terminal only.
#
# Doxygen is a static generator: a changed .md shows only after a rebuild, about a second, no C++
# compiled. With --watch, save the file and refresh the browser.
set -euo pipefail

WOOD="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="$WOOD/build"
HTML="$BUILD/docs/html"
LOG="$BUILD/docs/doxygen.log"

if [ "${1:-}" = "--hook" ]; then
    prompt=$(python3 -c 'import json,sys; print(json.load(sys.stdin).get("prompt","").strip())' 2>/dev/null || true)
    case "$prompt" in
        builddocs) args=() ;;
        "builddocs clean") args=(--clean) ;;
        *) exit 0 ;;
    esac
    echo "builddocs: running wood/.claude/hooks/build_docs.sh ${args[*]:-}"
    set +e; bash "$0" "${args[@]}" 2>&1; rc=$?
    echo "builddocs: exited $rc - report the result above to the user, do nothing else."
    exit 0
fi

CLEAN=0
SERVE=1
WATCH=0
PORT=8000

while [ $# -gt 0 ]; do
    case "$1" in
        --clean) CLEAN=1; shift ;;
        --watch) WATCH=1; shift ;;
        --no-serve) SERVE=0; shift ;;
        --port) PORT="${2:?--port needs a number}"; shift 2 ;;
        -h|--help) sed -n '2,16p' "$0"; exit 0 ;;
        *) echo "unknown argument: $1 (see --help)" >&2; exit 2 ;;
    esac
done

# cmake from uv where installed, never the snap (see wood_research/CLAUDE.md)
CMAKE=/usr/bin/cmake
[ -x "$HOME/.local/bin/cmake" ] && CMAKE="$HOME/.local/bin/cmake"

configure() {
    command -v doxygen > /dev/null || { echo "doxygen not found: sudo apt install doxygen" >&2; exit 1; }
    echo "configuring $BUILD"
    "$CMAKE" -S "$WOOD" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release > /dev/null
}

# Runs Doxygen; the full output goes to doxygen.log, only the warnings about wood's own files are shown.
build() {
    mkdir -p "$BUILD/docs"

    if ! "$CMAKE" --build "$BUILD" --target docs > "$LOG" 2>&1; then
        # the docs target exists only when cmake found doxygen at configure time
        if grep -q "No rule to make target 'docs'\|unknown target 'docs'" "$LOG"; then
            configure
            "$CMAKE" --build "$BUILD" --target docs > "$LOG" 2>&1 || { tail -20 "$LOG"; echo "docs build failed, full log: $LOG" >&2; return 1; }
        else
            tail -20 "$LOG"; echo "docs build failed, full log: $LOG" >&2; return 1
        fi
    fi

    local warnings
    warnings=$(grep -E "^$WOOD/(docs|src)/.*warning:" "$LOG" | sed "s|$WOOD/||g" || true)

    if [ -n "$warnings" ]; then
        echo "$warnings"
        echo "docs built with $(wc -l <<<"$warnings") warnings (above; full log $LOG)"
    else
        echo "docs built, no warnings"
    fi
}

# Whether the server on a port serves this build: its index.html is ours.
serves_this_build() {
    curl -sf "http://127.0.0.1:$1/index.html" 2>/dev/null | cmp -s - "$HTML/index.html"
}

port_taken() {
    curl -s -o /dev/null "http://127.0.0.1:$1/" 2>/dev/null
}

SERVER=""

serve() {
    local port="$PORT"

    while port_taken "$port" && ! serves_this_build "$port"; do
        port=$((port + 1))
    done

    if ! port_taken "$port"; then
        setsid python3 -m http.server "$port" --bind 127.0.0.1 -d "$HTML" > /dev/null 2>&1 < /dev/null &
        SERVER=$!
        sleep 0.5
    fi

    PORT="$port"
}

urls() {
    echo
    echo "  Open in the browser:"
    if [ "$SERVE" = 1 ]; then
        echo "    start page  http://127.0.0.1:$PORT/index.html"
        echo "    floor       http://127.0.0.1:$PORT/templates_floor.html"
        echo "    elements    http://127.0.0.1:$PORT/elements.html"
        [ -n "$SERVER" ] && echo "  (server pid $SERVER, stop it with: kill $SERVER)"
    else
        echo "    file://$HTML/index.html"
    fi
    echo
}

[ -f "$BUILD/CMakeCache.txt" ] || configure

if [ "$CLEAN" = 1 ]; then
    rm -rf "$BUILD/docs"
    echo "removed $BUILD/docs"
fi

build
[ "$SERVE" = 1 ] || [ "$WATCH" = 1 ] && { SERVE=1; serve; }
urls

if [ "$WATCH" = 1 ]; then
    trap '[ -n "$SERVER" ] && kill "$SERVER" 2>/dev/null; exit 0' INT TERM
    stamp=$(mktemp)
    echo "watching docs/, src/ and examples/: save a file, then refresh the browser (Ctrl+C stops)"

    while sleep 1; do
        changed=$(find "$WOOD/docs" "$WOOD/src" "$WOOD/examples" -newer "$stamp" -type f \
            \( -name '*.md' -o -name '*.h' -o -name '*.cpp' -o -name '*.png' -o -name '*.webp' -o -name '*.svg' \) -print -quit)

        if [ -n "$changed" ]; then
            touch "$stamp"
            echo "-- ${changed#"$WOOD"/} changed, rebuilding"
            build || true
        fi
    done
fi
