#!/usr/bin/env bash
# Run a command under a wall-clock timeout, a hard memory cap and a one-at-a-time lock.
# Linux/macOS counterpart of run_guarded.ps1 - same three guards, same defaults, same exit codes.
#
#   tools/run_guarded.sh [-t MINUTES] [-m GB] [-n NAME] [--allow-concurrent] -- <command> [args...]
#
#   -t  wall-clock limit, default 10. Anything that has not finished by then is wedged, not slow.
#   -m  memory cap in GB, default 4. Kernel-enforced: the process is refused memory past it.
#   -n  identity for the one-at-a-time check; defaults to the command plus its arguments, so two
#       different examples may run together but two copies of the same sweep may not.
#
# Exit codes: 124 timeout, 137 killed by the memory cap, 75 another copy is already running,
# otherwise the command's own.
#
# The cap uses a transient systemd scope (MemoryMax) when systemd-run is available - that is the
# cgroup limit and the only one that protects the machine - and falls back to ulimit -v, which
# bounds address space rather than resident memory and is therefore stricter for mmap-heavy
# programs (raise -m if a run dies at startup under the fallback).
set -u

minutes=10; gb=4; name=""; allow=0
while [ $# -gt 0 ]; do
    case "$1" in
        -t) minutes=$2; shift 2 ;;
        -m) gb=$2; shift 2 ;;
        -n) name=$2; shift 2 ;;
        --allow-concurrent) allow=1; shift ;;
        --) shift; break ;;
        -*) echo "run_guarded: unknown option $1" >&2; exit 2 ;;
        *) break ;;
    esac
done
[ $# -gt 0 ] || { echo "run_guarded: no command given" >&2; exit 2; }

[ -n "$name" ] || name="$*"
lock="${TMPDIR:-/tmp}/run_guarded-$(printf '%s' "$name" | cksum | cut -d' ' -f1).lock"

exec 9>"$lock"
if [ "$allow" -eq 0 ] && ! flock -n 9; then
    echo "run_guarded: '$name' is already running (lock $lock). Refusing to start a second copy -" \
         "concurrent runs are what exhausted memory on 2026-08-27. Wait for it, or pass --allow-concurrent." >&2
    exit 75
fi

secs=$(awk -v m="$minutes" 'BEGIN { printf "%d", m * 60 }')
if command -v systemd-run >/dev/null 2>&1 && systemd-run --user --scope -q true >/dev/null 2>&1; then
    timeout -k 10 "$secs" systemd-run --user --scope -q -p "MemoryMax=${gb}G" -p MemorySwapMax=0 "$@"
else
    kb=$(awk -v g="$gb" 'BEGIN { printf "%d", g * 1024 * 1024 }')
    ( ulimit -v "$kb" && exec timeout -k 10 "$secs" "$@" )
fi
rc=$?
[ "$rc" -eq 124 ] && echo "run_guarded: killed after $minutes minutes - investigate, do not rerun with a longer limit" >&2
[ "$rc" -eq 137 ] && echo "run_guarded: killed by the ${gb} GB memory cap" >&2
exit "$rc"
