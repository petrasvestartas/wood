# Folding {#templates_folding}

[TOC]

Folded plate structures after Heino Engel, *Structure Systems*, chapter 4 "Surface-active structure systems", Folded plate structures (Faltwerke, pp. 219-233), and two free-form folds. Each template is a `WoodSession` whose constructor is its recipe, one group per step; every picture below is one group of the example's scene.

The topology is the same in every system: plates one after another, each meeting the next at a fold. A template builds the folded mid-surface as a mesh of planar faces, the plates' outer faces, and `mitered_plates` (`template_chamfer.h`) turns every face into one plate of the thickness: its inner face is its outer face moved along its normal by the thickness, and its side at a fold is the plane that halves the fold's angle, shared with the neighbour. A plate with thickness is therefore planar exactly when its face is planar, whatever the thickness, and two plates meet over the whole fold. So the geometry of each system is the rule that keeps its faces planar: a profile carried straight (quads), a profile that changes along the span (triangles), a pyramid (triangles and trapezoids).

| pages | Engel | template |
| --- | --- | --- |
| 219-221 | singly folded plate: threefold action, advantages over a rib slab, the number of folds, stiffening of the profile and of the free edges | [`PrismaticFold`](#prismatic_fold) |
| 225-227 | linear structure systems of folded surfaces: frames and arches | [`ReflexFold`](#reflex_fold) |
| - | a free-form fold of rhombi on a surface | [`DiamondMesh`](#diamond_mesh) |

## Singly folded plate {#prismatic_fold}

A fold profile across y carried straight along x over the span, one plate per fold (`folding/prismatic_fold.h`, examples `templates_folding_3_prismatic_fold` and `templates_folding_4_folded_barrel`). Engel's threefold bearing action of the singly folded plate (p. 219): each plate carries as a slab between its folds (Plattenwirkung), as a deep beam along the span (Scheibenwirkung), and with its neighbours as a truss or frame across it (Fachwerkwirkung). Against a rib slab of the same plan it halves the slab span, since every fold acts as a rigid support, needs no ribs, since every plate is also a beam along the span, and gains carrying capacity with its construction height, the fold depth.

![prismatic fold](templates/templates_folding_3_prismatic_fold_all.webp)

```cpp
PrismaticFold fold(
    PrismaticFold::zigzag(4, 1500.0, 1200.0),   // four ridges, plates 1500 across in plan, 1200 deep
    12000.0,                                   // span
    60.0,                                      // thickness
    3,                                         // diaphragms: both ends and mid-span
    200.0,                                     // edge beam width
    400.0                                      // edge beam depth
);
```

| | Engel | the default here |
| --- | --- | --- |
| profile | a zigzag of ridges and valleys, the supports under the valleys (p. 219) | four ridges, eight plates 1500 across in plan and 1200 deep, 12 m wide |
| span | the folds span between end supports; the depth is the construction height | 12 m, the depth a tenth of the span |
| plates | each fold plane a slab, a beam and a truss member | 60 mm, mitered at every fold |
| stiffening of the profile | the fold profile deforms: the lower edges spread, the plates buckle, the fold angles change; transverse diaphragms below or above the folds, or rigid frames below them (p. 221) | a diaphragm under the folds at both ends and at mid-span, 60 mm |
| stiffening of the free edges | the outer plates buckle under components normal to their plane; a vertical edge beam for shallow folding, a horizontal one for steep folding (p. 221) | a 200 x 400 beam along each free edge, vertical, the folds being shallow (38.7 degrees) |

### Profile

The profile is the cross section of the folds in the yz plane, running along +y: `zigzag(ridges, fold_width, depth)` for the folded plate of p. 219, `polygon(folds, width, rise)` for the systems of p. 220.

![profile](templates/templates_folding_3_prismatic_fold_profile.webp)

```cpp
for (size_t i = 0; i <= 2 * ridges; i++)
    points.push_back(Point(0.0, fold_width * static_cast<double>(i), i % 2 == 1 ? depth : 0.0));
```

### Folds

A fold line along the span at every profile point: the ridges, the valleys and the two free edges.

![folds](templates/templates_folding_3_prismatic_fold_folds.webp)

```cpp
for (size_t i = 0; i < profile.point_count(); i++)
    add_polyline(Polyline({profile[i], profile[i] + x * span}), folds);
```

### Mesh

One quad per profile segment between its two fold lines. A straight segment carried straight is a planar quad, so every plate of a singly folded plate is planar. The quads face inwards, so the plates grow from the outer face into the fold.

![mesh](templates/templates_folding_3_prismatic_fold_mesh.webp)

```cpp
for (size_t i = 0; i + 1 < count; i++)
    faces.push_back({2 * i, 2 * i + 2, 2 * i + 3, 2 * i + 1});
```

### Plates

Every quad becomes a plate 60 thick, its sides at the folds on the bisector planes, so neighbours meet over the whole fold. Measured on the example: every outline lies in its plane within 0.00001 mm, and the two faces are parallel and 60.00 apart.

![plates](templates/templates_folding_3_prismatic_fold_plates.webp)

```cpp
for (const std::shared_ptr<Plate>& plate : mitered_plates(_mesh, thickness, 0.0, 0.0, 180.0))
    add(plate, plates);
```

### Diaphragms

The transverse diaphragms below the folds (p. 221, Untergesetzte Querscheiben): at both ends and evenly between, each fills the profile's inside under the plates' inner faces down to their lowest level, one diaphragm under every ridge between two valleys, a free edge dropped onto that level. The inner profile moves every fold along its bisector by the thickness over the cosine of its half angle, so the diaphragms touch the plates' inner faces exactly.

![diaphragms](templates/templates_folding_3_prismatic_fold_diaphragms.webp)

```cpp
Vector bisector = normals[i - 1] + normals[i];
bisector.normalize_self();
inner.push_back(profile[i] + bisector * (thickness / bisector.dot(normals[i])));
```

### Edge beams

A beam along each free edge under the outer plate: vertical beside a shallow fold, horizontal and reaching outwards beside a steep one (p. 221, Randversteifer).

![edge beams](templates/templates_folding_3_prismatic_fold_edge_beams.webp)

```cpp
const bool steep = std::abs(slope[2]) > std::abs(slope[1]);
const double across = steep ? edge_beam_depth : edge_beam_width;
const double up = steep ? edge_beam_width : edge_beam_depth;
```

From the end, the four diaphragms under the ridges and the two edge beams:

![end](templates/templates_folding_3_prismatic_fold_end.webp)

### Number of folds

Engel (p. 220): with one fold the edge stresses at the common edge have the same tendency and the stress distribution stays unchanged; with two the unstressed horizontal plate is loaded through shear and reduces the edge stresses; with three the edge stresses at the side folds oppose each other and largely compensate through shear; with many the stresses are further distributed, and form and behaviour approach those of a cylindrical shell. `polygon(folds, width, rise)` puts the corners of folds + 1 plates on the circular arc over the width rising rise; the example takes seven folds, 12 m wide and 4 m high, with a diaphragm at each end and no edge beams.

![folded barrel](templates/templates_folding_4_folded_barrel_all.webp)

![folded barrel end](templates/templates_folding_4_folded_barrel_end.webp)

```cpp
PrismaticFold barrel(PrismaticFold::polygon(7, 12000.0, 4000.0), 12000.0, 60.0, 2, 0.0, 0.0, "folded_barrel");
```

<details><summary>Example code: templates_folding_3_prismatic_fold.cpp</summary>

\include{lineno} templates_folding_3_prismatic_fold.cpp

</details>

<details><summary>Example code: templates_folding_4_folded_barrel.cpp</summary>

\include{lineno} templates_folding_4_folded_barrel.cpp

</details>

## Reflex fold {#reflex_fold}

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

## Diamond mesh {#diamond_mesh}

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
