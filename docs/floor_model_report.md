# Floor model: implementation report

Branch `floor-model`, 2026-10-02. Steps 6, 7 (two commits), 8 and 10 of `docs/floor_parametric_model.md`
section 9 on top of steps 0-5 (`e448bd7`). Trapezoids (step 9) skipped by decision 6. No kernel change.

## Baseline

Built at `0d123ce` in a clean worktree; examples 1, 2, 4, 5, 6, 8 and the tests floor_elements,
brep_drill, joint_elements and interaction_ownership run under `tools/run_guarded.sh -t 10 -m 4`, their
`data/output/pb/*.txt` dumps and consoles kept outside the repository. Before step 6 every gate of
steps 1-5 was re-run on `e448bd7` against it: G1 193 / 0 failing against compas_tf, identical to step 0
without `oculus/*` (those 0 deviation); G2 145 / 0; G3 example 2's carved column 176866956.467613
unchanged; G4 33 / 0 (wedge dowel prisms aside, by design); G5 contacts 9 / 0 and ties 9 / 0 against
compas_tf, every volume within 1e-9 relative of step 0; 44 / 44 contacts, 0 faceted, 396 / 396 bores.
One step-5 change surfaced: the wedge dowels are numbered along the contact the way compas_tf numbers
them (the step-0 cylinder `j` is now `n-1-j`, the same set).

## Steps

| step | commit | what | gates and key numbers |
|---|---|---|---|
| 6 | `249591d` | rule A (`CentralPanel`, shared rib sweep r, ruling u), per-face rib end outlines (R4), project-then-cut beds and bed planes, `CutterLevel::compas_factor`, `Floor::check()` / `FloorReport` printed by every example | G1 at 1e-6: 193 / 0 against compas_tf and equal to step 0 at every printed digit; G2 145 / 0; G3 unchanged; G4 33 / 0; G5 9 / 0 + 9 / 0, volumes within 1e-9; G6 test green; G7 44 / 44 and report ok. Square: r = u = chamfer direction, 10.704 deg oblique, 11.342 mm shear, closure 3.1e-12 mm, seats 20 / 19.296 / 20, rib bottoms 0.243 mm under -694.55 |
| 7a | `a9092e6` | `CentralLayers::section` the default, `Floor::compas_parity`, `--compas` on every floor example | parity mode identical to step 6 on every gate. G1b: only bed row 1, flanges 2b / 3a, `bed_top_planes/1`, `wedges_inner_beams/1` move, at most 2.666 mm (+t) / 5.332 mm (+2t) (bounds 2.897 / 5.794); central beds exactly 27.000000000 (compas_tf 27.003 .. 27.390); re-baselined `model_floorguide.txt`, `model_models.txt` |
| 7b | `cc760d6` | `CutterLevel::rib_bottom` the default | parity mode unchanged; middle level -694.793; the 12 cutter records move 0.243 mm at the level (0.741 at most); column contacts the full 70238.714887 mm2; head cut re-pinned 34771221.351479 (compas_tf 34777378.362 kept); example 2 carves 176873113.478; `model_contacts_cantilevers.txt`; 44 / 44, 0 faceted, 396 / 396 |
| 8 | `c3084b4` | `templates_floor_8_rectangle`, the 3000 x 2400 half spans bay; per-span compas_tf references | G8 R1-R5 below |
| 10 | `69bfed1` | example descriptions, `docs/templates.md` floor section, Implemented notes in the design | every floor example in both modes and the four tests green |
| R8 | `81969e0` | `RibLevel::shared_column` the default: each outer rib's run-in solved so both outer ribs of a corner end at the shallower compas_tf end | square and parity mode identical on every gate; 3000 x 2400: level -689.979, short run-in 187.667, eight rib bottoms within 0.307 mm, `model_rectangle.txt` re-baselined |
| R8 | (the column block commit) | the column blocks span their ribs' run-in, the middle one `middle_wedge_factor` x the mean | square and parity mode identical on every gate; 3000 x 2400: blocks 240 / 267.292 / 187.667, side far ends 0.992 mm apart, `model_rectangle.txt` re-baselined |

## Screws (branch `floor-screws`)

Pre-drilled assembly screws, 200 x d4, design section 8.1: `JointBeam::screws` (pre-drill lines on one
connector naming every member they pass, no cut), `WoodSession::pre_drill_lines`, five screw relations
appended to `relationships()`.

