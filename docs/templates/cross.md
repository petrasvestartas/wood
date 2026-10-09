# Cross {#templates_cross}

[TOC]

Plates on the faces of a mesh, joined by connector plates that cross every interior edge.

## vda_mesh

`wood_cross::VdaMesh` is a session that turns any mesh into one mitred plate per face and a row of connector plates across every interior edge.

![vda mesh](templates/templates_cross_1_vda_mesh_all.webp)

The constructor is the recipe, one block per step, each step in its own group:

```cpp
explicit VdaMesh(
    const Mesh& mesh = default_mesh(),
    double face_thickness = 20.0,
    const std::vector<double>& face_positions = {0.0},
    const std::vector<int>& edge_divisions = {2},
    const std::vector<double>& edge_division_length = {},
    const std::vector<Line>& insertion_lines = {},
    double connector_width = 200.0,
    double connector_height = 200.0,
    double connector_thickness = 20.0,
    const std::string& name = "vda_mesh"
);
```

```
vda_mesh
├── mesh               the welded input mesh
├── face_planes        Plane per face
├── edge_planes        Plane per face edge
├── bisector_planes    Plane per face corner
├── plates             Plate plate_<face>_<layer>
├── connector_frames   Plane per connector
└── connectors         Plate connector_<face0>_<face1>_<k>
```

### Mesh

The input mesh is welded at 0.01 and every edge learns the faces either side of it.

![mesh](templates/templates_cross_1_vda_mesh_mesh.webp)

```cpp
// mesh: the welded input mesh and the faces either side of every edge
add_mesh(this->mesh, group_named("mesh"));
const std::vector<std::vector<size_t>> edge_faces = compute_edge_faces();
_edge_faces = edge_faces;
```

### Face planes

Every face gets a plane at its centroid along its normal.

![face planes](templates/templates_cross_1_vda_mesh_face_planes.webp)

```cpp
for (const size_t face : mesh.faces()) {
    const Point centroid = mesh.face_centroid(face).value();
    const Vector normal = mesh.face_normal(face).value();
    planes.push_back(Plane::from_point_normal(centroid, normal));
}
```

### Edge planes

Every face edge gets a plane through the edge, its y axis the average normal of the faces that share it, so two neighbouring plates are cut by the same plane.

![edge planes](templates/templates_cross_1_vda_mesh_edge_planes.webp)

```cpp
Vector x_axis = Vector::from_points(end, start);
x_axis.normalize_self();
Vector y_axis(0.0, 0.0, 0.0);

for (const size_t neighbour : _edge_faces[edge_index.at(std::minmax(edge.first, edge.second))])
    y_axis += _face_planes[neighbour].z_axis();

y_axis.normalize_self();
planes[f].push_back(Plane(Point::mid_point(start, end), x_axis, y_axis));
```

### Bisector planes

Every face corner gets the plane through its vertex halving its two edge planes; it cuts the corner where the two edges run within 5.7 degrees of each other and their planes would cross too flat.

![bisector planes](templates/templates_cross_1_vda_mesh_bisector_planes.webp)

```cpp
const Point corner = mesh.vertex_point(face_edges[j].second).value();
const Vector seam = side.z_axis().cross(next.z_axis());

// edges in line: the plane across the edge at the corner; else the plane along the seam between the inward normals
if (seam.magnitude() < 1e-9)
    planes[f].push_back(Plane(corner, side.z_axis(), side.y_axis()));
else
    planes[f].push_back(Plane(corner, seam, side.z_axis() + next.z_axis()));
```

### Plates

Each face plane is offset to the bottom and top of every layer and consecutive edge planes, or the bisector at a nearly straight corner, cut its outline there, so neighbouring plates meet in mitres.

![plates](templates/templates_cross_1_vda_mesh_plates.webp)

```cpp
const Plane bottom = _face_planes[f].translate_by_normal(layer - 0.5 * face_thickness);
const Plane top = _face_planes[f].translate_by_normal(layer + 0.5 * face_thickness);
const Polyline bottom_loop = outline(bottom, _edge_planes[f], _bisector_planes[f]);
const Polyline top_loop = outline(top, _edge_planes[f], _bisector_planes[f]);
outlines[f].push_back({bottom_loop, top_loop});
```

### Connector frames

Every interior edge gets a frame at each division: z along the edge, y the average normal, x across the edge into its first face or along an insertion line.

![connector frames](templates/templates_cross_1_vda_mesh_connector_frames.webp)

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

### Connectors

Every frame becomes a `connector_width` by `connector_height` plate, `connector_thickness` along the edge, crossing both face plates.

![connectors](templates/templates_cross_1_vda_mesh_connectors.webp)

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

<details><summary>Example code: templates_cross_1_vda_mesh.cpp</summary>

\include{lineno} templates_cross_1_vda_mesh.cpp

</details>
