# Wood: what is wrong with it and the plan to fix it

Written 2026-11-05 from two days of work on the whole project: docs, pictures, three joints
rebuilt from photographs, the element rename, the vault speed-up and the kernel's booleans.
Everything under "What is wrong" was seen, not guessed. A separate read-only review adds
`file:line` smells; its findings go under section 4 when they arrive.

## 1. What is good, and must stay

- The pattern: a joint is an element, what it does is an interaction, `add` and
  `add_interaction` and nothing else. It held through every joint built this week, including
  a five-member node, without a special case.
- Constructor as recipe: a template reads top to bottom as the design. The floor is the model.
- Exact BRep for round parts, and now a native boolean that cuts real timber sizes.
- Pictures are real viewer screenshots, and every docs page shows a program that was built and
  run. There is now a check that keeps fragments out.
- The library is small and named for what it is: seven elements and the joints.

## 2. What is wrong

Ranked by how much it cost this week.

1. **Failures hide instead of failing.** `BRep::cut_by_plane` returned an empty solid on curved
   faces with no error, so the viewer drew nothing and nobody knew. `from_contact` swaps the
   joint design silently when a design severs a member. `check_volume` threw a good bored solid
   away because `volume()` read 1 percent low, and the beam fell back to plain faces: that was the
   whole "0 exact bores" CI failure. The column head was hand-built from mesh faces because the
   boolean threw and the fallback was accepted. Every one of these was found by looking at a
   picture, not by a test.
2. **Tests measured volume, not meaning.** The keyed box passed 180 lock checks while its keys
   went in sideways. `column_head` gave a wrong joint for any rotated beam, silently, because it
   read one beam's axis. `joint_library` ran for days at 124 of 135 with ids 14 and 15 failing
   as an accepted baseline; `reference_datasets` at 48 of 49. A baseline with known failures is
   not a test, it is a list of things nobody will fix.
3. **Memory bugs that Windows hides.** `strip_samples` looped over a freed temporary, giving
   random volumes on Linux since the native BRep landed; MSVC happened to keep the old bytes, so
   Windows green proved nothing. `floor_elements` overflowed the Windows stack. Nothing ran the
   tests under a sanitizer, on either platform.
4. **Over-general geometry.** The vault took 119 seconds because planar ribs were marched as
   general curves 50,000 times per rib, `place` rebuilt a mesh it threw away, and a mitre was a
   40-step search for what two vertical planes give directly. It is 0.4 seconds now. The same
   habit is likely elsewhere: the reflection folds took 18 seconds per lamella in the viewer at
   default samples, and `EdgeCurvedElement` is 1,600 lines.
5. **Settings as positional arrays.** `joint_parameters[11] = library_id(...)`,
   `joint_parameters[0] = 20`, `joint_volume_extension = {0, 0, -10}`, rows of three per joint
   family, integer ids 26, 33, 34 beside a name table. A caller cannot tell what a number means
   and the compiler cannot help. The finger-depth job spent its first hour finding which number
   sets what.
6. **Examples and tools that multiplied.** 30 of 186 template examples varied one parameter and
   were deleted without loss. Element pages showed snippets, not programs. `tools/` had five
   screenshot scripts and three picture pipelines, one of them a native renderer that nobody
   wanted. Pages carried hand-copied `main()` blocks that drifted from the files.
7. **Repo hygiene.** `generated/` and `wood_proto.rs` are committed and CI builds from them,
   while a rule said never to commit `wood_proto.rs`. protoc is pinned to a version not on the
   machine. `session_cpp` shows 168 modified files that are line-ending noise, so `git status` is
   useless there. Two agents from different vendors edit the same working tree.
8. **The viewer knew wood.** Six creation commands compiled the viewer against `wood/rust`, so
   session CI cloned wood `main` and every wood rename needed a paired session push. The user
   has decided these commands must not exist; the removal is in progress.

## 3. The plan

Each step has a gate. A step is done when its gate passes in CI, not on one machine.

### Step 1: nothing fails silently (1 to 2 days)

- Every kernel and wood path that could return "nothing" throws instead: `cut_by_plane`,
  `boolean_difference` fallbacks, `from_contact`'s design swap, `check_volume`, `drilled_brep`.
  A fallback to a mesh is allowed only when the caller asks for one by name.
