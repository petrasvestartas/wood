# Wood Kernel Reference

Permanent architecture reference for the wood C++ timber joinery kernel.
An AI can answer any architecture question from this file without reading source code.

---

## Section 1 — Repository Layout

```
cmake/
├── main.cpp                          # entry point: parses args, selects test/shape
├── main_closest_points.cpp           # closest_points_joints benchmark
├── main_closest_lines.cpp            # closest lines utility
├── CMakeLists.txt                    # build config; /utf-8 for fmt; unity build flags
├── build/Release/wood.exe            # compiled executable
│
├── src/
│   ├── wood/include/
│   │   ├── wood_element.h/.cpp       # element struct + construction helpers
│   │   ├── wood_joint.h/.cpp         # joint struct + operator()
│   │   ├── wood_joint_lib.h/.cpp     # ~60 parametric joint functions
│   │   ├── wood_cut.h                # cut_type enum
│   │   ├── wood_globals.h/.cpp       # GLOBALS: tolerances, paths, custom joint buffers
│   │   ├── wood_main.h/.cpp          # 9-step detection pipeline
│   │   ├── wood_xml.h/.cpp           # Boost property_tree XML read/write
│   │   ├── wood_test.h/.cpp          # test harness; calls xml datasets
│   │   ├── cgal_intersection_util.h  # plane/line/polyline intersections
│   │   ├── cgal_polyline_util.h      # polyline helpers (center, scale, average plane)
│   │   ├── cgal_xform_util.h         # frame transforms (plane_to_xy, plane_to_plane)
│   │   ├── cgal_box_util.h           # OBB SAT collision, point_at(oob,x,y,z)
│   │   ├── cgal_math_util.h          # unique_from_two_int (joints_map hash key)
│   │   ├── clipper_util.h            # Clipper2 2D polygon intersection
│   │   ├── rtree_util.h              # Boost.Geometry RTree AABB broad phase
│   │   └── stdafx.h                  # PCH; typedef CGAL::Surface_mesh<IK::Point_3> Mesh
│   │
│   ├── wood/dataset/                 # 47 XML test datasets
│   │   └── type_<structure>_name_<description>.xml
│   │
│   └── shapes/
│       ├── shapes.h                  # chevron_mesh, FoldedPlates, CrossConnectors decl
│       └── shapes.cpp                # shape implementations (primitives)
│
├── ext/session_cpp/src/
│   ├── session.h                     # Session container; pb_dump(); Geometry variant
│   ├── mesh.h                        # halfedge Mesh; from_polylines, weld, unify_winding
│   ├── nurbssurface.h                # NurbsSurface evaluation
│   ├── plane.h                       # Plane (normal + origin)
│   ├── xform.h                       # 4×4 transform
│   ├── bvh.h                         # BVH tree used by weld
│   └── primitives.h                  # CrossConnectors, FoldedPlates geometry builders
│
└── data/
    ├── session.pb                    # protobuf output loaded by Rhino Python
    ├── session2.pb                   # secondary session output
    └── annen_surfaces/
        └── surface_0..22.json        # Annen NURBS surface definitions
```

**Absolute project root:** `C:/pc/3_code/wood`

---

## Section 2 — Core Data Types

### `wood::element` (`cmake/src/wood/include/wood_element.h`)

```cpp
struct element {
    int id;
    double thickness;

    // Bounding volumes
    CGAL::Bbox_3 aabb;          // axis-aligned BB — used by RTree broad phase
    IK::Vector_3 oob[5];        // OBB: [0]=center, [1]=x-axis, [2]=y-axis,
                                //      [3]=z-axis, [4]=half-extents

    // Geometry
    std::vector<CGAL_Polyline> polylines;  // [0]=top face, [1]=bottom face, [2+]=side faces
    std::vector<IK::Plane_3>   planes;     // [0]=top,       [1]=bottom,      [2+]=sides
    std::vector<IK::Vector_3>  edge_vectors;

    // Joint assignments
    std::vector<int> JOINTS_TYPES;         // per-face type override; negative = female
    // j_mf[face_id] = vector<tuple<joint_id, is_male, param_on_edge>>
    std::vector<std::vector<std::tuple<int,bool,double>>> j_mf;

    // Beam-specific
    IK::Segment_3    axis;
    CGAL_Polyline    central_polyline;
};
```

