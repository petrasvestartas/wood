#!/usr/bin/env bash
# Run one heavy command in a machine-wide slot: builds take .buildlock or .buildlock2 (FIFO);
# `--run` commands (tests, example sweeps, dataset runs: they compile nothing) may also take
# .buildlock3, with no queue. Usage from Git bash or `bash` in PowerShell:
#   bash tools/buildslot.sh [--run] <agent-name> <command...>
# Slots: wood/build/.buildlock and .buildlock2 (mkdir is the lock). Tickets: wood/build/.buildqueue/.
# The owner file names the holder with its Windows pid so a stale lock can be told from a live one.
set -u
# `--run` marks a command that runs programs but compiles nothing (test suites, example sweeps,
# dataset runs, about 1 GB each): it may also use the third slot, which builds never take.
mode=build
if [ "${1:-}" = "--run" ]; then mode=run; shift; fi
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
    # A ticket is alive while its waiter keeps touching it; a ticket nobody has touched for two
    # minutes (a killed waiter, or one made by hand) no longer blocks the queue.
    touch "$ticket"
    # The run slot has no queue of its own: a run takes it the moment it is free.
    if [ "$mode" = run ] && mkdir "$build/.buildlock3" 2>/dev/null; then lock="$build/.buildlock3"; break; fi
    oldest="$(find "$queue" -mindepth 1 -maxdepth 1 -mmin -2 -printf '%f\n' 2>/dev/null | sort | head -n 1)"
    if [ "$(basename "$ticket")" = "$oldest" ]; then
        slots="$build/.buildlock $build/.buildlock2"
        [ "$mode" = run ] && slots="$slots $build/.buildlock3"
        for candidate in $slots; do
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