- `volume()` on a BRep either returns the exact volume or is renamed `mesh_volume()`; no caller
  may judge a solid by a number that reads 1 percent low.
- Gate: `git grep` finds no `return BRep()` or `return Mesh()` on an error path without a
  throw; every example and test still runs; the docs pictures are unchanged.

### Step 2: tests that mean something, and run everywhere (2 days)

- No accepted-failure baseline: `joint_library` and `reference_datasets` go to all-pass or the
  failing designs are fixed or removed. ids 14 and 15 first.
- Assembly tests for every joint, as the keyed box and column head now have: seated overlap
  zero, insertion along its direction free, every other move blocked, and the insertion
  direction checked against the design, not only volumes.
- Linux CI runs the C++ tests under AddressSanitizer and UBSan, with a longer limit for the
  boolean groups. The `spatial_bvh.cpp` stack-use-after-scope report gets explained or fixed.
- Gate: Wood Test, Build C++ and Session mini tests green on all platforms with no known
  failures listed anywhere.

### Step 3: typed settings (2 days)

- `Settings` becomes structs: one per joint family with named fields (`division_length`,
  `shift`, `design`), `joint_depth_scale` and `joint_volume_extension` as named members with
  units in the comment, library designs as an enum with the id table kept only for reading old
  files. The yaml keys follow the names.
- `library_id("...")` throwing on an unknown name stays; magic index arithmetic goes.
- Gate: every example reads as prose; old datasets and `.pb` scenes still load; reference tests
  unchanged.

### Step 4: simple geometry where the geometry is simple (3 to 5 days)

- Audit every template and curved element for marches, bisections and searches, starting from
  the vault profile and `EdgeCurvedElement`. Where a rib is planar, a plane is vertical or a
  profile is an arc, compute directly. Keep the general path only where the input is general.
- Every example runs in under a second and opens in the viewer in under a minute, measured and
  recorded in the recipe.
- Gate: baseline comparison of all BReps within a stated tolerance, as the vault had.

### Step 5: one tool for pictures, one for pages (1 day)

- `tools/`: one screenshot tool, one recipe file per docs page family, the sheet and landing
  composers. Delete `shoot_*.sh`, `ui_shot.rs`, `wood_shot.rs`, the native renderer path.
- `check_element_pages.py` extended to template pages: every picture comes from a recipe line.
- Gate: every docs picture can be regenerated by one command and a diff shows what changed.

### Step 6: repo hygiene (half a day)

- `.gitattributes` fixing line endings for `session_cpp` and wood; `generated/` and
  `wood_proto.rs` produced in CI from `src/proto` with the pinned protoc, and the never-commit
  rule retired or enforced, not both.
- One working tree per agent: a vendor that edits the shared tree gets its own worktree.
- Gate: `git status` clean on a fresh clone after a build.

### Step 7: the viewer knows one `Element` (in progress)

- The six wood creation commands removed; the viewer compiles without `wood/rust`; session CI
  stops cloning wood.
- Gate: every wood scene draws as before; session CI green without wood.

### Step 8: BRep only (decided 2026-11-05)

The user's decision: wood uses only BRep; the viewer deals with mesh conversion itself. So
an element has one geometry, its BRep. The mesh is not a model anywhere in wood: not stored,
not cut, not saved. The viewer tessellates what it draws.

- Measure first: every dataset and example run with exact booleans only, time per feature and
  every failure recorded, so the speed work below has a benchmark and the switch is proven
  before Manifold goes.
- Kernel speed: planar faces against planar faces take a direct polyhedral path; the general
  NURBS path stays for curved parts. Booleans run only when geometry is read.
- Elements: `model_geometry_brep` is the geometry; `model_geometry_mesh`, `mesh_brep`,
  `apply_solid_features` on meshes and the Manifold feature path go. Contacts, bounds and
  overlap checks read BRep faces and planar outlines; tests measure with exact volumes.
- Scenes: `pb_dump` writes BReps only. The viewer tessellates on load; its speed on the vault
  and floor scenes is measured and fixed in the viewer, not by writing meshes again.
- Where the mesh lives (decided 2026-11-05): the kernel's `Brep` owns its trimmed mesh. `Brep::mesh()`
  returns a cached triangulation of the trimmed faces, computed on first read and dropped when the
  BRep changes; it is derived data, never serialized. The same member exists in the C++, Python and
  Rust kernels. Wood's examples and the viewer read the mesh from the BRep, so the API is BRep-only
  and both live in the same object.
