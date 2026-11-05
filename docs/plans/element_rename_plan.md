# Element class rename and merge: plan

Phase 1 of the rename: inventory and plan only. Read from the code at wood `e919364c`, session `6e73b4c7`.
Nothing was built or run for this plan, so every geometry statement below is from reading the source; the
parity test in step 5 is what proves them.

The rule from the user: a class name starts with its category. Holes, sessions, plane cuts and joint assignment
are not elements; they are sections on the element pages.

## 1. Decisions

The user confirmed all of these on 2026-11-04, with decision 2 changed: nothing keeps the old names, `wood/rust` included.

1. **Tags in saved scenes**: write the new names, read the old names for good (section 3).
2. **`wood/rust` is renamed too**, together with `session_viewer`, in step 6b. The old public names stay only through
   steps 1 to 6, so session CI stays green and session changes once, not four times.
3. **Leaning grid columns**: they become `EdgeVariableElement`, not `EdgeStraightElement` (section 5, conflict 2).
4. **No column flag**: a column is an `EdgeStraightElement` named `column_<i>`; nothing in the class says column (section 5).
5. **`JointCutPlaneElement` becomes a real `JointElement`**, used as `add_interaction(cut, target, cut->interaction(0))` (section 6).
6. **Joints take the same suffix** (user, 2026-11-04): `JointElement`, `JointBeamElement`, `JointPlateElement`,
   `JointAnnenElement`, `JointVidyElement`, `JointPinElement`, `JointCutPlaneElement`, `JointConnectorPartElement`.
   `InteractionFeatureBeam`, `InteractionFeaturePlate`, `InteractionFeaturePlateBeam`, `JointPlateParameters`, `JointCutter`,
   `PinLayout`, `BeamUp`, `PlatePair`, `PlateGroup` stay.
7. **`NodeSupportElement` has no holes or plane cut section.** The code refuses both, so a working example cannot exist (section 7).
8. **A case that does not work when built is a finding, not a fragment.** Curved elements have cases no code has
   exercised yet; the class is fixed or the user drops the section (section 7).
9. **Base classes `EdgeElement`, `FaceElement`, `NodeElement` beside `JointElement`** (user, 2026-11-04). The concrete classes carry
   the full name too: `EdgeStraightElement`, `FacePlanarElement`, `NodeBlockElement` and so on, in C++, Rust, proto messages
   and scene tags, with old tags still read. Files, examples, pages and viewer commands keep their words (`wood_element_edge_straight`,
   `element_edge_straight`, `Element Edge Straight`).

10. **The pattern stays.** A joint is an element, what it does is an interaction, and a model is built with `add` and
    `add_interaction`, unchanged. This rename touches names, never that flow.
11. **Protobuf stays as it works today, only regenerated for the new names.** Same files in `src/proto`, same field numbers,
    same `element_data` bytes, same committed `generated/` and `wood_proto.rs`. Messages follow the class names. Regeneration
    is for wood only (`tools/regen_proto.sh`); the kernel's protos are not touched. Two additive fields come from other
    decisions: `round_radius` (the merge) and `cut_size` (decision 5).
12. **No collapsing on the pages**: the full code of every example is visible.
13. **Curved viewer commands are reserved, not built in step 6b.**

## 2. New names

| Old class | New class | Source file | Proto file | Tag written |
| --- | --- | --- | --- | --- |
| `Block` | `NodeBlockElement` | `wood_element_node_block` | `element_node_block` | `NodeBlockElement` |
| `Support` | `NodeSupportElement` | `wood_element_node_support` | `element_node_support` | `NodeSupportElement` |
| `Beam` | `EdgeStraightElement` | `wood_element_edge_straight` | `element_edge_straight` | `EdgeStraightElement` |
| `Column` | merged into `EdgeStraightElement` | deleted | `element_column` stays, read only | none |
| `BeamCurved` | `EdgeCurvedElement` | `wood_element_edge_curved` | none | `EdgeCurvedElement` |
| `BeamVariable` | `EdgeVariableElement` | `wood_element_edge_variable` | `element_edge_variable` | `EdgeVariableElement` |
| `Plate` | `FacePlanarElement` | `wood_element_face_planar` | `element_face_planar` | `FacePlanarElement` |
| `PlateCurved` | `FaceCurvedElement` | `wood_element_face_curved` | none | `FaceCurvedElement` |
| `CutPlane` | `JointCutPlaneElement` | `wood_element_joint_cut_plane` | fields in `element_joint` | `JointCutPlaneElement` |
| `Pin` | `JointPinElement` | `wood_element_joint_pin` | `element_joint` | `JointPinElement` |
| `Joint` | `JointElement` | `wood_element_joint` | `element_joint` | `JointElement` |
| `JointBeam` | `JointBeamElement` | `wood_element_joint_beam` | `element_joint` | `JointBeamElement` |
| `JointPlate` | `JointPlateElement` | `wood_element_joint_plate` | `element_joint` | `JointPlateElement` |
| `JointAnnen` | `JointAnnenElement` | `wood_element_joint_plate` | `element_joint` | `JointAnnenElement` |
| `JointVidy` | `JointVidyElement` | `wood_element_joint_plate` | `element_joint` | `JointVidyElement` |
| `ConnectorPart` | `JointConnectorPartElement` | `wood_element_joint_connector_part` | `element_joint` | `JointConnectorPartElement` |

`wood_profile.*` keeps its name and its `profile_*` functions: a profile is loops in the section frame, a
`std::vector<Polyline>`, no class and no element. No new name collides with `session_cpp`, `session_rust`, the
viewer or wood (checked by grep; `graph.h` has `Edge`, `brep_*` have `FaceUse`, `EdgeUse`, `FaceCurve`, none equal).

### Base classes

Four categories, four base classes. The tree of classes:

```
Element
├── WoodElement                     timber: solid features, plane features, stock
│   ├── EdgeElement  EdgeStraightElement, EdgeCurvedElement, EdgeVariableElement
│   ├── FaceElement  FacePlanarElement, FaceCurvedElement
│   └── NodeElement  NodeBlockElement, NodeSupportElement
└── JointElement     JointBeamElement, JointPlateElement, JointPinElement, JointCutPlaneElement, ...
```

`WoodElement` stays between: `JointElement` derives from `Element` directly, because a joint hosts no member state, and
`WoodSession::add` tells members from joints by that. A base has no tag, no proto and no `register_type`, so nothing
changes on the wire. `get_elements<EdgeElement>()` finds all three kinds, since it casts to the type it is given.

**`EdgeElement` holds what the three really share**, in `wood_element_edge.h/.cpp`:

| Member | Why it belongs here |
| --- | --- |
| `std::vector<Plane> cuts` | the same field with the same meaning in all three |
| `compute_planes()` | word for word `face_planes(model_geometry_mesh())` in all three |
| `trimmed_mesh()`, `trimmed_brep()` | identical in `EdgeStraightElement` and `EdgeVariableElement`; `EdgeCurvedElement` overrides them for its cross laps, as it does now |
| `virtual Line chord() const = 0` | its first to its last point. Each class spells this differently: `axis` ends, `axis`, `centreline` ends |

What that removes:

- `contact_type` (`wood_contact_detection.cpp:283-302`): the four-way `linear_a`, `linear_b` casts and the three-branch
  `is_end` become one `dynamic_cast<const EdgeElement*>` and `chord()`. Its "invalid centreline is not an end" check stays, as a zero
  length chord.
- `level_axis` (`wood_element_joint_beam.cpp:2250`): three branches become `chord()`.
- `add_plane_cut` (`wood_session.cpp:1668`): one `EdgeElement*` branch and the `NodeBlockElement` branch.

