# Coal ideas for wood's contact detection

Written 2026-11-05. [Coal](https://github.com/coal-library/coal) (formerly hpp-fcl, BSD-2) was read at
`83cdaa6` (2026-11-05) and never built or run. Wood `main` at `95a70554` with kernel `origin/main` at
`6277b430` was built in a worktree with phase timers added for this study only; they are not committed.
It follows `compas_model_comparison.md` section 4 and `compas_model_branches.md`.

## Summary

- **Contact detection is not where wood spends its time.** It is 0.6% of the dataset run, 4.6% of the
  floor test and 0% of the truss library. Writing scenes (`pb_dump`, which builds the BReps) is 85% of the
  dataset run.
- **The floor's contact time is not contact work.** Nearly all of it, about 4 s, is the first `planes()` call
  on each of 866 members, which builds that member's cut mesh (4.7 ms each). Contact detection is only the first
  caller; the 1,584 later fetches of the same members cost 24 ms. Searching and clipping take 23 ms.
- **The only repeated work is small.** In the grid, `add_cut_faces` rebuilds the cut-face list of a deck on
  every pair it is in (14 ms of 41 ms).
- **Wood already has Coal's leaf test.** The kernel's `OBB::collides_with_rtcd` is the same 15-axis
  separating test with the same epsilon padding as Coal's `obbDisjoint`, and `OBB::aabb` is Coal's
  `computeBV<AABB, Box>`.
- **One Coal routine fixes a real bug.** The kernel's segment-segment closest points clamp both parameters
  independently, so for skewed beams that meet near an end the contact point lands on the wrong spot and the
  gap is overstated. Coal's `capsule_capsule` distance does it right.
- **GJK and EPA (section 2) do not speed up contact detection.** For boxes the OBB test is already exact, the
  pairs that reach the coplanarity test are within tolerance by construction, and a GJK call on two boxes
  (an estimated 0.3 to 1 µs) costs about what the coplanarity loop costs for that pair (0.7 to 2.6 µs,
  measured) without replacing it. They are the right tool for a model-level overlap check that reports
  penetration depth, which wood lacks: members overlapping by more than `settings.distance` give no contact
  and no warning today. A kernel `Gjk` of about 450 lines is sketched for when that check is scheduled.
- **Decided:** take 1 (Ericson segment-segment closest points, a correctness fix), take later 4 (cached face
  lists, about 50 ms; a dynamic tree; a kernel GJK and EPA for the overlap check; a growable stack), reject 8.
  Everything Coal does to make the broad phase or the clipping faster saves at most 40 ms of the 155 s the
  four programs run.

## 1. What Coal does

Paths are relative to the Coal repository.

| Area | What it does | Where |
|---|---|---|
| Dynamic AABB tree | Pointer tree built top-down from Morton codes; a moved object is removed and re-inserted only when its new box leaves the old one; incremental rebalancing | `include/coal/broadphase/detail/hierarchy_tree-inl.h:151-156` (skip if contained), `:655-667` (`update_`), `:699-760` (`insertLeaf`), `:476-537` (Morton init); manager `src/broadphase/broadphase_dynamic_AABB_tree.cpp:555-616` |
| Self collision | One dual traversal of the tree against itself: recurse into both children, then test left subtree against right; descend the larger box first | `src/broadphase/broadphase_dynamic_AABB_tree.cpp:310-332`, `:361-373` |
| Sweep and prune | SaP keeps sorted end-point lists on three axes and an incremental overlap-pair list; SSaP sorts once and sweeps the axis with the widest spread | `src/broadphase/broadphase_SaP.cpp:274-287`; `src/broadphase/broadphase_SSaP.cpp:370-405`, `:409-458` |
| Interval tree, spatial hash | Interval tree per axis; hash grid with one fixed cell size inside a fixed scene box | `include/coal/broadphase/detail/interval_tree.h:63-88`; `include/coal/broadphase/broadphase_spatialhash-inl.h:48-51` |
| OBB | 15-axis separating test, absolute rotation padded by 1e-6 against parallel edges; a variant inflates by a security margin and returns a lower bound on the distance | `src/BV/OBB.cpp:161-240`, `:344-392` |
| RSS, OBBRSS | Rectangle swept by a sphere, for fast distance bounds; OBBRSS carries both | `include/coal/BV/RSS.h`, `src/BV/RSS.cpp:712-725`; `include/coal/BV/OBBRSS.h:48-55` |
| kDOP, kIOS | 16/18/24 fixed slab directions; a set of up to five spheres | `include/coal/BV/kDOP.h:45-80`, `src/BV/kDOP.cpp:135-141`; `include/coal/BV/kIOS.h:50-58` |
| BV of a rotated box | AABB = centre ± abs(R) · half sides; OBB = the box itself | `src/shape/geometric_shapes_utility.cpp:281-288`, `:459-470` |
| BVH vs BVH | Recursive and explicit-stack traversal; the stack is a growable `std::vector` (reserve 1000), not a fixed array | `src/traversal/traversal_recurse.cpp:43-84`, `:86-148`; `include/coal/internal/traversal_node_bvhs.h:88-96` |
| Front lists | Keep the frontier of the last traversal and restart from it next frame | `src/traversal/traversal_recurse.cpp:306-357` |
| Narrow phase | GJK distance and witness points, EPA for penetration depth and normal | `src/narrowphase/gjk.cpp:192`, `:1166`, `:1298-1300`; box support `src/narrowphase/support_functions.cpp:140-156` |
| Contact patch | Support set of each shape in the contact normal within `patch_tolerance` (default 1e-3), both made counter-clockwise, then Sutherland-Hodgman clipping of convex polygons | `include/coal/collision_data.h:1039-1126`; `include/coal/contact_patch/contact_patch_solver.hxx:107-147`, `:203-372` |
| Patch capping | Reduces a patch to k vertices of maximum area | `src/contact_patch/contact_patch_simplifier.cpp:328` |
| Segment-segment | Ericson's closest points of two segments: solve, clamp one parameter, re-solve the other, clamp | `src/distance/capsule_capsule.cpp:53-73`, `:84-160` |
| Cached object box | Each `CollisionObject` keeps its world AABB, recomputed only when its transform changes | `include/coal/collision_object.h:272-289`, `:376` |

## 2. GJK and EPA in Coal

Read in `src/narrowphase/gjk.cpp`, `include/coal/narrowphase/gjk.h`, `src/narrowphase/support_functions.cpp`,
`src/narrowphase/minkowski_difference.cpp`, `include/coal/narrowphase/narrowphase.h` (the `GJKSolver` that
drives both) and `src/narrowphase/details.h`. Coal has no `narrowphase.cpp`; the driver is header-only.
Box against box goes through GJK too: there is no separate box-box test (`src/collision_func_matrix.cpp:288`).

### The setup

- **Minkowski difference.** GJK works on A - B. A support point in direction d is `s_A(d) - (R s_B(-R^T d) + t)`,
  with B expressed in A's frame once per call (`minkowski_difference.cpp:308-336`, `gjk.h:162-170`). Every simplex
  vertex keeps both shape supports `w0`, `w1` beside `w = w0 - w1`, which is what makes witness points cheap later.
- **Initial guess** (`narrowphase.h:379-408`): (1, 0, 0), the last call's closest point (`CachedGuess`, for
  repeated queries on the same pair), or the difference of the two local AABB centres.