**Polyline indexing convention:**
- `polylines[0]` = top face outline
- `polylines[1]` = bottom face outline
- `polylines[2..N]` = side faces in order

### `wood::joint` (`cmake/src/wood/include/wood_joint.h`)

```cpp
struct joint {
    int id;
    int v0, v1;               // element indices in the element vector
    int f0_0, f1_0;           // face indices on element v0 (primary / secondary)
    int f0_1, f1_1;           // face indices on element v1

    int type;                 // detected joint type (see type table in Section 4)

    // Detection geometry
    CGAL_Polyline joint_area;         // intersection area polygon
    CGAL_Polyline joint_lines[2];     // centre lines on each element
    CGAL_Polyline joint_volumes[4];   // bounding volumes

    // Cutting outlines (unit-box space, then oriented)
    std::array<std::vector<CGAL_Polyline>, 2> m;  // m[0]=male top,    m[1]=male bottom
    std::array<std::vector<CGAL_Polyline>, 2> f;  // f[0]=female top,  f[1]=female bottom

    // Boolean operation per cut polygon
    std::vector<cut_type> m_boolean_type;  // same size as m[0]
    std::vector<cut_type> f_boolean_type;  // same size as f[0]

    // Parametric properties
    bool   unit_scale;
    double unit_scale_distance;   // maps to element thickness when unit_scale=true
    double orient;
    double division_length;
    double shift;
    double scale[3];
    int    divisions;
    double length;

    std::string name;   // set to __func__ in joint lib — used for caching

    std::string key = "";   // cache key; format: "name;divisions;shift"

    // Linking (3-valence junctions)
    bool link = false;
    std::vector<int> linked_joints;
    std::vector<std::vector<std::array<int,4>>> linked_joints_seq;

    // Access operator: joint(is_male, is_top) → polyline vector
    // operator()(true,  true)  → m[0]  male top
    // operator()(true,  false) → m[1]  male bottom
    // operator()(false, true)  → f[0]  female top
    // operator()(false, false) → f[1]  female bottom
};
```

### `wood::cut::cut_type` (`cmake/src/wood/include/wood_cut.h`)

```cpp
enum cut_type {
    nothing,
    edge_insertion,
    hole,
    insert_between_multiple_edges,
    slice,
    slice_projectsheer,
    mill,
    mill_project,
    mill_projectsheer,
    cut,
    cut_project,
    cut_projectsheer,
    cut_reverse,
    conic,
    conic_reverse,
    drill,
    drill_50,
    drill_10
};
```

### `wood::GLOBALS` (`cmake/src/wood/include/wood_globals.h`)

```cpp
namespace wood::GLOBALS {
    // Tolerances
    double DISTANCE         = 0.1;    // coplanarity / contact test
    double ANGLE            = 0.11;   // parallelism / perpendicularity (radians)
    double LIMIT_MIN_JOINT_LENGTH;    // minimum joint edge length

    // Paths
    std::string PATH_AND_FILE_FOR_JOINTS;  // precomputed unit-box joint geometry XML
    std::string DATA_SET_INPUT_FOLDER;     // XML dataset folder

    // Output
    int OUTPUT_GEOMETRY_TYPE;   // 0=joints only, 1=+elements, 2=+volumes, 3=+areas, 4=all

    // Custom joint buffers (runtime injection)
    std::vector<CGAL_Polyline> CUSTOM_JOINTS_SS_E_IP_MALE;
    std::vector<CGAL_Polyline> CUSTOM_JOINTS_SS_E_IP_FEMALE;
    // ... similar for other categories
}
```

---

## Section 3 — Detection Pipeline (`cmake/src/wood/include/wood_main.cpp`)

9 sequential steps:

