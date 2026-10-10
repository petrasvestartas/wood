# Joinery Solver (thesis chapter 5) against WoodSession

How each concept and example of chapter 5 (Vestartas 2021, *Joinery Solver*, p. 63 to 114) maps onto this port, and what is not covered.

## Data structures

| Thesis | WoodSession | Status |
| --- | --- | --- |
| Element: a minimal model (planar outlines plus an axis), a reference mesh and a collection of cuts (5.3, Fig. 5.4, 5.5) | `Plate` (top and bottom outlines), `Beam` and `BeamVariable` (axis, section, planes), `Column`, `Block`; cuts are interactions on the target (`InteractionFeaturePlate`, `InteractionFeatureSolid`, `InteractionFeaturePlane`), applied in `model_geometry_mesh` and `model_geometry_brep` | covered for plates and rectangular or variable-section beams. **No reference mesh**: raw wood (irregular trunks, Fig. 5.6, 5.21, 5.42) has no element type |
| Element grouping, the sorted list with keys `x;y0;y1` (Fig. 5.6) | the session tree: groups by place and family (`group_named`, `get_branch`), read back by name | covered, as a tree instead of a sorted key list |
| Joint = undirected graph + tiles (Fig. 5.8) | the session graph: `add_interaction(source, target, interaction)` puts each contact and joint on an edge; `JointPlate` and `JointBeam` are elements; adjacency sidecars `*_adjacency.txt` | covered |
| Tile = male + female cuts in a unit box (Fig. 5.7) | one design function per joint (`wood_interaction_feature_plate_joints/*.h`) filling `male_outlines` and `female_outlines` of an `InteractionFeaturePlate` in the unit box | covered; drawn in `docs/joint_library.md` |
| Tile transformation: plane to plane, change of basis from two rectangles, translation (Fig. 5.9 to 5.11) | `joint_orient_to_connection_area` (change of basis onto `joint_volumes`), `apply_unit_scale` | covered |
| Tile reuse versus rebuild per edge (Fig. 5.34) | rebuilt per connection (divisions from the edge length or given) | rebuild only |
| Cut: a volume plus a tool-path type (5.3, Fig. 5.12) | `FabricationType` per outline: hole, edge insertion, insert between edges, slice, mill, mill_project, cut, conic, drill, drill_50, drill_10 | covered as types; tool-paths are not generated here |

## Search methods (5.4)

| Thesis | WoodSession | Status |
| --- | --- | --- |
| R-Tree bounding boxes, then a precise test (Fig. 5.13 A) | `compute_face_contacts` and `compute_cross_contacts`, a bounding-box search first | covered |
| Curve to curve: end to end, cross, side to top on axes (Fig. 5.14) | `compute_axis_contacts` for beams (phanomema_node) | covered for straight axes |
| Face to face: side to side, top to top, side to top (Fig. 5.15) | `compute_face_contact`, `InteractionContactFace` | covered |
| Plane to face: cross joints (Fig. 5.16) | `compute_cross_contact`, `InteractionContactCross` | covered |
| Mesh to mesh (Fig. 5.17) | none | **not covered** |

## Connection types (5.5)