- **Defaults** (`narrowphase_defaults.h:43-58`): GJK 128 iterations, tolerance 1e-6; EPA 64 iterations,
  tolerance 1e-6. The note there says 1e-6 means micrometres for shapes measured in metres; wood works in
  millimetres, so 1e-6 is a nanometre.

### The GJK loop (`gjk.cpp:192-377`)

`ray` is the current closest point of the simplex to the origin, `rl` its length. Each iteration:

1. **Check A** (`:233-248`): `rl < tolerance` means the origin is on the simplex. Status `Collision`; EPA is
   needed for depth.
2. **Direction** (`:251-285`). Vanilla GJK searches along `-ray`. The Nesterov variant mixes in momentum:
   `y = m ray + (1 - m) w` with `m = (k + 2) / (k + 3)`, and for shapes that are not strictly convex (boxes,
   polytopes) it averages normalised directions, `dir = m dir/|dir| + (1 - m) y/|y|` (`:259-270`). Polyak uses
   `m = 1/(k + 1)` (`:278-281`).
3. **Support** of the new vertex `w` along `-dir` (`:288-292`).
4. **Check B, the early exit** (`:294-300`): `omega = dir.w / |dir|` is the distance of a separating plane, so
   a lower bound on the distance. If `omega > distance_upper_bound` the shapes are proved farther apart than the
   bound: status `NoCollisionEarlyStopped`, no witness points. For a collision query the solver sets the bound
   to `max(0, security_margin + collision_distance_threshold)` (`narrowphase.h:233-239`); for a distance query
   it is infinite. This is the yes/no separation test.