```
Step 1: get_elements()
    Input:  flat polyline pairs (2×N polylines) + insertion_vectors + JOINTS_TYPES
    Output: vector<wood::element>
    Work:   compute AABB, fit OBB (PCA), build side planes, initialise j_mf maps

Step 2: rtree_search()
    Input:  vector<element>
    Output: vector<pair<int,int>> — candidate element index pairs
    Work:   insert all AABBs into Boost RTree; query each against others;
            then OBB SAT (box_util) to filter false positives

Step 3: adjacency_search()
    Input:  element pairs (from XML adjacency OR rtree_search result)
    Output: ordered pair list for pair_search
    Work:   uses explicit adjacency list if provided in XML, else delegates to rtree_search

Step 4: pair_search()
    Input:  element pair + both elements
    Output: vector<wood::joint> (appended to global list)
    Work:   wraps beam volumes as temporary elements;
            dispatches to face_to_face / plane_to_face / border_to_face

Step 5a: face_to_face()
    Trigger: face normals approximately parallel (ANGLE tolerance)
    Method:  transform both face polylines to XY; Clipper2 2D intersection;
             back-project; fill joint_area, joint_lines, joint_volumes
    Types:   12 (SS_InPlane), 11 (SS_OutOfPlane), 13 (SS_Rotate), 40 (Top-Top)

Step 5b: plane_to_face()
    Trigger: face normal approximately perpendicular to other face
    Method:  cross face polyline with the plane of other element;
             build rectangle from intersection segment + thickness
    Types:   20 (Top-Side), 30 (Cross)

Step 5c: border_to_face()
    Trigger: boundary/single-element edge
    Method:  offset rectangle from edge
    Types:   60 (Boundary)

Step 6: three_valence_joint_addition_vidy()
    Input:  detected joints with coincident vertices
    Output: linked_joints / linked_joints_seq populated on joint structs
    Work:   identifies 3-element junction points; creates linking between joints

Step 7: construct_joint_by_index()
    Input:  joint with .type set
    Output: m[], f[], boolean_type arrays filled
    Work:   reads JOINTS_TYPES[face] to override type;
            looks up cache by "name;divisions;shift";
            calls the parametric function from joint_lib;
            applies unit_scale → remap to unit_scale_distance;
            orients cutting geometry from unit-box to world space

Step 8: element.get_joints_geometry()
    Input:  OUTPUT_GEOMETRY_TYPE flag
    Output: collected polylines per element for serialisation

Step 9: write_xml_polylines_and_types()
    Output: XML with cutting outlines + cut_type strings for Rhino
```

---

## Section 4 — Joint Library (`cmake/src/wood/include/wood_joint_lib.h/.cpp`)

### Type / Group / Slot Table

| Detected type | Group | Slot range | Category token | Description |
|---|---|---|---|---|
| 12 | 0 | 1–9 | `ss_e_ip` | side-side, edge, in-plane |
| 11 | 1 | 10–19 | `ss_e_op` | side-side, edge, out-of-plane |
| 20 | 2 | 20–29 | `ts_e_p` | top-side, edge, perpendicular |
| 30 | 3 | 30–39 | `cr_c_ip` | cross, centre, in-plane |
| 40 | 4 | 40–49 | `tt_e_p` | top-top, edge, perpendicular |
| 13 | 5 | 50–59 | `ss_e_r` | side-side, edge, rotated |
| 60 | 6 | 60–69 | `b` | boundary |

### Representative functions per category

Functions are named `<category>_N` where N starts at 0. The dispatch skips id=0
(`id_representing_joint_name > 0`), so effective slot range for dispatch is 1–9 per group.
Slot 9 within each group is always the custom XML-driven joint.

| Category | Functions | Notes |
|---|---|---|
| ss_e_ip | `ss_e_ip_0..5`, `side_removal`, `ss_e_ip_custom` (slot 9) | finger, dovetail, Hilti, custom |
| ss_e_op | `ss_e_op_0..6`, `side_removal`, `ss_e_op_custom` (slot 9) | finger, Vidy tenon-mortise (4), Theater Wall (5/6) |
| ts_e_p  | `ts_e_p_0..5`, `side_removal`, `ts_e_p_custom` (slot 9) | tenon-mortise, snap-fit, Annen (5) |
| cr_c_ip | `cr_c_ip_0..5`, `side_removal`, `cr_c_ip_custom` (slot 9) | rect(0), conical 9-cut(1), 5-cut Brussels(2), drill variants(3–5) |
| tt_e_p  | `tt_e_p_0..5`, `side_removal`, `tt_e_p_custom` (slot 9) | centre drill, inscribed circle, rect, grid |
| ss_e_r  | `ss_e_r_0..3`, `side_removal_ss_e_r_1`, `ss_e_r_custom` (slot 9) | rectangles, HILTI (Balteschwiler, slot 2), 3DEC (slot 3) |
| b       | `b_0`, `b_custom` (slot 9) | boundary cut, custom |