- Gate: oracle and reference goldens unchanged, every example's solids closed and exact,
  every scene opens in the viewer in the time recorded in the recipe.

### Step 9: one `apply` per interaction, geometry folded from the graph (from the compas_model comparison)

`docs/plans/compas_model_comparison.md` measured it: following one centred-pin joint from contact to
cut takes about 12 files and 10,000 lines, and what a joint does is decided twice, once when it builds
its payload and once in the session's 357-line type ladder. The one thing worth taking from
compas_model is the shape of its `Modifier.apply`.

- Every `InteractionFeature*` gets one virtual `apply(source, target BRep)`. `add_interaction` only
  stores the interaction on the edge and invalidates the target; the ladder in `wood_session.cpp:877-1068`
  and `:1754-1918` goes. This is review item 6's fix.
- An element's geometry is computed on read from its own BRep and its incoming edges in insertion
  order, so the copies hosted on targets that `remove_interaction` has to erase in three places go.
  Invalidation runs through setters and reaches dependents along outgoing edges.
- Contact provenance: computed and user-authored contacts are distinguished, and recomputing contacts
  never wipes named ones.
- Contacts keep every overlap region, not only `solution[0]`.
- Beam axis contacts go through `SpatialBVH` instead of every pair against every pair; the unused
  world-size code goes; the kernel's traversal stack gets a real bound.
- Gate: oracle and reference goldens unchanged, every example's output identical, the floor template
  unchanged in its text.

### Step 9b: contacts measured against Coal (`coal_contacts.md`, 2026-11-05)

Contact detection is 0.6-11 % of any run (grid 41 ms of 390, floor 3.9 s of 83.6 and that is
the first `planes()` of 866 cut members, datasets 317 ms of 56.7 s, truss none). So the Coal
study found nothing to speed up and one correctness bug: the kernel's
`Intersection::line_line_parameters` clamps both segment parameters independently
(`intersection.cpp:394-397`); on skewed beams the contact point lands 53 mm off and the gap is
overstated by 3.4 mm, so contacts near `min_distance` are dropped.

- Take now: Ericson's segment-segment closest points in all three kernels, callers listed,
  axis contact count changes listed pair by pair.
- Take later: a kernel `gjk.{h,cpp}` (about 450 lines, no Eigen, exact cylinder support) for a
  model-level overlap check with penetration depth, which wood lacks today; cached face lists
  (about 50 ms); a dynamic AABB tree at about 10^4 elements.
- Rejected with numbers: tighter bounding volumes, dual-tree traversal, sweep-and-prune, GJK
  before Clipper2, Sutherland-Hodgman instead of Clipper2.
- Open design question: the floor's 4 s is cut members being evaluated for contacts; if
  contacts may use uncut members it goes away.

### Step 10a: joints that lock (from the insertion check, 2026-11-06)

Step 2's new oracle check measures, per design, which moves the male can still make with the
joint seated. It found real geometry defects, not thresholds (`docs/plans/joint_insertion_defects.md`
once `fix-ci-tests` lands):

- ids 14/15 (side-side out-of-plane tenons) cut outside the stock: floor edge 40 mm past the
  wall's outer face, chamfered tips 30 mm proud. User's decision: fix the tile and regenerate the
  2025 reference of every dataset that uses them, named, with before/after volumes.
- `side_side_inplane_dovetails` at the default shift 0.5 has straight fingers and pulls out of
  the seam; only shift 1.0 locks. The datasets use 0.5.
- `side_side_inplane_splayed_scarf` and `jupiter_scarf`: the key or peg does not cross the seam,
  the male still slides.
- `side_side_rotated_hooks` and `top_top_custom`: keys sit wholly inside the plates, no pocket cut.
- `cross_half_lap` at shift 0.5: the second plate cannot be lifted out along the crossing.
- `hex_block_rossiniere`'s 2025 golden is invalid geometry (outlines jumping across the plate).

Order: `fix-joint-registry` lands first (it rewrites the tile headers), then one agent fixes the
tiles one by one, geometry at its cause, goldens regenerated by name. Until then the oracle
carries an explicit known-defects table that can only shrink.

### Step 10: fewer casts, shorter functions (from `casting_and_readability.md`)