5. **Leaving acceleration** (`:303-311`, `:318-322`): when the Frank-Wolfe duality gap `2 ray.(ray - w)` falls
   below the tolerance, or the convergence check passes while accelerated, the last vertex is dropped and the
   loop continues as vanilla GJK. Momentum only speeds the approach; vanilla GJK finishes.
6. **Check C, convergence** (`:313-333`, `checkConvergence` `:379-433`):
   - default, van den Bergen: `alpha = max(alpha, omega)` is the best lower bound so far; stop when
     `rl - alpha <= tol + tol rl`, a relative gap on the distance;
   - duality gap: `2 ray.(ray - w) <= tol`, a bound on the squared error of the closest point;
   - hybrid: `rl^2 - alpha^2 <= tol`.
   On convergence with `rl > tolerance`, status `NoCollision`, or `CollisionWithPenetrationInformation` when the
   distance net of swept-sphere radii is below the tolerance. Note: the "relative" forms of the duality-gap and
   hybrid tests (`:404`, `:422`) reduce algebraically to `diff <= tol^2`, so they do not scale with `rl`; not
   worth copying.
7. **Simplex projection** (`:338-357`), then `rl = |ray|`; the origin inside the simplex means `Collision`.
   After `max_iterations`, `Failed`.

### Simplex projection, 1 to 4 vertices (`gjk.cpp:502-1019`)

- Only the Voronoi regions of the newest vertex A are tested: the origin cannot lie in the region of an older
  vertex, since A was found by searching towards the origin. This cuts the cases and avoids testing regions
  that rounding could pick wrongly.
- The closest point is written in closed form from dot products, with no barycentric solve:
  segment `ray = ((AB.B) A - (AB.A) B) / |AB|^2` (`:510-524`), triangle `ray = -(ABC.A) / |ABC|^2 ABC`
  (`:526-550`), with the triangle's winding flipped so its normal faces the origin (`:539-545`).
- Segment (`:552-578`) and triangle (`:580-620`): sign tests on `AB.AO`, `(ABC x AC).AO`, `(AB x ABC).AO`.
  Exact zeros are handled on their own (`d == 0`, `ABCdotAO == 0`).
- Tetrahedron (`:622-1019`): 15 dot products of the four points are computed once, then a fixed branch tree
  generated by `doc/gjk.py` picks the region (a face, an edge, A, or inside). The edge tests are written as
  `ba * da_ba + bd * ba_aa - bb * da_aa` style expressions of those dot products, so no cross product is
  formed per test. Debug asserts use a `3 sqrt(eps)` margin where two tests must agree.
- The weights for witness points are recomputed only at the end, by projecting the origin on the final
  simplex (`getClosestPoints`, `:96-154`); the loop itself never needs them.

### Support functions (`support_functions.cpp`)

- **Box** (`:140-156`): pick `+h` or `-h` per axis by the sign of the direction. A component within rounding of
  zero gives 0 on that axis, so a face-on direction returns the middle of the face, not an arbitrary corner;
  when any component is zero the negative side is inflated by `1e-10` so ties have a preferred side.
- **Convex hull, up to 32 vertices** (`:386-404`, threshold `geometric_shapes.h:834`): a linear scan of dot
  products.
- **Larger hulls** (`:307-383`): hill climbing on vertex adjacency from a hint vertex, keeping a `visited`
  list. A "loose check" accepts equal dot products until the first strict improvement, so the walk can cross a
  face orthogonal to the direction. When the direction turns by more than about 25 degrees (dot of the
  normalised directions below 0.9) the hint restarts from the best of 14 precomputed warm starts, the supports
  along the 6 axis and 8 diagonal directions (`:313-328`, `gjk.cpp:1481-1549`).

### EPA (`gjk.cpp:1021-1477`)

