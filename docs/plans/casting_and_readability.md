# Wood: casting and readability audit

A read-only audit of `wood/src` on 2026-11-05. Nothing was built or run. It covers 207 files and
57,367 lines, without `build/` and the generated `*.pb.*`. Paths are under `wood/`.
It does not repeat the ten items of `wood_improvement_plan.md` section 4. It points to them where a
finding belongs to one.

How it was measured: Python scripts over the comment- and string-stripped source. Casts by regex,
each numeric cast classified by its argument. Functions by brace matching: 1,892 found. Nesting is
the depth of blocks inside the body, with a brace-less `if`/`for` counted as a level. Copy-paste
by hashing every window of five normalised non-trivial lines.

## 1. Casting census

| Cast | Count |
|---|---|
| `static_cast` (one is a `static_pointer_cast`) | 336 |
| C-style `(int)x`, `(size_t)x`, `(double)x` | 159 |
| `dynamic_cast` and `dynamic_pointer_cast` | 113 |
| functional `int(x)` | 2 |
| `const_cast` | 1 |
| `reinterpret_cast` | 0 |
| total | 611 |

By purpose:

| Group | Count | What it is |
|---|---|---|
| (a) type dispatch | 117 | `dynamic_cast` ladders and filters, and the `static_cast` downcasts after a check |
| (b) numeric | 446 | int, size_t and double conversions |
| (c) enum and int | 26 | enums built from ints, stored as ints or as double attributes |
| (d) protobuf | 8 | enums kept as `int32` in the `.proto` files |
| (e) const stripping | 1 | `wood_session.cpp:85` |
| (f) other | 13 | base slice assignment in loaders |

Top ten files:

| Casts | (a) | (b) | (c) | other | File |
|---|---|---|---|---|---|
| 88 | 69 | 11 | 2 | 6 | `src/joinery_solver/wood_session.cpp` |
| 34 | 12 | 19 | 3 | 0 | `src/joinery_solver/wood_algorithms/wood_contact_detection.cpp` |
| 30 | 0 | 30 | 0 | 0 | `src/templates/chevron/chevron_shifted.h` |
| 25 | 0 | 25 | 0 | 0 | `src/joinery_solver/wood_algorithms/wood_merge_modifier.cpp` |
| 24 | 0 | 23 | 0 | 1 | `src/joinery_solver/wood_elements/wood_element_edge_curved.cpp` |
| 19 | 0 | 19 | 0 | 0 | `src/joinery_solver/wood_algorithms/wood_feature_solver.cpp` |
| 17 | 0 | 17 | 0 | 0 | `src/templates/cylindrical_folding/cylinder_carving.h` |
| 17 | 2 | 13 | 2 | 0 | `src/joinery_solver/wood_elements/wood_element_joint_plate.cpp` |
| 15 | 0 | 15 | 0 | 0 | `src/templates/vault/vault.cpp` |
| 15 | 0 | 15 | 0 | 0 | `src/templates/cylindrical_folding/reflection_fold.h` |

Group (a) per file: `wood_session.cpp` 69, `wood_contact_detection.cpp` 12, `wood_instance.cpp` 11,
`wood_session.h` 7 (the typed lookup templates), `wood_element_joint_beam.cpp` 5,
`module_contact.cpp` 4, `wood_element_joint_plate.cpp` 2, `cassette_shell.cpp` 2, six files with 1.
The most tested types: `FacePlanarElement` 17, `JointElement` 9, `EdgeStraightElement` 7,
`JointBeamElement` 7, `InteractionFeatureBeam` 7, `InteractionFeaturePlate` 7, `WoodElement` 6,
`InteractionContact` 6.

## 2. Type dispatch: what would remove the ladders

### Tags that already exist

- `InteractionContact::kind()` and `InteractionFeature::kind()` return a `string_view`
  (`wood_interaction_contact.h:17`, `wood_interaction_feature.h:17`). Each class also has the
  kernel's `interaction_type_name()` string. Two string tags per class.
- `kind()` is compared as a string once: `contact->kind() == kind` (`wood_session.cpp:387`). It is
  otherwise only printed (`:1227`). It dispatches nothing.
- The doc of `InteractionFeature::kind()` names three kinds, "plate", "beam" and "plate_beam". There
  are five subclasses: also plane and solid (`wood_interaction_feature.h:16`).