Not moved up, because it is not shared: `extension` (not on `EdgeVariableElement`), the axis itself (`Polyline`, `Line`, `NurbsCurve`),
`aabb`, `base_plane`, `transformed`, `place`, the features.

**`FaceElement` and `NodeElement` hold nothing today.** That is the honest result of comparing them, and nothing is moved up to fill them:

- `FacePlanarElement` has outlines, planes, thickness and feature tables. `FaceCurvedElement` has four meridian curves and no thickness
  field. No field and no override body in common.
- `NodeBlockElement` has loops, cuts and a source mesh. `NodeSupportElement` is a steel part with 21 datasheet dimensions, and it refuses
  features altogether. `cuts` on `NodeBlockElement` is not shared, so it stays there.

Each is a header of a few lines, `class FaceElement : public WoodElement { public: using WoodElement::WoodElement; };`. What they
give is the category: the class names read as the library does, `get_elements<FaceElement>()` works, and a joint can take a `const
FaceElement&` when one needs to. If a shared member turns up later, it has a home.

`EdgeElement` is abstract, because of `chord()`. Rust gets no base classes: the crate has no inheritance, only the `WoodElement`
trait, and nothing there needs one. So `wood` adds no base type.

### Name clashes

`EdgeElement`, `FaceElement` and `NodeElement` appear nowhere in `session_cpp`, `session_rust`, `session_proto`,
`session_viewer` or wood, so nothing clashes and no kernel use needs qualifying. Bare `Edge` would have collided with
`session_cpp::Edge`; `graph.h` and wood's uses of it stay as they are.

### Accessors

The accessors `plates()`, `beams()`, `columns()`, `supports()`, `blocks()`, `beam_variables()`, `beam_curveds()` wrap
one call each, which wood's house style forbids, and their new names would read badly. Remove them; the 31 call
sites use `get_elements<FacePlanarElement>()` and so on. `Module::columns()` in the module templates is a different
function and stays. Default element names (`"beam"`, `"column"`, `"plate"`) are data, not class names, and stay.

## 3. Where the names are load-bearing

**On the wire.** Only two strings are stored: `Element.element_type` and, for joints, `JointElement.kind`. `element_data` is
plain `bytes` (`session_proto/element.proto:69`), so protobuf message names are not on the wire and can change freely.

**What happens on an unknown tag.** `Element::pb_loads_polymorphic` falls back to a base `Element` that keeps its tag,
data and geometry (`session_cpp/src/element.h:512`, same in `session_rust`). So a wrong tag does not lose the picture;
it loses the class: `get_elements<T>()` no longer finds it, joints cannot read it.

| Consumer | What it does with the names | What the rename means |
| --- | --- | --- |
| wood C++ `register_type` (`wood_session.cpp:20-35`) | maps tag to factory | register new and old tags |
| `JointElement::from_element`, `JointElement::register_type` (`wood_element_joint.cpp:345-397`) | maps `kind` to class | add every new name, keep every old one (`Joint`, `JointBeam`, `JointPlate`, `JointAnnen`, `JointVidy`, `Pin`, `ConnectorPart`); drop the doubled `"Pin"` |
| `Plate::from_element` (`wood_element_plate.cpp:332`) | JSON `"type": "Plate"` | keep reading it |
| Tracked scenes: 14 `.pb` in `git ls-files`, of which 7 in `rust/tests/data` | old tags | never rewritten; they are the proof old scenes load |
| `data/floor_model.pb`, `compas_tf/session.py` | `"Plate"`, `"Column"`, `"Solid"` with JSON payloads; `compas_tf` is never pushed from here | old tags and JSON read for good |
| `data/*.yml`, `data/templates/*.json` | keys only (`beams:`, `obj:`, no class tags) | nothing |
| wood `Settings.beams`, `settings.proto` | a yaml field, not a class | stays |
| `session_cpp`, `session_py`, `session_rust` | carry `element_type` as an opaque string; no wood class named | nothing |
| viewer display (`scene_display.rs`, `source_memory.rs`) | draws `element.geometry()`, never decodes `element_data`, never calls `from_element` | scenes with new tags draw the same |
| viewer colour | none by type: the node colour in the scene, set by `WoodSession::add` | same grey for every member |
| viewer commands (`element_beam.rs`, `element_column.rs`, `element_plate.rs`, `element_block.rs`, `element_support.rs`, `element_beam_variable.rs`, `tool/elements.rs`) | compile against `wood::Beam`, `wood::Column`, `wood::Plate`, `wood::Block`, `wood::Support`, `wood::BeamVariable`, `Column::from_profile`, `Beam::from_profile`, and a test asserts `e.element_type == "Plate"` | changes with `wood/rust` in step 6b |
| session CI `viewer-check.yml:38`, `viewer-pages.yml:52` | clone wood `main` HEAD, sparse `rust` | a wood push starts nothing in session; the next session run builds against whatever wood's tip is then |
| `wood/rust/build.rs` | regenerates `wood_proto.rs` from `src/proto` when that folder is there; the sparse clone has only the committed file | commit the regenerated file with every proto change |
| `wood_nano` | **cannot build against current wood today**: its `CMakeLists.txt` lists files that no longer exist at those paths (`wood_joint.cpp`, `wood_face_to_face.cpp`, `wood_element_beam.cpp`, `wood_element.cpp`, ...) and its sources use `WoodJoint` and `WoodElement::thickness`, both gone from wood. Seen from the sources; I did not try a build | no effect; it needs its own repair, against the new names |
| `compas_wood` | its `Plate` is its own Python `Data` class; it reaches wood only through `wood_nano` | nothing |

**Policy (decision 1).** C++ writes the new tag and registers every old tag beside it. The pattern exists:
`Block::LEGACY_ELEMENT_TYPE`, Rust `LEGACY_TYPE`. Registering them costs one line per tag and means a saved scene says
what the code says. Freezing the old tags would be safer still but leaves a scene full of names the code no longer has.

| Class | Also reads |
| --- | --- |
| `EdgeStraightElement` | `Beam`, `Column` (`Column` through its own legacy reader, see below) |
| `EdgeCurvedElement` | `BeamCurved` |
| `EdgeVariableElement` | `BeamVariable` |
| `FacePlanarElement` | `Plate`, `WoodElement` (already legacy) |
| `NodeBlockElement` | `Solid`, `BlockElement` (already legacy) |
| `NodeSupportElement` | `Support` |
| `JointElement` | `Joint`; the old tag `JointElement` becomes the written one, and `kind` still picks the class |
| `JointBeamElement`, `JointPlateElement`, `JointAnnenElement`, `JointVidyElement` | the same names without the suffix, as tag and as `kind` |
| `JointPinElement`, `JointConnectorPartElement` | `Pin`, `ConnectorPart` as tag and as `kind` |
| `FaceCurvedElement`, `JointCutPlaneElement` | nothing: neither is registered today, so a saved one already loads as a plain element |

**Rust through steps 1 to 6.** In `wood/rust` only the `proto::` paths change, so it still compiles after each proto
rename; its public structs, `TYPE` strings and goldens wait for step 6b. Meanwhile the viewer writes old tags and C++
reads them as the new classes.

## 4. What Column has that Beam lacks

