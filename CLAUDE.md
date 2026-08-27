# wood — Agent Instructions

## Running anything: use the guard

**Never launch a solver, example or dataset sweep directly.** Run it through
`tools/run_guarded.ps1`, which applies a wall-clock timeout, a kernel-enforced
memory cap, and a one-at-a-time check:

```powershell
tools/run_guarded.ps1 -FilePath build/Release/main_all_datasets.exe `
    -TimeoutMinutes 10 -MemoryLimitGB 4
```

Defaults are 10 minutes and 4 GB. Exit code 124 means the timeout killed it.

Why this exists: on 2026-08-27 three concurrent copies of
`main_wood_04_all_datasets.exe` reached 51 GB, 45 GB and 18 GB of committed
memory on a 32 GB machine. Windows logged Resource-Exhaustion-Detector (event
2004) three times and then took a dirty shutdown (Kernel-Power 41).

Three rules follow from that:

1. **Bound every run.** These algorithms are small — the whole floor_model solve
   is ~200 ms. Anything that has not finished in ten minutes is wedged, not
   slow. Kill it and investigate.
2. **Bound the memory too.** A timeout alone would not have saved that machine:
   it was thrashing long before any sensible deadline expired. The memory cap is
   the guard that protects the box, because the kernel refuses the allocation
   rather than letting it eat the page file.
3. **One run at a time.** Never start a second copy of a sweep while one is
   running. Concurrency is what turned a survivable leak into a crash, and the
   three copies also raced on each other's `data/output/` files, so their
   results were meaningless anyway.

## Prefer one dataset over all of them

`main_all_datasets` runs 44 datasets in a single process, so any per-dataset
leak accumulates and one bad dataset takes the whole sweep down with it. When
investigating, use `main_dataset_runner` for the single dataset you care about.
Reach for the full sweep only to confirm a finished change, and only guarded.

## Benchmarking

Build the comparison binary into a **separate build directory**, never over the
current one, and run the two alternately rather than in parallel — parallel runs
contend for memory and cache and give numbers that are noise.

## Verifying "output unchanged"

`WOOD_F2F_DUMP` writes `<name>.pb_coords.txt` and `<name>.pb_meta.txt` next to
each `.pb`. Diff those between two builds to prove a refactor changed nothing.
Compare against a dump taken from a **clean checkout of the baseline commit** —
a dump left behind by a half-finished or concurrent run is not a baseline.

## Trace and diagnostics flags

`WOOD_TRACE`, `WOOD_VERBOSE`, `WOOD_MERGE_DUMP`, `WOOD_APPLY_DUMP` and
`WOOD_F2F_DUMP` are all off unless set. They cost real time when on — the [GCZ]
trace alone was 7-12% of a solve — so leave them off for timing runs.
