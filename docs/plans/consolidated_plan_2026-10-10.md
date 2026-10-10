# Consolidated plan, 10 October 2026

The user's requests since the joint library page, gathered into one plan and worked in this order.

## 1. Joint as element, add_interaction as the only way in (CLAUDE.md "Using the session")

Audit (file:line in the PR comment):
- **Plate solver:** it builds `JointPlate`, `JointAnnen` and `JointVidy` elements. Each goes through the private wrapper `WoodSession::apply_joint` (wood_session.cpp:1838), which `add`s the joint with no group and puts its features on the plates through `Session::add_interaction` directly. Before that, `merge_features` (wood_feature_solver.cpp:111) writes the plates' `features` itself.
- **Beam solver:** `compute_beam_features` also goes through `apply_joint`.
- **Examples and templates:** the element examples and the floor template follow the rule.

To do:
- remove `apply_joint`. Each solver adds each joint to a group by place and family (`joints_<family>`), then calls `add_interaction(joint, target, joint->interaction(i))` once per target, in target order;
- a plate joint's merged outline becomes what its `interaction(i)` carries to that plate, never a direct write to `plate->features`;
- the joint library check, the reference datasets and every test stay where they are.

## 2. The user interface: an edge line for the direction, a named point for the type

- Already there: `assign_insertion_vectors_by_lines(lines, snap_radius)`, a line at a plate edge sets that face's insertion direction.
- Add `assign_joint_types_by_points(points, names, snap_radius)`: a point with a text name (`"ss_e_ip_1"`, `"ts_e_p_3"`) sets the face's joint type through one name-to-id table next to the id-to-design tables in wood_element_joint_plate.cpp; an unknown name throws naming it.
- Extend `element_plate_assign_joints` to use names, and test it in `tests/plate_assignment.cpp`.

## 3. Hilti against the original implementation

Compare `JointBeam::hilti` with any older implementation (wood_research, compas_wood, wood_rhino history) and the photo; fix the cuts where they differ.

## 4. Pictures from the session viewer, Arctic with black outlines

- Headless Chromium on the software Vulkan driver gave white screenshots, and github.io is not reachable from the cloud session. So the pictures are drawn by the viewer's own renderer without a browser: `tools/wood_shot.rs`, built as an example of session_viewer, on the native headless GPU (lavapipe when there is no GPU).
- `Arctic On` (soft shadows) and the black outlines, orthographic isometric view fitted; no more matplotlib.
- `tools/shoot_native.sh` shoots any session file, or builds and runs an element example and shoots its `live.pb` into `docs/images/elements`.

## 5. What a picture shows

- Plate joints that merge into the outlines (every `JointPlate` design except the key and drill parts): their outlines only, polylines, in the unit box, a picture of its own; the plates with the merged outlines, drawn apart, a second picture, never overlapping the first.
- Joints that are solid booleans (keys, drills, pins, the Hilti parts, beam joints, cutters): solids, as BReps.
- Redo the joint library page and the element pages' pictures this way.