| Thing | `Beam` | `Column` | In `EdgeStraightElement` |
| --- | --- | --- | --- |
| Axis | `Polyline` | `Line` | `Polyline`; column code reads `axis.get_lines().front()` |
| Section | `radii` square or `profile` loops | world `section` polyline, or `profile` + `rotation` | `radii` or `profile` as now; `section` and `rotation` gone |
| Round | none | `radius`, `Column::round`, exact `drill_brep` | `round_radius`, `EdgeStraightElement::round` |
| Up | `directions` per segment, default world z | none, frame from world x and `rotation` | `directions`, always explicit for a column |
| `cuts` | yes | yes | yes |
| `extension`, `allowed_type` | yes | no | yes; default for a column |
| Solid BRep with cuts | base class | own `model_geometry_brep` when round | that override moves over, only for `round_radius > 0` |
| `head_blocks` | no | yes, one user: `Floor::add_column` (`floor.cpp:187`) | moves to `Floor`, a private `compute_head_blocks`: a beam has no head |
| Glued blocks, seat, cutters | `add_interaction` | `add_interaction` | unchanged: not class state |
| `aabb` | axis widened by reach | solid when there is one | see conflict 6 |
| `base_plane` | at the axis start | at the section's first corner | axis start, conflict 8 |
| Features | axis, a section per ring | axis, one section | axis, a section per ring; hidden attributes |
| `Column(solid, axis, section, name)` | | no caller | dropped |

### EdgeStraightElement

One constructor, the profile one; the rest are named static constructors that say what they take:

```cpp
class EdgeStraightElement : public WoodElement {
public:
    static constexpr std::string_view ELEMENT_TYPE = "EdgeStraightElement";
    Polyline axis;                  // Centreline, one segment per span, in world space.
    std::vector<double> radii;      // Section half-width per segment.
    std::vector<Vector> directions; // Section up per segment; none takes world z.
    int allowed_type = -1;          // Which contacts it accepts.
    std::vector<Plane> cuts;        // Planes the solid is cut by.
    double extension = 0.0;         // Length the solid runs past each axis end.
    std::vector<Polyline> profile;  // Section loops in the section frame, loop 0 the outline, then holes.
    double round_radius = 0.0;      // A round edge's radius, its BRep the exact cylinder; zero otherwise.

    EdgeStraightElement();
    EdgeStraightElement(const Polyline& axis, const std::vector<Polyline>& profile,
                 const std::vector<Vector>& directions = {}, const std::string& name = "beam");

    static std::shared_ptr<EdgeStraightElement> square(const Polyline& axis, double radius, const std::string& name = "beam");
    static std::shared_ptr<EdgeStraightElement> corner_square(const Line& axis, const Plane& corner, double side, const std::string& name = "column");
    static std::shared_ptr<EdgeStraightElement> from_section(const Line& axis, const Polyline& section, const std::string& name = "column");
    static std::shared_ptr<EdgeStraightElement> round(const Line& axis, double radius, double chord_tolerance = 0.02, const std::string& name = "column");
};
```

- A profile is what `EdgeStraightElement` takes, as `Beam(axis, profile, ...)` does now: `profile_rectangle`, `profile_w`,
  `profile_hss`, `profile_t` and the rest. `profile_round` stays a polygon; `round` is the exact one.
- `from_section` turns a world loop into a profile: x along its first edge, y = axis × x, one `directions` entry y.
- `compute_up(axis, rotation)` beside `compute_size` in `wood_profile`, and in Rust `wood::geometry`: the up of a standing
  member, axis × world x across the axis, turned by rotation degrees. It replaces `profile_x` and `rotation`. Callers: the
  legacy `Column` readers and the viewer's Element Edge Straight Column.
- The field `radii` stays, on the wire and in `square`. Every caller today passes one radius for all segments.
- The `pair_crossing`, `pair_tee`, `pair_end_to_end` statics move over unchanged.
- `allowed_type` callers (the three `std::vector<double>(segment_count, r)` sites) use `square` and set the field.

### Is a column distinguished? No (decision 4)

Nothing needs it. Every place that asks is listed here, with what it does after the merge:

- `JointBeamElement::column_head` takes the column as its first argument, by position, so class identity was never the
  mechanism. Signature: `column_head(const EdgeStraightElement& column, const EdgeStraightElement& a, ... d, const EdgeStraightElement* main_beam, ...)`.
  It reads `column.round_radius` and throws when it is zero, as now, so swapped arguments still fail loudly. One
  column and four beams works exactly as before.
- `JointElement::support(const NodeSupportElement&, const EdgeStraightElement&)`: reads only the guid.
- `contact_type` (`wood_contact_detection.cpp:283-302`), `butt_member` (`wood_element_joint_beam.cpp:785-796`),
  `add_plane_cut` (`wood_session.cpp:1668-1671`), `transformed` and `element_key` (`wood_instance.cpp:195-198, 275-278`):
  the `Column` branch disappears; the `Beam` branch already does the same work.
- `scene.columns()` (`tests/floor_elements.cpp:277,1499,1507`, `main_session_round_trip.cpp:73`): read back by name
  with `get_element_by_name` and `get_elements_numbered`, as wood's rules ask.
- Viewer picture and colour: the same grey (`MEMBER_COLOR` for every non-`JointElement`), the geometry slot the same solid.
  The tree group `column_<i>` and the name `column_<i>` still say column in the layer panel.
- `tests/interaction_ownership.cpp:22` already makes a column as a `Beam`.

## 5. Hardest conflicts

1. **The viewer compiles against `wood/rust`.** One struct cannot sit behind both `Column::from_profile` and
   `Beam::from_profile`, so the viewer has to change with the crate. Step 6b below is how they land together.

2. **Leaning grid columns have sections that are not square to the axis.** `grid.cpp:537-546` sweeps a level polygon
   along an axis that leans up to 30 degrees (`grid.h:72`). `Column` translates the section along the axis, a sheared
   prism with level caps; `profile_section` always places loops square to the axis. Recommendation: `to_column` uses
   `EdgeStraightElement::from_section` when the axis is square to the section and `EdgeVariableElement::between(bottom, top, name)`
   when it leans, which is exactly the same loft with no new field. Cost: `element_key` has no `EdgeVariableElement` branch, so a
   leaning column is not instanced by `instance_by_key`. `from_section` throws on a section not square to the axis, so
   nothing shears silently. The old alternative, a section-plane field on `EdgeStraightElement`, adds machinery for one case.

3. **A default up flips a vertical section.** For a vertical axis `profile_section` falls back to world x as rise, so
   with no direction a section comes out with width along -y and depth along +x, a quarter turn from `Column`. Fix: a
   column always has an explicit up. With up = axis × `profile_x(axis, rotation)`, `profile_section` gives
   side = x and rise = y, exactly what `placed_profile` does, for any axis (checked by hand, `(along, x, y)` is
   right-handed). `rotation` then disappears, and `transformed` and `place` no longer recompute it.

4. **The name `radius`.** `Beam` has a method `radius(int)`, `Column` a field `radius`, with different meanings. Hence
   `round_radius`.

5. **Old column payloads are not beam payloads.** Field 1 is a `Line` in `Column`, a `Polyline` in `Beam`, so one
   parser cannot read both. Keep `element_column.proto` and its generated code, marked read only, and register
   `"Column"` to a legacy reader that returns a `shared_ptr<Element>`: an `EdgeStraightElement`, or an `EdgeVariableElement` when the
   section leans. It also keeps the JSON path `compas_tf` writes. `element_type_name()` is overridden, so a loaded scene
   saves with the new tag.

6. **`aabb`.** `Beam` uses axis + reach; `Column` uses the solid, which includes glued head blocks. Reach alone would
   miss a rib resting on a head block, and an off-centre `corner_square` section is not reached by half the width.
   Recommendation: `Column`'s rule (solid when there is one), else the axis widened by the farthest section point. This
   can narrow a beam's box; contact counts in `floor_elements`, `joint_butt`, truss and module are the check.