- **Start** (`:1166-1205`): GJK's final simplex is grown to a tetrahedron by `encloseOrigin` (`:445-500`), which
  adds supports along the axes and edge or face normals until the volume is non-zero. The tetrahedron is
  turned so its normals point out (`:1189-1195`).
- **Faces** (`newFace`, `:1078-1148`): each face stores its unit normal and distance `d` to the origin; a face
  onto which the origin does not project is flagged `ignore` and never chosen. The four starting faces are
  forced in even when the origin lies outside them: GJK may have stopped by tolerance with the shapes just
  apart, and EPA then returns the right signed distance anyway (comment at `:1114-1131`).
- **Loop** (`:1224-1296`): take the closest non-ignored face (a linear scan, `:1151-1164`), get the support
  `w` along its normal, and stop when `n.(w - v) <= tol + tol |w|`, the most depth still to gain (`:1260-1267`),
  or when `w` coincides with a face vertex (`:1272-1277`).
- **Expansion and horizon** (`expand`, `:1372-1460`): from the closest face, walk into each neighbour. If `w`
  is below the neighbour (`n.(w - v) < 3 sqrt(eps)`) the edge is on the horizon: a new face (edge plus `w`) is
  made and linked to the previous new face. Otherwise the neighbour is deleted and the walk recurses into its
  two other edges. A per-iteration `pass` mark stops a cycle (`InvalidHull`). The horizon is closed by linking
  the first and last new faces (`:1291`). Fewer than 3 horizon faces ends the loop.
- **Memory**: preallocated, `max_iterations + 4` vertices and `2 max_iterations + 4` faces (`:1023-1046`); no
  allocation in the loop.
- **Output** (`:1298-1304`, `:1462-1477`): depth = the closest face's distance (plus swept-sphere radii), normal
  = its normal, witness points = the origin's barycentric projection on that face applied to `w0` and `w1`;
  the normal is re-derived from the witness points when they are apart. Rank-1 fallback: depth 0 and
  normal `-guess` (`:1310-1326`). Every failure status (`OutOfFaces`, `OutOfVertices`, `Failed`, `Degenerated`,
  `NonConvex`, `InvalidHull`) still returns its last depth, normal and witness points, at lower precision
  (`narrowphase.h:553-600`).
- For two boxes pressed face to face, the origin lies on a facet of A - B whose normal is the contact normal,
  so EPA's normal is well defined; it is ambiguous only for edge-to-edge or corner contact.

### What wood could use GJK and EPA for

Numbers are from the profile in section 3.


Wood's members are convex only before their cuts: a box beam, a prism plate, a cylinder pin. After laps,
notches and drills they are not, so GJK applies to the stock solid (an outer bound) or to convex pieces. Point
sets come from BRep vertices and pins from an analytic cylinder support, so no mesh is needed.

| Use | Wood code path | What the profile says | Verdict |
|---|---|---|---|
| (a) Yes/no separation before Clipper2, or instead of the OBB leaf test | `adjacency_search` leaf test (`wood_contact_detection.cpp:371-373`, kernel `OBB::collides_with_rtcd`); `face_overlap_area` (`:403-450`) | False pairs cost 6.2 ms (datasets) and 15.2 ms (grid). The grid's 192 are members meeting at an edge, at distance 0, which GJK keeps. Clipper2 calls that find nothing cost 0.57 and 0.66 ms. For boxes the OBB test is already exact and cheaper than GJK. Upper bound on the gain: about 6 ms of 56.7 s, before paying an estimated 3 to 9 ms for 8,821 GJK calls. | Reject |
| (b) Penetration depth of members that overlap by more than the tolerance | `faces_coplanar` with `settings.distance_squared` (`:509`), the broad-phase inflate (`:583`), the curved-contact `10 x distance` (`:490`) | Today an overlap deeper than `settings.distance` gives no coplanar faces and no contact, silently. EPA would report "these two overlap by 3.2 mm along n". Not a speed gain; checking only the 871 broad-phase pairs with no contact (679 datasets, 192 grid) costs a few ms. | Take later, with the model-level overlap check (`compas_model_comparison.md` item 6) |
| (c) Closest points of two convex pieces instead of segment-segment | `closest_pairs` (`:924-966`) | Axis contacts take 0.02 ms. On two segments GJK needs 2 to 4 iterations and stops at a 1e-6 relative tolerance; the closed form is about 30 flops and exact. | Reject; use the closed form (decided item 1) |