- The kernel's `element_type_name()` is compared as a string to mean "a plain `JointElement`, not a
  subclass": `element_type_name() == "JointElement"` (`wood_element_joint.cpp:160, 203, 206`). A
  base class asking whether it is exactly itself is a virtual turned inside out.

### Why the ladders exist

There are two wood roots under the kernel's `Element`: `WoodElement` for members and `JointElement`
for joints. Wood cannot add a virtual to the kernel `Element`. So whatever both roots do is asked by
type. `JointBeamElement` keeps its own `solid_features` beside `WoodElement::solid_features`.
`EdgeElement::cuts` and `NodeBlockElement::cuts` are the same field twice, beside
`WoodElement::plane_features`, a second way to cut by planes (`wood_element_edge.h:14`,
`wood_element_node_block.h:22`, `wood_element.h:18`).

### The worst three ladders

**1. `WoodSession::add_interaction` and `add_joint_interaction`: 27 casts in 357 lines**
(`wood_session.cpp:877-1068`, `:1754-1918`). It tests the source as `JointElement`,
`JointPlateElement` and `JointBeamElement`, the payload as five feature classes and the contact base,
and then the flags `is_connector()` and `is_butt`. Plan item 6 and step 9 already name it. The
replacement is one virtual per behaviour on the feature base:

```cpp
class InteractionFeature : public Interaction {
public:
    /// The target's solid after this feature, given the source; stores nothing on the target.
    virtual BRep apply(const Element& source, const BRep& target) const = 0;

    /// The same feature seen from the other end of the edge.
    virtual std::shared_ptr<InteractionFeature> flipped() const = 0;
};
```

`add_interaction` then orients the payload when the edge runs the other way, stores it and
invalidates the target. The beam's volume swap (`:1043-1046`) becomes
`InteractionFeatureBeam::flipped`. Contacts already have `flipped()` and `coincides()`.

**2. `transformed` in `wood_instance.cpp:171-204`: 7 casts that the kernel's virtuals already do.**
Every wood class overrides the kernel's virtual `clone()` and `place()`. The kernel's
`Element::place` moves the features and insertion vectors (`session_cpp/src/element.cpp:464-482`).
`JointElement::transformed` is already clone and place (`wood_element_joint.cpp:276-283`). The ladder
and six hand-written `transformed` bodies, about 20 lines each, become:

```cpp
if (is_mirror(xform))
    return nullptr;
std::shared_ptr<Element> copy = element->clone();
copy->guid() = element->guid();
copy->place(xform);
return copy;
```

Two things to check first. `Element::place` calls `ensure_geometry()` before moving, so a copy
computes its solid where `transformed` did not: measure an instanced scene. And the copies differ
today: `FacePlanarElement::transformed` drops `plane_features` (`wood_element_face_planar.cpp:493-515`)
and `FacePlanarElement::place` does not move them (`:517-530`). `EdgeStraightElement::place` does
(`wood_element_edge_straight.cpp:431`). See Suspected.

**3. Contact detection: 12 casts, some in the inner face loop** (`wood_contact_detection.cpp:243-306`,
`:486-550`). `face_contacts_for_pair` reads `plate_a ? plate_a->planes : ea.planes()` four times
(`:496-499`). `FacePlanarElement` already overrides `compute_planes` and `compute_polylines` to return
those fields (`wood_element_face_planar.h:185-192`), so `ea.planes()` alone gives the same. The casts
stay for `add_cut_faces` (`:500-503`), but the four ternaries go. `is_plate` is a `dynamic_cast`
called per face pair inside the `i` by `j` loop (`:516`, `:517`, `:538`, through `triangle_contact`,
`outer_face` and `contact_type`). The pair classification in `contact_type` (`:278-306`) needs the
shape of both sides: edge, plate or other. That is double dispatch, and the one place a small enum
computed once per element beats a virtual.

### The choice: one virtual per behaviour

- **Virtual per behaviour** removes the cast and puts the behaviour on the class. Interactions have
  few behaviours (apply, flip, show as a feature) and wood owns both intermediate bases.
- **A `kind()` enum switch** still downcasts after every case. It is the same ladder with
  `static_cast`. The string `kind()` already exists and nobody dispatches on it.
- **`std::variant` with `std::visit`** does not fit storage. The kernel `Session` keeps
  `shared_ptr<Element>` and `shared_ptr<Interaction>`. A variant needs a parallel store or a wrap at
  every read.

