# Cross {#templates_cross}

[TOC]

Segmented plate shells joined across interior edges by cross-slotted connector plates (`src/templates/cross/vda_mesh.h`, example `templates_cross_1_vda_mesh`). `wood_cross::VdaMesh` takes any polygon `Mesh` and constructs mitered face plates, edge-spanning connector plates, and boolean `JointPlate::cross_slot()` half-laps across eight scene-tree groups (`mesh` → `face_planes` → `edge_planes` → `bisector_planes` → `plates` → `connector_frames` → `connectors` → `cross_laps`).

![](diagrams/template_cross_pipeline.webp)

## Parameters

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `mesh` | `Mesh` | `default_mesh()` (`15`-face dome) | Input polygon mesh (automatically welded at `0.01 mm`) |
| `face_thickness` | `double` | `20.0` (`40.0` in example) | Thickness (`mm`) of each face plate layer along the face normal |
| `face_positions` | `std::vector<double>` | `{0.0}` | Signed normal offsets of plate layers per face (supports single or multi-layer shells) |
| `edge_divisions` | `std::vector<int>` | `{2}` | Number of connectors per interior edge (single value for all edges, or one per edge) |
| `edge_division_length` | `std::vector<double>` | `{}` | Target spacing (`mm`) along interior edges (overrides `edge_divisions` when non-empty) |
| `insertion_lines` | `std::vector<Line>` | `{}` | Optional guide lines orienting the `x`-axis of connectors on touched edges |
| `connector_width` | `double` | `200.0` (`300.0` in example) | Connector length (`mm`) across the seam in the plane of the faces |
| `connector_height` | `double` | `200.0` (`300.0` in example) | Connector height (`mm`) along the average normal of the two faces |
| `connector_thickness` | `double` | `20.0` (`40.0` in example) | Connector plate thickness (`mm`) along the mesh edge |

```
vda_mesh
├── mesh               welded input Mesh
├── face_planes        Plane per face centroid
├── edge_planes        Plane per face edge (leaning on average face normal)
├── bisector_planes    Plane per face corner (halving adjacent edge planes)
├── plates             Plate plate_<face>_<layer>
├── connector_frames   Plane per interior edge division
├── connectors         Plate connector_<face0>_<face1>_<k>
└── cross_laps         JointPlate cross_lap_<face0>_<face1>_<k>_<face>_<layer>
```

---

## Geometry Generation Pipeline

### 1. Welded Input Mesh

The input mesh is welded at `0.01 mm` tolerance and `compute_edge_faces()` indexes the 1 or 2 faces adjacent to every mesh edge:

![](templates/templates_cross_1_vda_mesh_mesh.webp)

```cpp
add_mesh(this->mesh, group_named("mesh"));
const std::vector<std::vector<size_t>> edge_faces = compute_edge_faces();
_edge_faces = edge_faces;
```

### 2. Face Planes

Each face receives a reference plane at its centroid oriented along its outward face normal:

![](templates/templates_cross_1_vda_mesh_face_planes.webp)

```cpp
for (const size_t face : mesh.faces()) {
    const Point centroid = mesh.face_centroid(face).value();
    const Vector normal = mesh.face_normal(face).value();
    planes.push_back(Plane::from_point_normal(centroid, normal));
}
```

### 3. Shared Miter Edge Planes

Every edge of a face receives a plane through the edge midpoint whose `x`-axis runs along the directed half-edge and whose `y`-axis is the normalized sum of the normals of the faces sharing that edge, ensuring neighbouring face plates meet on a common miter plane:

![](templates/templates_cross_1_vda_mesh_edge_planes.webp)

```cpp
Vector x_axis = Vector::from_points(end, start);
x_axis.normalize_self();
Vector y_axis(0.0, 0.0, 0.0);

for (const size_t neighbour : _edge_faces[edge_index.at(std::minmax(edge.first, edge.second))])
    y_axis += _face_planes[neighbour].z_axis();

y_axis.normalize_self();
planes[f].push_back(Plane(Point::mid_point(start, end), x_axis, y_axis));
```

