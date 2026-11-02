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
  <a class="wood-card" href="#chevron_max2">
    <img src="templates_chevron_3_max2_arch_joints.webp" alt="MAX2 Arch">
    <span class="wood-card-title">MAX2 Arch</span>
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
| `edge_offset` | `double` | `0.5` | Share of the plate thickness the odd interior chevron edge planes move along their normal |
| `tab_length` | `double` | `80.0` | Edge length per tenon division, Christopher's `L_tab`: `round(0.8 edge / tab_length)` segments, raised to one more than a multiple of four, a tenon on every fourth |
| `finger_angle` | `double` | `15.0` | Turn of the corner finger cuts from the box faces in degrees, each way in turn |
| `finger_divisions` | `int` | `7` | Parts every wall corner is cut in |
| `stop_threshold` | `double` | `2000.0` | The rows stop and mirror when less than this remains to the middle, in surface units (`Schwellwert`) |
| `ortho_edges` | `std::array<int, 4>` | `{1, 1, 1, 1}` | Per edge index the world axis a boundary edge plane snaps to, `1` the dominant one, `0` none |
| `boundary_planes` | `std::array<double, 3>` | `{NAN, NAN, NAN}` | The y of the front and of the back boundary plane and the z of the ground; `NAN` keeps a boundary plane through its edge |

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

Three-plane intersections clip top, bottom, and two active chevron side `Plate` solids for every cell, the four of a face kept in its group `box_<face>`. The box sides are the face's edge planes with the chevron ones moved out by a plate, where the walls stand; the top and bottom outlines run the way Christopher's contours run from their first corner, so the solver's side `2 + j` lies on mesh edge `(4 - j) % 4`.

![](templates/templates_chevron_1_chevron_plates.webp)

```cpp
const std::vector<Plane> sides = box_sides(face);

for (const double level : levels)
    outlines.push_back(
        Polyline::from_planes(sides, face_plane.translate_by_normal(level)).reversed()
    );

outlines.push_back(Polyline::from_planes(box, edges[edge]));
outlines.push_back(Polyline::from_planes(box, edges[edge].translate_by_normal(t)));
```

## Insertion Vectors and Joinery {#chevron_insertion}

`compute_joinery()` fills the tables the wood solver reads, as Christopher Robeller's 2017 plugin (`AnnenComponent.cs`) made them. The insertion vector of a box is the diagonal at the corner opposite the one its two walls share, `-(unit(B - A) + unit(D - A))` in the box plane, the same on all four sides of its top and bottom plates; the walls' mitre faces take the box's up. The plates' sides carry joint type 22, `top_side_annen_tenons`, with `round(0.8 mesh edge / tab_length)` divisions each; the mitre face of the wall on the second chevron edge carries type 17, `side_side_outplane_annen_fingers`, with `finger_divisions`. The adjacency joins every plate to its box's two walls and to the two walls of the neighbours across the chevron edges, and the three-valence rows pair the two plates that meet through one wall. `compute_features()` then cuts the joints in world space, on the plates' own edges: a through-tenon is the middle 80% of a plate edge, moved up to 200 mm towards the edge middle it shares with the plate across the wall, in `4m + 1` equal segments with a tenon on every fourth from the third, each tenon the edge pushed along the insertion vector through the wall to its outer face, so the plate across the wall, which runs the shared edge the other way, lands its tenons in the segments between; the wall takes a slot per tenon. The corner of two walls is cut from the box's bottom face to its top in seven parts by planes parallel to the box faces turned `finger_angle` each way in turn about the second wall's long edge, Christopher's `dovetail2`, the first part a finger of that wall, every finger reaching the other wall's outer face. On the first Annen surface the 736 plates are those of the 2024 Annen dataset `annen_grid_full_arch` within 0.03 mm; their joints are the plugin's, not the dataset's generic ones. The picture shows the four boxes at the crown, `box_34`, `box_57`, `box_80` and `box_81`, with the joints cut into their plates.

![](templates/templates_chevron_1_chevron_joints.webp)

```cpp
const std::array<int, 2> sorted = sorted_chevron_edges(face);
const Polyline corners = Polyline::from_planes(box_sides(face), _face_planes[face]);
const int a = (sorted[0] + 2) % 4;
const Vector diagonal = ((corners[(a + 1) % 4] - corners[a]).normalized() +
                         (corners[(a + 3) % 4] - corners[a]).normalized()) *
                        -1.0;

_joints_per_face[pair + 0] = {0, 0, 22, 22, 22, 22};
_joints_per_face[pair + 1] = {0, 0, 22, 22, 22, 22};
_joints_per_face[pair + 2] = {0, 0, 0, 0, 10, 0};
_joints_per_face[pair + 3] = {0, 0, 17, 0, 0, 0};
```

## MAX2 Arch {#chevron_max2}

`examples/templates_chevron_3_max2_arch.cpp` builds the second Annen surface with the slider values of the Grasshopper definition that drove the plugin for the 2017 MAX2 prototype of arches 21 and 22 (`2017_08_08_MAX2_22.gh`): rows from 900 growing by 0.04993 and stopping 2000 before the middle, 760 mm boxes with 40 mm plates 80 mm in from the box faces, the chevron edge planes turned 1.1 degrees and moved 18 mm, 80 mm tabs, corner fingers turned 15 degrees in seven parts, and the boundary planes of the prototype: the sides closed at y 132000 and 126000, the ground at z 3520. Its ground truth is the prototype's Rhino file, layer `solids_22`, the 800 jointed solids of arch 22 as the plugin cut them. Every one of the 800 plates built here has the vertex count of its solid; the two-way vertex distance per plate has a median of 0.77 mm, 739 plates lie within 2 mm and all but the plates of box 150 and four plates of its neighbours 101 and 151 within 2.71 mm, the rest a drift of the mesh itself, under 1 mm at the crown and up to 2.7 mm at the feet, carried by the raw plate corners: against its own outline every plate's tenons and fingers agree with the file's within 0.6 mm. Box 150 is the quirk of the definition: its ground edge is missing from its ground list, so the file's plates there stand up to 51 mm off the ground plane, and the four plates that meet them through its walls follow by up to 17 mm.

![](templates/templates_chevron_3_max2_arch_plates.webp)

![](templates/templates_chevron_3_max2_arch_joints.webp)

```cpp
Chevron chevron(
    surfaces[1], 4, 900.0, 0.5, 0.04993, 760.0, 80.0, 40.0, 1.1, 0.45, 80.0, 15.0, 7, 2000.0,
    {1, 1, 1, 1}, {132000.0, 126000.0, 3520.0}
);
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

Every wall is joined to the top and bottom plates of both cells on its edge: adjacency rows `(wall, plate)`, wall first so the solver meets the wall's face before a plate's kink side meets the wall's end, and the three-valence rows `{top0, wall, top1, wall}` and `{bottom0, wall, bottom1, wall}` that align the tenons of the two plates sharing a wall. The insertion direction of a cell is the bisector of two of its own sides on its outline, `unit(e5) - unit(e0)` for an even hexagon, and goes to the edges whose walls the cell slides into; a plate's side face carries the direction of its edge and the joint type 20, the Annen tenon. From these tables the solver makes 1150 Annen joints and 138 plate joints and cuts them into all 1076 plates; the picture looks into the crown of the shell, every plate and wall in place.

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
