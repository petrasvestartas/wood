# Kernel planar boolean profile

Plan step 8, "planar faces against planar faces take a direct polyhedral path". Measured
2026-11-06 on session_cpp branch `planar-boolean` (commit on top of `dad16e63`), Windows,
MSVC Release.

## How it was measured

- The wood cases ran with exact booleans only (the `WOOD_EXACT_ONLY` switch from the brep-only
  worktree), and every `brep_boolean::apply` wrote its two operands to disk. The tables replay
  exactly those booleans in a small driver, so wood's own time is not in them.
- Phases are the four stages of `brep_boolean::apply`: section (`brep_section::interferences`),
  partition (both operands' face splits), select (classification of every split face) and
  assemble (`brep_assemble::assemble`).
- Before is `origin/main` at `dad16e63`; after is the branch. Both were built in the same tree
  with the same uncommitted phase timer, run alternately three times, best total kept. The machine
  was shared with other agents' builds; single runs varied by up to 2x.
- `floor_elements` did not finish in 10 minutes with exact booleans; the 453 booleans it ran in
  that time are the floor case.

## The three cases

| case | booleans | what they are |
|---|---|---|
| `two_plates` | 0 | plate joints are outline edits, no solid boolean runs |
| `joint_beams` | 8 | a 6-face beam less a 14 to 31 face planar joint cutter; 7 of 8 throw in both kernels |
| `floor_elements` | 453 | 35 planar against planar, 418 with a drill cylinder |

## Before (origin/main)

| case | total ms | section | partition | select | assemble |
|---|---|---|---|---|---|
| joint_beams | 3881 | 2334 (60 %) | 864 (22 %) | 392 (10 %) | 291 (8 %) |
| floor, 35 planar booleans | 13558 | 6692 (49 %) | 5160 (38 %) | 597 (4 %) | 1109 (8 %) |
| floor, all 453 booleans | 317463 | 115322 (36 %) | 101074 (32 %) | 8533 (3 %) | 92535 (29 %) |

## After (planar-boolean)

| case | total ms | section | partition | select | assemble | speed-up |
|---|---|---|---|---|---|---|
| joint_beams | 800 | 419 (52 %) | 258 (32 %) | 30 (4 %) | 93 (12 %) | 4.9x |
| floor, 35 planar booleans | 1564 | 579 (37 %) | 597 (38 %) | 94 (6 %) | 294 (19 %) | 8.7x |
| floor, all 453 booleans | 280074 | 111456 (40 %) | 75570 (27 %) | 7783 (3 %) | 85264 (30 %) | 1.13x |

The all-floor rows are single runs.

## Where the time was

Scoped timers inside the phases, on the planar floor booleans before any change:

- Curve-curve intersection: 134 000 calls, about 65 % of all time, spread over every phase.
  Most were pairs of two-point lines, each running closest-point searches, interval
  certificates and Newton refinement.
- Boundary events matched to section curves: the same edge against the same curve was
  intersected once per event on the edge (69 % of the section phase).
- Face splits: each split re-validated every edge of the whole solid (5.4 s of 13.8 s), and
  every `std::move` of a `BRep` was a deep copy, because `BRep` declared a copy constructor
  and no move.
- Whole-domain coincidence tests copied the surface for eight parameter variants per edge.
- The root search of the section ran on flat patches that do not meet, to find nothing.
- Point classification shot NURBS rays at flat faces.

## What changed

- `BRep` move construction and assignment.
- Curve intersection: control hulls apart return at once; results are cached by the exact
  curve bits; endpoint projections are computed once per pair; two-point lines seed their
  nearest points in closed form.
- Section: one intersection per edge and section curve in the boundary-node pass.
- Face splits validate only the edges added since the previous split.
- Coincidence tests reject surfaces whose corners lie apart.
- Bilinear patches that a plane holds apart skip the root search.
- Polyhedral solids are classified by plane heights and polygon parity; points near the
  boundary still go to the general classification.

## Results

- All 453 floor booleans and all 8 joint-beam booleans: same status, same face, edge and curve
  counts, volumes equal to 12 digits.
- Coordinates move by at most 9.1e-13 mm (one or two units in the last place at 4000 mm). The
  closed-form line seeds land Newton on a neighbouring rounding-level root; every other change
  returns bit-identical results, which was checked call by call during development.
- session_cpp minitests on the rebased commit: 1554 of 1555 pass. The one failure is
  `FileObj::Read Bunny`, whose data file is not in a standalone session_cpp worktree. The rhino
  exact-volume gate (`Rhino ... Original Oracle`, 1e-7) and every fixture test pass unchanged.
- Five new minitests in `BRep Planar Boolean`: two boxes, box minus rotated box, prism through
  plate, coplanar faces, touching faces, against analytic volumes or the suite's polygon
  clipping oracle.

## Not reached

- Joint beams are 4.9x, not 10x. Seven of the eight throw in partition or assembly in both
  kernels, so their time is mostly section work before the failure.
- The floor is 1.13x. Its time is in the drill booleans (a cylinder against planar faces),
  which this work does not target. That is the case to speed up next for the floor and the
  vault.
