# Joinery library: cloud hand-off (state of 2026-10-10)

This file is the complete brief for a cloud session continuing the joint-library work on branch
`joinery-library` of `petrasvestartas/wood`. Read it to the end before touching code. Everything
here was established by audits, fixes and reviews run on 2026-10-09/10 (Fable 5.1, ~60 agents).

---

## 0. How to work (binding)

- Read `CLAUDE.md` (repo root) first: house style, **no lambdas**, explicit types, one concept per
  file, short functions, section banners, kernel first (session_cpp headers before any helper),
  the key pattern: a joint is an element computed from the contact, `add(joint, group)`, then
  `add_interaction(joint, member, joint->interaction(i))` per target. One header/source pair for
  Joint, JointPlate, JointBeam; per-joint builders live in
  `src/joinery_solver/wood_interaction/wood_interaction_feature/wood_interaction_feature_plate_joints/*.h`.
- Setup:
  ```
  git fetch origin && git checkout joinery-library && git submodule update --init --recursive
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --parallel 8 --target joint_library reference_datasets
  ```
- Work on a branch per task (`cloud/<task>`) or directly on `joinery-library` if you run the tasks
  one after another; never on `main`. Commit messages: `wood: <what>` + short body.
  **No `Co-Authored-By`, no `Claude-Session`, no AI attribution of any kind** (strip what the
  harness adds). Stage named paths only (`git add <paths>`), never `git add -A`.
- Push the branch when a task is green. Do not merge to `main`; the user merges after looking at
  the pictures locally.