7. **One constructor against about 35 `Beam(axis, radius)` sites.** They cannot be routed through
   `profile_rectangle(2r, 2r)`: a one-segment profile goes through `Mesh::loft` and `brep_between_loops`, a radius through
   `sweep_sections` and `brep_sections`, and contacts and tests store face numbers (`tests/joint_elements.cpp` checks
   `face_a == 5`). So `square` keeps the radius path. Every site fails to compile (a double is not a loop list) until
   moved, so none is missed.

8. **Base plane moves for columns**, from the section corner to the axis base, x still along the first section edge.
   It is drawn as a hidden attribute and `stock_mesh` unites glued blocks in that frame, snapped on a 1e-6 grid. Both
   solids snap in the same frame, so no change is expected. `floor_elements` is the check.
   `templates_floor_9_orientation.cpp:23` uses `column->section`; it becomes `column->sections()[0]`.

9. **Columns now take part in beam-only code.** `compute_axis_contacts`, `compute_beam_features`, `add_module_contacts`
   use `Beam`. No template calls the first two (only `wood_datasets.cpp` and tests) and no module has a `Column`, so
   nothing changes today. A scene mixing them would now get axis joints on its columns, which is the right meaning of
   one class.

10. **`level_axis`** (`wood_element_joint_beam.cpp:2250`): it returned nothing for a `Column`. A vertical `EdgeStraightElement`
    still returns nothing, but a tilted one now returns its level axis, so `headed_pins` can lay out differently on a
    tilted column. Grid leaning columns become `EdgeVariableElement`, which already has a branch. Check `joint_pins`.

11. **Other jobs are in these files.** `wood_element_column.cpp` changed under me while reading: 549 lines with an
    in-flight rewrite of `model_geometry_brep`, then 666 and clean. The rewrite was not committed and carried a
    `// TEMPORARY` `std::cout` seam print. `wood_element_joint_beam.cpp` and `tests/joint_beams.cpp` had edits too.
    Renames must start only when these jobs have landed: step 5 moves whichever `model_geometry_brep` is final.

### Two existing faults the merge fixes

- `radius` is not saved: `element_column.proto` has no field for it and `Column::from_element` never sets it, so a
  round column reloads as chords. New field `double round_radius = 10;` in `EdgeStraightElement`.
- A round column with no solid features is written as a mesh: `compute_breps` (`wood_session.cpp:1720`) lists
  `Support`, `PlateCurved`, `BeamCurved` but not it. That breaks "Round means BRep". Add `round_radius > 0`.

## 6. Modifiers become joints

- `Pin` and `ConnectorPart` already derive from `JointElement`; only names and tags change.
- `CutPlane` derives from `WoodElement`, has no proto, is not registered and uses `feature()`. Renaming alone would
  give a `JointElement...` that `get_elements<JointElement>()` does not find. Decision 5:
  - `class JointCutPlaneElement : public JointElement`, `plane` kept in `cuts[0]`, `size` in a new `element_joint.proto` field
    `double cut_size = 25;`, registered in `JointElement::register_type`, so it now survives a save.
  - `interaction(0)` returns the `InteractionFeaturePlane` that `feature()` returns today; `feature()` goes. Generic
    routing at `wood_session.cpp:984` hosts it on any member; check `add_joint_interaction` passes it through.
  - `WoodSession::add` paints only non-`JointElement` nodes, so the square would lose its grey: set the colour in the class,
    as wood's rule says, and compare `element_beam_variable_cut.png`.
  - `JointElement.cuts` with no loops still goes through `add_plane_cut`, a different path that throws on plates. Leave it; the
    two paths are a separate clean-up.

## 7. Files, examples and pages

From the user: **each element page must have a fully working example, not three-line snippets.** So every section on
every element page and every joint page shows a complete example, built and run, by `\include{lineno}
elements/<example>.cpp` from `examples/elements/`, with its picture. The only code block left on a page is the
`## Constructors` list of signatures, which is what `wood/CLAUDE.md` asks for and is not an example.

### Where the pages stand today

- **Every element page is snippets only.** No section on `element_beam`, `column`, `profile`, `plate`, `plate_curved`,
  `beam_curved`, `beam_variable`, `block`, `support`, `cut_plane` or `pin` uses `\include`.
- Across all 30 pages: 87 case sections, 4 use `\include` (`column_head` two, `keyed_box`, `top_side_bridle`).
- **Examples are not the problem**: 80 of the 87 sections already have a complete example on disk with a main, a
  description and a `pb_dump`. All 81 files in `examples/elements/` passed that check, and each has a picture. Only the pages do not show them.
- Two examples no page shows: `element_joint_plate_border`, `element_joint_plate_parameters`.
- Most joint sections end in a one-line call (`JointPlateElement::top_top_centre_pin(8.0);`) with the whole file sitting unused
  beside them.

### Examples: renamed, by `git mv`

The picture follows its example; content is unchanged, so no re-shoot.

| Old example and picture | New |
| --- | --- |
| `element_beam` | `element_edge_straight` |
| `element_profile` | `element_edge_straight_sections` |
| `element_column` | `element_edge_straight_column` |
| `element_column_session` | `element_edge_straight_column_session` |
| `element_beam_curved` | `element_edge_curved` |
| `element_beam_variable` | `element_edge_variable` |
| `element_beam_variable_cut` | `element_edge_variable_cut_plane` |
| `element_plate` | `element_face_planar` |
| `element_plate_holes` | `element_face_planar_holes` |
| `element_plate_session` | `element_face_planar_session` |
| `element_plate_assign_joints` | `element_face_planar_joints` |
| `element_plate_curved` | `element_face_curved` |
| `element_block` | `element_node_block` |
| `element_support` | `element_node_support` |
| `element_pin` | `element_joint_pin` |

`element_joint_beam_*` and `element_joint_plate_*` keep their names, including `element_joint_beam_column_head*`.

**Merges.** Pages merge; examples do not, because each example is one case. `element_edge_straight` (a beam),
`..._column` (the same class standing up, its section given) and `..._sections` (every profile) are three different
things to show. A case page may `\include` an example from a joint page rather than write a second one, marked "reused"
below. The numbered `examples/8_beams`, `10_profile_cuts`, `11_drill_solids`, `12_solid_features` stay on the examples
page: they show solver and cutter behaviour, not one element case, and are galleries rather than minimal examples.

### Element pages: example per section

Key: **have** exists, renamed above. **reused** is an existing example from another page. **new** must be written, shot
and registered. **n/a** cannot work.

| Page | Make one | Session of several | Holes | Cut by plane | Assign joints |
| --- | --- | --- | --- | --- | --- |
| `edge_straight` | have, plus `_sections`, `_column`, `_column_session` | new `_session` | new `_holes` | new `_cut_plane` | reused `element_joint_beam_cross_half_lap` |
| `edge_curved` | have | new `_session` | new `_holes`, unproven | new `_cut_plane`, unproven | new `_joints`, unproven |
| `edge_variable` | have | new `_session` | new `_holes` | have | reused `element_joint_beam_wedge` |
| `face_planar` | have | have | have | new `_cut_plane` | have |
| `face_curved` | have | new `_session` | new `_holes`, unproven | new `_cut_plane`, unproven | new `_joints`, unproven |
| `node_block` | have | new `_session` | reused `element_joint_drill` | new `_cut_plane` | new `_joints` |
| `node_support` | have | reused `element_edge_straight_column_session` | n/a | n/a | new `element_joint_support` |

**19 new examples with pictures**: 18 from the table, plus `element_joint_cut_plane`, the cut plane page's own one instance
(only its picture exists today). `element_joint_support` also serves the Support seat section of the joint page.