**Does a 3D GJK on two 8-vertex boxes beat the coplanarity test wood already has?** No.

- The coplanarity loop costs 18 to 73 ns per face pair, measured: 11.4 ms for 634,056 face pairs in the floor,
  9.6 ms for 130,875 in the datasets, loop overhead included. A box pair has 36 face pairs, so about 0.7 to
  2.6 µs.
- One GJK call on two boxes: 4 to 10 iterations, each a support on both boxes (16 dot products), a projection
  (about 50 flops) and the checks. That is a few hundred to a thousand flops, about 0.3 to 1 µs. This is an
  estimate from the operation count, not a measurement (no build slot).
- So the costs are similar, but GJK answers a different question. Every pair that reaches the coplanarity
  loop has already passed the OBB test within the tolerance, so GJK would say "touching" for nearly all of
  them and the loop would still have to find which faces are back to back. The one way GJK could cut that loop,
  using its normal to pick the face pair directly, fails on cut members, which are not convex, and saves at
  most the 10 ms the loop costs.

### Is a kernel `gjk.{h,cpp}` worth writing?

**Not for contact detection: nothing in the profile gets faster.** It is worth writing when the model-level
overlap check is scheduled, because that check needs exactly what GJK and EPA give: an absolute-tolerance
yes/no with an early exit, and depth, normal and witness points when members overlap, exact for pins through
a cylinder support instead of a meshed cylinder.

If it is written, keep it small: vanilla GJK with the van den Bergen criterion and the early exit; Coal's
newest-vertex projections for segment and triangle; the tetrahedron as the three faces around A through the
triangle projection, instead of the 400-line generated tree; EPA with the horizon walk and preallocated
storage. No Nesterov: for 8 to 20 vertices vanilla GJK already converges in a handful of iterations, and
Coal's own heuristic treats polytopes separately. That is an opinion, not measured. Estimated size: about 450
lines of C++, no Eigen, then the Python and Rust twins with identical minitests (`session/.claude/skills/new-class`).
Coal is BSD-2, so adapted parts keep a one-line attribution.

API sketch, in the style of the kernel's static query classes (`Closest`, `Intersection`):

```cpp
/// Distance, witness points and normal of two convex shapes, by GJK and, when they overlap, EPA.
struct ConvexDistance {
    double distance = 0.0; // Signed: negative is penetration depth.
    Point point_a; // Witness point on a.
    Point point_b; // Witness point on b.
    Vector normal; // Unit, from a to b.
    bool converged = false; // False when an iteration cap was hit; the values are then approximate.
};

/// A convex shape for GJK: the hull of its points swept by a sphere, or a cylinder.
struct ConvexShape {
    std::vector<Point> points; // Hull vertices; two for a segment, one for a point.
    double radius = 0.0; // Swept-sphere radius: a capsule is a segment with a radius.
    std::optional<Line> cylinder_axis; // A cylinder of `radius` around this axis instead of the hull.

    static ConvexShape from_obb(const OBB& box);
    static ConvexShape from_cylinder(const Line& axis, double radius);
};

/// Separation, distance and penetration of convex shapes.
class Gjk {
public:
    static constexpr int MAX_ITERATIONS = 128;
    static constexpr int EPA_MAX_ITERATIONS = 64;
    static constexpr double TOLERANCE = 1e-6; // Relative to the distance, as Coal's default criterion.

    /// True when a and b are farther apart than gap; stops at the first separating plane.
    static bool separated(const ConvexShape& a, const ConvexShape& b, double gap = 0.0);

    /// Signed distance, witness points and normal; runs EPA only when they overlap and penetration is asked.
    static ConvexDistance distance(const ConvexShape& a, const ConvexShape& b, bool penetration = true);
};
```

Tests, the same in the three languages: two boxes apart along an axis and diagonally (distance and witness
points against the closed form); boxes face to face, edge to edge and corner to face (distance 0, normal on the
face case); boxes overlapping by 3 mm (depth 3, normal); a box and a cylinder pin bored through it; two skew
segments against the Ericson result; the early exit (`separated` false at gap 0.5 when the distance is 0.4);
the iteration cap reported through `converged`.