What each removes, of the 117:

| Change | Casts removed |
|---|---|
| `apply` and `flipped` on `InteractionFeature` (step 9), `feature()` on `InteractionContact` (now) | about 31: the two ladders 27, `contact_feature` 3 (`wood_session.cpp:221-232`), `remove_interaction` 1 |
| the kernel's existing `clone` and `place` | 7: `transformed` |
| step 8, BRep only: `compute_breps` goes | 8 (`wood_session.cpp:1721-1730`) |
| step 9 folding geometry from edges: no target hosts `solid_features` or `cuts` | 8: `get_solid_features` 3, `stock_of` 1, `add_plane_cut` 2, `refresh_target` 1, `remove_interaction` 1; and the `const_cast` |
| left | about 63 |

The 63 left are what `dynamic_cast` is for. Typed lookups (`get_element<T>`, `world_elements<T>`,
113 call sites through the `wood_session.h:284-331` templates). "Not a joint" filters
(`wood_session.cpp:414, 633, 1264`). Templates picking their own types (`module_contact.cpp:17-37`,
`cassette_shell.cpp:646`). Same-type equality in `coincides` (three contact classes). Keep them.

## 3. Numeric casts: needed or noise

| Kind | Count | Verdict |
|---|---|---|
| int to double in arithmetic (`t = i / n`) | 149 | needed |
| double to integer by `lround`, `ceil`, `floor`, `min` | 40 | needed |
| 64-bit hash or grid keys | 6 | needed |
| iterator differences | 4 | needed |
| index between `int` and `size_t` | 141 | noise |
| `size()` to `int` for a compare or a store | 81 | noise |
| `i >= 0 && i < (int)v.size()`: an int id with -1 for none | 21 | sentinel, goes with a type change |
| a setting or a flag held as double or bool | 4 | typed wrong (plan item 4) |

Needed: 199. Noise: 222, in 48 files. The noise comes from ids kept as `int` or `vector<int>` while
loops run over `size_t`. `chevron_shifted.h` keeps cells and joints as `std::vector<int>`
(`:25-33`, `:103-127`) and casts every `size_t` loop variable back (30 casts). The rest:
`wood_feature_solver.cpp` 18, `wood_contact_detection.cpp` 15, `wood_merge_modifier.cpp` 13,
`chevron_assembly.h` 10, `wood_session.cpp` 9, `vault.cpp` 9. One index type per container
removes them.

The 21 sentinels: 8 in `wood_element_joint_plate.cpp:480-497, 650`, the rest in the plate designs
(`top_side_annen_shifted_tenons.h:105, 233, 267`, `side_side_outplane_annen_fingers.h:35, 106`).
They exist because face and element ids are `int` with -1 for none. `std::optional<size_t>` removes
both the cast and the check, but it changes `InteractionFeaturePlate` and the plate feature arrays.

A bool used as a number: `bool type0`, then `(int)type0 + (int)type1` and `type0 == 1`
(`wood_feature_detection_beam.cpp:126, 149, 165`).

## 4. Enum, protobuf, const and other casts

(c) enum and int, 26:

- **Grid enums stored as double attributes, 11.** `system`, `span`, `role` and `family` go through
  `face_attribute(...).value_or(...)` and back to `int` (`grid.cpp:739, 743, 987`,
  `grid_joints.cpp:17, 22, 23, 86, 449, 520, 525, 613, 663`). The plan already says these ints
  should be enums (`grid.h:58-69`).
- **`ContactType` built by adding two bools**: `static_cast<ContactType>(int(outer_face(a, i)) +
  int(outer_face(b, j)))` (`wood_contact_detection.cpp:306`). It works only because `side_side`,
  `side_top` and `top_top` are 0, 1, 2. Three casts; write the three cases.
- **`CrossDrills` from `id - 32`** (`wood_element_joint_plate.cpp:281, 2184`). Plan item 1's table.
- **`butt_ends` is `std::array<int, 2>` with -1 for none** (`wood_element_joint_beam.h:71`), cast at
  `wood_element_joint_beam.cpp:1161` and `wood_session.cpp:1840, 1844`. `std::optional<size_t>`.