611 casts in `src/`: 117 type dispatch (69 in `wood_session.cpp`), 446 numeric of which 222 are
`int`/`size_t` noise, 26 enum arithmetic. 41 functions over 100 lines, 74 with more than six
parameters, 77 call sites passing literal bools.

- Dispatch: one virtual per behaviour; the `add_interaction` ladders go with step 9.
- Now: the six BRep patch helpers copied between `edge_curved` and `face_curved` into one header;
  the instance `transformed` ladder becomes `clone()` + `place()`; `ContactType` and the other
  enums built by arithmetic written as cases; `-1` sentinels become `optional`; the string type
  tests become virtuals; the 222 index casts; `plane_to_face` (12 parameters) and
  `face_to_face_wood` return results; `Truss::add_web_butts` (441 lines) and the other long
  template functions split; literal-bool parameters on wood's own functions become enums.
- Check: `FacePlanarElement::place` does not move `plane_features` and `transformed` drops them
  (`wood_element_face_planar.cpp:493-530`), unlike edges and blocks.
- Gate: every example's output identical, all ctest targets, counts unchanged.

### Later, after the above

- Curved elements with solid features now cut exactly; the five held docs sections return as
  that lands. The oblique-plane and plate-drill residuals (1.4e-6 and 2.5e-6 mm3) are recorded in
  the tests and the 2.5e-6 one is unexplained: find it.
- `EdgeCurvedElement`'s own `cuts` path and the kernel boolean do the same job; keep one.
- Python and Rust kernels: the `Closest` seam bug existed in all three. Parity checks should
  compare outputs on shared fixtures, not only method lists.

## 4. Smells with `file:line`

From a read-only review of `main` on 2026-11-05: `git grep` and reading, nothing built or run.
Everything is verified unless it is under "Suspected". Paths are under `wood/` unless they name
`session_cpp`.

### What is good

- The floor template follows its own recipe and throws naming the contact when a factory gives
  nothing (`src/templates/floor/floor.cpp:401-545`).
- `wood_core` is one OBJECT library with a precompiled header, so the 170 targets compile the
  solver once (`CMakeLists.txt:168-193`).
- The joint oracle checks closed outlines and solids, no interpenetration, conservation, fit,
  protobuf round trip and rigid motion.
- Both baselines are ratchets: a variant that passed and now fails breaks CI.
- `JointPlateElement::library_id` throws with a useful message on an unknown name
  (`wood_element_joint_plate.cpp:730-745`).
- Line endings in the index are all LF (`.gitattributes` `text=auto`).

### The ten that matter most, ranked

1. **The seven joint families are written about eight times, and two design registries have
   already drifted.** Integer ids and the 2024 aliases (`ss_e_ip_1` ...) in `LIBRARY_IDS`
   (`wood_element_joint_plate.cpp:20-113`); dispatch by id in the `build_*` switches (`:159-389`);
   a second dispatch by name prefix in `compute_library` and the `compute_side_side_inplane`,
   `compute_top_side` ... if-chains (`:1543-1584`, `:1689-1712`, `:1967-2000`); the family
   mapping four times: `family_of_type` (`:132-151`), `parameter_row` (the same, defaulting to 1
   instead of -1, `:511-530`), `families = {12, 11, 20, 30, 40, 13, 60}` (`:813`),
   `build_family_default` (`:392-418`); and the name tables `joint_type_name`
   (`wood_view.cpp:32-50`), `contact_type_name` (a copy of `to_string(ContactType)`) and the
   `Settings::custom` family strings with `"b"`. The drift is already there: the bridle tongue is
   1/3 hard-coded on the id path (`:256`) and `parameters.z` on the name path (`:1991`);
   `top_side_annen_shifted_tenons` takes divisions 0 on one path and `parameters.divisions` on
   the other; `contact_type_name` has no `border` and returns "unknown" for it
   (`wood_view.cpp:10-30`); `joint_type_name` has no 60; `plate_joints.h` calls type 60
   "b: beam" when it is border. The same design gives different geometry by the way it is
   reached, and a new design touches about six places. Fix: one table
   `{name, enum class JointFamily, builder}` and an `enum class JointType`; the 2024 ids only
   as an import map. Size M to L, about 300 lines fewer.