## 3. Wood's profile

Release build, MSVC, Windows. Median of three runs. Other agents were building at the same time, so wall
times vary by about 15% between runs; the counts are exact. Times in ms.

### Where the time goes

| Program | Wall | Contact detection | Share | Largest part of it |
|---|---|---|---|---|
| `templates_grid_6_five_bay` | 390 | 41 | 11% | Face lists: first fetch in the broad phase 22, cut faces 14 |
| `floor_elements` (test, 18 floors) | 83,600 | 3,890 | 4.6% | First face fetch per member, 3,850: building its cut mesh |
| `templates_truss_2_library` | 14,160 | 0 | 0% | No contact detection: butt joints come from topology |
| `main_all_datasets` (45 datasets) | 56,750 | 317 | 0.6% | Cross contacts (`plane_to_face`), 235 |

For context, `main_all_datasets` spends 48,390 ms (85%) writing scenes, 5,790 ms in `compute_features` and
420 ms loading.

### Contact detection by phase

| Phase | Grid | Floor | Datasets |
|---|---|---|---|
| Broad: OBB fit (`bounding_points`, planes) | 21.7 | - | 15.2 |
| Broad: BVH build | 0.04 | - | 1.4 |
| Broad: queries and OBB test | 0.10 | - | 8.9 |
| Pair: fetch face planes and outlines, first time per member | 2.2 (all fetches) | 4,035 (866 members) | 26.3 (all prep) |
| Pair: same, member fetched before | (above) | 24 (1,584 fetches) | (above) |
| Pair: `add_cut_faces` | 14.3 | 0.05 | (in prep) |
| Coplanarity loop | 0.68 | 11.4 | 9.6 |
| Clipper2, overlap found | 0.78 | 12.3 | 19.2 |
| Clipper2, no overlap | 0.66 | 0 | 0.57 |
| Cross contacts (`plane_to_face`) | - | 3.8 | 235 |
| Axis contacts (`closest_pairs`) | - | - | 0.02 |

The floor test uses the per-pair `compute_face_contact`, so it has no broad phase.

### Counts

| Count | Grid | Floor | Datasets |
|---|---|---|---|
| Elements in broad-phase calls | 84 | - | 2,590 |
| Pairs whose AABBs overlap | 365 | - | 15,518 |
| Pairs that pass the OBB test | 365 | - | 8,821 |
| Face-contact pair calls | 365 | 1,225 | 3,311 |
| Pairs with no contact (false pairs) | 192 | 0 | 679 |
| Time spent on false pairs | 15.2 | 0 | 6.2 |
| Face pairs tested for coplanarity | 18,650 | 634,056 | 130,875 |
| Coplanar face pairs | 443 | 1,225 | 2,764 |
| Clipper2 calls with no overlap | 270 | 0 | 132 |
| Cut faces added by `add_cut_faces` | 560 | 0 | - |
| Cross calls / rejected by angle / at first chord / found | - | 72 / 0 / 0 / 72 | 7,539 / 2,849 / 857 / 3,811 |
| Axis segments / segment pairs / pairs whose boxes overlap | - | - | 6 / 15 / 7 |

What the counts say:

- **The broad phase is already tight.** In the datasets the OBB test removes 43% of the AABB pairs; only
  679 false pairs remain, costing 6 ms. In the grid every member is axis-aligned, so the OBB test removes
  nothing, and the 192 false pairs are members that meet at an edge or corner within the tolerance. No
  bounding volume can reject a pair that really is within the tolerance.
- **The BVH itself is negligible:** build plus queries are 10 ms over 45 datasets and 0.14 ms in the grid.
- **Clipper2 is cheap:** 20 ms for 2,764 calls in the datasets (7 µs a call), 12 ms in the floor.
- **The floor's cost is first evaluation, not repeated work.** The face fetch was split per member guid:
  the first fetch of each of the 866 members costs 4,035 ms, because `planes()` of an `EdgeVariableElement`
  reads `model_geometry_mesh()`, which applies the member's cuts the first time it is asked
  (`wood_element.cpp:69-76`). The 1,584 later fetches cost 24 ms. Contact detection is the first caller, not
  the cause; a cache cannot remove it. (The floor run was repeated separately for this split, two runs.)
