# Chevron {#templates_chevron}

[TOC]

Free-form chevron box-plate shells on NURBS surfaces in `src/templates/chevron/chevron.h`. `Chevron` subdivides an input `NurbsSurface` into herringbone quad strips and builds a four-plate hollow box per cell.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#chevron_surface">
    <img src="templates_chevron_1_chevron_surface.webp" alt="Input Surface">
    <span class="wood-card-title">Input Surface</span>
  </a>
  <a class="wood-card" href="#chevron_mesh">
    <img src="templates_chevron_1_chevron_mesh.webp" alt="Herringbone Mesh">
    <span class="wood-card-title">Herringbone Mesh</span>
  </a>
  <a class="wood-card" href="#chevron_edge_planes">
    <img src="templates_chevron_1_chevron_edge_planes.webp" alt="Strips and Bisectors">
    <span class="wood-card-title">Strips and Bisectors</span>
  </a>
  <a class="wood-card" href="#chevron_plates">
    <img src="templates_chevron_1_chevron_plates.webp" alt="Hollow Boxes">
    <span class="wood-card-title">Hollow Boxes</span>
  </a>
  <a class="wood-card" href="#chevron_insertion">
    <img src="templates_chevron_1_chevron_joints.webp" alt="Joinery">
    <span class="wood-card-title">Joinery</span>
  </a>
</div>

## Parameters

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `surface` | `NurbsSurface` | `default_surface()` | Free-form NURBS reference surface |
| `u_divisions` | `int` | `4` | Number of longitudinal chevron strips |
| `v_division_dist` | `double` | `900.0` | Initial row step at strip supports in mm |
| `shift` | `double` | `0.5` | Chevron apex advance as a fraction of row height |
| `scale` | `double` | `0.05799` | Geometric step growth factor toward mid-span |
| `box_height` | `double` | `760.0` | Structural box depth along face normal in mm |
| `top_plate_inlet` | `double` | `80.0` | Inset of top and bottom plates from outer box planes in mm |
| `plate_thickness` | `double` | `40.0` | Thickness of top, bottom, and side plates in mm |
| `edge_rotation` | `double` | `1.0` | Rotation of even interior chevron edge planes in degrees |
| `edge_offset` | `double` | `0.5` | Normal shift of odd interior chevron edge planes |

## Input Surface {#chevron_surface}

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

Three-plane intersections clip top, bottom, and two active chevron side `Plate` solids for every cell, the four of a face kept in its group `box_<face>`.

![](templates/templates_chevron_1_chevron_plates.webp)

```cpp
for (const double level : levels)
    outlines.push_back(Polyline::from_planes(sides, face_plane.translate_by_normal(level)));

outlines.push_back(Polyline::from_planes(box, edges[edge]));
outlines.push_back(Polyline::from_planes(box, edges[edge].translate_by_normal(t)));
```

## Insertion Vectors and Joinery {#chevron_insertion}

`compute_joinery()` assigns assembly insertion vectors along the corner bisectors together with mortise and tenon face codes, the three-valence groups and the plate adjacency. The constructor hands these tables to the wood solver, `compute_features()`, which cuts the Annen tenons and mortises and the side joints at every mitre. On the first Annen surface the 736 plates and their joints are the 2024 Annen dataset `annen_grid_full_arch`. The picture shows the four boxes at the crown, `box_34`, `box_57`, `box_80` and `box_81`, with the joints cut into their plates.

![](templates/templates_chevron_1_chevron_joints.webp)