- `element_edge_straight_session`, `_holes`, `_cut_plane`
- `element_edge_curved_session`, `_holes`, `_cut_plane`, `_joints`
- `element_edge_variable_session`, `_holes`
- `element_face_planar_cut_plane`
- `element_face_curved_session`, `_holes`, `_cut_plane`, `_joints`
- `element_node_block_session`, `_cut_plane`, `_joints`
- `element_joint_support`
- `element_joint_cut_plane`

**`n/a` on `node_support`** (decision 7): `get_solid_features` returns null for a `Support`
(`wood_session.cpp:58`) and it overrides `model_geometry_*`, so it takes no features: it is steel with its own bores.

**Unproven cases** (decision 8). A plane cut reaches any `WoodElement` through `model_geometry_mesh`
(`wood_session.cpp:984`), and holes through `solid_features`, so these should work on the curved classes, but nothing runs
them today: `BeamCurved` is used only by its own example and the vault, which joins nothing with `add_interaction`, and
`PlateCurved` templates only call `compute_face_contacts`. `butt_member` throws on `BeamCurved`. `PlateCurved` has no
`trimmed_*` override. Write these first; each must produce an exact BRep ("Round means BRep"). If one fails, either fix the
class or ask the user to drop that section. Never ship a fragment or a picture of something that did not run.

### Joint pages

Every case section: keep the text and both pictures (`docs/images/elements` and the `joint_library` tile), replace the
one-line call with `\include{lineno}`. No new example; 76 sections in all, 62 on these pages and 14 on the element pages:

| Page | Sections to convert |
| --- | --- |
| `joint_beam_butt` | 2 |
| `joint_beam_connectors` | 3 |
| `joint_beam_cross` | 6 |
| `joint_beam_end_to_end` | 4 |
| `joint_beam_pins` | 2 |
| `joint_beam_top_side` | 1 |
| `joint_plate_cross` | 2 |
| `joint_plate_side_side_inplane` | 7 |
| `joint_plate_side_side_outplane` | 12 |
| `joint_plate_side_side_rotated` | 5 |
| `joint_plate_top_side` | 7 |
| `joint_plate_top_top` | 7 |
| `joint` | 2 (drill, cutter) |
| `joint_pin`, `joint_cut_plane` | 2 |
| element pages | 14, the "have" cells above |

The 7 sections with no example on disk today, and what each becomes:

| Page, section | Today | Becomes |
| --- | --- | --- |
| `joint`, Usage | snippet | removed; the drill section right below shows the same, complete |
| `joint`, Support seat | text only | `element_joint_support`, new |
| `joint_beam`, Usage | snippet | reused `element_joint_beam_cross_half_lap` |
| `joint_beam_cross`, Milled Half-Lap | snippet over three sub-cases | snippet removed; its three sub-sections have examples |
| `joint_beam_keyed_box`, Assembly | text only | stays: prose beside the page's example |
| `joint_plate`, Usage | snippet | reused `element_joint_plate_top_side_tenons` |
| `joint_plate_border`, Usage | snippet, no picture | `element_joint_plate_border`, already there, with its picture |

Also `joint_plate`, Parameters: its two-line snippet becomes `element_joint_plate_parameters`, already there.

### Page length

`\include` shows the whole file: a joint example is about 58 lines, 15 of them the description and build commands. Twelve
sections add about 700 lines to `side_side_outplane`. "Lean" means few sections, not short code, and `column_head`
already looks this way, so plain `\include` it is, with the full code visible (decision 12).

### Pages

Each page: one sentence, `## Constructors`, then the sections in the matrix order, each with one line, its picture and its
`\include{lineno}`. The `## Parameters` tables on element pages go, as `wood/CLAUDE.md` asks for none. Holes are explained
on each element's page, as a modifier, not as an element.

| New page | Replaces |
| --- | --- |
| `element_edge_straight.md` `{#elements_edge_straight}` | `element_beam`, `element_column`, `element_profile` |
| `element_edge_curved.md` | `element_beam_curved` |
| `element_edge_variable.md` | `element_beam_variable` |
| `element_face_planar.md` | `element_plate` |
| `element_face_curved.md` | `element_plate_curved` |
| `element_node_block.md` | `element_block` |
| `element_node_support.md` | `element_support` |
| `element_joint_cut_plane.md` | `element_cut_plane` |
| `element_joint_pin.md` | `element_pin` |

### Keeping it true

Two small checks, so a snippet cannot creep back:

- `tools/check_element_pages.py`, run in `wood-docs.yml` before Doxygen: fails on a ```` ```cpp ```` block outside
  `## Constructors`, on a case section without one picture and one `\include`, and on an `\include` with no `ADD_EXE`.
- A step in `wood-test.yml` runs every `element_*` target one after another, stopping on a failure. "Built and run" is then
  checked on every push, not trusted. CI builds all targets already; a few minutes more.

### Also touched

- `docs/elements.md` cards and `@subpage` list (lines 155-165).
- `CMakeLists.txt` lines 87-100, 427-439, and an `ADD_EXE` per new example.
- `tools/render_element_sheets.py` (names at 30-61 and the regexes at 98-99 and 119: `^element_edge`, `^element_face`,
  and `(_pins|element_joint_pin)$`).
- `tools/screenshot_element_docs.sh` lines 9, 19, 66 and `tools/shoot_elements_ui.sh` line 19: keep the back view for
  `element_edge_straight_column*`.
- `tools/crop_viewer_shot.py`, the usage line.
- `docs/templates/floor_07_elements.md`, `docs/templates/floor_model.md`, `docs/floor_parametric_model.md`, `README.md`:
  code listings that name `Column::head_blocks`, `Plate` and so on.

Old dated plans in `docs/plans/`, `pictures_to_shoot.txt` among them, are history and stay.

## 8. Order of work

Each step is one wood commit that compiles and passes. Files with old names are only ever renamed by `git mv`, one
step at a time, so a failure bisects to a step. Stage named paths; bump the superproject pointer after.

**Gates**

- **G1, every step**: take `build/.buildlock`; `cmake --build build --parallel 6`; `ctest --test-dir build
  --output-on-failure` (the 14 `WOOD_TEST_TARGETS`, `rust_elements`, `joint_library`, `reference_datasets`,
  `main_session_round_trip` among them); `build/main_all_datasets`. Compare against step 0.
- **G2, steps touching `src/proto` or `rust/`**: `tools/regen_proto.sh` (see section 11), then in `wood/rust` `cargo test -j8`, then in
  `session/session_viewer` `cargo check --all-targets -j8` and `cargo xtest`. This proves the unchanged viewer still
  builds. One heavy command at a time.
- **G3, before each push**: run the examples as `wood/CLAUDE.md` asks, then `bash/ci_status.sh --wait`. Session's CI
  is on that list, because it follows wood `main`.
- **G4, step 7**: every `element_*` target built and run, `tools/check_element_pages.py` clean, `cmake --build build
  --target docs` with no missing `\include`, and every new picture looked at: a hole with flat facets is a bug.

Push after each finished stage, not once at the end: the user follows progress on the live docs site. That standing
request covers wood docs work only, so it does not extend to `session`.

The mechanical steps use a script in the scratchpad that rewrites the class name only in code positions (`<Plate>`,
`Plate::`, `Plate&`, `Plate*`, `class Plate`, `wood_proto::Plate`), then `git grep -w <old>` is read by hand: `\bPlate\b`
also matches sentences such as "Plate thickness".