- `c.joint_type = static_cast<int>(c.ctype)` keeps a `ContactType` as an `int`
  (`wood_feature_detection.cpp:147`). `names[(size_t)joint->configuration]` indexes a name table by
  enum (`wood_element_joint_beam.cpp:1153`). A `to_string` does both.
- Two are fine: an enum passed to the kernel's int API (`wood_profile_boolean.cpp:231`) and
  Manifold's status printed (`wood_solid_boolean.cpp:248`).

(d) protobuf, 8. Four enums are `int32` fields: `element_joint.proto:25`,
`interaction_contact_face.proto:16`, `interaction_feature_solid.proto:13`, `settings.proto:16`. One
cast on write and one on read each. A proto `enum` only turns them into enum-to-enum casts. Keep.

(e) `const_cast`, 1. `solid_features_of` strips const to call the mutable `get_solid_features`
(`wood_session.cpp:84-86`). It goes with the ladder under step 9.

(f) base slice assignment, 13. Seven loaders build a default derived element and then assign the
base: `static_cast<Element&>(*beam) = std::move(e)` (`wood_element_edge_curved.cpp:348`,
`wood_element_edge_straight.cpp:152, 598`, `wood_element_edge_variable.cpp:91`,
`wood_element_face_planar.cpp:363`, `wood_element_joint.cpp:362`, `wood_element_node_block.cpp:52`,
`wood_element_node_support.cpp:28`). `WoodSession` does the same with `Session&` five times
(`wood_session.cpp:124, 149, 162, 171, 1361`). It belongs to plan item 10's loader rewrite.

## 5. Readability beyond casts

### Length and nesting

| Lines | Count of functions |
|---|---|
| over 50 | 195 |
| over 100 | 41 |
| over 150 | 17 |
| over 200 | 6 |
| over 300 | 4 |

| Nesting depth | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| Functions | 621 | 513 | 418 | 232 | 78 | 26 | 3 | 1 |

The ten worst by length times depth, and what each splits into:

| Lines | Depth | Function | Splits into |
|---|---|---|---|
| 654 | 5 | `EdgeCurvedElement::trimmed_brep`, `wood_element_edge_curved.cpp:813` | ten lambdas make it a class in disguise: an envelope solver (`:827-933`), the miter solve (`:1012`), the cross-lap notches (`:1072`), a BRep topology builder (`:1146-1210`), then the six face groups it already numbers (`:1255-1418`) |
| 441 | 7 | `Truss::add_web_butts`, `truss.cpp:279` | the member graph (`:332`), layers (`:382`), the given hierarchy (`:457`), end lets (`:477`), crossing laps (`:582`), node butts (`:618`), lets by rank (`:688`) |
| 580 | 5 | `build_cylindrical_ribbed_vault_from_graph`, `vault.cpp:742`, 12 parameters | barrel surfaces (`:756`), adjacency (`:801`), corner heads (`:827`), through-ribs and cuts (`:962`), ribs (`:1098`), cross laps (`:1174`), panels (`:1235`); the floor pattern's `add_<part>` steps |
| 366 | 4 | `JointBeamElement::column_head`, `wood_element_joint_beam.cpp:1630`, 15 parameters | the head frame (`:1658-1707`), sizes (`:1718`), slot and housings (`:1739`), tongues (`:1778`), slot outline (`:1941`), posts (`:1956`) |
| 275 | 4 | `gothic_ribbed_vault`, `vault.cpp:1325`, 8 parameters | the same steps as the cylindrical vault; share them |
| 183 | 5 | `CassetteShell::add_studs`, `cassette_shell.cpp:429` | posts (`:461`), then uprights (`:485`), one function per face |
| 149 | 6 | `butt_joint`, `wood_element_joint_beam.cpp:1025` | one function per configuration it chooses: top-side (`:1044`), cross or lap (`:1048-1095`), one end (`:1096`), both ends (`:1104`) |
| 144 | 5 | `side_side_outplane_tenons`, `side_side_outplane_tenons.h:1`, 10 parameters, two bools | its callers pass positional geometry (see below); split by the two bools |
| 192 and 165 | 3 and 4 | `WoodSession::add_interaction` `:877` and `add_joint_interaction` `:1754` | `apply` per feature class (section 2) |
| 156 and 164 | 4 and 3 | `top_side_annen_shifted_tenons.h:186`, `top_side_annen_tenons.h:6` | the wall, the bound and the tenons; the two share the insertion and division lookups |

