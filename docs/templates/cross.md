# Cross {#templates_cross}

[TOC]

Segmented plate shells joined across interior edges by cross-slotted connector plates in `src/templates/cross/vda_mesh.h`. `wood_cross::VdaMesh` takes a polygon `Mesh` and builds mitered face plates, edge connector plates, and `JointPlate::cross_slot()` half-laps.

## Parameters

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| mesh | Mesh | default_mesh() | Input polygon mesh welded at 0.01 mm |
| face_thickness | double | 20.0 | Face plate layer thickness in mm |
| face_positions | std::vector<double> | {0.0} | Signed normal offsets of plate layers per face |
| edge_divisions | std::vector<int> | {2} | Number of connectors per interior edge |
| edge_division_length | std::vector<double> | {} | Target connector spacing along interior edges in mm |
| insertion_lines | std::vector<Line> | {} | Optional guide lines orienting connector axes |
| connector_width | double | 200.0 | Connector length across the seam in mm |
| connector_height | double | 200.0 | Connector height along the average face normal in mm |
| connector_thickness | double | 20.0 | Connector plate thickness along the mesh edge in mm |

## Mesh and Reference Planes {#cross_planes}

The input mesh is welded and indexed by half-edge adjacency to construct face centroid planes, shared miter edge planes along average face normals, and corner bisector planes.

![](templates/templates_cross_1_vda_mesh_edge_planes.webp)

```cpp
add_mesh(this->mesh, group_named("mesh"));
_edge_faces = compute_edge_faces();
_face_planes = compute_face_planes();
_edge_planes = compute_edge_planes();
_bisector_planes = compute_bisector_planes();
```

## Mitered Face Plates {#cross_plates}

Each face plane is offset by `±0.5 * face_thickness` per layer in `face_positions` and clipped against the shared edge and bisector planes.

![](templates/templates_cross_1_vda_mesh_plates.webp)

```cpp
const Plane bottom = _face_planes[f].translate_by_normal(layer - 0.5 * face_thickness);
const Plane top = _face_planes[f].translate_by_normal(layer + 0.5 * face_thickness);
const Polyline bottom_loop = outline(bottom, _edge_planes[f], _bisector_planes[f]);
const Polyline top_loop = outline(top, _edge_planes[f], _bisector_planes[f]);
outlines[f].push_back({bottom_loop, top_loop});
```

## Edge Connector Plates {#cross_connectors}

Interior edges are divided into evenly spaced connector frames aligned with the average normal of the two adjacent faces and extruded into rectangular `Plate` connectors.

![](templates/templates_cross_1_vda_mesh_connector_frames.webp)

![](templates/templates_cross_1_vda_mesh_connectors.webp)

```cpp
for (int k = 1; k <= divisions; k++) {
    const Point station = lines[e].point_at(static_cast<double>(k) / (1.0 + divisions));
    frames[e].push_back(Plane(station, x_axis, y_axis));
}
```

## Cross-Lap Slots and Assembly {#cross_laps}

`compute_cross_contact` locates each intersection between a connector and its adjacent face plates, and `JointPlate::cross_slot()` cuts complementary half-depth slots into both outlines.

![](templates/templates_cross_1_vda_mesh_cross_laps.webp)

![](templates/templates_cross_1_vda_mesh_all.webp)

```cpp
#include "wood_session.h"
#include "src/templates/cross/vda_mesh.h"

int main() {
    wood_cross::VdaMesh shell(
        wood_cross::VdaMesh::default_mesh(),
        40.0, {0.0}, {2}, {}, {}, 300.0, 300.0, 40.0
    );
    shell.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```