### Unit-box convention

All joint geometry lives in normalised space before orientation:
- x ∈ [-0.5, 0.5] — perpendicular to edge
- y ∈ [-0.5, 0.5] — along edge direction
- z ∈ [-0.5, 0.5] — thickness direction
- **Last polyline** in `m[0]` and `f[0]` is always the boundary outline rectangle

### Caching

Cache key = `"function_name;divisions;shift"`. Joints sharing key share geometry until the orientation step. Any change to `divisions` or `shift` busts the cache.

### Adding a new joint type — 7-step protocol

```
1. Pick category (detect type) + choose unused slot index N
2. Declare: void ss_e_ip_N(joint& j) in wood_joint_lib.h
3. Implement in wood_joint_lib.cpp:
      - fill j.m[0], j.m[1], j.f[0], j.f[1] in unit-box space
      - fill j.m_boolean_type, j.f_boolean_type (same size as m[0], f[0])
      - last polyline in m[0]/f[0] = boundary rectangle
      - j.name = __func__;
4. If joint should scale with element thickness: j.unit_scale = true
5. Register in construct_joint_by_index():
      joint_names[N] = "ss_e_ip_N";
6. Add dispatch: case N: ss_e_ip_N(j); break;
7. Add XML dataset + test case in wood_test.cpp
```

### Custom joint injection at runtime

```cpp
// Fill global buffers (each pair = top outline + bottom outline)
wood::GLOBALS::CUSTOM_JOINTS_SS_E_IP_MALE   = { top0, bottom0, top1, bottom1 };
wood::GLOBALS::CUSTOM_JOINTS_SS_E_IP_FEMALE = { top0, bottom0, top1, bottom1 };
// Set element face to slot 9 (custom slot for that category)
element.JOINTS_TYPES[face_id] = 9;   // positive = male, negative = female
```

---

## Section 5 — XML System (`cmake/src/wood/include/wood_xml.h/.cpp`)

### XML element format

```xml
<input_polylines>                       <!-- root element -->
  <Polyline>                            <!-- one polygon (5 pts = 4 corners + close) -->
    <point><x>...</x><y>...</y><z>...</z></point>
    ...
  </Polyline>
</input_polylines>
```

### Reading datasets

`read_xml_polylines_and_properties()` uses `boost::property_tree::ptree` and produces:
- `polyline_pairs` — flat vector of CGAL_Polyline (2×N, top/bottom alternating)
- `insertion_vectors[N]` — per-element insertion direction
- `JOINTS_TYPES[N]` — per-face type override vector
- `three_valence_instructions` — 3-element junction data
- `adjacency_pairs` — explicit element neighbour list (optional; falls back to rtree)

### Writing output

`write_xml_polylines_and_types()` writes cutting outlines + `cut_type` enum strings to XML for Rhino interoperability.

### Dataset naming convention

```
type_<structure>_name_<description>.xml
Examples:
  type_plates_name_cross_and_T_beam.xml
  type_beams_name_vidy_theater.xml
  type_shell_name_hexshell.xml
```

### Joint geometry XML

Located at `wood::GLOBALS::PATH_AND_FILE_FOR_JOINTS`. Format:

```xml
<custom_joints>
  <ss_e_ip_9>
    <!-- polyline data for custom slot -->
  </ss_e_ip_9>
</custom_joints>
```

---

## Section 6 — session_cpp Integration

**Module:** `cmake/ext/session_cpp/src/`

### Key classes

**`session_cpp::Mesh`** (`mesh.h`)
- Halfedge data structure
- `vertex_map`, `face_map`
- `halfedge[u][v] → face_index`
- Static factory: `Mesh::from_polylines(polygons, snap_tol)`
- Mutators: `weld(merge_dist)`, `unify_winding()`

**`session_cpp::Session`** (`session.h`)
- Container for geometry objects
- `pb_dump(filename)` — writes protobuf to `cmake/data/session.pb`
- `add_mesh(m)`, `add_polylines(pl)`, etc.
- `using Geometry = std::variant<Mesh, Polylines, ...>` (extend here for new types)

