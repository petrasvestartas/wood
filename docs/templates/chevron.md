# Chevron {#templates_chevron}

[TOC]

Free-form chevron box-plate shells on NURBS surfaces in `src/templates/chevron/chevron.h`. `Chevron` subdivides an input `NurbsSurface` into herringbone quad strips and builds a four-plate hollow box per cell.

## Parameters

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| surface | NurbsSurface | default_surface() | Free-form NURBS reference surface |
| u_divisions | int | 4 | Number of longitudinal chevron strips |
| v_division_dist | double | 900.0 | Initial row step at strip supports in mm |
| shift | double | 0.5 | Chevron apex advance as a fraction of row height |
| scale | double | 0.05799 | Geometric step growth factor toward mid-span |
| box_height | double | 760.0 | Structural box depth along face normal in mm |
| top_plate_inlet | double | 80.0 | Inset of top and bottom plates from outer box planes in mm |
| plate_thickness | double | 40.0 | Thickness of top, bottom, and side plates in mm |
| edge_rotation | double | 1.0 | Rotation of even interior chevron edge planes in degrees |
| edge_offset | double | 0.5 | Normal shift of odd interior chevron edge planes |

## Input Surface

`wood_chevron::annen_surfaces` loads the NURBS vault surfaces and adds the reference surface to the `surface` group.

![](templates/templates_chevron_1_chevron_surface.webp)

```cpp
const std::vector<NurbsSurface> surfaces = wood_chevron::annen_surfaces(path.string());
add_nurbssurface(surface, add_group("surface"));
```

## Herringbone Quad Mesh {#chevron_mesh}

Each strip steps from the support toward mid-span with geometric growth `1 + scale` and chevron offset `shift`, mirrored symmetrically to the opposite support.

![](templates/templates_chevron_1_chevron_mesh.webp)

```cpp
const std::array<Point, 3> lower = row == 0 ? chevron_row(transposed, u, v, v) : upper;
upper = chevron_row(
    transposed, u,
    v + step * (1.0 - shift / 2.0),
    v + step * (1.0 + shift / 2.0)
);
polygons.push_back({lower[0], upper[0], upper[1], lower[1]});
polygons.push_back({lower[1], upper[1], upper[2], lower[2]});
```

## Strips, Edge Planes, and Bisectors {#chevron_edge_planes}

Alternating strip orientations select two active chevron side edges per quad, rotate even interior seams by `edge_rotation`, offset odd interior seams by `plate_thickness * edge_offset`, and compute corner dihedral bisectors.

![](templates/templates_chevron_1_chevron_strips.webp)

![](templates/templates_chevron_1_chevron_edge_planes.webp)

![](templates/templates_chevron_1_chevron_bisectors.webp)

```cpp
bisectors[face][j] = dihedral_plane(_edge_planes[face][(j + 1) % 4], _edge_planes[face][j]);
```

## Four-Plate Hollow Boxes {#chevron_plates}

Three-plane intersections clip top, bottom, and two active chevron side `Plate` solids for every cell.

![](templates/templates_chevron_1_chevron_plates.webp)

```cpp
for (const double level : levels)
    outlines.push_back(polygon_from_planes(face_plane.translate_by_normal(level), sides));

outlines.push_back(polygon_from_planes(edges[edge], box));
outlines.push_back(polygon_from_planes(edges[edge].translate_by_normal(t), box));
```

## Insertion Vectors and Joinery {#chevron_insertion}

`compute_joinery()` assigns assembly insertion vectors along the corner bisectors together with mortise and tenon face codes.

![](templates/templates_chevron_1_chevron_insertion.webp)

```cpp
#include "wood_session.h"
#include "src/templates/chevron/chevron.h"

int main() {
    Chevron shell;
    shell.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```