Plan item 6 names four of these (`trimmed_brep`, the cylindrical vault, `gothic_ribbed_vault`,
`column_head`). The other six are new.

### More than six parameters

74 functions. 24 are template constructors, which `CLAUDE.md` asks to take their parameters with
defaults. Of the other 50, 30 are in the solver and 20 in templates. The named joint factories
(`JointBeamElement::wedge` 12, `butterfly_key` 12, `keyed_box` 10, `headed_pins` 10,
`JointPlateElement::side_side_inplane_hilti` 15) take design parameters by design. The solver
internals do not need theirs:

- `plane_to_face` takes 12: eight are the two plates' outlines and planes, and the result is an out
  parameter behind a `bool` (`wood_contact_detection.cpp:745-758`). Pass the two plates and return
  `std::optional<InteractionContactCross>`.
- `face_to_face_wood` takes 10 with two out parameters (`wood_feature_detection.cpp:881-892`).
- `rotated_volumes` 10 (`wood_feature_detection.cpp:280`), `alignment_line` 9 (`:84`),
  `rotated_core` 9 (`rotated_core.h:12`), `annen_shifted_bound` 9
  (`top_side_annen_shifted_tenons.h:48`).

### Boolean parameters that select behaviour

51 functions take a `bool`. 233 call sites pass a literal `true` or `false`. 156 of them call the
kernel: `Mesh::loft` 27, `Intersection::line_plane` 25, `NurbsCurve::create` 15,
`Intersection::line_line_parameters` 11. That is the kernel's API to change, not wood's. 77 call 29
wood functions, and 4 call the `solve_miter_v` lambda. The worst:

- `side_side_outplane_tenons(jo, 0.00, true, true, -0.75, 0.5, -0.5, 0.5, -0.5, 0.5)`
  (`side_side_outplane_linked_tenons.h:11`), 8 sites.
- `crossed_side(member, end, true, within)` and `(…, false, …)`, 7 sites
  (`wood_element_joint_beam.cpp:1101-1127`). The bool is `facing`.
- `clipper_path` 5, `zigzag` 5 (`counter_fold.h:128`), `stepped` 4 (`chevron_shifted.h:695`),
  `meet_line` 4 (`reflection_fold.h:434`), `rib` 4 (`floor_guide.cpp:965`), `compute_pieces` 4,
  `compute_continuation` 4 (`grid_joints.cpp:152`), `clip_polygon` 4 (`reciprocal_surface.h:463`).

The fix is an `enum class` per flag, or two named functions where the flag picks the algorithm.

### Copy-paste

Of 35,843 non-trivial lines, 1,728 (4.8%) lie in a five-line window that repeats elsewhere. They
form 239 blocks.

- **Six static helpers copied between two files**: `unit_domain`, `corner_vertex`, `edge_between`,
  `patch_face`, `patch_centre`, `patch_normal` in `wood_element_edge_curved.cpp:77-102, 160-194` and
  `wood_element_face_curved.cpp:43-69, 143-184`. 39 shared windows. `CLAUDE.md` says a helper two
  files need lives once in the module's internal header.
- **Plate designs**: `side_side_outplane_fingers.h:31-79` and `side_side_outplane_graded_fingers.h:36-84`
  (38 lines); `top_side_tenons.h:21-31, 64-98` and `top_side_dense_tenons.h:32-89` (29 windows);
  `side_side_inplane_splayed_scarf.h:63-82` and `side_side_inplane_cross_splayed_scarf.h:103-124`;
  `side_side_inplane_butterfly_keys.h:61-80`, `side_side_inplane_tooth_keys.h:56-75` and
  `custom_outlines.h:30-46`.
- **Templates**: `reciprocal_move.h:135-156` and `reciprocal_rotation.h:120-141` (the frame beams,
  16 lines); `single_fold.h:83-101`, `triangle_fold.h:83-101` and `truncated_pyramid.h:121-139`;
  `side_tilt_boundary_ups` and `through_side_priority` each in both `reciprocal_boundary.h` and
  `reciprocal_surface.h`.
- **Within one file**: `truss_guide.cpp` repeats its `panel`, `top` and `bottom` lambdas per truss
  type (`:261-279`, `:326-344`, `:1053-1071`, `:1484-1503`, `:1659-1710`), 38 windows.
- The `clone`, `place` and `element_type_name` overrides repeat in 13 headers. They are required
  overrides.

