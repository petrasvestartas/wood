# Plan, 10 October 2026 (evening): beam joints, drill clearance, real use cases

The user's review of the joint library page, gathered and worked in this order. Sources: the review, thesis chapter 5 (figures and pages
below) and the code. The NGon 3.0.0 thread on discourse.mcneel.com is blocked by the session's network policy, so nothing here comes
from it.

## 1. Joints that belong to beams

The user: ss_e_ip_3, ss_e_ip_4, ts_e_p_4 and the cross joints cr_c_ip_1 to 5 belong to the beam.

- Thesis: Fig 5.51 row C, 5.55 and 5.56 (p.102 to 105) show the cross joints on rectangular and round beams: a 3-cut half-lap, the
  extended conical 6-cut tile and the compact 9-cut tile. The side cuts widen the slot so the other member can come in at an angle
  (reciprocal rotation, Fig 5.53, 5.60) and still sit on a bottom contact. Crossings are drawn at 90 degrees, skewed (Fig 5.57 C, about
  45 to 60 degrees), at 60 degrees in hexagonal grids (Fig 5.53, 5.62 A) and on bent members (Fig 5.58, 5.59).
- Code today: `JointBeam::from_contact` turns two beams into box plates, solves them as plates and cuts each beam with the merged
  outlines only (`compute_joinery`). The conic cr_c_ip_1 to 5 are solid designs (`side_solids`), so on beams they are dropped; only
  cr_c_ip_0 (the 3-cut half-lap, phanomema_node) and the ts_e_p tenons take effect.
- To do:
  - carry the solid outlines (mill, mill_project, conic, drills) of a plate design onto the beams in `compute_joinery`, so cr_c_ip_1 to 5,
    ss_e_ip_3/4 and ts_e_p_4 cut beams as solids;
  - a `tests/joint_beams.cpp` case per design: two rectangular beams crossing at 90, 60 and 45 degrees, each losing the design's volume;
  - the joint library page shows these designs on two crossing beams (90 and 60 degrees), their unit box beside, the plate pictures gone.

## 2. Top-to-top drills keep wood around each hole

The user: in-plane pin connections need an offset of at least a pin radius, otherwise they eat too much material.

- Today tt_e_p_3 and tt_e_p_4 inset the contact by `shift` in millimetres (12 on the page, with 8 mm holes: 4 mm of wood left);
  tt_e_p_2 and tt_e_p_5 scale a circle or rectangle by 0.95, holes run to the edge.
- Thesis p.98 (Nabucco): screws inset half a plate thickness along the contour, 20 mm dowels near the centre.
- To do: every drill centre at least two radii inside the contact, one radius of wood between hole and edge; tt_e_p_2 to 5 clamp
  their inset or scale to that. Check `top_to_top_pairs` against the 2025 reference and say exactly which drills move.

## 3. Real use cases on the page

- ss_e_r_0 on two plates folded about their shared edge (the rotated fixture at 120 degrees), not flat.
- The parameter sweeps (divisions, shift) shown at several angles, each on a pair of plates taken from a template (the folding chevron
  and diamond mesh, the shells), so a reader sees the parameter where it is used.
- The snap fit ts_e_p_5 on the page (its tile and the top_to_side_snap_fit pair). Thesis: text only (p.86, p.91), no figure.
- A Vidy four-plate node as an element example: two wall plates and two roof plates of one vidy_full node, the main ss_e_op_6 joint and
  its link. Vidy is beyond chapter 5 (not in the thesis).
- Custom joints: tt_e_p_custom and the other `*_custom` examples redone with a readable user tile (a unit box drawn by hand, its male
  and female polylines named), following Fig 5.7 and 5.32 C.

## 4. Pictures with the viewer's layers

The user: element screenshots with joints must show the viewer's layer panel. Under investigation: whether the native renderer can
draw the panel offscreen, or headless Chromium can run the web viewer on this GPU-less machine.

## 5. Thesis coverage

`docs/plans/thesis_chapter5_coverage.md` corrected: Vidy is not in chapter 5; the snap fit is text only; the rectangular beam half-lap is
covered while the conic beam cross joints, skewed and bent beams and fasteners through a beam cross are not; the butterfly key on beams
(Fig 5.37 A) is missing; `JointBeam::tie` covers part of the short-end butterfly key; the top-to-top spacing of p.98.
