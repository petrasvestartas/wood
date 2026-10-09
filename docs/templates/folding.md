# Folding {#templates_folding}

[TOC]

Folded plate shells: a reflex fold, the chevron strips of an Annen surface and a diamond mesh. Each template is a `WoodSession` whose constructor is its recipe, one group per step; every picture below is one group of the example's scene.

## Reflex fold

A zigzag profile carried along an arch and folded at every arch point, one plate per fold (`folding/reflex_fold.h`, example `templates_folding_1_reflex_fold`).

![reflex fold](templates/templates_folding_1_reflex_fold_plates.webp)

```cpp
ReflexFold fold(
    ReflexFold::default_cross_section(),
    ReflexFold::default_profile(),
    10.0,   // thickness
    20.0,   // chamfer_bottom
    20.0,   // chamfer_top
    180.0   // chamfer_angle: corners sharper than this are chamfered
);
```

### Curves

The cross section is the arch the profile travels along; the profile is the zigzag that folds.

![curves](templates/templates_folding_1_reflex_fold_curves.webp)

```cpp
const std::shared_ptr<TreeNode> curves = add_group("curves");
add_polyline(cross_section, curves);
add_polyline(profile, curves);
```

### Fold planes

Each inner arch point gets the plane that halves the arch's angle there, the two ends a plane normal to z.

![fold planes](templates/templates_folding_1_reflex_fold_fold_planes.webp)

```cpp
const Vector previous = (cross_section[i - 1] - cross_section[i]).normalized();
const Vector next = (cross_section[i + 1] - cross_section[i]).normalized();
normal = previous + next;
planes.push_back(Plane::from_point_normal(cross_section[i], normal));
```

### Mesh

Every profile point slides along the arch segment until it meets the next fold plane, and two rows make a strip of quads.

![mesh](templates/templates_folding_1_reflex_fold_mesh.webp)

```cpp
const Line along = Line::from_points(previous, previous + step);
const std::optional<Point> moved = Intersection::line_plane(along, fold_planes[i], false);
points.push_back(moved.value_or(previous));
```

### Plates

Every quad becomes a plate mitred at its folds, its corners chamfered (`mitred_plates`, shared by the shells and folding templates).

![plates](templates/templates_folding_1_reflex_fold_plates.webp)

```cpp
const std::vector<std::shared_ptr<Plate>> folds = mitred_plates(
    _mesh,
    thickness,
    chamfer_bottom,
    chamfer_top,
    chamfer_angle
);

for (const std::shared_ptr<Plate>& plate : folds)
    add(plate, plates);
```

<details><summary>Example code: templates_folding_1_reflex_fold.cpp</summary>

\include{lineno} templates_folding_1_reflex_fold.cpp

</details>

## Chevron

An Annen surface folded into chevron strips, each face a box of four plates: a top, a bottom and a side on each chevron edge (`folding/chevron.h`, example `templates_folding_2_chevron`).

![chevron](templates/templates_folding_2_chevron_plates.webp)

```cpp
Chevron chevron(
    surface,
    4,        // u_divisions: strips across u
    900.0,    // v_division_dist: the first row's height
    0.5,      // shift: how far the chevron's middle stands ahead
    0.05799,  // scale: row growth towards the middle
    760.0,    // box_height
    80.0,     // top_plate_inlet
    40.0,     // plate_thickness
    1.0,      // edge_rotation, degrees
    0.5,      // edge_offset, share of the plate thickness
    {1, 1, 1, 1}  // ortho_edges: boundary planes snapped to the dominant axis
);
```

### Surface

One of the 23 Annen surfaces read by `wood_chevron::annen_surfaces`.

![surface](templates/templates_folding_2_chevron_surface.webp)

```cpp
const std::vector<NurbsSurface> surfaces = wood_chevron::annen_surfaces(path.string());
add_nurbssurface(surface, add_group("surface"));
```

### Mesh

Each strip across u is cut into chevron rows that grow by `scale` from both ends and meet in the middle.

![mesh](templates/templates_folding_2_chevron_mesh.webp)

```cpp
const std::array<Point, 3> lower = row == 0 ? chevron_row(transposed, u, v, v) : upper;
upper = chevron_row(
    transposed,
    u,
    v + step * (1.0 - shift / 2.0),
    v + step * (1.0 + shift / 2.0)
);
polygons.push_back({lower[0], upper[0], upper[1], lower[1]});
polygons.push_back({lower[1], upper[1], upper[2], lower[2]});
```

