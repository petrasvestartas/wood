# Wood compared with compas_model

Written 2026-11-05 from a read-only review of wood `main` and of
[compas_model](https://github.com/BlockResearchGroup/compas_model) `main` at 07fa02d (2026-10-15).
Nothing was built or run. It builds on `wood_improvement_plan.md` and does not repeat it.
"Checked" means read in the code; "Opinion" is marked.

## Summary

- Wood holds its pattern in the floor template and most element examples. But the step that applies
  a joint's effect is the hardest code in the project to read: `add_interaction` plus
  `add_joint_interaction`, 357 lines that decide by type what each joint does.
- The one idea worth taking from compas_model: an interaction's effect is a single `apply` method, and
  an element computes its geometry from its incoming edges when it is read.
- compas_model's BVH is not better than ours, and wood's `SpatialBVH` is adequate. The one real gap is
  beam axis contacts, which use no spatial index at all.

## 0. Which compas_model was read (checked)

- 6,802 Python lines, tests and examples included.
- **Its docs describe more than its code.** `docs/collision_detection.md`, `docs/contact_detection.md`
  and the examples use `Model.collisions(precision=...)`, `ElementBVH.nearby_elements`, `PlanarRegion`,
  a Coal backend and a dual-tree traversal. None of these exist in `src/` at that commit. Every function
  in `src/compas_model/algorithms/collision.py` raises `NotImplementedError`.
- So the comparison below is against two things: the shipped code and the documented design.
- Correction (2026-11-05, `compas_model_branches.md`): contact detection is shipped on `main`
  (`Model.compute_contacts`, a Python BVH, then shapely face overlap); only collision is stubs. No other
  branch or fork of compas_model has working collision code.

## 1. Leanness and readability

| Measure | Value |
|---|---|
| wood `src/` | 207 files, 57,367 lines (solver 31,103, templates 26,264) |
| `wood_session.cpp` | 1,920 lines (header 432), 69 `dynamic_cast` / `dynamic_pointer_cast` |
| `add_interaction` + `add_joint_interaction` | 192 + 165 lines, 26 casts, about 8 static helpers (`wood_session.cpp:877-1068`, `:1754-1918`) |
| `JointPlateElement` | 4 constructors, 4 `orient`, 3 `construct`, 53 static factories (`wood_element_joint_plate.h:233-324`) |
| `JointBeamElement` | 14 static factories, `.cpp` is 2,650 lines |
| Following centred pins from contact to cut | about 12 files, 10,332 lines |
| compas_model's equivalent core | `model.py` 822 + `element.py` 544 + `interactiongraph.py` 88 + `elementtree.py` 85 + `modifier.py` 39 = 1,578 lines. An interaction's effect is one method, applied in an 18-line loop (`element.py:324-342`). It has no joinery, so this is not a like-for-like size comparison. |

The centred-pins path, file by file:

1. `floor.cpp:394-410` (`add_contact`)
2. `wood_session.cpp:437` (`compute_face_contact`)
3. `wood_contact_detection.cpp:482-574` (`face_contacts_for_pair`) and `:403-451` (`face_overlap_area`)
4. `wood_interaction_contact_face.h` / `.cpp`
5. `wood_element_joint_beam.h` (`PinLayout`) and `.cpp:2197-2241` (`centred_pins`), `:2407` (`interaction(i)`)
6. `wood_session.cpp:877` (`add_interaction`), then `:1754` (`add_joint_interaction`), then
   `target_drills`, `host_drills`, `nest_children` and `host_solid_feature` (`:1405-1587`)
7. `wood_interaction_feature_solid.h` / `.cpp`
8. `wood_element_geometry.cpp`, then `wood_solid_boolean.cpp` / `wood_brep_drill.cpp`

**Where the reading breaks (checked):** what a joint does is decided in two separate if-chains that
test the same flags.

| Flag | The joint builds the payload | The session decides what it does |
|---|---|---|
| `is_butt` | `wood_element_joint_beam.cpp:2410` | `wood_session.cpp:1814` |
| `pre_drill` | `wood_element_joint_beam.cpp:2414` | `wood_session.cpp:1773` |
| `is_connector()` | `wood_element_joint_beam.cpp:2418` | `wood_session.cpp:1769` |

To follow one joint, a reader has to cross-reference both chains.

**Verdict:** the element files and the floor template read well; the core does not. The session and the
joint classes are where the pattern lives, and they are the least lean code in the project.

## 2. Does the code follow the interaction pattern?

The floor does: 23 `add_interaction` calls, in the order `wood/CLAUDE.md` describes.

`add_interaction` calls per template family:

| Family | Calls |
|---|---|
| floor | 23 |
| module | 3 |
| cross | 2 |
| truss | 2 |
| grid | 1 |
| chevron, cylindrical_folding, developable_folding, folding, reciprocal, shells, vault | 0 |

7 of the 12 families call it zero times. I did not check which of them have joints at all.

Deviations (all checked):

| Where | Deviation |
|---|---|
| chevron | Settings-driven solver: `chevron.h:315` writes `settings.joint_parameters[6] = tab_length`, `:318-336` sets per-face integer joint types and insertion vectors, `:338` calls `compute_features()`. The same in `chevron_shifted.h:273, 288`. |
| vault | Writes `cuts` and invalidates by hand: `vault.cpp:328-329, 1168-1169, 1220, 1230`. Members come from free functions that return element lists (`vault.cpp:133-1325`). |
| grid | Writes `cuts` and invalidates by hand at `grid.cpp:378-379, 400-403, 474, 576, 726`. Its one `add_interaction` (`:1039`) carries a cutter pyramid computed in the template. |
| truss | The template writes joint geometry into the joint after the factory: `butt_extensions` / `butt_solids` at `truss.cpp:203-210, 266-269, 712-713`. |
| floor (the reference) | Glue and cutter payloads are built in the template from meshes (`floor.cpp:231-235, 246-250`). The cutter is a `FacePlanarElement`, not a `JointElement`. |
| session | `add_interaction` changes the joint (`targets`) and the caller's payload (`:931-936`), and reruns `merge_features` on every plate-joint add (`:938-941`). `add_joint_interaction` removes and re-adds edges (`:1771, 1815, 1874, 1899`). The butt branch rewrites features that belong to other butts' edges (`:1826-1860`). |
| plate joints | Built in two phases: a factory, then `joint->orient(contact, {left, right})` (for example `examples/elements/element_joint_plate_side_side_inplane_dovetails.cpp`). This is against "the constructor is the recipe". |
| beam joint examples | The design is picked through `scene.settings.joint_parameters[11] = JointPlateElement::library_id(...)`, with a zero-length `InteractionContactAxis` and `from_contact(..., 500.0, 0.91, 1, scene.settings)` (`element_joint_beam_cross_half_lap.cpp`). 15 example files touch `joint_parameters`; 10 call `compute_features` or `compute_beam_features`. |
| Features applied outside `add_interaction` | `compute_features` → `merge_features`; `compute_beam_features` (`wood_session.cpp:520-583`); the free function `get_connection_zones`; direct writes to `cuts`. |
| Contacts | `compute_face_contacts` calls `erase_contacts("face")` (`wood_session.cpp:406`), which drops every face contact, including ones a template named by hand. Traced in the code, not run. |
| Element examples | The pins and plate examples compute a contact but never store it with `add_interaction(a, b, contact)`; the floor does. Minor. |

82 of the 192 example `.cpp` files call `add_interaction`.

## 3. compas_model compared with wood

| Concept | compas_model | wood |
|---|---|---|
| Model | `Model`: elements by guid, an `ElementTree` (transforms compose down the tree) and an `InteractionGraph` whose edges carry `modifiers` and `contacts` | `WoodSession : Session`: the same shape (tree, `world_xform` composed in `session_cpp/src/session.cpp:741`, a graph, interactions per edge) |
| Element geometry | Computed on read: local geometry, then the model transform, then every incoming edge's `modifier.apply(source, geometry)` (`element.py:324-342`). Lazy caches are reset by `@reset_computed` on setters (`element.py:34`). | `add_interaction` copies each effect into the target (`solid_features`, `plane_features`, `cuts`, display `ElementFeature`s); `invalidate_geometry` is called by hand |
| Effect of an interaction | One method, `Modifier.apply` (`modifiers/modifier.py`, 39 lines) | No such method on the kernel's `Interaction`; the session's type ladder decides |
| Joint as an element | None; modifiers are plain data on edges | `JointElement` with its own geometry, targets and `interaction(i)`. Wood is richer here. |
| Contacts | `Contact(points, frame, size, holes)`; every face against every face, intersected with shapely; BRep through OCC `overlap` | Typed face / axis / cross contacts, contact-type classification, joint volumes; but keeps only the first overlap region (`solution[0]`) |
| Serialisation | JSON by guid; tree nodes refuse to load outside a model (`elementtree.py:34, 63`). The base `Element.__data__` leaves out `geometry` (`element.py:103-110`). | protobuf, with settings at field 101; loaders swallow exceptions (plan item 10) |
| Collision | None shipped. The documented design has a fast AABB check and a precise one (Coal for meshes, OCC `BRepExtrema` for BReps). | None at model level; only the test oracle |

### Where compas_model is weaker (checked)

- `add_interaction(a, b, modifier=None)` ignores its `modifier` argument (`models/model.py:549`).
- The BVH search box is inflated by a relative factor of 1.2 (`models/bvh.py:78`), so the gap it
  tolerates depends on element size.
- `nearest_neighbors` reads only `objects[0]` of each leaf (`models/bvh.py:81`), so with a leaf size
  above 1 it silently drops neighbours.
- Tree traversal uses `queue.pop(0)` on a Python list (`datastructures/bvh.py:138` and others), which is
  O(n) per pop.
- When a source changes, the targets it modifies are not refreshed: `reset_computed` resets only the
  element itself, and `TODO.md` lists cache invalidation as open.

### What wood should take

| # | Item | Source in compas_model | What it fixes in wood |
|---|---|---|---|
| 1 | **One method for "what this interaction does to its target"**: a virtual such as `apply(source, to_target, BRep target)` on each `InteractionFeature*` subclass | `modifiers/modifier.py`, `elements/element.py:324-342` | `add_interaction` becomes "store on the edge, invalidate the target"; the 357-line ladder goes away |
| 2 | **Geometry folded from the graph** instead of copies hosted on the target; viewer features derived in `pb_dump` | `elements/element.py:324` | The duplicated state that `remove_interaction` has to erase in three places (`wood_session.cpp:1070-1107`). Do not copy compas's unspecified edge order: apply in insertion order. |
| 3 | **Invalidation through setters**, extended to dependents through outgoing edges | `reset_computed`, `elements/element.py:34` (dependents are open in its `TODO.md`) | Hand-written `invalidate_geometry` in grid, vault and module |
| 4 | **Contact provenance**: computed versus user-authored contacts, with a transactional recompute | `docs/contact_detection.md`, "Model and interaction-graph updates" (`contacts_computed`, `contact_edge`) | `erase_contacts("face")` wiping named contacts |
| 5 | **Contacts as several regions with holes** | `algorithms/contacts.py: brepface_brepface_overlap_holes`; `PlanarRegion` in the docs | Keeping only `solution[0]` (`wood_contact_detection.cpp:439-444`) |
| 6 | **A fast/precise collision split with documented guarantees**; for BRep-only wood, "precise" is an exact BRep distance or boolean common, as in their OCC path | `docs/collision_detection.md` | A model-level overlap check, not only the test oracle |
| 7 | **A methodology and limitations page per algorithm** | `docs/contact_detection.md` | Plan step 1, "nothing fails silently" |
| 8 | **Failing loudly on deserialisation** | `models/elementtree.py:34` | Loaders that swallow exceptions (plan item 10) |

### What not to take

- The shipped BVH.
- The relative inflation.
- One tolerance controlling three different decisions. Wood has the same smell: `settings.distance` sets
  the broad-phase inflation (`wood_contact_detection.cpp:583`), `distance_squared` the coplanarity test
  (`:510`), and 10× distance the curved contacts (`:490`).

## 4. Is compas_model's BVH better?

| | wood (`session_cpp` `SpatialBVH`) | compas_model shipped | compas_model in docs only |
|---|---|---|---|
| Structure | Linear BVH (Karras 2012), 30-bit Morton codes, flat binary node array | Python object tree, median split on the longest axis (`datastructures/bvh.py:278-312`) | Binary tree, median split |
| Build cost | O(n log n) | O(n log² n), in Python | Same as shipped |
| Node volume | AABB, then an OBB separating-axis filter (`wood_contact_detection.cpp:371-373`) | AABB, or a PCA-fitted OBB | AABB / OBB |
| Query | n box queries, explicit stack | Breadth-first with `pop(0)` | Dual-tree traversal |
| Tolerance | Absolute: OBB half-size + inflate (`session_cpp/src/obb.cpp:144-146`) | Relative × 1.2 | Absolute, but with a positive tolerance it falls back to checking every pair, O(n²) |
| Narrow phase | Face against face, Clipper2 | Face against face, shapely | Coal triangle BVH (meshes) or OCC (BReps) |

**Verdict: no, compas_model's BVH is not better for timber contact detection, and wood's `SpatialBVH` is
adequate.**

- The shipped compas_model version is weaker on every row of the table.
- The documented version's real gain is Coal's triangle-level narrow phase on meshes, which wood's
  BRep-only decision rules out. It also drops to O(n²) whenever the tolerance is positive, and timber
  always needs one.
- For 10² to 10⁴ members the broad phase is negligible; the time goes into the face-pair tests and the
  booleans.

### Gaps in wood's spatial search (checked)

- **Beam axis contacts use no spatial index at all.** `closest_pairs` tests every segment pair,
  O(N² · segments²) (`wood_contact_detection.cpp:924-962`). This is the one place a BVH would help
  (truss, grid).
- Wood computes a world size `ws` (`wood_contact_detection.cpp:352-361`) that the build never uses: the
  Morton codes are scaled to the box centres instead (`session_cpp/src/spatial_bvh.cpp:312-343`). About
  ten lines of dead code.
- The traversal stack is a fixed 64 entries guarded only by `assert` (`spatial_bvh.cpp:161-186`), so in a
  release build an overflow writes out of bounds. A linear BVH's depth stays far below 64 in practice,
  but a real bound check costs nothing and bears on the plan's open ASan report.
- The kernel's `SpatialBVH::nearest_neighbors` defaults to the same relative 1.2 inflation as
  compas_model's shipped code (`spatial_bvh.h`). Wood does not call it, but the kernel should switch to
  an absolute value.

## 5. What to do first (opinion)

1. The single `apply` method, with geometry folded from incoming edges (items 1 and 2 in section 3).
2. Contact provenance (item 4).
3. Beam axis contacts through the existing `SpatialBVH`.
