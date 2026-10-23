# Folding {#templates_folding}

[TOC]

Folded plate shells: a reflex fold and a diamond mesh. Each template is a `WoodSession` whose constructor is its recipe, one group per step; every picture below is one group of the example's scene.

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

Every quad becomes a plate mitered at its folds, its corners chamfered (`mitered_plates`, shared by the shells and folding templates).

![plates](templates/templates_folding_1_reflex_fold_plates.webp)

```cpp
const std::vector<std::shared_ptr<Plate>> folds = mitered_plates(
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

## Diamond mesh

A NURBS surface split into rhombi of two triangles each, one plate per triangle (`folding/diamond_mesh.h`, example `templates_folding_2_diamond_mesh`).

![diamond mesh](templates/templates_folding_2_diamond_mesh_plates.webp)

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

![surface](templates/templates_folding_2_diamond_mesh_surface.webp)

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

![mesh](templates/templates_folding_2_diamond_mesh_mesh.webp)

```cpp
triangles.push_back({b, d, centre});
triangles.push_back({a, centre, c});
triangles.push_back({d, next, centre});
triangles.push_back({next, c, centre});
```

### Plates

Every triangle becomes a plate mitered at its folds, its corners chamfered.

![plates](templates/templates_folding_2_diamond_mesh_plates.webp)

```cpp
const std::vector<std::shared_ptr<Plate>> triangles = mitered_plates(
    _mesh,
    thickness,
    chamfer,
    chamfer,
    chamfer_angle
);
```

<details><summary>Example code: templates_folding_2_diamond_mesh.cpp</summary>

\include{lineno} templates_folding_2_diamond_mesh.cpp

</details>
