# Joinery solver coverage

Coverage of joinery geometry concepts and examples in this port.

## Data structures

| Concept | WoodSession | Status |
| --- | --- | --- |
| Element: a minimal model (planar outlines plus an axis), a reference mesh and a collection of cuts | `Plate` (top and bottom outlines), `Beam` and `BeamVariable` (axis, section, planes), `Column`, `Block`; cuts are interactions on the target (`InteractionFeaturePlate`, `InteractionFeatureSolid`, `InteractionFeaturePlane`), applied in `model_geometry_mesh` and `model_geometry_brep` | covered for plates and rectangular or variable-section beams. **No reference mesh**: raw wood (irregular trunks) has no element type |
| Element grouping, the sorted list with keys `x;y0;y1` | the session tree: groups by place and family (`group_named`, `get_branch`), read back by name | covered, as a tree instead of a sorted key list |
| Joint = undirected graph + tiles | the session graph: `add_interaction(source, target, interaction)` puts each contact and joint on an edge; `JointPlate` and `JointBeam` are elements; adjacency sidecars `*_adjacency.txt` | covered |
| Tile = male + female cuts in a unit box | one design function per joint (`wood_interaction_feature_plate_joints/*.h`) filling `male_outlines` and `female_outlines` of an `InteractionFeaturePlate` in the unit box | covered; drawn in `docs/joint_library.md` |
| Tile transformation: plane to plane, change of basis from two rectangles, translation | `joint_orient_to_connection_area` (change of basis onto `joint_volumes`), `apply_unit_scale` | covered |
| Tile reuse versus rebuild per edge | rebuilt per connection (divisions from the edge length or given) | rebuild only |
| Cut: a volume plus a tool-path type | `FabricationType` per outline: hole, edge insertion, insert between edges, slice, mill, mill_project, cut, conic, drill, drill_50, drill_10 | covered as types; tool-paths are not generated here |

## Search methods | `compute_face_contacts` and `compute_cross_contacts`, a bounding-box search first | covered |
| Curve to curve: end to end, cross, side to top on axes | `compute_axis_contacts` for beams (phanomema_node) | covered for straight axes |
| Face to face: side to side, top to top, side to top | `compute_face_contact`, `InteractionContactFace` | covered |
| Plane to face: cross joints | `compute_cross_contact`, `InteractionContactCross` | covered |
| Mesh to mesh | none | **not covered** |

## Connection types | ss_e_r_0, side_removal, ss_e_r_2/3 keys | covered |
| Side to side in plane and out of plane by dihedral angle | ss_e_ip_0 to 5, ss_e_op_0 to 6; the platonic solids and hexboxes datasets | covered, 30 degree limit as 2024 |
| Insertion direction by user lines | `assign_insertion_vectors_by_lines`, `assign_joint_types_by_points` | covered |
| Butterfly keys between plates | ss_e_ip_2/5 cut the pockets | **the key itself is not a piece** (2024 neither); ss_e_r_2/3 and ss_e_ip_3/4 own their keys |
| Butterfly keys in milled half-key pockets on flattened raw beams | none | **not covered** |
| Side to top: tenon mortise, with the rectangle boundary moved and turned | ts_e_p_0 to 5 on plates, `JointBeam::from_contact` on beams | covered for plates; beam tenons as 2024 built them |
| Half dovetail oriented by outline winding | none | **not covered** |
| Dowel-nut connector, bisector cuts | pins: `JointBeam::centred_pins`, `headed_pins` (bisector cuts: none) | pins covered, **dowel-nut and bisector cuts not** |
| Three-valence coupling of two side-to-top joints (Annen) | `three_valence` sidecar, annen alignment; annen_box, annen_box_pair, annen_corner, annen_grid_small, annen_grid_full_arch match the 2025 reference | covered |
| Four-plate nodes: in plane, out of plane with more than three plates | the hexshell and platonic datasets | covered |
| Vidy linked joints (four plates per node) | extended joinery: `add_vidy_shadow_joints`, `merge_linked_joints`, ss_e_op_5/6; vidy_full has 46 four-valence nodes, all matching the 2025 reference | covered |
| Snap fit | ts_e_p_5 with 2024's literals; top_to_side_snap_fit and top_to_side_box match the 2025 reference | covered |
| Screws and tenons together on one edge (Nabucco) | several joints per pair are possible as separate interactions | no dataset |
| Cross: half-lap, angled half cuts, conic cuts with 6 or 9 side cuts | cr_c_ip_0 half-lap, cr_c_ip_1 to 5 conic half-laps with drills; on beams `JointBeam::from_contact` cuts the rectangular 3-cut half-lap | covered for plates and the rectangular beam half-lap; **on beams `compute_joinery` takes the merged outlines only, so the conic cr_c_ip_1 to 5 and their drills are dropped; round sections, skewed and bent beam crossings and fasteners through a beam cross not covered** |
| Cross on reciprocal grids (Iseya, Seiwa Bunraku) | cross_square_reciprocal_iseya, cross_square_reciprocal_two_sides | covered for plates |
| Top to top: dowels and screws through two plates | tt_e_p_0 to 5 drill patterns | covered; 2024's defaults (division length 6, shift 0.95) do not encode the required spacing, and holes can run to the edge |
| Custom cuts | `*_custom` designs and `b_custom` | covered, semantics on skewed pairs pending |
| Boundary and foundation cuts (5.5, two-layer system) | b_0 on `compute_border_contact` | covered for plates |
| Short-end beam joints: miter with dowel, scarf, double scarf, angled double scarf, finger, butterfly key, feather | `JointBeam::tie`, a bow-tie key across an end-to-end contact of rectangular members | **all but the butterfly key not covered** |
| Stacked raw timber, grooves and oblique dowels | none | **not covered** |

## Boolean methods | `Joint::body_mesh`, the joint volumes in the scene | covered |
| 2D polyline merge for plates | the plate outline merge of every edge joint | covered |
| 3D mesh boolean difference | Manifold solid booleans (`solid_boolean`) | covered |
| BRep boolean difference | `model_geometry_brep`: exact cylinders for drills and pins, written by `pb_dump` | covered |

## Datasets

Hexagonal shell: `inplane_hexshell`, `cross_vda_hexshell`. Hexagonal boxes: `hexboxes`, `hexbox_and_corner`. Platonic solids: `outofplane_tetra`, `_octahedron`, `_dodecahedron`, `_icosahedron`. Annen arch: `annen_grid_full_arch`. Iseya: `cross_square_reciprocal_iseya`, as plates (represented here as plates). Orthogonal plate crossings: `cross_corners`, `cross_and_sides_corner`. Slotted hexagonal plates and Nabucco have no dataset. Every one is solved and matches the 2025 reference (`tests/reference_datasets.cpp`). The raw-wood prototypes (tree forks, the two-layer reciprocal grid, the stacked slabs) have no dataset, since raw wood has no element.
