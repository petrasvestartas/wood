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

## Progress (10 October, night)

- Done: drill clearance (C7 proves it); the conic cross joints and the wedge cut beams with exact bores (`joint_beams` at 90, 60, 45
  degrees); the layer-panel screenshots (`tools/shoot_ui.sh`, `tools/shoot_elements_ui.sh`); the Vidy node example; the snap fit, the folded
  ss_e_r pair and the beams on the page; custom tiles that cut: ss_e_ip_custom (three jigsaw tabs), ts_e_p_custom (dovetailed tenons, the
  base's mortises cut as holes), ss_e_op_custom (fingers at the user's stations, an open profile merged as the library's fingers are).
- Open:
  - tt_e_p_custom: giving its tile a cut (a closed pair a milled pocket, a two-point pair a drill) fails the oracle's rigid-motion check, the
    tile landing differently once the pair is moved: the top-top joint volumes take the frame of the contact's minimum-area rectangle, whose
    first corner and winding are not fixed under a motion. The frame has to be made rigid first.
  - The tenon designs ts_e_p_2 and 3 on a square beam tee take most of a beam (14.6e6 mm3 for an overlap of 1.7e6), and at 60 degrees leave
    1.2e6 of the overlap; as before the solids were carried onto beams. Measured: the male beam loses 0.56e6, right; the female beam loses
    14.06e6 of its 27e6, though its cut (the zone less its box, with the two mortises, 4.69e6) holds only 0.75e6 of it. The cut is a
    manifold solid of the right volume (its mortises too), the beam carries no cut planes, only that one solid feature: the loss appears
    when the cut is hosted on the female beam. Next: the frame `host_solid_feature` moves the cut through (joint to target), on a tee.
  - ss_e_ip_3 and 4 on beams: a side-by-side beam contact that picks the in-plane family is not built yet.
  - ss_e_r_custom and cr_c_ip_custom cut 2024's rectangle notches; their examples keep them.
