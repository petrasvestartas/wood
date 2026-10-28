# Chevron {#templates_chevron}

[TOC]

Free-form chevron box-plate shells on Annen NURBS surfaces (`src/templates/chevron/chevron.h`, example `templates_chevron_1_chevron`). `Chevron` subdivides a primary input `NurbsSurface` into herringbone quad strips and constructs a 4-plate hollow box per quad face across seven scene-tree groups (`surface` $\to$ `mesh` $\to$ `strips` $\to$ `edge_planes` $\to$ `bisectors` $\to$ `plates` $\to$ `insertion`).

![Chevron Shell Primary Inputs and 6-Step Pipeline](diagrams/template_chevron_pipeline.svg)

## Primary Inputs & Parameters

| Input / Parameter | Type | Default | Geometric Role |
| --- | --- | --- | --- |
| `surface` | `NurbsSurface` | `default_surface()` / `annen_surfaces(json)[0]` | Free-form NURBS reference surface $(u, v)$ |
| `u_divisions` | `int` | `4` | Number of longitudinal chevron strips across $u$ |
| `v_division_dist` | `double` | `900.0` | Initial row step $\Delta v_0$ at both ends of each strip |
| `shift` | `double` | `0.5` | Chevron apex advance along $v$ as a fraction of row height |
| `scale` | `double` | `0.05799` | Row-to-row geometric growth factor toward mid-span |
| `box_height` | `double` | `760.0` | Total structural box depth (`mm`) along the face normal |
| `top_plate_inlet` | `double` | `80.0` | Inset of top and bottom flange plates from outer box planes (`mm`) |
| `plate_thickness` | `double` | `40.0` | Thickness $t$ (`mm`) of all top, bottom, and side plates |
| `edge_rotation` | `double` | `1.0` | Rotation (`deg`) of even interior chevron edge planes about their $y$-axis |
| `edge_offset` | `double` | `0.5` | Normal shift of odd interior chevron edge planes as a fraction of $t$ |
| `ortho_edges` | `std::array<int, 4>` | `{1, 1, 1, 1}` | World-axis snap mode (`1` = dominant axis) for naked boundary edge planes |

---

## Geometry Generation Pipeline

### 1. Primary Input Surface (`surface`)

`wood_chevron::annen_surfaces` loads the 23 NURBS vault surfaces from `annen_surfaces.json` (reconstructing OpenNURBS knot vectors from `u_mults`/`v_mults`) and places the target surface into group `surface`:

![surface](templates/templates_chevron_1_chevron_surface.webp)

```cpp
const std::vector<NurbsSurface> surfaces = wood_chevron::annen_surfaces(path.string());
add_nurbssurface(surface, add_group("surface"));
```

### 2. Herringbone Quad Mesh (`mesh`)

The surface is transposed and divided into `u_divisions` strips. Within each strip, `chevron_row(u, v_side, v_middle)` advances from $v_{\min}$ toward the midpoint with step $\Delta v_{k+1} = \Delta v_k(1 + \text{scale})$ and apex offset `shift`, then mirrors the step sequence to $v_{\max}$:

![mesh](templates/templates_chevron_1_chevron_mesh.webp)

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

### 3. Chevron Strips (`strips`)

`compute_strips()` grows strips of faces sharing longitudinal edges `1-2` or `3-0` and assigns alternating `flips` (`strip_index % 2 == 0`) together with each face's two active `chevron_edges` (`{3, 0}` on even strips, `{0, 1}` on odd strips):

![strips](templates/templates_chevron_1_chevron_strips.webp)

```cpp
const bool flip = strip_index % 2 == 0;
const std::array<int, 2> edges = flip ? std::array<int, 2>{3, 0} : std::array<int, 2>{0, 1};
```

### 4. Turned & Offset Edge Planes (`edge_planes`)

Every face edge starts with a plane through its midpoint along the mean normal of its adjacent faces. `compute_turned_edge_planes` rotates even interior chevron edges by $\pm\text{edge\_rotation}$, shifts odd interior chevron edges along their normal by $t \cdot \text{edge\_offset}$ (copying the flipped plane to the neighbour face so they share one seam), and snaps naked boundary edges via `snapped_to_axis`:

![edge planes](templates/templates_chevron_1_chevron_edge_planes.webp)

```cpp
if (j % 2 == 1) {
    plane = plane.translate_by_normal(plate_thickness * edge_offset);
} else {
    const double turn = _strips.flips[face] ? angle : -angle;
    ...
}
```

### 5. Corner Dihedral Bisectors (`bisectors`)

At each face corner $j$, `dihedral_plane(edge_planes[f][(j+1)%4], edge_planes[f][j])` constructs the angle-bisector plane along the line where the two adjacent edge planes intersect:

![bisectors](templates/templates_chevron_1_chevron_bisectors.webp)

```cpp
bisectors[face][j] = dihedral_plane(_edge_planes[face][(j + 1) % 4], _edge_planes[face][j]);
```

### 6. 4-Plate Hollow Boxes (`plates`)

For each face in strip order, `polygon_from_planes` intersects 3-plane corners (`Intersection::plane_plane_plane`) to produce four `Plate` solids (`plate_<4*c + role>`):
- **Top & bottom plates (`role = 0, 1`)**: `face_plane` offset to $\pm(\frac{1}{2}\text{box\_height} - \text{top\_plate\_inlet}) \mp \frac{1}{2}t$, clipped by the four edge planes (with the two chevron side planes pushed out by $+t$).
- **Two chevron side plates (`role = 2, 3`)**: each active chevron `edge_plane` (and its $+t$ offset) clipped by the box top/bottom planes, the non-chevron neighbour edge plane, and the corner `bisector` between the two side plates:

![plates](templates/templates_chevron_1_chevron_plates.webp)

```cpp
for (const double level : levels)
    outlines.push_back(polygon_from_planes(face_plane.translate_by_normal(level), sides));

outlines.push_back(polygon_from_planes(edges[edge], box));
outlines.push_back(polygon_from_planes(edges[edge].translate_by_normal(t), box));
```

### 7. Insertion Vectors & Solver Joinery (`insertion`)

`compute_joinery()` assigns assembly `insertion_vectors` along the corner bisector seams, face joint codes (`20` mortise on top/bottom plates, `10` tenon on side plates), `three_valence` groups, and `adjacency` pairs:

![insertion](templates/templates_chevron_1_chevron_insertion.webp)

```cpp
_joints_per_face[pair + 0] = {0, 0, 20, 20, 20, 20};
_joints_per_face[pair + 1] = {0, 0, 20, 20, 20, 20};
_joints_per_face[pair + 2] = {0, 0, 10, 10, 10, 10};
_joints_per_face[pair + 3] = {0, 0, 10, 10, 10, 10};
```

---

## Full C++ Example (`examples/templates_chevron_1_chevron.cpp`)

\include{lineno} templates_chevron_1_chevron.cpp