```cpp
#include "wood_session.h"
#include "src/templates/chevron/chevron.h"

int main() {
    Chevron shell;
    shell.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

## Shifted Pattern {#chevron_shifted}

`ChevronShifted` in `src/templates/chevron/chevron_shifted.h` is the second version of the template: the 2018 Annen pattern of the Grasshopper definition `ShiftedPattern-OptimizationUpdate/Annen200917.gh`, hexagonal cells in staggered columns instead of chevron strips. `Chevron` stays the default; the caller picks the class. Both build their plates from planes with the kernel's `Polyline::from_planes`, as `Plate` elements, and hand the same tables to the wood solver. Its ground truth is the definition's own output `Annen2.3dm`: every one of its 1075 plates is within 0.011 mm of a plate built here.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#chevron_shifted_cells">
    <img src="templates_chevron_2_shifted_mesh.webp" alt="Staggered Cells">
    <span class="wood-card-title">Staggered Cells</span>
  </a>
  <a class="wood-card" href="#chevron_shifted_planes">
    <img src="templates_chevron_2_shifted_edge_planes.webp" alt="Edge Planes">
    <span class="wood-card-title">Edge Planes</span>
  </a>
  <a class="wood-card" href="#chevron_shifted_plates">
    <img src="templates_chevron_2_shifted_plates.webp" alt="Cell Boxes">
    <span class="wood-card-title">Cell Boxes</span>
  </a>
  <a class="wood-card" href="#chevron_shifted_joinery">
    <img src="templates_chevron_2_shifted_joints.webp" alt="Joinery">
    <span class="wood-card-title">Joinery</span>
  </a>
</div>

### Parameters

The defaults are the slider values of the Grasshopper definition, read in millimetres.

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `surface` | `NurbsSurface` | `Chevron::default_surface()` | Reference surface, its u running along the shell as the Annen surfaces do |
| `iso_count` | `int` | `54` | Iso curves across the surface, the rows of the point grid (`DivIso`) |
| `point_count` | `int` | `9` | Points on every iso curve, one more than the columns (`DivIsoPts`) |
| `shift` | `double` | `0.4` | Share of the row spacing the interior points move along the rows |
| `edge_rotation` | `double` | `2.865` | Turn of the merged long-side planes about the surface normal in degrees, the definition's 0.05 rad, alternating per column pair |
| `plate_offset` | `double` | `300.0` | Distance from the surface to the middle of the top and bottom plates in mm (`offset`) |
| `plate_thickness` | `double` | `40.0` | Thickness of every plate in mm (`plateWidth`) |
| `wall_extend` | `double` | `80.0` | Length the walls run past the outer faces of the top and bottom plates in mm (`extend`) |
| `average_planes` | `bool` | `true` | Merge both halves of every even cell's long sides into one turned plane (`averagePlanes`) |

### Point Grid and Cells {#chevron_shifted_cells}

The definition rebuilt its surface before taking iso curves, which spaces them by arc length: the rows are the stations at equal arc length along the middle iso curve, the two ends and then half a step in, 54 in all. Every row is divided into 8 equal lengths across, and on the interior rows each point moves by `shift` of the row spacing towards the next row in even columns and the previous row in odd columns, so the cells lean alternately. A cell is a hexagon over three rows, `[i, j], [i, j+1], [i+1, j+1], [i+2, j+1], [i+2, j], [i+1, j]`, with the middle vertices shared with the neighbouring columns a row apart: a running bond. Even columns start on the first row and end with a half-cell quad; odd columns start with a quad. The cells are kept column by column, the odd columns from the far end, as the definition sorted them.

![](templates/templates_chevron_2_shifted_mesh.webp)

```cpp
const std::pair<double, double>& here = parameters[i][j];
const std::pair<double, double>& there = parameters[j % 2 == 0 ? i + 1 : i - 1][j];
points[i][j] = _across.point_at(
    here.first + (there.first - here.first) * shift,
    here.second + (there.second - here.second) * shift
);
```

### Edge Planes {#chevron_shifted_planes}

A plane stands on every mesh edge: through its middle, along it, with the surface normal as y. A naked edge, the supports included, takes the second derivative of the nearest long side curve instead. In the even columns the two halves of each long side are merged into one plane through their middle and turned by `edge_rotation` about the normal, `+` in columns 0 and 4, `-` in columns 2 and 6, which gives every wall between two columns its slant; a naked side is merged but not turned. Every cell then keeps its loop of side planes with the neighbour's cross plane inserted after edges 1 and 4, through its middle vertex, so a plate outline can kink where a long side meets walls from two offset neighbour cells.

![](templates/templates_chevron_2_shifted_edge_planes.webp)

```cpp
if (_cells.faces.edge_faces.at(first).size() != 1) {
    const std::pair<double, double> uv = _across.closest_parameters(middle);
    merged = turned_about_y(
        Plane(middle, a.x_axis() + b.x_axis(), _across.normal_at(uv.first, uv.second)),
        turn
    );
}
```

### Plates {#chevron_shifted_plates}

The face plane of a cell is the surface frame at the point nearest the length-weighted centre of its outline, Rhino's `Polyline.CenterPoint`, z into the surface. The top and bottom plates are the face plane moved by `plate_offset` ± half a plate, cut by the side loop stepped in by a wall on the sides where the cell's own walls stand: the bottom edge and the left side of a hexagon, the bottom and left of a last quad, nothing on a first quad. The walls stand on the sides `0, 1, 2, 4, 5` of an even hexagon, on side `3` of an odd one, on all four sides of a last quad and on side `2` of a first quad, each between the outer box faces and the two neighbouring side planes, a plate thick inward on the cell's own sides and outward on the sides it gives to the neighbouring column. Where a wall's slanted end crosses the middle of a merged side, two walls overlap by a thickness, as they do in the ground truth. A cell's plates and walls are kept in its group `box_<cell>`.

![](templates/templates_chevron_2_shifted_plates.webp)

```cpp
loop.push_back(
    flags[k] == 0
        ? sides[k].plane
        : stepped(sides[k].plane, plate_thickness, near.origin(), flags[k] < 0)
);
```

### Joinery {#chevron_shifted_joinery}

Every wall is joined to the top and bottom plates of both cells on its edge: adjacency rows `(wall, plate)`, wall first so the solver meets the wall's face before a plate's kink side meets the wall's end, and the three-valence rows `{top0, wall, top1, wall}` and `{bottom0, wall, bottom1, wall}` that align the tenons of the two plates sharing a wall. The insertion direction of a cell is the bisector of two of its own sides on its outline, `unit(e5) - unit(e0)` for an even hexagon, and goes to the edges whose walls the cell slides into; a plate's side face carries the direction of its edge and the joint type 20, the Annen tenon. From these tables the solver makes 1150 Annen joints and 138 plate joints and cuts them into all 1076 plates; the picture shows the four boxes at the crown, `box_67`, `box_68`, `box_93` and `box_94`.

![](templates/templates_chevron_2_shifted_joints.webp)

```cpp
for (const int cell : cells) {
    _adjacency.emplace_back(wall, plates.boxes[cell][0]);
    _adjacency.emplace_back(wall, plates.boxes[cell][1]);
}
```

### Ground Truth {#chevron_shifted_ground_truth}

`examples/templates_chevron_2_shifted.cpp` builds the pattern on the first Annen surface, the surface internalised in the definition. The top and bottom plates of `Annen2.3dm` carry the definition's own finger joints; here the wood solver cuts the joints, so the plates are compared under them. Against its 431 top and bottom plates and 644 walls, scaled from metres: every corner of every plate within 0.011 mm of a ground-truth vertex, every face plane within 0.007 mm, every side plane within 0.007 mm, and every ground-truth wall vertex within 0.011 mm of our outlines; the ground truth lacks the top plate of the last quad of column 0, which this template builds.

```cpp
#include "wood_session.h"
#include "src/templates/chevron/chevron_shifted.h"

int main() {
    ChevronShifted shell;
    shell.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```