### Mesh construction order

```cpp
// 1. Build from polygon soup
session_cpp::Mesh m = session_cpp::Mesh::from_polylines(polygons, 0.01);

// 2. Merge near-duplicate vertices (REQUIRED before winding)
//    tolerance 1.0 needed for hexshell: vertex gaps are ~0.93 units
m.weld(1.0);

// 3. BFS-based face orientation (requires shared edges from weld)
m.unify_winding();

// 4. Build shape structures
CrossConnectors cc(m, thickness, positions, edge_div, rect_w, rect_h, rect_t);
```

### CGAL ↔ session_cpp point conversion

No bridge library exists; convert manually:

```cpp
// CGAL → session_cpp
session_cpp::Point sp{ cgal_pt.x(), cgal_pt.y(), cgal_pt.z() };

// session_cpp → CGAL
IK::Point_3 cp( sp[0], sp[1], sp[2] );
```

### Extending the Session geometry variant

1. Add type to `using Geometry = std::variant<Mesh, Polylines, NewType, ...>` in `session.h`
2. Add member to `Objects` class
3. Add `Session::add_NewType()` method
4. Update all visitor patterns: `compute_bounding_box`, `ray_intersect_geometry`, and any other `std::visit` call

### Name disambiguation

`stdafx.h` defines `typedef CGAL::Surface_mesh<IK::Point_3> Mesh` — always qualify:
```cpp
session_cpp::Mesh  // session halfedge mesh
CGAL::Surface_mesh<IK::Point_3>  // CGAL mesh (or use the typedef in wood headers)
```

---

## Section 7 — Shapes Pipeline (`cmake/src/shapes/shapes.h/.cpp`)

### `chevron_mesh`

```cpp
session_cpp::Mesh chevron_mesh(
    NurbsSurface srf,
    int    u_div  = 4,
    double v_dist = 900.0,
    double shift  = 0.5,
    double scale  = 0.05799
);
```
Output: halfedge Mesh with diamond tiling derived from the NURBS surface.

### `FoldedPlates`

```cpp
FoldedPlates(NurbsSurface srf, int u_div, int v_div,
             double thickness, double chamfer);
// Outputs:
//   .mesh           — the folded plate mesh
//   .polylines      — per-plate outline polylines
//   .insertion_lines — per-plate insertion vectors
```

### `CrossConnectors`

```cpp
CrossConnectors(session_cpp::Mesh& mesh,
                double thickness,
                std::vector<Point> positions,
                int    edge_div,
                double rect_w, double rect_h, double rect_t,
                double chamfer = 0.0);
// Outputs:
//   .face_polylines  — offset face outlines
//   .edge_polylines  — connector geometry along edges
```

**Constructor internal call order (strict):**
```
build_topology()
compute_face_planes()
compute_face_edge_planes()     // sum adjacent face normals, normalise
compute_bisector_planes()
compute_face_polylines()       // plane-intersection; NOT vertex projection
compute_edges()
compute_edge_faces()
compute_edge_planes_method()
compute_connectors()
```

**`compute_face_edge_planes()` implementation note:**
- Builds `edge → averaged_normal` map
- For each shared edge: sum normals of both adjacent faces, normalise
- Never use a single face normal at a shared edge (wrong dihedral)

**`compute_face_polylines()` implementation note:**
- Calls `outline_from_planes(offset_face_plane, fe_planes[i], bisector_planes[i])`
- Uses plane-plane-plane intersection
- Direct vertex projection is wrong when adjacent faces are non-coplanar

---

## Section 8 — Build Notes

| Topic | Detail |
|---|---|
| Executable | `cmake/build/Release/wood.exe` |
| Compiler flag | `/utf-8` required for fmt library (already in CMakeLists.txt) |
| session_core | `UNITY_BUILD OBJECT` library; `proto_objects` must be explicitly linked |
| cmake exit code | Exits 1 even on success (absl/protoc deps). Confirm success by `wood.exe` line in output |
| stdout | Buffered — apparent truncation on crash = buffer loss. Use `std::cerr` for debug |
| Mesh typedef | `stdafx.h` defines `Mesh = CGAL::Surface_mesh<IK::Point_3>`; always qualify `session_cpp::Mesh` |