| Step | What | Size | Gate |
| --- | --- | --- | --- |
| 0 | Wait for other jobs. Baseline: ctest, and `std::cout << scene` of every `element_*` and `templates_*` example saved to compare | no edits | G1 |
| 1 | Add `NodeElement`. `Block`, `Support` to `NodeBlockElement`, `NodeSupportElement`: classes, files, protos, tags, legacy reads, accessors. Smallest, proves the mechanism. Add a load test: every tracked `.pb` loads into the right classes | 248 lines | G1, G2 |
| 2 | Add `FaceElement`. `Plate`, `PlateCurved` to `FacePlanarElement`, `FaceCurvedElement` | 1091 lines, the largest | G1, G2 |
| 3 | Add `EdgeElement` with `cuts`, `compute_planes`, `trimmed_*`, `chord()`. `Beam`, `BeamCurved`, `BeamVariable` to `EdgeStraightElement`, `EdgeCurvedElement`, `EdgeVariableElement`, the dispatch sites onto `EdgeElement`. `Column` stays a `WoodElement` with its own branches until step 5 | 720 lines | G1, G2 |
| 4 | All joints, names only: `Joint`, `JointBeam`, `JointPlate`, `JointAnnen`, `JointVidy`, and `Pin`, `ConnectorPart`, `CutPlane`, each once. Tags, `kind`, proto messages, the three `element_type_name()` checks, `tests/joint_elements.cpp:429`. Generated joints then sit under a group named by the new tag | 1,285 lines | G1, G2 |
| 5a | `EdgeStraightElement` gains `round_radius`, the statics, the `model_geometry_brep` branch, the `aabb` rule. Parity test: `Column` against `EdgeStraightElement` over vertical, tilted, rotated, hollow, round, off-centre, cut, transformed; compare mesh volume, face count, BRep faces and volume, box, `sections()` | about 300 lines by hand | G1, G2 |
| 5b | Move the 14 column sites and the about 35 radius sites. Remove the remaining `Column` dispatch branches (4 source files, plus the forward declaration and the accessor). `Floor::compute_head_blocks`. `Grid::to_column` picks `EdgeVariableElement` when leaning | about 80 sites | G1 |
| 5c | Legacy `"Column"` reader. Delete `wood_element_column.*`. `tests/rust_elements.cpp` compares a golden column by solid, not by payload bytes. Parity test reduced to measured numbers | one class gone | G1, G2 |
| 6 | `JointCutPlaneElement` becomes a `JointElement` (section 6) | 60 lines | G1, G2 |
| 6b | `wood/rust` and `session_viewer` land together (below) | 6 Rust modules into 5, 4 test files, 6 verb files into 5, 2 others, about 80 viewer lines | G5 |
| 7a | `git mv` 15 examples and their pictures, merge 11 pages into 9, tools, CMake | no new content | G1, G4 |
| 7b | Replace snippets by `\include` on the 76 sections that have an example; add the two checks | 30 pages | G4 |
| 7c | The 6 unproven curved cases first, then the 13 other new examples, each with its picture; push as they land | 19 examples and pictures, by hand | G1, G4 |
| 8 | Apply section 10 to `wood/CLAUDE.md`; propose skill text | text | none |

Total: 3,519 lines naming these classes in 380 files (`src`, `examples`, `tests`, `rust`, `docs/*.md`, `tools`, `CMakeLists.txt`); about 3,340 of them are scripted, the 175 `Column` lines are not. Steps 1 to 4 need four full rebuilds, since each touches
headers everyone includes. The real work is steps 5 and 7. Step 5 has the design risk; 7c can turn up class bugs and is the largest piece of
hand work after it, at 19 examples and shots.

Stop rule for step 5: if the parity test fails on anything but the base plane, stop and report instead of adjusting the test.

### Step 6b: `wood/rust` and `session_viewer` land together

**When.** Right after step 6, once the C++ classes are final and before the docs work. One session commit, no more.

**What session CI does.**

- `viewer-check` and `viewer-pages` clone wood's `main` tip, sparse `rust`, on every run.
- A wood push does not start them. They start on a session push touching `session_viewer/**` or `session_rust`, on a pull
  request, by `workflow_dispatch`, and through `viewer-pages` when `Session mini tests` finishes.
- The reviewer bot bumps `session_rust` on Mondays at 03:00 and 05:00 UTC, which starts `viewer-check`.
- `viewer-pages` needs `check` to pass, so a broken viewer never deploys: the worst case is a red run, not a broken site.

**Order: wood first, then session, by hand.**

1. Preconditions: the kernel and column jobs have landed, steps 1 to 6 are pushed and green, no session run is in
   progress, it is not Monday morning UTC, and **the user says yes to this session push**: the standing push request
   covers wood docs only.
2. Edit `wood/rust` and `session_viewer` together. The viewer's path dependency `../../wood/rust` sees the local crate.
3. Gate G5 below, all local, before anything is pushed.
4. Commit in wood, by named paths. Commit in session, by named paths only: the session tree holds other jobs' files.
5. `git push` wood, then `git push` session straight after. Do not wait for wood's CI in between: it does not build the
   viewer, and every minute between the two is a window where a session run would fail.
6. `bash/ci_status.sh --wait`. Then bump both pointers in the superproject with one `superproject:` commit.

**Do not use `pushmono` for this.** It pushes session before wood, so session CI would build the new viewer against the old
wood tip and fail. Its `-m` form also commits every tracked change, in a session tree full of other jobs' work.

**If a run starts in the seconds between the two pushes** it fails on names; re-run it once session is up. Red after both are
up is a real fault: fix forward in session; the site stays on its last good deploy meanwhile.

**Rejected.**

- Session first: its CI clones the old wood tip and fails, every time.
- Both names in the crate for a while (add new, move viewer, remove old): three pushes to avoid a window of seconds, and
  it is the leftover the user said no to. `Column` could not be kept that way in any case; it would stay a second struct.
- Pinning session CI to a wood commit: a better arrangement long term, since the superproject already pins both, but every
  wood push would then need a session bump. Not part of this rename.

**The one struct.** `Beam::from_profile(&Polyline, profile, name)` and `Column::from_profile(&Line, profile, rotation,
name)` become one function, mirroring the one C++ constructor:

```rust
impl EdgeStraightElement {
    pub fn from_profile(axis: &Polyline, profile: Vec<Polyline>, directions: Vec<Vector>, name: &str) -> Self;
    pub fn from_section(axis: &Line, section: &Polyline, name: &str) -> Self; // was Column::new
}
pub fn compute_up(axis: &Line, rotation: f64) -> Vector; // in wood::geometry
```

- A beam passes no directions, as before.
- A column passes `vec![compute_up(&line, 0.0)]`, which gives the same section as `Column::from_profile` (section 5,
  conflict 3). Without it a vertical section turns a quarter.
- The caller's `Line` becomes a two-point `Polyline`. Rust has no `transformed`, so `rotation` has no other job.

**`wood/rust` changes.**

- `beam.rs` and `column.rs` merge into `edge_straight.rs`; `beam_variable.rs`, `plate.rs`, `block.rs`, `support.rs` become
  `edge_variable.rs`, `face_planar.rs`, `node_block.rs`, `node_support.rs`. `lib.rs` exports only the new names. The crate
  has no `BeamCurved` or `PlateCurved` today, and needs none.
- `TYPE` becomes the new tags. `LEGACY_TYPE: &str` becomes `LEGACY_TYPES: &[&str]`.
- `EdgeStraightElement` takes `round_radius`; its `brep()` uses the existing `geometry::drill_brep` when that is set.
- `EdgeStraightElement` overrides `from_element`. The default decodes the payload under the tag's own type, and a `Column`
  payload may decode as an `EdgeStraightElement` without an error, with its `Line` read as a `Polyline`. For `"Column"` it decodes
  `proto::Column` and converts. A leaning one returns `None`; C++ reads those as `EdgeVariableElement`.