- **The grid's `add_cut_faces` is repeated work.** It calls `model_geometry_mesh().face_outlines()` and
  rebuilds and deduplicates a plane per mesh face on every pair (`wood_contact_detection.cpp:453-480`), so
  a deck touching four beams does it four times.
- **Cross contacts are real work.** 2,849 of 7,539 calls stop at the angle test, which costs almost nothing;
  most of the 235 ms is in the 3,811 calls that find a crossing.

## 4. Decided list

Expected gains are from the profile above.

### Take

1. **Ericson segment-segment closest points** from `capsule_capsule.cpp:84-160`, in the kernel's
   `Intersection::line_line_parameters` when `intersect_segments` is set (`session_cpp/src/intersection.cpp:394-397`,
   and its Python and Rust twins). Today both parameters are clamped independently; when one falls outside
   its segment the other is not re-solved. Checked numerically: beam A from (0, 0, 0) to (3000, 0, 0), beam B
   from (3200, -1500, 100) to (3600, 1500, 100) gives t1 = 0.5 and a gap of 412.3 mm; the true closest
   points are at t1 = 0.4825 and 408.9 mm, so the contact point on B is 53 mm off. With a `min_distance`
   just above the true gap the contact is dropped. No speed gain (axis contacts take 0.02 ms); this is a
   correctness fix, and it can change axis contact positions and counts on skewed beams.

### Take later

2. **Cached face lists, read by reference.** Keep each plate's cut-face list on the element, cleared by
   `invalidate_geometry`, the way a Coal `CollisionObject` keeps its world box until it moves
   (`collision_object.h:272-289`), and read `planes()` / `polylines()` by `const&` instead of copying the
   vectors per pair. Expected: grid `add_cut_faces` 14.3 ms to about 3 ms (one build per deck instead of one
   per pair), floor 24 ms of repeated fetches, datasets a few ms of the 26 ms preparation: about 50 ms in all.
   Worth doing when that code is next touched, not on its own.
3. **Dynamic AABB tree with insert, remove and lazy refit** for `WoodSession` edits
   (`hierarchy_tree-inl.h:151-156`, `:655-760`). Not now: rebuilding the linear BVH takes 1.4 ms for 2,590
   elements over 45 datasets, and an edit's real cost is the member's geometry and `compute_face_contacts`
   erasing and recomputing every face contact. Worth it when an interactive session holds about 10^4 elements
   and contacts are updated per element instead of recomputed.
4. **A small kernel GJK and EPA** (`Gjk::separated`, `Gjk::distance`, section 2) for the model-level overlap
   check that `compas_model_comparison.md` item 6 asks for, and to report members that overlap by more than
   `settings.distance`, which today give no contact without a word. Not for contact detection: section 2 shows
   no measured cost it would cut, and a GJK call on two boxes costs about what the coplanarity loop costs for
   that pair while still leaving the loop to run.
5. **A growable traversal stack** as in `traversal_recurse.cpp:86-148`, if kernel-full-build's bound check
   on `SpatialBVH`'s fixed 64-entry stack turns out to need a fallback. Their call.

### Reject

6. **Tighter bounding volumes (OBB, RSS, OBBRSS, kDOP, kIOS).** Wood already tests each candidate pair with
   an OBB fitted to the element's own plane, which is exact for a box beam. RSS and kIOS are looser than an
   OBB around a box; kDOP's fixed directions are looser than an OBB for a rotated box. All false pairs
   together cost 6 ms in the datasets and 15 ms in the grid, and the grid's are edge contacts within
   tolerance that no volume removes.
7. **Dual-tree self traversal** (`broadphase_dynamic_AABB_tree.cpp:361-373`) instead of n root-down queries.
   It at most halves the query time: 4.5 ms of 56.7 s in the datasets, 0.05 ms in the grid.
8. **Front lists** (`traversal_recurse.cpp:306-357`). They pay off between frames of a simulation; wood
   detects once per model build.
9. **SaP, SSaP, interval tree, spatial hash.** SSaP sweeps one axis and degrades when beams lie along it,
   which grids and trusses do; a spatial hash needs one cell size for members from pins to 6 m beams; the
   BVH is already in the kernel and costs 10 ms over 45 datasets.