2. **Round parts fall back to faceted mesh without a word.** `solid_features_brep` ends in
   `mesh_brep(apply_solid_features(...))` with polygon drills (`wood_element_geometry.cpp:879-885`);
   `EdgeCurvedElement::trimmed_brep` has eight `return mesh_brep(trimmed_mesh())` exits
   (`wood_element_edge_curved.cpp:818, 824, 958, 979, 1008, 1070, 1109, 1112`); the variable edge
   is always a meshed BRep (`wood_element_edge_variable.cpp:207`), and so are
   `wood_element_joint.cpp:202, 225` and `wood_element_joint_cut_plane.cpp:48`; `drilled_brep`
   returns empty for "unsupported intersections" (`wood_brep_drill.h:31`). Nothing logs or marks
   it, and it is the bug class of October. Fix: an `exact` flag or a throw, and a test that every
   drill in a dumped scene is a cylinder face. Size S for the flag and test, M to remove the
   fallbacks.

3. **Beam joints swap or drop the asked-for design, and the design is chosen through global
   settings.** In `JointBeamElement::from_contact` (`wood_element_joint_beam.cpp:157-275`) every
   end-to-end pair at an angle becomes `side_side_rotated_miter` whatever was asked (`:189-216`);
   a design that fails a volume heuristic is rebuilt as `top_side_milled_wedge` by writing
   `single.joint_parameters[8]`, and the retry ignores what `beam_to_beam` returns (`:256-271`);
   `compute_joinery` returns quietly when nothing is detected (`:73-74`);
   `compute_beam_features` skips with `if (!joint) continue` (`wood_session.cpp:573`). A
   constructor does the same job and throws (`:35-52`). The twelve beam element examples pick
   their design with `scene.settings.joint_parameters[11] = ...` and a degenerate
   `InteractionContactAxis(Line((0,0,0), (0,0,0)), 0.5, 0.5, 0, 0, 0, 0)`. Fix: one named factory
   per design (`JointBeamElement::cross_half_lap(a, b, contact, ...)`) that throws naming the
   design and the pair; drop the swap or make it an argument. Size M.

4. **Settings are positional, and defaults take over silently.** `joint_parameters` is a flat
   `vector<double>` of 21 read as `row * 3 + col` (`wood_settings.h:19-22`); its defaults are
   copied in `PARAMETER_DEFAULTS` (`wood_element_joint_plate.cpp:533`); a vector shorter than 21
   replaces every user value by the defaults with no message (`parameters_ok`, `:549`); an
   unknown joint type reads row 1 (`:527`); ids are stored as `double`; `chevron.h:315` writes a
   tab length into `[6]`, the top-side division length; `Settings::beams` is a positional `[6]`
   (`wood_datasets.cpp:214-219`); `distance` and `distance_squared` are set apart
   (`wood_settings.h:32-33`, `wood_config.cpp:212`); comments end mid-word, "6=b." and
   "Additive [width." (`wood_settings.h:22-23`). Fix: `std::array<FamilyParameters, 7>` by the
   family enum, a `BeamSettings` struct, `distance_squared` derived. Size M.

5. **Caches invalidated by hand and objects built in several phases, against "the constructor
   is the recipe".** Public mutable `cuts` and `cross_laps` say "call invalidate_geometry() after
   assigning" (`wood_element_edge.h:14`, `wood_element_edge_curved.h:36`,
   `wood_element_node_block.h:22`), and templates do it by hand (`grid.cpp:379, 403, 576`,
   `vault.cpp:329, 1220, 1230`, `framed_panel.cpp:278`, `cassette_shell.cpp:336`).
   `JointPlateElement` has four constructors, four `orient` and three `construct` overloads,
   `from_library` and `side_to_top`, all changing it after construction
   (`wood_element_joint_plate.h:233-283`). `merge_deferred` is not exception safe
   (`wood_feature_solver.cpp:317-354`). `add_interaction` runs `merge_features` again on every
   plate joint it adds (`wood_session.cpp:938-941`). Fix: immutable elements, derived
   constructors (`with_cuts`), an RAII guard for `merge_deferred`. Size L; JointPlate first.