- `profile_ends` gates on `profile.len() < 2`, C++ on `profile.empty()`. A one-loop column would take another path than
  C++. Match C++: `is_empty()`.
- Tests: `common/mod.rs`, `cpp_to_rust.rs`, `rust_to_cpp.rs`, `features.rs`. The seven `tests/data/*.pb` stay, as a new
  old-scenes test. New fixtures, dumped by the renamed C++ classes, carry `reads_back`, which compares payload bytes.
  `rust_elements.pb` is regenerated with `WOOD_WRITE_GOLDEN=1`; the old file stays as `rust_elements_legacy.pb` and
  `tests/rust_elements.cpp` reads both, so a scene the old viewer wrote is proven to load.

**`session_viewer` changes.** The user's decision: the typed commands are renamed to follow the classes, not removed.
All in `src/app/command`; the old words do not stay as `Spec.aliases`.

| Command today | New command | Verb file today | Verb file new |
| --- | --- | --- | --- |
| `Element Beam` | `Element Edge Straight` | `verbs/element_beam.rs` | `verbs/element_edge_straight.rs` |
| `Element Column` | `Element Edge Straight Column`, an option of it | `verbs/element_column.rs` | merged into the same file |
| `Element Beam Variable` | `Element Edge Variable` | `verbs/element_beam_variable.rs` | `verbs/element_edge_variable.rs` |
| `Element Plate` | `Element Face Planar` | `verbs/element_plate.rs` | `verbs/element_face_planar.rs` |
| `Element Block` | `Element Node Block` | `verbs/element_block.rs` | `verbs/element_node_block.rs` |
| `Element Support` | `Element Node Support` | `verbs/element_support.rs` | `verbs/element_node_support.rs` |

`Element Attributes` and `Element Interactions` stay: they are view switches, not elements.

**The column option.** It uses the mechanism `Box` already has, an option spelled `<Verb> <Word>` with a bare verb that still
runs:

```rust
pub const SPEC: Spec = Spec {
    options: &["Element Edge Straight Column"],
    ..Spec::new(&["Element Edge Straight"], "...", parse)
};
```

- `parse` reads `Column` as the first word in any case, then the two lengths from the rest.
- Two static `Recipe`s in the one file, no change to `ElementTool`:
  - `Element Edge Straight`: lines or polylines, `Width 120`, `Height 200`, no direction, name `beam`, mitred at corners.
  - `Element Edge Straight Column`: lines, `Width 200`, `Depth 300`, `compute_up(&line, 0.0)`, name `column`.
- `wait_for_option` and `wait_after_option` stay false, as `Box`: Enter runs the bare verb, and the tool asks for values.
- `Element Edge Straight Column 200 300` on a selection creates at once, as `Element Column 200 300` did.

**Why not a toggle inside the running command:** it would mean new state in `ElementTool` for one verb, and the two cases
differ in defaults, picks and value names, not just one flag.

**Each verb changes.**

- `SPEC.names`, the hint, `USAGE`, `Recipe.name`, the struct (`ElementFacePlanar` and so on), the `wood::` type and its
  constructor. `Recipe.picks` and the `Recipe.options` buttons keep their words.
- `create(state, elements, what)`: `what` becomes `Edge Straight`, `Edge Straight Column`, `Edge Variable`, `Face Planar`,
  `Node Block`, `Node Support`. That makes the undo label `Element <what>`. The status line lower-cases it and adds an s, so
  it reads `Created 3 edge straights from its curves`. Accepted: plain, and `create` stays as it is.
- Element names are data and stay `beam`, `column`, `plate`, `block`, `support`, except `beam_variable`, which is the old class
  name: it becomes `edge_variable` here and in the C++ defaults (`wood_element_beam_variable.h:40,49`, `.cpp:14`).

**Help text**, the `SPEC` hints, in the same form as today:

| Command | Hint |
| --- | --- |
| `Element Edge Straight` | `Element Edge Straight (Column): pick lines or polylines, the axes · Width 150, Height 250 or two numbers change them · Enter creates a beam on each, mitred at its corners · Column: pick lines, Width 200, Depth 300, a standing column on each · Element Edge Straight 120 200 on a selection creates at once` |
| `Element Edge Variable` | `Element Edge Variable: pick closed sections in station order · Enter creates the edge, ...` (the rest as today) |
| `Element Face Planar` | `Element Face Planar: pick closed polylines, the outlines · Thickness 60 or a number changes it · Enter creates a plate on each · Element Face Planar 40 on a selection creates at once` |
| `Element Node Block` | `Element Node Block: pick closed polylines, bottom then top, then hole pairs in the same order · Enter creates the block · on a selection it creates at once` |
| `Element Node Support` | `Element Node Support: pick points or planes, where the supports stand · Enter creates a support on each · on a selection it creates at once` |

`USAGE` follows: `Element Edge Straight width height · Example: select lines or polylines, then Element Edge Straight 120 200`,
and `Element Edge Straight Column width depth · Example: select lines, then Element Edge Straight Column 200 300`.

**Completions need no edit.** They come from `Spec.names` and `Spec.options`, sorted by `REGISTRY`, and the tests in
`tests.rs` read `REGISTRY` rather than a list: `every_verb_is_offered_in_alphabetical_order` and
`every_name_and_option_is_title_case_words` cover the new names. Typing `Element E` now offers the edge commands, `Element F`
and `Element N` the others. The renames also remove today's prefix clash, `Element Beam` inside `Element Beam Variable`.

**Other files.**

| File | Change |
| --- | --- |
| `verbs/mod.rs` | lines 51-56: five new module names and their `register:` comments, `element_column` gone |
| `tool/elements.rs` | line 165 comment; line 442 test recipe; lines 524, 558 labels; lines 422, 557 `FacePlanarElement`; lines 501, 522 `EdgeStraightElement` with the fourth argument; line 571 asserts `"FacePlanarElement"` |
| `wood/rust/tests/common/mod.rs` | line 102 comment names the new command |

**Verb tests.**

| Test today | New test | Parse cases |
| --- | --- | --- |
| `element_beam_sweeps_the_rectangle_along_the_axis` | `element_edge_straight_sweeps_the_rectangle_along_the_axis` | `Element Edge Straight 100 200` ok, bare ok, three numbers refused |
| `element_column_lofts_the_rectangle_up_the_line` | `element_edge_straight_column_lofts_the_rectangle_up_the_line` | `Element Edge Straight Column 200 300` ok, `element edge straight column` ok, `... Column x` refused; add: width lies along x on a vertical line |
| `element_beam_variable_lofts_through_its_stations` | `element_edge_variable_lofts_through_its_stations` | `Element Edge Variable`; the "longest name wins" comment goes, as the clash is gone |
| `element_plate_lofts_the_outline_by_its_thickness` | `element_face_planar_lofts_the_outline_by_its_thickness` | the same cases with the new words |
| `element_block_lofts_bottom_to_top` | `element_node_block_lofts_bottom_to_top` | the same |
| `element_support_stands_on_its_plane` | `element_node_support_stands_on_its_plane` | the same |

The new width check is what guards `compute_up`: volume alone passes with the section a quarter turn off.

Nothing else names these commands: `git grep` finds them only in these files. `docs/journey/` is a history no CI runs, so it
stays, and `wood/tools` drives the viewer by `?cmd=` with view commands only.