### Strips

Faces that share an edge 1-2 or 3-0 grow into a strip, and each face marks its two chevron edges against the next strip.

![strips](templates/templates_folding_2_chevron_strips.webp)

```cpp
const bool flip = strip_index % 2 == 0;
const std::array<int, 2> edges = flip ? std::array<int, 2>{3, 0} : std::array<int, 2>{0, 1};
```

### Edge planes

A plane stands on every face edge; inner chevron edges turn by `edge_rotation` or move by `edge_offset`, boundary edges snap upright.

![edge planes](templates/templates_folding_2_chevron_edge_planes.webp)

```cpp
if (j % 2 == 1) {
    plane = plane.translate_by_normal(plate_thickness * edge_offset);
} else {
    const double turn = _strips.flips[face] ? angle : -angle;
    ...
}
```

### Bisectors

Every face corner gets the plane halfway between its two edge planes, where two side plates meet.

![bisectors](templates/templates_folding_2_chevron_bisectors.webp)

```cpp
bisectors[face][j] = dihedral_plane(_edge_planes[face][(j + 1) % 4], _edge_planes[face][j]);
```

### Plates

The face plane moved into the box gives the top and bottom plates, and each chevron edge plane cut by the box and the bisector gives a side.

![plates](templates/templates_folding_2_chevron_plates.webp)

```cpp
for (const double level : levels)
    outlines.push_back(polygon_from_planes(face_plane.translate_by_normal(level), sides));

outlines.push_back(polygon_from_planes(edges[edge], box));
outlines.push_back(polygon_from_planes(edges[edge].translate_by_normal(t), box));
```

### Joinery

Per plate pair the insertion vectors along the bisectors, mortises in the top and bottom, tenons in the sides, and the adjacency the solver reads; the line shows each face's insertion.

![insertion](templates/templates_folding_2_chevron_insertion.webp)

```cpp
_joints_per_face[pair + 0] = {0, 0, 20, 20, 20, 20};
_joints_per_face[pair + 1] = {0, 0, 20, 20, 20, 20};
_joints_per_face[pair + 2] = {0, 0, 10, 10, 10, 10};
_joints_per_face[pair + 3] = {0, 0, 10, 10, 10, 10};
```

<details><summary>Example code: templates_folding_2_chevron.cpp</summary>

\include{lineno} templates_folding_2_chevron.cpp

</details>

## Diamond mesh

A NURBS surface split into rhombi of two triangles each, one plate per triangle (`folding/diamond_mesh.h`, example `templates_folding_3_diamond_mesh`).

![diamond mesh](templates/templates_folding_3_diamond_mesh_plates.webp)

```cpp
DiamondMesh diamond(
    DiamondMesh::default_surface(),
    8,      // u_divisions
    4,      // v_divisions
    40.0,   // thickness
    10.0,   // chamfer
    180.0   // chamfer_angle
);
```

### Surface

The default surface is a bicubic arch, 3000 across, 5000 long and 1500 high.

![surface](templates/templates_folding_3_diamond_mesh_surface.webp)

```cpp
NurbsSurface arch = NurbsSurface::create(
    false,
    false,
    3,
    3,
    4,
    4,
    points
);
```

### Mesh

Each cell's corners join its centre and the next cell's centre, so two triangles make a rhombus across every cell edge.

![mesh](templates/templates_folding_3_diamond_mesh_mesh.webp)

```cpp
triangles.push_back({b, d, centre});
triangles.push_back({a, centre, c});
triangles.push_back({d, next, centre});
triangles.push_back({next, c, centre});
```

### Plates

Every triangle becomes a plate mitred at its folds, its corners chamfered.

![plates](templates/templates_folding_3_diamond_mesh_plates.webp)

```cpp
const std::vector<std::shared_ptr<Plate>> triangles = mitred_plates(
    _mesh,
    thickness,
    chamfer,
    chamfer,
    chamfer_angle
);
```

<details><summary>Example code: templates_folding_3_diamond_mesh.cpp</summary>

\include{lineno} templates_folding_3_diamond_mesh.cpp

</details>