6. **God classes and long functions.** `WoodSession` has 92 methods over IO, detection,
   contacts, instances, naming and visibility. `add_interaction` is a 192-line `dynamic_cast`
   ladder that also changes the caller's interaction (`wood_session.cpp:877-1068`, changes at
   `:931-935`); `add_joint_interaction` is 165 lines. `EdgeCurvedElement::trimmed_brep` is 654
   lines (`wood_element_edge_curved.cpp:813`), `build_cylindrical_ribbed_vault_from_graph` 580
   (`vault.cpp:742`), `gothic_ribbed_vault` 275 (`vault.cpp:1325`), `JointBeamElement::column_head`
   366 (`wood_element_joint_beam.cpp:1630`). `JointBeamElement` is a tagged union of beam joint,
   connector and butt fields with an `is_butt` flag and comments that say only "A butt's."
   (`wood_element_joint_beam.h:44-79`). `JointPlateParameters` is a grab-bag whose names lie: the
   bridle's tongue is kept in `z` and its shoulder in `shift` (`wood_element_joint_plate.cpp:1945-1950`),
   the Annen angle in `taper` (`:1816`), and `distance_squared` is written and read back but never
   used (`:2344`, `:2378`). Fix: split by concern, one virtual "apply to target" per interaction
   kind, butt joints and connectors as their own classes. Size L.

7. **Searches where the geometry is direct.** EdgeCurved runs a 40-step bisection over v
   (`wood_element_edge_curved.cpp:1032`) whose every step calls `front_envelope`, which
   root-finds every cut plane by an 8-step scan and up to 64 false-position steps (`:861-910`);
   `bisect_front` and `bisect_back` are near copies; the kernel has `Intersection::curve_plane`
   (`session_cpp/src/intersection.h:206`). The floor guide scans 85 degrees either side in 0.5
   degree steps (`floor_guide.cpp:665`), then bisects up to 200 times (`:738`), and finds the rib
   start by secant (`:487`). The vault samples its surface edges and interpolates them again
   (`vault.cpp:665-673`). Size M each.

8. **Kernel math written again in wood.** Newell's normal three times:
   `compute_newell` (`wood_element_geometry.cpp:58`), `rust/src/geometry.rs:15`,
   `src/templates/template_chamfer.h:12`, beside the kernel's `Polyline::newell_normal`
   (`session_cpp/src/polyline.h:441`) and `Vector::average_normal` (`vector.h:218`).
   `compute_volume` (`wood_element_geometry.cpp:136`) against `Mesh::volume` (`mesh.h:872`); a
   static `signed_distance` (`wood_element_geometry.cpp:429`) against `Plane::signed_distance`;
   `cgal_base1` (`:74`); `drill_mesh` beside `Primitives::cylinder_mesh`. Clipper2 is compiled by
   wood (`CMakeLists.txt:162-166`) and called in six files, 39 times, beside the kernel's
   `BooleanPolyline`; `face_overlap_area` keeps only `solution[0]` and drops every other overlap
   region (`wood_contact_detection.cpp:439-444`). `wood_brep_drill.cpp` is 1,279 lines of BRep
   building in wood; `rust/src/geometry.rs` repeats the C++ helpers in 746. Size M.

9. **CI tests less than it says, and the baselines keep broken geometry.** Seven of the fifteen
   ctest targets never run in CI: `joint_elements`, `interaction_ownership`, `floor_elements`,
   `brep_drill`, `rust_elements`, `element_tags`, `main_session_round_trip`
   (`CMakeLists.txt:311-327` against `.github/workflows/wood-test.yml`); `grid_massing` runs in CI
   and is no ctest target. CI never runs `ctest`, never builds or tests `wood/rust` and runs no
   template example. It fetches `session_cpp` at unpinned `main` (`CMakeLists.txt:49-53`), so it
   tests another kernel than the superproject pins. `CMAKE_BUILD_TYPE` is forced to Release
   (`CMakeLists.txt:5`), so the `-g` and ASan run the CI comment plans cannot happen. The Manifold
   patches are string `REPLACE`s that do nothing, silently, when upstream changes
   (`CMakeLists.txt:213-232`). All eleven `joint_library` failures (124 of 135) are every variant of
   ids 14 and 15, `side_side_outplane_tenons` and `side_side_outplane_linked_tenons`, and their
   goldens are committed; a missing golden is written and passes
   (`tests/joint_library.cpp:1536-1547`). `reference_datasets` is 48 of 49 without
   `hex_block_rossiniere` (`tests/golden/reference_2025/matched.txt`). No check looks at plate
   joint insertion or assembly direction (`CHECKS`, `tests/joint_library.cpp:1653-1665`);
   `plate_assignment` checks only that solving twice gives the same answer
   (`tests/plate_assignment.cpp:27-46`); `main_all_datasets` checks only that nothing threw. The
   test `check()` helper is copied thirteen times, twice not static (`tests/brep_drill.cpp:10`,
   `tests/floor_elements.cpp:45`). Size S for the CI, M for an insertion check.