10. **Sutherland-Hodgman instead of Clipper2 for convex faces** (`contact_patch_solver.hxx:203-372`). At
    most 33 ms over all four programs (Clipper2 total: 20 datasets, 12 floor, 1.4 grid), and it would change
    the rounding Clipper2 applies (`CONTACT_DECIMALS`), which the dataset parity depends on.
11. **Separating-axis early-out before Clipper2.** Clipper2 calls that find nothing cost 0.57 ms in the
    datasets and 0.66 ms in the grid.
12. **Patch capping** (`contact_patch_simplifier.cpp:328`). Wood needs the whole overlap polygon to place
    joints and pins; a k-vertex patch is for physics solvers.
13. **Coal's `computeBV<AABB, Box>`, OBB test and support functions as code.** The kernel already has the
    same math (`obb.cpp:361-371`, `:540-600`).

Not from Coal but measured on the way:

- The floor's 4 s is the first application of each member's cuts. If the floor's contacts may be taken on
  the uncut members (its connectors already are built on uncut members), that time leaves contact detection;
  whether the test pays it later anyway was not measured. A design question for the floor, not a Coal idea.
- `adjacency_search` computes a world size `ws` (`wood_contact_detection.cpp:353-361`) that the BVH build does
  not use, as `compas_model_comparison.md` already noted.

## 5. Stage 2 plan

One item: Ericson segment-segment closest points (item 1). Item 2 only if the lead wants the 50 ms.

### Segment-segment closest points

- Files: the kernel's `Intersection::line_line_parameters` in `session_cpp/src/intersection.cpp`,
  `session_py` and `session_rust`, with one new minitest in each (the two-beam case above, plus parallel,
  degenerate and crossing segments). Coordinate with kernel-full-build, which is in the same kernel. Adapted
  from Coal's BSD-2 code, so the function keeps a one-line attribution.
- Fix it in place rather than adding a second function: every caller that asks for segment clamping wants
  the closest points of the two segments. Those callers are the test surface:
  - kernel `Intersection::line_line` (`intersection.cpp:312`), which many kernel functions use;
  - wood `closest_pairs` (axis contacts, `wood_contact_detection.cpp:944`);
  - wood `WoodSession::compute_line_contacts` (`wood_session.cpp:650`);
  - wood `grid_plan.cpp:148` (segment crossings with a tolerance);
  - the test oracle `tests/oracle_polyline.h:60`.
  Callers that pass `intersect_segments = false` (chevron, reciprocal, truss) are unchanged.
- Wood: no change beyond the kernel bump.
- Tests: kernel minitests in all three languages and the kernel's full test run; wood `joint_beams`,
  `reference_datasets` (`phanomema_node`), the grid examples, and every example that calls
  `compute_axis_contacts` or `compute_line_contacts`.

### Gates

- Contact counts identical on every dataset (`main_all_datasets`, all 45) and on the grid, floor and truss
  examples: the number of face, cross and axis contacts per pair, compared with a dump from `main` before the
  change. Face and cross contacts must not move at all. Axis and line contacts may: any change is listed pair
  by pair with the old and new gap and explained before it is accepted.
- `joint_library` and `reference_datasets` pass at their present counts.
- Timings before and after with the same timers, median of three runs, on an idle machine, in this file.
- CI green after the push.

## 6. Method

- Coal: `git clone --depth 1` into an empty scratch folder; read only.
- Wood: worktree `C:\pc\wood_wt_coal` on branch `coal-contacts` from `main`, kernel worktree
  `C:\pc\sc_coal` at `origin/main`, Visual Studio 17 2022, Release, `--parallel 6`, built under the shared
  build lock.
- Timers: `std::chrono::steady_clock` around each phase of `adjacency_search`, `face_contacts_for_pair`,
  `plane_to_face` and `closest_pairs`, plus `face_to_face_wood`, `compute_features`, loading and scene
  writing in `run_dataset`; counters for pairs and face pairs. The face fetch was timed twice in a row and
  split by first and repeated fetch per element guid to tell first evaluation from repeated work.
- GJK and EPA were read only. Their per-call cost on two boxes is an estimate from the operation count; it
  was not measured, because build slots were paused. Measuring it is the first test of a kernel `Gjk` if one
  is written.
- The CPU frequency check in `CLAUDE.md` is Linux-only; on this Windows machine the runs were repeated three
  times and the median taken.
