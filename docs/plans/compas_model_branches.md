# compas_model branches: collision and contact code

Written 2026-11-05 from a read-only review of every branch of
[compas_model](https://github.com/BlockResearchGroup/compas_model) and its three forks, compared with wood
`main`. Nothing from the clone was built or run, and nothing in wood was timed. It follows
`compas_model_comparison.md` and corrects one point in it (section 1).

## Summary

- **Collision is stubs on every branch.** The four functions in `algorithms/collision.py` raise
  `NotImplementedError` on `main`, and no branch or fork holds a working replacement. The Coal and
  `BRepExtrema_DistShapeShape` design in `docs/collision_detection.md` exists in no source tree.
- **Contact detection is real code on `main`**, not only in the docs. It is a Python BVH broad phase, then a
  face-against-face shapely overlap. Wood already runs the same algorithm, compiled, with a better broad phase.
- **Wood is not slower in principle anywhere except beam axis contacts.** `closest_pairs` tests every segment
  pair. The fix uses wood's own `SpatialBVH`; compas_model has nothing to borrow for it.

## 1. Branches checked

22 branches: 20 on the upstream repository and 2 that exist only on forks. Dates are last commit dates.

| Branch | Date | Ahead of main | Collision / contact content |
|---|---|---|---|
| `main` | 2026-10-15 | - | The shipped code in section 2 |
| `gh-pages` | 2026-11-03 | - | Built docs only |
| `viewer_objects` | 2025-04-09 | merged | - |
| `transformation_issue` | 2025-01-31 | 2 | k-d tree builds on `element.point` instead of the AABB centre (one line) |
| `new_feature_elements_and_interactions` | 2025-01-21 | merged | - |
| `algox` | 2025-01-17 | merged | - |
| `interactions_split_into_modifiers_collisions_contacts` | 2025-01-16 | 1 | Moves files into `interactions/collisions/`; no algorithm |
| `barrel_vault_elements` | 2025-01-16 | 18 | Same reshuffle, plus boolean and slicer modifiers |
| `elements_and_contacts` | 2025-01-10 | merged | - |
| `element_is_dirty` | 2025-01-10 | merged | - |
| `element_interactions` | 2025-01-09 | 26 | `boolean_difference` through compas_cgal, an interface element; contacts through the k-NN in section 3 |
| `documentation_cleanup` | 2025-01-06 | merged | - |
| `modelgeometry` | 2024-12-19 | merged | - |
| `interactions_and_algorithms` | 2024-12-19 | 21 | `interactions/collision.py`: a `Collision` class whose `collide` returns `False` |
| `remove-groupnode` | 2024-08-07 | merged | - |
| `new_materials`, `materials_plastic_and_timber`, `materials` | 2024-04 to 05 | 3 to 4 | Materials only |
| `groupnode` | 2024-02-26 | 2 | Element tree only |
| `elements_refactoring` | 2024-02-08 | 5 | `algorithms/collider.py` (365 lines) and `algorithms/algorithms.py` (441 lines, all commented out), section 3 |
| fork `petrasvestartas/missing_examples` | 2026-10-01 | 2 | Docs pages and `__all__` exports; no new code |
| fork `dflorenzano/fix/model_class_method` | 2026-06-19 | 2 | A `Self` return type on a model class method and its test |

The third fork, `jf---/compas_model`, has only `main` from 2024-05-21, behind upstream. The other fork branches
mirror the upstream ones above.

Every branch other than `main` is older than `main`'s collision and contact work, so `main` is the newest
code. The docs pages describing collision were last changed on 2026-10-15; the source files on 2026-05-20.

**Correction to `compas_model_comparison.md` section 0:** that page says the docs describe more than the code
and that every collision function is a stub. Both are true. But `Model.compute_contacts`, `ElementBVH`,
`KDTree`, the shapely face overlap and OCC BRep overlap are shipped and working on `main`; only "collision"
in the boolean sense is missing.

## 2. What `main` ships

| Piece | File, lines, last change | Algorithm | Cost |
|---|---|---|---|
| Collision | `algorithms/collision.py`, 20 lines, 2026-05-20 | `is_box_box_collision`, `is_polyhedron_polyhedron_collision`, `is_mesh_mesh_collision`, `is_brep_brep_collision`: each `raise NotImplementedError` | - |
| Broad phase | `datastructures/bvh.py` 552 lines, `models/bvh.py` 84 lines, 2026-05-20 | Python object tree, median split on the longest axis, AABB or PCA-fitted OBB nodes. `ElementBVH.nearest_neighbors` inflates the query box by a relative 1.2 and reads only `objects[0]` of each leaf | Build O(n log² n) in Python. Each query is breadth-first with `list.pop(0)` |
| Driver | `models/model.py:726-782` `compute_contacts` | For every element, BVH neighbours, then `element.compute_contacts(nbr)` unless the edge already holds contacts | n BVH queries, each pair met from both ends |
| Mesh narrow phase | `algorithms/contacts.py:74-140` `mesh_mesh_contacts` | Every face of a against every face of b; opposite normals, then `polygon_polygon_overlap` | F_a x F_b per pair |
| Face overlap | `algorithms/contacts.py:232-305` | numpy SVD best-fit frame of both polygons, project, reject if any point is off the plane, shapely `intersection` | One SVD per opposite-normal face pair, before coplanarity is known |
| BRep narrow phase | `algorithms/contacts.py:143-229` | compas_occ `Brep.overlap`: meshes both BReps (`BRepMesh_IncrementalMesh`), then `BRepExtrema_ShapeProximity`, which uses OCC's triangle BVH to return only faces within tolerance; then the same shapely overlap, plus holes | Meshing both shapes per pair; face pairs pruned by a triangle BVH |
| Nearest points | `datastructures/kdtree.py`, 140 lines | Pure-Python k-d tree on AABB centres; `nearest_neighbors(k)` reruns a full search k times | O(k log n) per query |
| 2D collision | `geometry/gjk2.py`, 249 lines, 2026-05-07 | GJK on convex polygons in the XY plane; its test file is entirely commented out | Boolean answer only |
| Primitives | `geometry/intersections.py`, 872 lines | Line, ray and segment against box and AABB, box-box by separating axes, sphere-box, ray-triangle | - |

## 3. Older collision code on branches

- **`elements_refactoring` (2024-02-07), `algorithms/collider.py:23-365`.** `get_collision_pairs` loops over all
  element pairs (`:345-363`): AABB overlap, then an OBB separating-axis test over 15 axes (`:60-127`), then
  `face_to_face` with shapely (`:130-324`). This is wood's own pipeline in Python, without a spatial index:
  O(N²) pairs. Never merged.
- **`elements_refactoring`, `algorithms/algorithms.py`.** Entirely commented out. Its
  `get_collision_pairs_kdtree` builds a scipy `cKDTree` on AABB centres, takes the 10 nearest, and keeps a
  neighbour when its centre distance is below the element's AABB diagonal.
- **`nnbrs.py` on 11 branches (2024-05-12 to 2025-01-16).** `find_nearest_neighbours` is a scipy `cKDTree`
  k-nearest query on block centroids with a fixed `nmax`. `algorithms/interfaces.py` and `overlaps.py` run the
  face or BRep overlap on those k neighbours. Removed from `main` on 2025-01-16 when the BVH and k-d tree
  replaced it.
- **`interactions_and_algorithms` (2024-12-19), `interactions/collision.py`.** A data class; `collide` returns
  `False`.

The k-nearest approach is fast (scipy is compiled) but not exact: a member with more than k neighbours loses
some, and a long beam touching another near its end has its centroid far away. Wood's AABB-overlap query is
exact for any member length.

## 4. Wood, for comparison

| Stage | Wood | compas_model `main` |
|---|---|---|
| Face broad phase | `adjacency_search` (`wood_contact_detection.cpp:320-380`): an OBB per element inflated by `settings.distance` (absolute), its AABB, a linear BVH (Karras 2012, 30-bit Morton codes, `session_cpp/src/spatial_bvh.cpp:312-343`), one stack query per element (`:192-219`), keep `i < j`, then OBB separating axes | Python median-split BVH, relative 1.2 inflation, breadth-first with `pop(0)` |
| Face narrow phase | `face_contacts_for_pair` (`:482-574`): every face pair, `faces_coplanar` (`:382-401`, two dot products and two distances) before any clipping, then Clipper2 `Intersect` (`:403-450`) | Every opposite-normal face pair runs an SVD fit and a projection before coplanarity is rejected; then shapely |
| Beam axes | `closest_pairs` (`:924-966`): every segment of every beam against every segment of every later beam; no spatial index | No axis contacts |
| Collision | None at model level | Stubs |

`compute_axis_contacts` is called from `examples/8_beams.cpp`, `wood_datasets.cpp`, the joint figure tool and
four test files.

## 5. Verdict

**Can wood be as fast as these sources?** For face contacts it already is in principle: same algorithm class,
compiled, an O(n log n) broad phase with an absolute tolerance, and a cheaper coplanarity reject. Nothing was
timed, so this is from the code, not a measurement. The one asymptotic gap is the beam axis search, which
compas_model does not have at all.

### What wood could borrow or do

| # | Change | Source | Gain |
|---|---|---|---|
| 1 | **Axis contacts through `SpatialBVH`.** One AABB per axis segment, grown by `min_distance` (absolute), `build_from_aabbs`, one `query_aabb` per segment, keep pairs from different beams with `a < b`, then the existing `line_line_parameters` test and the per-beam-pair minimum | Wood's own kernel, as `adjacency_search` already does | O(S²) segment pairs become O(S log S + k). The only change that alters complexity |
| 2 | **Dual-tree self traversal** for `adjacency_search`: traverse the tree against itself once, emitting each overlapping leaf pair once, instead of n root-down queries that find every pair twice | `docs/collision_detection.md` (documented, not shipped) | At most about 2x on the broad phase, which is not where the time goes. Only after profiling shows it matters |
| 3 | **Face-pair pruning per element pair** as in OCC `BRepExtrema_ShapeProximity` | compas_occ, called from `contacts.py:187` | Only for elements with many faces. A beam has 6 faces and a plate n + 2; the plane test is cheaper than meshing |

For item 1, sort-and-sweep on one axis is the simpler alternative. It degrades when the beams lie along the
sweep axis, which floor grids and trusses do, and the BVH is already in the kernel, so use the BVH.

### Not worth borrowing

- **A k-d tree radius query on axis endpoints.** Cross laps and X-joints meet mid-span; their endpoints can be
  far apart, so an endpoint query misses them. The segment AABB query in item 1 does not.
- **k-nearest-neighbour candidate lists** (`nnbrs.py`, the commented `cKDTree` code): a fixed k and centroid
  distances drop real contacts.
- **compas_model's shipped BVH and k-d tree:** Python, relative inflation, `pop(0)` traversal, `objects[0]` only.
- **The 2024 `Collider`:** wood's pipeline without an index, O(N²).
- **2D GJK** (`gjk2.py`): convex polygons only and a yes/no answer. Wood needs the overlap region, which Clipper2
  gives for non-convex outlines.
- **Coal and `BRepExtrema_DistShapeShape`:** described only in the docs. Coal works on triangle meshes, which
  wood's BRep-only rule excludes.

## 6. Method

- `git clone --no-checkout` of upstream, then the three forks fetched as extra remotes into the same clone;
  every remote branch listed with `git for-each-ref --sort=-committerdate refs/remotes`.
- `git branch -r --merged origin/main` and `git rev-list --count origin/main..<branch>` to find unmerged work;
  `git diff --stat origin/main...<branch>` filtered for collision, contact, interface, BVH, intersection,
  modifier, interaction and tree files; every hit read with `git show`.
- `git grep` on every branch for `coal`, `hppfcl`, `BRepExtrema`, `cKDTree`, `pybullet`, `fcl`, `rtree`,
  sort-and-sweep and `is_collision(`. Only `cKDTree` matched, in the files of section 3.
- compas_occ `Brep.overlap` read from its `main` because `contacts.py` calls it. compas_cgal is used on one
  branch for a boolean difference, not for collision, so it was not read further. No branch refers to
  compas_fab.