### Build command

```bash
cmake --build cmake/build --config Release 2>&1 | grep -E "(wood\.exe|error|warning)" | head -40
```

---

## Section 9 — CGAL Utility Namespaces

### `cgal::intersection_util` (`cgal_intersection_util.h`)

```cpp
IK::Point_3  line_plane(IK::Line_3, IK::Plane_3);
IK::Point_3  plane_plane_plane(IK::Plane_3, IK::Plane_3, IK::Plane_3);
IK::Line_3   plane_plane(IK::Plane_3, IK::Plane_3);
CGAL_Polyline polyline_plane_cross_joint(CGAL_Polyline, IK::Plane_3);
```

### `cgal::polyline_util` (`cgal_polyline_util.h`)

```cpp
IK::Point_3  center(CGAL_Polyline);
IK::Plane_3  get_average_plane(std::vector<CGAL_Polyline>);
IK::Segment_3 get_middle_line(CGAL_Polyline top, CGAL_Polyline bottom);
CGAL_Polyline scale_line(IK::Segment_3, double scale);
```

### `cgal::xform_util` (`cgal_xform_util.h`)

```cpp
CGAL::Aff_transformation_3<IK> plane_to_xy(IK::Plane_3);
CGAL::Aff_transformation_3<IK> plane_to_plane(IK::Plane_3 from, IK::Plane_3 to);
CGAL::Aff_transformation_3<IK> rotation_in_xy_plane(double angle_radians);
```

### `cgal::box_util` (`cgal_box_util.h`)

```cpp
bool obb_obb_sat(IK::Vector_3 oob0[5], IK::Vector_3 oob1[5]); // SAT collision
IK::Point_3 point_at(IK::Vector_3 oob[5], double x, double y, double z);
```

### `cgal::math_util` (`cgal_math_util.h`)

```cpp
size_t unique_from_two_int(int a, int b);  // Cantor pairing — joints_map hash key
```

### `collider::clipper_util` (`clipper_util.h`)

```cpp
// Clipper2 2D polygon intersection (used in face_to_face step)
std::vector<CGAL_Polyline> intersect(CGAL_Polyline, CGAL_Polyline, IK::Plane_3);
```

### `collider::rtree_util` (`rtree_util.h`)

```cpp
// Boost.Geometry RTree AABB broad phase
std::vector<std::pair<int,int>> search(std::vector<wood::element>);
```

---

## Section 10 — Known Pitfalls

| # | Pitfall | Fix |
|---|---|---|
| 1 | Calling `unify_winding()` without prior `weld()` | BFS can't reach all faces without shared edges; always `weld` first |
| 2 | Hexshell vertex gaps ~0.93 units | Use `weld(1.0)`, not `weld(0.01)` |
| 3 | Single face normal at shared edges | Sum adjacent face normals, normalise; see `compute_face_edge_planes()` |
| 4 | Vertex projection in `compute_face_polylines` | Use `outline_from_planes(...)` plane intersection instead |
| 5 | `JOINTS_TYPES` negative = female | Zero = auto-detect; positive = male; check sign before dispatch |
| 6 | Slot 0 skipped in `construct_joint_by_index` | id=0 triggers `continue`; start real joints at slot 1 |
| 7 | Joint cache collision | Key = "name;divisions;shift"; different divisions must produce different key |
| 8 | `unit_scale` not set | Joint geometry remains in unit-box; will not match element thickness |
| 9 | `session_cpp::Mesh` vs typedef `Mesh` | `stdafx.h` typedef causes ambiguity; always qualify namespace |
| 10 | cmake exit code 1 on success | Caused by absl/protoc post-build steps; confirm by grepping for `wood.exe` |
| 11 | stdout truncation on crash | Buffer loss; use `std::cerr` for critical debug output |
| 12 | `proto_objects` not linked | session_core is UNITY_BUILD OBJECT; must explicitly list `proto_objects` in target_link_libraries |
| 13 | Missing `operator()` access | `joint(male, top)` maps to: (true,true)→m[0], (true,false)→m[1], (false,true)→f[0], (false,false)→f[1] |
| 14 | Boundary outline missing | Last polyline in `m[0]`/`f[0]` must be the boundary rectangle; omitting breaks boolean operations |