- **No screenshots in the cloud** (the viewer needs the user's GPU). Write examples and docs pages
  with image references `docs/images/elements/<example>.png`, and list in your final report every
  example target whose picture must be shot locally with
  `bash wood/tools/screenshot_element_docs.sh <example>`.
- After every task run both oracles and report the summary lines before and after:
  - `./build/joint_library` -> `joint library: N / M variants pass, K skipped` (exit 1 while any fails)
  - `./build/reference_datasets` -> `reference 2025: N / 46 datasets match, 0 throw`
    (`tests/golden/reference_2025/matched.txt` is a ratchet: a listed dataset may never regress).
  A review counts **any oracle failure, dataset regression, outline leaving its plate, hollow or
  self-crossing merged loop, or faceted pin** as a blocker, not a "remaining" note.

## 1. Reference material

- **2024 library** (the truth for every joint): `git show 351933c:cmake/src/wood/include/<file>`:
  `wood_joint_lib.cpp/.h` (all joints, `construct_joint_by_index` id table at ~6300-6560),
  `wood_joint.cpp/.h` (joint struct, `m_boolean_type`/`f_boolean_type`, `unit_scale`, `shift`,
  `divisions`, `linked_joints`), `wood_main.cpp` (detection: face_to_face, plane_to_face,
  border_to_face), `wood_element.cpp` (`merge_joints` ~740-990: pair [0] merged, marker [1] of 2
  points = edge insertion, 5 points = rectangle; holes ~1362-1410), `wood_cut.h`, `cgal_*.cpp`.
- **2025 reference solver** (same CGAL code, runnable): `pip install wood_nano==0.3.5
  compas-wood==2.4.0 compas==2.8.1` (Python 3.12, manylinux wheels). `tools/reference_2025.py`
  dumps every dataset into `tests/golden/reference_2025/<name>.json` (already committed, 46
  datasets; `cross_brg_slab_0` segfaults the reference solver and has no reference).
  Call pattern: `get_connection_zones(pairs, vecs, types, three, adj, joint_parameters_and_types,
  search_type, [1,1,1], output_type, [0,0,0], [], [])`; output_type 4 = merged outlines,
  3 = joints, 2 = volumes.
- **Audit data** in `docs/plans/joinery_audit/`: per family `<family>.json` (every 2024 function
  vs its port: status, params, fabrication types, differences with old/new line refs, insertion
  direction, adversarially verified findings), `mechanism.json` (merge/cut semantics + bugs),
  `beams.json` (JointBeam bugs + fix plan), `oracle.json` (oracle design C1-C12, kernel gaps),
  `round2_notes.md` (open items collected during the fixes). Line numbers refer to the code at
  audit time (branch start `f9d9d2f`); re-locate before editing.
- **User scope** (from the compas_wood Rhino plugin presentation, Feb 2025): joint id ranges
  1-9 ss_e_ip (side-side in-plane), 10-19 ss_e_op (out-of-plane, 15 = ss_e_op_5 for Annen/Vidy),
  20-29 ts_e_p (top-to-side), 30-39 cr_c_ip (cross), 40-49 tt_e_p (top-to-top drills),
  50-59 ss_e_r (rotated), 60-69 b (boundary); x8 = side_removal, x9 = custom, each with a family
  default. Templates in the plugin: cross connectors (butterfly plates on a hex dome), Annen
  (surface -> quad mesh -> two-layer box plates), Vidy chapel (folded panels, linked joints),
  folded plates, round-log beams (cross saddle + pin, top-to-side tenon + pin, in-line scarf +
  pins, parallel beams joined by a plate with pins). All 44 plate datasets + `phanomema_node`
  are in `data/` and must run through the new solver.

## 2. What is done on `joinery-library` (commits since `main`)

| Commit | What |
|---|---|
| f9d9d2f | oracle: `tests/joint_library.cpp` (fixtures, checks C1-C4, C6, C7, C10, C11 goldens), `tools/reference_2025.py`, `tests/reference_datasets.cpp`, CI wiring |
| 1ba22e3 | mechanism: plate joints land on the solver's male (ss_e_op male/female swap fixed), merge by fabrication type, mills and drills become solid cuts |
| 79cd6c6, 0109a52, 7629bb5 | ss_e_ip on 2024 defaults; key only where male/female runs lie on opposite sides; docs per design |
| a348321, fb252f4 | ss_e_op on 2024 defaults, ss_e_op_6, 2024 id table; docs per design |
| 7b36e05, 3e17594, ea6fb22, 989a58f | ss_e_r as 2024 built it, side_removal_ss_e_r_1 merge branch, side removal on the right plate, ss_e_r_1 = tile only; docs |
| 66fbf23 | face contacts clipped on the 2024 grid: every out-of-plane and Annen dataset matches the reference (systematic 0.001-0.007 mm offset was detection, not joints) |
| 6bcb114 | merge drops what it stitches to nothing: ss_e_op_3..6, 17 whole on the right-angle pair |
| 4955235 | custom pairs merge as 2024 did (2-point line or 5-point rectangle marker) |
| 8c86db0, 8e8c465 | ts_e_p as 2024 built it, ts_e_p_4 ported (240 points, mill types), unit scale; docs |
| e3879e1, f1c30e4, 3f369ab, 2e2d2db | cr_c_ip as 2024 built it, slot clipped by Clipper2 at two decimals, clip frame as 2024; docs |
| fe50dbd, 8d0956d, a0aaa63 | tt_e_p drills as 2024 assigned them, offset ring as Clipper inset it, plane-only swap in detection |

Numbers on `2e2d2db` (clean build): oracle **56 / 79 variants pass**, reference **33 / 46 datasets
match**. Re-measure on `a0aaa63` first thing (tt_e_p commits came after).

Branch `joinery-library-wip` (272b80d) holds work set aside, **not reviewed**: tt_e_p per-design
examples and pictures a docs step had begun (no page yet), an earlier agent's uncommitted
`key_mesh`/`compute_key` change for ss_e_r keys in src (HEAD builds without it), and the Hilti
reference photo `docs/images/hilti_reference_photo.jpg`. Take from it what you need.

## 3. Remaining tasks, in order

### T1. tt_e_p documentation
Per design `tt_e_p_0..5`, `tt_e_p_custom`: a minimal example
`examples/elements/element_joint_plate_tt_e_p_<n>.cpp` (one pair, pulled apart along the plate
normal; start from the WIP branch's files), `ADD_EXE`, the page
`docs/elements/element_joint_plate_tt_e_p.md` in the structure of
`docs/elements/element_joint_plate_ss_e_op.md` (anchor, `[TOC]`, family sentence, `## Constructors`
with real signatures, one `##` per design with picture refs and `\include{lineno}`), row and
`@subpage` in `docs/elements.md`, pointer in `docs/elements/element_joint_plate.md`. Remove the
old row example `element_joint_plate_tt_e_p.cpp` and its PNG.

### T2. Solid cuts must reach the written plate geometry (top blocker)
The docs steps saw mill / slice / drill cuts of ss_e_ip_3/4, ts_e_p_4, tt_e_p_*, cr_c_ip_1..5
**drawn as feature outlines on an uncut plate mesh** in the viewer. The oracle's model mesh may be
cut while `pb_dumps` / `compute_breps` / `Plate::model_geometry_brep` export the uncut one. Make the
exported mesh and BRep the cut solid (exact BRep cylinders for drills), prove it with a test that
reads the `.pb` back, and list the examples to reshoot.

### T3. Hilti connector (new, from the user's photo)
Photo: `git show origin/joinery-library-wip:docs/images/hilti_reference_photo.jpg`. CLT slabs
(200 mm) joined along a straight seam, flat (0 degrees) or folded up to 50 degrees. Parts:
(a) two **identical** half-dovetail blocks of birch plywood (trapezoid wing widening away from the
seam, rectangular neck toward it, plywood layers along the neck), one sunk into each slab across
the seam, square to its own slab's seam face, at mid-thickness; (b) one threaded steel rod, a
`Pin`, through both necks across the seam; (c) on the outer end of each half a round steel disc
(centre hole, four screws) taking the nut; (d) in each slab a pocket receiving its half plus an
obround access slot milled from the **top face**, as long as the half. **The parts never change
with the angle** (rigid frames, never sheared or scaled by a joint volume's change of basis);
only pockets and slots follow seam and angle. Build with the connector-of-parts pattern
(`JointBeam::wedge`, `ConnectorPart`, `Pin`, the floor template's `add_interaction`); a factory
named `hilti` with real parameters (half length, wing width, neck width, rod and disc diameter,
slot width, depth; defaults from the photo for 200 mm CLT); computed from the face contact at any
fold angle 0..50; parts as children; `interaction(i)` cuts slab i (pocket + slot as solid
subtraction, rod bore as drill); colour `JointBeam::CONNECTOR_COLOR`. Check whether
`data/inplane_hilti.yml` (today id 3 = ss_e_ip_2) should use it. Tests: fixtures at
0/10/20/30/40/50 degrees; every part's volume and edge lengths identical at all angles; parts
inside their slab and not overlapping the cut slab; each half fills its pocket; the rod passes both
halves and bores. Examples `element_joint_hilti.cpp` (one pair at 30 degrees pulled apart) and
`element_joint_hilti_angles.cpp` (six angles side by side like the photo); page
`docs/elements/element_joint_hilti.md`.

### T4. Plate joints at various angles
ss_e_op and ss_e_r at dihedral 90/120/150; ts_e_p skewed in plan (60, 75) and leaning (80);
cr_c_ip crossing at 60 and 45; ss_e_ip with a trapezoid pair and a seam shorter than the plates.
Angle fixtures in `tests/joint_library.cpp` running every variant with C1-C6 + C11; fix builders /
merge / detection, never weaken a check. **Rigid keys**: the loose keys ss_e_ip_2, ss_e_ip_5,
ss_e_r_2, ss_e_r_3 keep identical volume and edge lengths at every angle (add the check, fix the
placement in `joint_orient_to_connection_area` / unit_scale path). Examples
`element_joint_plate_<family>_angle_<deg>.cpp` with the clearest design per family (ss_e_op_3,
ss_e_r_2, ts_e_p_3, cr_c_ip_2, ss_e_ip_1) and an `## Angles` section on each family page.

### T5. Boundary family and beam joints
From `docs/plans/joinery_audit/b.json` and `beams.json`: (1) family 60 reachable as in 2024: a
boundary joint on a plate side face from a self-adjacency (`border_to_face`), scale semantics,
b_0 / b_custom with 2024 outlines; (2) a beam-to-beam joint must **cut** its members
(`InteractionFeatureBeam` volumes become each target's cut; `from_contact` builds it,
`add_joint_interaction` applies it), the 2024 beam slice b_0 on JointBeam, `headed_pins`
pre-drilled holes exist in mesh and BRep, `rectangle_plate` pins truly flush, crossing normal and
axes stored on `InteractionFeatureBeam` (+ proto) and exposed as `JointBeam::insertion(target)`;
(3) port the 2024 beam dataset `phanomema_node` and compare with the reference; example
`element_joint_beam_b_0.cpp`. Beam doc pictures must show the members pulled apart along the
insertion direction so the cut reads.

### T6. Annen and Vidy
WoodSession API as in the plugin: `assign_joint_types_by_points(points, types, snap_radius)`
(each point snaps to the closest side edge of the closest plate outline and sets that face's joint
type, `Plate::feature_types`) and `assign_insertion_vectors_by_lines(lines, snap_radius)` (line
start snaps to an edge, its direction becomes that face's insertion vector,
`Plate::insertion_vectors`). Kernel closest-point routines only. Sidecars `*_joints_types.txt` /
`*_insertion_vectors.txt` keep working through the same two functions. Example
`element_plate_assign_joints.cpp` on the annen_box pair, a test. Then Vidy (vidy_corner,
vidy_one_layer, vidy_one_axis_two_layers, vidy_full: three- and four-valence linked joints,
`three_valence` + `adjacency`, ss_e_op_5 with linked joints, `wood_three_valence.h`,
`merge_linked_joints`) and Annen (annen_box, annen_box_pair, annen_corner, annen_grid_small,
annen_grid_full_arch) must match the reference.

### T7. Open items from the reviews (`round2_notes.md`)
- ss_e_op at non-default parameters still fails C4/C6 (self-crossing loops at some divisions/shift).
- ss_e_ip_3/4: C6 key check; mill_project pockets must be real pockets (T2).
- cr_c_ip_2..5: C3 was loosened to 1e-2 of a member: tighten back once T2 is right; the
  cr_c_ip_2 picture showed feature lines far outside the plates: verify the ring extension
  (absolute 0.15 / 0.6, cr_c_ip_5 asymmetric +0.27 / -0.075) numerically against the reference;
  cr_c_ip_5 drill_50 / drill_10 must bore.
- cross_vda_single_arch plate 37: 14/15 vs 17 points (drop_folded_corners vs reference).
- Kernel gap: `Polyline::extend_segment_equally(int)` never re-syncs a closed ring's closing
  vertex: fix it in session_cpp, session_py and session_rust together (the kernel repo
  petrasvestartas/session) and drop the wood workaround; other kernel gaps in `oracle.json`
  (`Polyline::is_planar`, `is_simple`, a point-set distance, tolerant `is_closed`).
- ss_e_op_17 and ss_e_op_tutorial have no 2024 counterpart: keep by name, out of the id table,
  until the user decides.
- `tools/render_element_docs.py` still names removed examples; delete the PIL renderer (pictures
  come from the real viewer now).
- `tests/golden/reference_2025` is 15 MB of json: consider gzip.
- `Plate::features.top/bottom` naming is inverted against `polylines[0]/[1]`: settle it.

### T8. Final sweep
Build all targets, run `joint_library`, `reference_datasets`, `joint_elements`, `brep_drill`,
`interaction_ownership`, `floor_elements`, `shells_gridshell`, every `examples/elements/element_*`
binary; the CI workflow `.github/workflows/wood-test.yml` must pass (it runs on `main` only; run
its steps by hand). Then a completeness pass against the 2024 id table: every id of every family,
x8, x9, defaults; all datasets; Annen points/lines; Vidy linking; beams cut; families 1-69
reachable. Report the scoreboard and every open item.

## 4. Pictures the user's machine shoots afterwards

Every new or changed `examples/elements/element_*` target (list them in your report), plus the
reshoots already known: ts_e_p_4 (features off, close-up on the pockets), cr_c_ip_2 (features off,
close-up per slot), side_removal_ss_e_r_1 (close-up on the bevel), ss_e_r_2 (keys hidden).
Rules: real viewer with its layers panel, one pair per picture, members pulled apart along the
insertion direction, pins exact cylinders, several pictures per joint when it helps; clarity first.