10. **Serialization debt that loses data quietly.** Element loaders tell legacy JSON by
    `bytes.front() == '{'` and swallow every exception with an empty catch
    (`wood_element_edge_curved.cpp:399`, `wood_element_edge_straight.cpp:165`,
    `wood_element_face_planar.cpp:348, 382`); a failed protobuf parse returns a default element
    (`wood_element_edge_straight.cpp:171`). EdgeCurved is saved as JSON with protobuf blobs inside
    it as byte arrays (`wood_element_edge_curved.cpp:1609-1658`). `FaceCurvedElement` has no
    serialization and is not in `register_factories` (`wood_session.cpp:18-38`). An instance with
    a mirror transform is dropped without a word (`wood_instance.cpp:196-197, 291-292`). Seven
    legacy type tags are still registered, with a read-only `Column` proto and its generated code
    (`src/proto/element_column.proto`), `rust/tests/data/rust_elements_legacy.pb`, and
    "0 legacy pair" in `target_side` (`wood_interaction_feature_plate.h:26`). Fix: a one-time
    migration tool and loaders that throw naming the element. Size M.

### Smaller smells

- `wood_datasets.cpp` has 43 one-line wrappers `type_plates_name_*` that only call
  `run_dataset("...")`, under names that differ from the datasets' (`examples/main_all_datasets.cpp:6-48`).
- Dead code: `compute_lines_contacts` (`wood_session.h:152`), `EdgeCurvedElement::from_surface`
  (`wood_element_edge_curved.h:116`), `WoodSession::get_features` and `set_features_visible`
  (`wood_session.h:214, 223`), the empty `InteractionStructure` (`wood_interaction_structure.h:10`),
  `examples/main_element_mapping_check.cpp` (not in CMake).
- Ids 27, 36-37, 46-47, 50-53 and 61-68 fall through to the family's default design without a
  word (`wood_element_joint_plate.cpp:432-452`).
- `cross_half_lap` with drills (ids 33, 34) is reachable by name only as the legacy `cr_c_ip_3`
  and `cr_c_ip_4` (`wood_element_joint_plate.cpp:94-96`).
- The geometry cache key rounds to two decimals and makes an exception for `family.id != 26`
  (`wood_element_joint_plate.cpp:578-606`); `side_side_outplane_annen_fingers(..., 17, 15.0)`
  takes magic numbers (`:219`).
- `#pragma warning(disable : 4505)` hides unused static functions
  (`wood_interaction_feature_plate_joints.h:6`); the design bodies are 53 headers included inside
  an anonymous namespace (`wood_element_joint_plate.cpp:9-13`), against "no files per factory or
  family".
- 83 headers have a file-scope `using namespace`; `SearchType` and `get_connection_zones` live in
  the global namespace (`wood_settings.h:8`, `wood_session.h:36`).
- Comments cut off mid-sentence: `wood_element_joint_beam.cpp:111, 127, 188, 220`; the
  `column_head` doc starts mid-sentence (`wood_element_joint_beam.h:191`); a rename made "the
  NodeBlockElement Research Group" of the Block Research Group (`wood_element_joint_beam.h:38`);
  "source static" (`wood_session.cpp:17`).
- `wood/CLAUDE.md` is stale: `examples/1_elements.cpp` does not exist, "wood-agent Protocol 5"
  does not exist, and it bans `add_column`-style helpers while the reference has
  `Floor::add_column` (`floor.h:95`).
- CI comments tell old history (`wood-test.yml:28-62`: the Vue viewer, a gdb and ASan plan never
  done, "segfaults on Linux"); `submodules: recursive` with no `.gitmodules`.
- CMake: stale comments (`:150-161`, "11 sources" and `wood_face_to_face.cpp`, which is gone); a
  hard-coded `C:/pc/...` candidate (`:39`); the deprecated `FetchContent_Populate` (`:56`);
  `ENABLE_UNITY_BUILD` forced into a subproject's cache (`:77`); `wood_datasets.cpp` compiled for
  two targets (`:274-275`); `reference_datasets`' definition apart from its target (`:341` against
  `:306`); `/STACK:32MB` because a `Floor` does not fit the stack (`:287`).