| | square | 3000 x 2400 |
|---|---|---|
| `screw_rib_beam` (outer rib into seam beam) | 16 | 16 |
| `screw_beam_mitre` (red, seam beam into oculus beam) | 16 | 16 |
| `screw_rib_corner` (blue, inner rib end through the beam corner) | 16 | 16 |
| `screw_ring` (ring pinwheel corners) | 8 | 8 |
| `screw_oculus` (ring toe screws into the quarters' oculus beams) | 16 | 16 |
| screws / connectors | 72 / 36 | 72 / 36 |
| screw contacts verified by the kernel's search | 36 / 36 | 36 / 36 |
| closest screw axes | 28.143 mm | 28.143 mm |
| closest screw to a dowel bore (surfaces, bore run on by its overshoot) | 137.272 mm | 137.272 mm |
| closest screw to a pocket or connector part (surfaces) | 5.441 mm | 5.441 mm |
| held by the members named | 200.000 mm, every screw | 200.000 mm, every screw |
| misfits | 0 | 0 |

No beam is too shallow (every joint 197 deep takes its two levels) and no screw needed shortening or
dropping. Reported, not a misfit: the blue screws run along the inner rib's axis through the beam
corner, so they pass three members (the seam beam's end, the oculus beam, at least 24.9 mm of them, and
the rib) and name all three. Every existing gate unchanged: example 1 byte-identical (193 / 0 against
compas_tf), examples 2, 4, 5, 6 and 8 dumps and consoles identical, example 9's records unchanged with
the screw lines added (`model_rectangle.txt` 0 failing), 44 / 44 contacts, 0 faceted, 396 / 396 and
384 / 384 bores; the four tests pass.

## The rectangle, `Floor(FloorPlan::rectangle(3000, 2400), FloorSizes{})`

| | model (default) | `--compas` (compas_tf oculus, parity definitions) |
|---|---|---|
| connectors | 48 of 48: 8 wedges with 28 dowels (3 on short seams, 5 on long, 3 per oculus beam), 8 rectangle plates, 4 cross laps, 4 ties, 24 dowel sets of 96 dowels | 48 of 48, same counts |
| ties | 4 x 19700 mm2 | 4 x 19700 mm2 |
| column plate contacts | 69995.051 / 69605.340 mm2, full end faces | 70076.481 / 70148.209 (the probes' numbers) |
| seam / ring contacts | 258500.873 / 376700.873; ring 4 x 242696.248 | 297515.590 / 328199.359; ring 4 x 253629.409 |
| contacts verified by the kernel's search | 44 / 44 | 44 / 44 |
| faceted | 0 | 0 |
| dowel bores | 384 of 384 stretches exact | 384 of 384 |
| report | ok: closure 4.3e-12, end faces 5.1e-13, beds on flanges 0, ring overlap 1.1e-11 mm2, uncovered 0 | ok |
| rule A | u 0.839 deg off the chamfer, r 20.703 / 3.338 deg oblique, 22.675 / 3.500 mm shear (r 0.474 deg off the chamfer) | u 16.059 deg, 16.957 / 41.686 deg, 18.295 / 53.432 mm |
| rib bottoms vs cutter level | 0 / 0 mm, the eight rib bottoms at a head within 0.307 mm (one rib level per column, -689.979) |
| column blocks | 240 / 267.292 / 187.667 over the run-ins, the side far ends 0.992 mm apart | 2.887 / -15.160 mm (compas_factor) |

R1: every quarter written in its corner frame against compas_tf's `FloorGuide(3000, 2400)` (quarters 0,
2) and `FloorGuide(2400, 3000)` (1, 3): 141 records, 0 failing, worst deviation 0 in all four views.
R3: every member face planar within 2.8e-11 mm (test, 1e-9 required).

## Deviations

* `CentralPanel` stores the six central traces and an informational `layer_shift` instead of the
  cross-section polylines. `central_layer_shift_vs_compas_mm` is the same-index vertex shift the
  design names; on a rectangle it is dominated by the stations sliding along the layer (40-73 mm on
  3000 x 2400), not a distance between the layers.
* A bed row whose layers fall on different facets on its two faces throws instead of being built
  (never on the square or 3000 x 2400).
* `Floor::compas_parity` and the examples' `--compas` switch are additions to the API of 6.3, so the
  compas_tf parity gates stay runnable after the defaults moved to the model's definitions.
* R1 also leaves out `oculus_points` (compas_tf lists the four corners from its own quarter 0).
* The bounding box and column positions of R2 hold by construction and are not measured separately.
* Decision 10 not needed: 0 faceted without the kernel changes of `1b42862`.
* The model's own re-baselined dumps are in `data/reference/floor/model_*.txt` (README there).

Final commit before this report: `69bfed1`.
