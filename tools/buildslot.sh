#!/usr/bin/env bash
# Run one heavy command (build, test sweep, dataset run) in one of the two machine-wide build
# slots, waiting FIFO behind earlier callers. Usage from Git bash or `bash` in PowerShell:
#   bash tools/buildslot.sh <agent-name> <command...>
# Slots: wood/build/.buildlock and .buildlock2 (mkdir is the lock). Tickets: wood/build/.buildqueue/.
# The owner file names the holder with its Windows pid so a stale lock can be told from a live one.
set -u
agent="${1:?agent name}"; shift
[ $# -gt 0 ] || { echo "buildslot: command missing" >&2; exit 2; }
here="$(cd "$(dirname "$0")/.." && pwd)"
build="$here/build"
queue="$build/.buildqueue"
mkdir -p "$queue"
ticket="$queue/$(date +%s)_${agent}_$$"
: > "$ticket"
lock=""
release() {
    [ -n "$lock" ] && rm -rf "$lock"
    rm -f "$ticket"
}
trap release EXIT
trap 'exit 143' TERM
trap 'exit 130' INT
trap 'exit 129' HUP
winpid="$(cat /proc/$$/winpid 2>/dev/null || echo $$)"
waited=0
while :; do
    oldest="$(ls "$queue" 2>/dev/null | sort | head -n 1)"
    if [ "$(basename "$ticket")" = "$oldest" ]; then
        for candidate in "$build/.buildlock" "$build/.buildlock2"; do
            if mkdir "$candidate" 2>/dev/null; then
                lock="$candidate"
                break
            fi
        done
        [ -n "$lock" ] && break
    fi
    sleep 3
    waited=$((waited + 3))
    if [ $((waited % 300)) -eq 0 ]; then
        echo "buildslot: $agent waiting ${waited}s; queue: $(ls "$queue" | tr '\n' ' ')" >&2
    fi
done
rm -f "$ticket"
echo "$agent $winpid $(date +%H:%M) $*" > "$lock/owner"
echo "buildslot: $agent holds $(basename "$lock") after ${waited}s: $*" >&2
"$@"
status=$?
echo "buildslot: $agent releases $(basename "$lock"), exit $status" >&2
exit $status