- `.gitignore` is mostly Python, Django and celery boilerplate, lists `*.bin`, `*.so` and `*.suo`
  twice, has the typo `.VC,db-wal`, ignores `cmake/build/**`, which no longer exists, and every
  `lib/`.
- `rust/build.rs` writes generated code into `rust/src/proto`, the source tree, and calls
  `std::env::set_var`; the Rust crate covers 5 of the 13 element classes (`rust/src/`).
- `tools/shoot_pictures.sh:9, 20-25` commits and pushes to the dead branch `joinery-library` in a
  watch loop.
- Five picture pipelines (`screenshot_element_docs.sh` with `screenshot_viewer.mjs`,
  `shoot_ui.sh` with `ui_shot.rs`, `shoot_native.sh` with `wood_shot.rs`,
  `render_template_docs.py` with `render_scene_picture.py`, `render_architecture_pictures.py`),
  the viewer command line pasted into each (`shoot_butt.sh:7`, `shoot_ui.sh:4`, `ui_shot.rs:19`);
  the crop written twice (`crop_viewer_shot.py`, inline in `shoot_butt.sh:12-22`);
  `shoot_native.sh:16-19` copies a file into the viewer repository.
- Hard-coded home paths: `examples/10_profile_cuts.cpp:44`, `11_drill_solids.cpp:39`,
  `12_solid_features.cpp:41` (whose footers also ignore the CLAUDE.md format) and
  `tools/reference_2025.py:8-10`.
- Examples that are not minimal: `main_session_round_trip.cpp` (358 lines, and a ctest target in
  `examples/`), `templates_grid_5_framing.cpp` (288), `templates_grid_4_reference.cpp` (231),
  `templates_grid_1_footprint.cpp` (229).
- Integers where an enum belongs: the grid's `system`, `span`, `node` and `capital`
  (`grid.h:58-69`), `rank` 7 to 1 (`grid_plan.h:144`), fabrication types as `vector<int>`
  (`wood_interaction_feature_plate.h:42-46`), `joint_type` (`:31`, whose comment also leaves out 60).
- Names left from the CGAL port: `cx0_py0__cy0_px0`, `e0_0__e1_0`
  (`wood_contact_detection.cpp:771-778`); `rl.ua_botL`, `dummy`, `sd`, `fr` in EdgeCurved
  (`wood_element_edge_curved.cpp:850-1110`); `k_cnt`, `e_inner`, `n_sub` in the vault
  (`vault.cpp:655-680, 1337`).
- `TRACE` debug prints left in (`wood_brep_drill.cpp:10, 235, 502, 533, 565`); `pulled_apart`
  ignores its first parameter (`wood_element_joint_beam.cpp:416`).
- An Annen or a Vidy element is chosen by group size and the guess `three_valence[0][0] == 0`
  (`wood_feature_solver.cpp:301-327`).
- `joint_library`'s check labels repeat "C10" and skip C5, C8, C9 and C12
  (`tests/joint_library.cpp:1653-1665`); a missing `passing.txt` reads as an empty list and
  protects nothing (`:1708-1717`).
- The chevrons colour their guide nodes (`chevron.h:308-312`, `chevron_shifted.h:265-269`),
  against "a template does not paint nodes".
- `wood_instance.cpp:170-200` dispatches `transformed` by a `dynamic_pointer_cast` ladder where a
  virtual would do.

### Suspected

- The floor's `closure` is rational in the direction and its rib start a parabola against a
  plane, so both likely have a closed form (`floor_guide.cpp:487, 665, 738`); the vault's edges
  along uv lines could be exact isocurves (`vault.cpp:665-673`).
- The vault and the grid build members with free functions that return element lists
  (`vault.h:19-92`, `grid.cpp:448-691`), off the floor pattern.
- The two `side_tilt_boundary_ups` (`reciprocal_boundary.h:629`, `reciprocal_surface.h:819`) are
  near copies with different arguments.
- A scene with a `FaceCurvedElement` loads it back as a plain kernel `Element`.
- `tools/run_guarded.ps1` and `run_guarded.sh` repeat the superproject's `buildslot`.
- 68 plan and log files under `docs/plans` are never pruned.
