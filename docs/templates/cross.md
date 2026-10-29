# Cross {#templates_cross}

[TOC]

Segmented plate shells joined across interior edges by cross-slotted connector plates (`src/templates/cross/vda_mesh.h`, example `templates_cross_1_vda_mesh`). `wood_cross::VdaMesh` takes any polygon `Mesh` and constructs mitered face plates, edge-spanning connector plates, and boolean `JointPlate::cross_slot()` half-laps across eight scene-tree groups (`mesh` → `face_planes` → `edge_planes` → `bisector_planes` → `plates` → `connector_frames` → `connectors` → `cross_laps`).

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#cross_planes">
    <img src="templates_cross_1_vda_mesh_edge_planes.webp" alt="Welded Input Mesh &amp; Reference Planes">
    <span class="wood-card-title">1. Mesh &amp; Reference Planes</span>
    <span class="wood-card-sub">Face planes, shared miter edge planes &amp; corner bisectors</span>
  </a>
  <a class="wood-card" href="#cross_plates">
    <img src="templates_cross_1_vda_mesh_plates.webp" alt="Mitered Face Plates">
    <span class="wood-card-title">2. Mitered Face Plates</span>
    <span class="wood-card-sub">Offset polygonal `Plate` layers (`plate_<f>_<layer>`)</span>
  </a>
  <a class="wood-card" href="#cross_connectors">
    <img src="templates_cross_1_vda_mesh_connectors.webp" alt="Edge Connector Plates">
    <span class="wood-card-title">3. Edge Connector Plates</span>
    <span class="wood-card-sub">Interior edge division frames &amp; `connector_<f0>_<f1>_<k>` plates</span>
  </a>
  <a class="wood-card" href="#cross_laps">
    <img src="templates_cross_1_vda_mesh_all.webp" alt="Cross-Lap Slots &amp; Assembly">
    <span class="wood-card-title">4. Cross-Lap Slots &amp; Assembly</span>
    <span class="wood-card-sub">`JointPlate::cross_slot()` across 15 face panels &amp; 58 connectors</span>
  </a>
</div>

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

### 1. Welded Input Mesh & Reference Planes {#cross_planes}

The input polygon mesh is welded at `0.01 mm` tolerance and `compute_edge_faces()` indexes the 1 or 2 faces adjacent to every mesh edge. Three sets of reference planes are then constructed directly on the mesh:

- **Face planes (`face_planes`)**: one `Plane` per face centroid oriented along its outward face normal.
- **Shared miter edge planes (`edge_planes`)**: one `Plane` at each half-edge midpoint whose `x`-axis runs along the directed edge and whose `y`-axis is the normalized sum of the normals of the faces sharing that edge, ensuring neighbouring face plates meet on a common miter plane.
- **Corner bisector planes (`bisector_planes`)**: one `Plane` at each polygon corner halving the two meeting edge planes (used as fallback when consecutive polygon edges are within `5.7°` of collinear, `cos > 0.995`).

![](templates/templates_cross_1_vda_mesh_edge_planes.webp)

```cpp
add_mesh(this->mesh, group_named("mesh"));
_edge_faces = compute_edge_faces();
_face_planes = compute_face_planes();
_edge_planes = compute_edge_planes();
_bisector_planes = compute_bisector_planes();
```

### 2. Mitered Face Plates {#cross_plates}

For each face and layer offset in `face_positions`, `compute_face_outlines()` offsets `_face_planes[f]` by `±0.5 * face_thickness` and clips it against `_edge_planes[f]` and `_bisector_planes[f]` to form the mitered polygonal `Plate` (`plate_<f>_<layer>`):

![](templates/templates_cross_1_vda_mesh_plates.webp)

```cpp
const Plane bottom = _face_planes[f].translate_by_normal(layer - 0.5 * face_thickness);
const Plane top = _face_planes[f].translate_by_normal(layer + 0.5 * face_thickness);
const Polyline bottom_loop = outline(bottom, _edge_planes[f], _bisector_planes[f]);
const Polyline top_loop = outline(top, _edge_planes[f], _bisector_planes[f]);
outlines[f].push_back({bottom_loop, top_loop});
```

### 3. Edge Connector Frames & Connector Plates {#cross_connectors}

Every interior edge (`_edge_faces[e].size() == 2`) is divided into evenly spaced stations `k / (1 + divisions)` with frame axes `z` along the edge, `y` along the average normal of the two faces, and `x = z × y` (or aligned to a matching `insertion_line`). Each connector frame is then extruded by `±0.5 * connector_thickness` along `z` as a `connector_width × connector_height` rectangular `Plate` (`connector_<f0>_<f1>_<k>`):

![](templates/templates_cross_1_vda_mesh_connector_frames.webp)

![](templates/templates_cross_1_vda_mesh_connectors.webp)

```cpp
for (int k = 1; k <= divisions; k++) {
    const Point station = lines[e].point_at(static_cast<double>(k) / (1.0 + divisions));
    frames[e].push_back(Plane(station, x_axis, y_axis));
}
...
const Vector to_corner = frame.x_axis() * (-0.5 * connector_width) + frame.y_axis() * (-0.5 * connector_height);
const Vector half = frame.z_axis() * (0.5 * connector_thickness);
const Polyline bottom = Polyline::rectangle(
    frame.origin() - half + to_corner, frame.x_axis(), frame.y_axis(), connector_width, connector_height
);
```

### 4. Cross-Lap Slots & Complete Crossed Assembly {#cross_laps}

For every connector and each layer plate of its two adjacent faces, `compute_cross_contact(connector, plate)` locates the 3D intersection segment and `JointPlate::cross_slot()` cuts complementary half-depth slots into both outlines, assembling the 15 face panels and 58 connector plates into a single interlocked shell:

![](templates/templates_cross_1_vda_mesh_cross_laps.webp)

![](templates/templates_cross_1_vda_mesh_all.webp)

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