## 6. Ranked

### (i) Mechanical, safe, worth doing now

Gate for each: oracle and reference goldens unchanged, every example's output identical.

1. **One copy of the six BRep patch helpers** in an internal header for EdgeCurved and FaceCurved.
   6 functions, about 90 lines fewer.
2. **`transformed` as `clone` plus `place`**: 7 casts and about 120 lines in six bodies. First settle
   the plate's `plane_features` and measure the `ensure_geometry` cost on an instanced scene.
3. **Contact detection reads `ea.planes()` and `ea.polylines()`**: 4 ternaries go. Plate or not is
   decided once per pair, not per face pair, which takes about six `dynamic_cast` out of the inner loop.
4. **`contact_feature` as a virtual `feature()` on `InteractionContact`**: 3 casts.
5. **Index noise**: 222 casts in 48 files. Start with `chevron_shifted.h` (30),
   `wood_feature_solver.cpp` (18), `wood_contact_detection.cpp` (15), `wood_merge_modifier.cpp` (13).
   One index type per container.
6. **Enums written as cases**: `ContactType` from two bools (3 casts), `bool type0` used as an int
   (2), `ContactType` kept as `int joint_type` (1), the `ButtConfiguration` name table (1).
7. **`butt_ends` as `std::array<std::optional<size_t>, 2>`**: 3 casts and the `< 0` check at
   `wood_session.cpp:1837`.
8. **String type tests gone**: `element_type_name() == "JointElement"` (3 sites) becomes a virtual the
   subclasses override; `kind() == kind` (1 site) compares types; fix the `kind()` doc.
9. **Bool flags on wood functions**: 77 sites in 29 functions. `crossed_side` (7) and
   `side_side_outplane_tenons` (8) first.
10. **`plane_to_face` and `face_to_face_wood` return their result**: 22 parameters become about 8.
11. **Template splits, one file each**: `Truss::add_web_butts` (441 lines, depth 7) into its seven
    commented steps; `CassetteShell::add_studs`; the shared steps of the two vaults; the
    `truss_guide.cpp` lambdas, the reciprocal frame setup and the three folding blocks once each.

### (ii) Needs step 9 or another plan step first

- **The `add_interaction` and `add_joint_interaction` ladders**: 27 casts, 357 lines. Step 9's `apply`
  and a `flipped` on features.
- **Targets hosting solid features and cuts**: 8 casts and the `const_cast` (`get_solid_features`,
  `stock_of`, `add_plane_cut`, `refresh_target`, `remove_interaction`). Step 9 folds
  geometry from the edges. Merge `EdgeElement::cuts` and `NodeBlockElement::cuts` into plane
  features then.
- **`compute_breps`**: 8 casts. Step 8 makes every element a BRep.
- **Sentinel ids**: 21 casts. `std::optional<size_t>` in `InteractionFeaturePlate` and the plate
  feature arrays, with plan items 1 and 4.
- **Settings and grid values as doubles**: 4 plus 11 casts. Plan item 4 and the grid enums.
- **`CrossDrills` from `id - 32`** and **the copied plate designs** (fingers, tenons, scarfs, keys):
  plan item 1's family table.
- **Solver splits**: `trimmed_brep` with plan items 2 and 7, `column_head` and `butt_joint` with plan
  item 3's named factories.
- **Base slice assignment in loaders**: 13 casts. Plan item 10.

### (iii) Not worth it

- The 199 needed numeric casts: arithmetic, rounding, keys, iterator offsets.
- The 8 protobuf boundary casts.
- About 63 `dynamic_cast` left after (i) and (ii): typed lookups, filters, `coincides`.
- The 156 literal bools passed to kernel calls. Propose to the kernel if anywhere.
- Long parameter lists on template constructors (24) and on named joint factories: by design.
- The `clone` and `element_type_name` boilerplate in 13 headers: CRTP would hide it and read worse.
- `std::variant` for elements or interactions: the kernel stores base pointers.

## Suspected

- A plate's plane features are not moved by `place` and are dropped by `transformed`
  (`wood_element_face_planar.cpp:493-530`). Edges and blocks move theirs. `add_interaction` puts a
  plane feature on any `WoodElement` target (`wood_session.cpp:983-1009`), so a placed or instanced
  plate with a plane cut would keep it in the old place. Not run.
