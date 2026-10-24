# Plan, 10 October 2026 (evening): beam joints, drill clearance, real use cases

Joint-library review, organised by beam joints, drill clearance and examples.

## 1. Joints that belong to beams

The user: ss_e_ip_3, ss_e_ip_4, ts_e_p_4 and the cross joints cr_c_ip_1 to 5 belong to the beam.

- Cross joints on rectangular and round beams include a 3-cut half-lap, an extended conical 6-cut tile and a compact 9-cut tile. Side cuts widen the slot for reciprocal rotation while retaining a bottom contact. Examples cover crossings at 90, 60 and 45 degrees and crossings on bent members.
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
- Drill layout: screws inset half a plate thickness along the contour, 20 mm dowels near the centre.
- To do: every drill centre at least two radii inside the contact, one radius of wood between hole and edge; tt_e_p_2 to 5 clamp
  their inset or scale to that. Check `top_to_top_pairs` against the 2025 reference and say exactly which drills move.

## 3. Real use cases on the page

- ss_e_r_0 on two plates folded about their shared edge (the rotated fixture at 120 degrees), not flat.
- The parameter sweeps (divisions, shift) shown at several angles, each on a pair of plates taken from a template (the folding chevron
  and diamond mesh, the shells), so a reader sees the parameter where it is used.
- The snap fit ts_e_p_5 on the page (its tile and the top_to_side_snap_fit pair).
- A Vidy four-plate node as an element example: two wall plates and two roof plates of one vidy_full node, the main ss_e_op_6 joint and
  its link.
- Custom joints: tt_e_p_custom and the other `*_custom` examples redone with a readable user tile (a unit box drawn by hand, its male
  and female polylines named).7 and 5.32 C.

## 4. Pictures with the viewer's layers

The user: element screenshots with joints must show the viewer's layer panel. Under investigation: whether the native renderer can
draw the panel offscreen, or headless Chromium can run the web viewer on this GPU-less machine.

## 5. Joinery coverage

`docs/plans/thesis_chapter5_coverage.md` tracks coverage: rectangular beam half-laps, conic beam cross joints, skewed and bent beams, fasteners and butterfly keys.

## Progress (10 October, night)

- Done: drill clearance (C7 proves it); the conic cross joints and the wedge cut beams with exact bores (`joint_beams` at 90, 60, 45
  degrees); the layer-panel screenshots (`tools/shoot_ui.sh`, `tools/shoot_elements_ui.sh`); the Vidy node example; the snap fit, the folded
  ss_e_r pair and the beams on the page; custom tiles that cut: ss_e_ip_custom (three jigsaw tabs), ts_e_p_custom (dovetailed tenons, the
  base's mortises cut as holes), ss_e_op_custom (fingers at the user's stations, an open profile merged as the library's fingers are).
- Open:
  - tt_e_p_custom: giving its tile a cut (a closed pair a milled pocket, a two-point pair a drill) fails the oracle's rigid-motion check, the
    tile landing differently once the pair is moved: the top-top joint volumes take the frame of the contact's minimum-area rectangle, whose
    first corner and winding are not fixed under a motion. The frame has to be made rigid first.
  - The tenon designs ts_e_p_2 and 3 on a square beam tee of two equal beams cut the female beam in two: the tenons are as wide as the male
    box, which is as wide as the female, so each mortise runs through the female's whole width, and `solid_difference` keeps the larger
    piece (575 of 1200 mm, the 14e6 mm3 "lost"). Measured: the cut is a clean solid that shares only its two mortises (0.75e6) with the
    beam; stock less cut is the larger half. The tenon should fit inside a rectangular beam; on beams the male
    box should be narrowed before the plate design runs (a width inset of the box, or the joint volume extension), so the mortise stays
    inside the female. To decide: the inset (a share of the width, or a fixed wood thickness each side).
  - ss_e_ip_3 and 4 on beams: a side-by-side beam contact that picks the in-plane family is not built yet.
  - ss_e_r_custom and cr_c_ip_custom cut 2024's rectangle notches; their examples keep them.