### 4. Corner Bisector Planes

At each vertex corner of a face, `compute_bisector_planes()` constructs the plane halving the two meeting edge planes. When two consecutive edges of a polygon run within `5.7°` of collinear (`cos > 0.995`), `outline()` intersects the base plane with the corner bisector plane instead of the nearly parallel edge planes:

![](templates/templates_cross_1_vda_mesh_bisector_planes.webp)

```cpp
const Point corner = mesh.vertex_point(face_edges[j].second).value();
const Vector seam = side.z_axis().cross(next.z_axis());

if (seam.magnitude() < 1e-9)
    planes[f].push_back(Plane(corner, side.z_axis(), side.y_axis()));
else
    planes[f].push_back(Plane(corner, seam, side.z_axis() + next.z_axis()));
```

### 5. Mitered Face Plates

For each face and layer offset in `face_positions`, `compute_face_outlines()` offsets `_face_planes[f]` by `±0.5 * face_thickness` and clips it against `_edge_planes[f]` and `_bisector_planes[f]` to form `plate_<f>_<layer>`:

![](templates/templates_cross_1_vda_mesh_plates.webp)

```cpp
const Plane bottom = _face_planes[f].translate_by_normal(layer - 0.5 * face_thickness);
const Plane top = _face_planes[f].translate_by_normal(layer + 0.5 * face_thickness);
const Polyline bottom_loop = outline(bottom, _edge_planes[f], _bisector_planes[f]);
const Polyline top_loop = outline(top, _edge_planes[f], _bisector_planes[f]);
outlines[f].push_back({bottom_loop, top_loop});
```

### 6. Interior-Edge Connector Frames

Every interior edge (`_edge_faces[e].size() == 2`) is divided into evenly spaced stations `k / (1 + divisions)` with frame axes `z` along the edge, `y` along the average normal of the two faces, and `x = z × y` (or aligned to a matching `insertion_line`):

![](templates/templates_cross_1_vda_mesh_connector_frames.webp)

```cpp
Vector z_axis = lines[e].to_direction();
z_axis.normalize_self();
Vector y_axis = face0.z_axis() + face1.z_axis();
y_axis.normalize_self();
Vector x_axis = z_axis.cross(y_axis);
x_axis.normalize_self();
...
for (int k = 1; k <= divisions; k++) {
    const Point station = lines[e].point_at(static_cast<double>(k) / (1.0 + divisions));
    frames[e].push_back(Plane(station, x_axis, y_axis));
}
```

### 7. Connector Plates

Each connector frame is extruded by `±0.5 * connector_thickness` along `z` as a `connector_width × connector_height` rectangular `Plate` (`connector_<f0>_<f1>_<k>`):

![](templates/templates_cross_1_vda_mesh_connectors.webp)

```cpp
const Vector to_corner = frame.x_axis() * (-0.5 * connector_width) + frame.y_axis() * (-0.5 * connector_height);
const Vector half = frame.z_axis() * (0.5 * connector_thickness);
const Polyline bottom = Polyline::rectangle(
    frame.origin() - half + to_corner,
    frame.x_axis(),
    frame.y_axis(),
    connector_width,
    connector_height
);
```

### 8. Cross-Lap Slots

For every connector and each layer plate of its two adjacent faces, `compute_cross_contact(connector, plate)` locates the 3D intersection segment and `JointPlate::cross_slot()` cuts complementary half-depth slots into both outlines:

![](templates/templates_cross_1_vda_mesh_cross_laps.webp)

```cpp
const std::shared_ptr<InteractionContactCross> crossing = compute_cross_contact(connector, plate);

if (!crossing)
    throw std::runtime_error(fmt::format("{} does not cross {}", connector_name, plate->name));

const std::shared_ptr<JointPlate> joint = JointPlate::cross_slot();
joint->orient(crossing, {connector, plate});
```

<details>
<summary>Example code: examples/templates_cross_1_vda_mesh.cpp</summary>

\include{lineno} templates_cross_1_vda_mesh.cpp

</details>