**`Element Edge Curved` and `Element Face Curved` are not renames.** Neither command exists, and `wood/rust` has no
`BeamCurved` or `PlateCurved` to back them. Adding them means porting those classes to Rust, `wood_element_beam_curved.cpp` alone
being 1,660 lines, plus a design for what the user picks. Recommendation: reserve the names, keep them out of step 6b so it stays
one reviewable rename (decision 13). They stay reserved for a later job.

Two visible differences from the merge: the column's base plane sits at the axis base, not the section corner, and it
carries a section feature at each end. Volume and picture stay the same.

**Short commands: no.** `Edge Straight`, `Face Planar`, `Node Block` against `Element Edge Straight`. Recommendation: keep
`Element`, so a command reads `Element <category> <kind>` and the four words are its second word: `Element Edge Straight`,
`Element Face Planar`, `Element Node Block`, and `Element Joint ...` reserved. Reasons:

- `Edge` and `Face` are already commands: the selection modes, "click picks single edges". `Edge Straight` would parse, as the
  longest name wins, but the first word would mean two unrelated things, a picking mode and a timber member.
- The bare verb stops being alone: typing `Edge` today offers one command. It would offer four, and `Edge S` would run `Edge` with
  an argument and be refused.
- `session_viewer` is a general geometry viewer, where `Edge Straight` reads as drawing a line. `Element` says it is a wood member.
- `Element` keeps them together in the completion list, beside `Element Attributes` and `Element Interactions`.
- `Joint ...` has nothing to name yet: the viewer has no joint command today.

**G5, all local, before the first push.**

- `wood/rust`: `cargo test -j8`.
- wood: `rust_elements` and `floor_elements`, `build/rust_elements` reading the new and the legacy golden.
- `session_viewer`: `cargo check --target wasm32-unknown-unknown --lib -j8`, `cargo check --all-targets -j8`, `cargo xtest`.
  These are CI's three commands. The wasm target is installed here; CI's native target is Linux, this machine's is Windows.
- CI builds from the committed `wood_proto.rs`, not `src/proto`: `git show HEAD:rust/src/proto/wood_proto.rs` must hold
  `pub struct EdgeStraightElement` and no `pub struct Beam`.
- `git grep -wE 'Beam|Column|Plate|Block|Support|BeamVariable' -- wood/rust/src` shows nothing but legacy tags and
  `proto::Column`.
- Run `Element Edge Straight` and `Element Edge Straight Column` in the viewer once, by the picks and by typed numbers.

## 9. Deprecated aliases

**No aliases in C++.** Every C++ consumer of these classes is in this repo and moves in the same commit. `wood_nano`
does not build today, so an alias helps nobody; when it is repaired it should bind the new names. `compas_wood` does
not touch these classes. `Column` cannot be an alias anyway: its constructors differ. Aliases would also keep the old
names alive in new code, which is what the user wants gone.

The only transition is on the wire: old tags are read for good. `wood/rust` keeps its old public names through steps 1 to
6 only, and step 6b removes them with no alias left behind.

## 10. Text for wood/CLAUDE.md

Line 68, replace the first sentence:

```
- Every screw, dowel or pin is a `JointPinElement`: one cylinder along its axis, a child of its connector. No class, factory,
```

Line 217 onward, the section `## Elements: example, picture and page`, replace the lead-in and the example bullet, and add
the naming rule:

```
- An element class name starts with its category, ends in `Element` and derives from its base: `EdgeElement` (`EdgeStraightElement`,
  `EdgeCurvedElement`, `EdgeVariableElement`), `FaceElement` (`FacePlanarElement`, `FaceCurvedElement`), `NodeElement` (`NodeBlockElement`, `NodeSupportElement`) and `JointElement` for everything that
  modifies another element (`JointBeamElement`, `JointPlateElement`, `JointPinElement`, `JointCutPlaneElement`, `JointConnectorPartElement`). A base holds only
  what its classes really share. The library is these seven elements and the joints. Holes, sessions, plane cuts and joint
  assignment are not elements: they are sections on the element pages.
- A column is an `EdgeStraightElement` named `column_<i>`; a section is a profile it takes, never its own element.
- Every element class in `src/joinery_solver/wood_elements/` has all three, added or updated with
  every new element or constructor:
  - `examples/elements/element_<type>.cpp`: minimal like an API, one instance of the element and
    nothing else, registered as `ADD_EXE(element_<type> ...)`; a case that needs other elements (a
    column with glued blocks and cuts, an edge cut by a `JointCutPlaneElement`) is its own example,
    `element_<type>_<case>.cpp`, showing them through `add_interaction(source, target, feature)`.
```

The page bullet, replace its first line and add the section list:

```
  - `docs/elements/element_<type>.md` (`{#elements_<type>}`, `[TOC]` under the title): one sentence
    on what it is, a `## Constructors` section with the constructors as a code block, then one `##`
    section per case: holes, a session of several elements, cut by plane, assign joints. Every case,
    on element and joint pages alike, is a complete example that was built and run, shown by its
    picture and `\include{lineno} elements/<example>.cpp`; the constructors list is the only code
    block on a page, never a snippet. `tools/check_element_pages.py` enforces it.
```

and drop the old wording of that bullet down to "`docs/elements.md`. The sections fill the page outline on the right.".

The joint names, by rename only, at lines 14, 15, 17, 49, 69, 70, 77 and 126: `Joint` to `JointElement`, `JointPlate` to
`JointPlateElement`, `JointBeam` to `JointBeamElement`.

Everything else in `wood/CLAUDE.md` stays. "The key pattern: a joint is an element, and what it does is an interaction", the
`add` and `add_interaction` rules, `add_columns`, `add_column` and "the column" as words for a building part are not touched.

`wood/.claude/skills/wood-film-docs/templates/{chapter_example.cpp,movie.cpp,movie.h}` name the old classes and need the
same rename at step 8. `wood/.claude/agents/wood-agent.md` and `joint-from-photos/SKILL.md` do not.

## 11. Blockers to clear first

- **The user must install protoc 36.2.** The download was blocked, so it cannot come from here. Regeneration is wood only: `tools/regen_proto.sh` runs
  `session_cpp/.protoc/36.2/bin/protoc`, a gitignored cache; put the release there, or set `WOOD_PROTOC` to it. No `protoc`
  is on the path. Without it `generated/` (34 tracked files) cannot be regenerated, and steps 1 to 4, 5c and 6 all change
  protos. A mismatched version does not compile against the kernel's libprotobuf, so `generated/` must not be hand-edited.
  Do not run `session/bash/gen_proto.sh` whole: it regenerates the kernel's files too.
- **The kernel and column jobs land first.** Section 5, conflict 11.
- **The session push in step 6b needs the user's yes at that moment.**

## 12. Status and what is held

Steps 0 to 7 done; step 8 is proposed text for the lead to apply; step 6b waits for step 8 and the user's yes on the session push.

Differences from this plan as it was written:

- 18 new examples, not 19: `element_node_block_joints` would duplicate `element_joint_beam_centred_pins`, which pins two blocks.
- `FaceCurvedElement` keeps its patch mesh and takes a closed tessellation only when it has solid features. Tessellating every curved
  plate stalled or crashed the folding templates.
- Plane features on `EdgeCurvedElement` go through its `cuts` path, the one that trims the exact surfaces.

**Waiting on the kernel's exact boolean on curved faces, examples in the tree, sections and pictures out of the pages:** `element_face_curved_cut_plane`, `element_face_curved_holes`, `element_face_curved_joints`, `element_edge_curved_joints`, and `element_edge_curved_holes` for the same ribbed surface.

To restore one: remove it from `HELD` in the page list, shoot its picture, look at it.