| Thesis | WoodSession | Status |
| --- | --- | --- |
| Side to side rotated, a bounding rectangle per contact (Fig. 5.19) | ss_e_r_0, side_removal, ss_e_r_2/3 keys | covered |
| Side to side in plane and out of plane by dihedral angle (Fig. 5.20, 5.31 to 5.33) | ss_e_ip_0 to 5, ss_e_op_0 to 6; the platonic solids and hexboxes datasets | covered, 30 degree limit as 2024 |
| Insertion direction by user lines (Fig. 5.32 A, 5.39 D, 5.48 B) | `assign_insertion_vectors_by_lines`, `assign_joint_types_by_points` | covered |
| Butterfly keys between plates (Fig. 5.32 bottom, 5.36 B) | ss_e_ip_2/5 cut the pockets | **the key itself is not a piece** (2024 neither); ss_e_r_2/3 and ss_e_ip_3/4 own their keys |
| Side to top: tenon mortise, with the rectangle boundary moved and turned (Fig. 5.38 to 5.41) | ts_e_p_0 to 5 on plates, `JointBeam::from_contact` on beams | covered for plates; beam tenons as 2024 built them |
| Half dovetail oriented by outline winding (Fig. 5.45, 5.46) | none | **not covered** |
| Dowel-nut connector, bisector cuts (Fig. 5.47) | pins: `JointBeam::centred_pins`, `headed_pins` (bisector cuts: none) | pins covered, **bisector cuts not** |
| Three-valence coupling of two side-to-top joints (Annen, Fig. 5.49, 5.50) | `three_valence` sidecar, annen alignment; annen_box, annen_box_pair, annen_corner, annen_grid_small, annen_grid_full_arch match the 2025 reference | covered |
| Vidy linked joints (four plates per node) | `add_vidy_shadow_joints`, `merge_linked_joints`, ss_e_op_5/6; vidy_full has 46 four-valence nodes, all matching the 2025 reference | covered |
| Screws and tenons together on one edge (Nabucco, Fig. 5.48 C, D) | several joints per pair are possible as separate interactions | no dataset |
| Cross: half-lap, angled half cuts, conic cuts with 6 or 9 side cuts (Fig. 5.51, 5.55, 5.56) | cr_c_ip_0 half-lap, cr_c_ip_1 to 5 conic half-laps with drills | covered for plates; **beam cross joints with 6 and 9 cuts, round sections and bent members (Fig. 5.55 to 5.59) not covered** |
| Cross on reciprocal grids (Iseya, Seiwa Bunraku, Fig. 5.60, 5.62) | cross_square_reciprocal_iseya, cross_square_reciprocal_two_sides | covered for plates |
| Top to top: dowels and screws through two plates (Fig. 5.3 D) | tt_e_p_0 to 5 drill patterns | covered |
| Custom cuts (Fig. 5.3 E) | `*_custom` designs and `b_custom` | covered, semantics on skewed pairs pending |
| Boundary and foundation cuts (5.5, two-layer system) | b_0 on `compute_border_contact` | covered for plates |
| Short-end beam joints: scarf, double scarf, finger, butterfly key, feather (Fig. 5.27 to 5.30) | none | **not covered** |
| Stacked raw timber, grooves and oblique dowels (Fig. 5.22 to 5.25) | none | **not covered** |

## Boolean methods (5.6)

| Thesis | WoodSession | Status |
| --- | --- | --- |
| Display of cutting volumes (Fig. 5.64 A) | `Joint::body_mesh`, the joint volumes in the scene | covered |
| 2D polyline merge for plates (Fig. 5.66) | the plate outline merge of every edge joint | covered |
| 3D mesh boolean difference (Fig. 5.65) | Manifold solid booleans (`solid_boolean`) | covered |
| BRep boolean difference (Fig. 5.64 D) | `model_geometry_brep`: exact cylinders for drills and pins, written by `pb_dump` | covered |

## Datasets of the chapter

Hexagonal shell (Fig. 5.35): `inplane_hexshell`, `cross_vda_hexshell`. Hexagonal boxes (Fig. 5.36 B, 5.37 B1): `hexboxes`, `hexbox_and_corner`. Platonic solids (Fig. 5.33): `outofplane_tetra`, `_octahedron`, `_dodecahedron`, `_icosahedron`. Annen arch (Fig. 5.50): `annen_grid_full_arch`. Iseya (Fig. 5.62 B): `cross_square_reciprocal_iseya`. Every one is solved and matches the 2025 reference (`tests/reference_datasets.cpp`). The raw-wood prototypes (tree forks, the two-layer reciprocal grid, the stacked slabs) have no dataset, since raw wood has no element.
