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

`compute_joinery()` fills the tables the wood solver reads, as Christopher Robeller's 2017 plugin (`AnnenComponent.cs`) made them. The insertion vector of a box is the diagonal at the corner opposite the one its two walls share, `-(unit(B - A) + unit(D - A))` in the box plane, the same on all four sides of its top and bottom plates; the walls' mitre faces take the box's up. The plates' sides carry joint type 22, `top_side_annen_tenons`, with `round(0.8 mesh edge / tab_length)` divisions each; the mitre face of the wall on the second chevron edge carries type 17, `side_side_outplane_annen_fingers`, with `finger_divisions`. The adjacency joins every plate to its box's two walls and to the two walls of the neighbours across the chevron edges, and the three-valence rows pair the two plates that meet through one wall. `compute_features()` then cuts the joints in world space, on the plates' own edges: a through-tenon is the middle 80% of a plate edge, moved up to 200 mm towards the edge middle it shares with the plate across the wall, in `4m + 1` equal segments with a tenon on every fourth from the third, each tenon the edge pushed along the insertion vector through the wall to its outer face, so the plate across the wall, which runs the shared edge the other way, lands its tenons in the segments between; the wall takes a slot per tenon. The corner of two walls is cut from the box's bottom face to its top in seven parts by planes parallel to the box faces turned `finger_angle` each way in turn about the second wall's long edge, Christopher's `dovetail2`, the first part a finger of that wall, every finger reaching the other wall's outer face. On the first Annen surface the 736 plates are those of the 2024 Annen dataset `annen_grid_full_arch` within 0.03 mm; their joints are the plugin's, not the dataset's generic ones. The picture shows a 4 x 4 block at the crown, `box_56` to `box_59`, `box_79` to `box_82`, `box_102` to `box_105` and `box_125` to `box_128`, four neighbouring boxes of each of the four middle rows of boxes, with the joints cut into their plates: the tenons on the open sides wait for the walls of the boxes around the block.

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

`examples/templates_chevron_3_max2_arch.cpp` builds the second Annen surface with the slider values of the Grasshopper definition that drove the plugin for the 2017 MAX2 prototype of arches 21 and 22 (`2017_08_08_MAX2_22.gh`): rows from 900 growing by 0.04993 and stopping 2000 before the middle, 760 mm boxes with 40 mm plates 80 mm in from the box faces, the chevron edge planes turned 1.1 degrees and moved 18 mm, 80 mm tabs, corner fingers turned 15 degrees in seven parts, and the boundary planes of the prototype: the sides closed at y 132000 and 126000, the ground at z 3520. Its ground truth is the prototype's Rhino file, layer `solids_22`, the 800 jointed solids of arch 22 as the plugin cut them. Every one of the 800 plates built here has the vertex count of its solid; the two-way vertex distance per plate has a median of 0.77 mm, 739 plates lie within 2 mm and all but the plates of box 150 and four plates of its neighbours 101 and 151 within 2.71 mm, the rest a drift of the mesh itself, under 1 mm at the crown and up to 2.7 mm at the feet, carried by the raw plate corners: against its own outline every plate's tenons and fingers agree with the file's within 0.6 mm. Box 150 is the quirk of the definition: its ground edge is missing from its ground list, so the file's plates there stand up to 51 mm off the ground plane, and the four plates that meet them through its walls follow by up to 17 mm. The pictures show the arch and a 4 x 4 block at its crown, `box_61` to `box_64`, `box_86` to `box_89`, `box_111` to `box_114` and `box_136` to `box_139`, four neighbouring boxes of each of the four middle rows of boxes.

![](templates/templates_chevron_3_max2_arch_plates.webp)

![](templates/templates_chevron_3_max2_arch_joints.webp)

```cpp
Chevron chevron(
    surfaces[1], 4, 900.0, 0.5, 0.04993, 760.0, 80.0, 40.0, 1.1, 0.45, 80.0, 15.0, 7, 2000.0,
    {1, 1, 1, 1}, {132000.0, 126000.0, 3520.0}
);
```

## Shifted Pattern {#chevron_shifted}

`ChevronShifted` in `src/templates/chevron/chevron_shifted.h` is the second version of the template: the 2018 Annen pattern of the Grasshopper definition `ShiftedPattern-OptimizationUpdate/Annen200917.gh`, hexagonal cells in staggered columns instead of chevron strips. `Chevron` stays the default; the caller picks the class. Both build their plates from planes with the kernel's `Polyline::from_planes`, as `Plate` elements, and hand the same tables to the wood solver. Its ground truth is the definition's own output `Annen2.3dm`: every one of its 1075 plates is within 0.011 mm of a plate built here, and every tenon it carries within 1 mm of one the solver cuts.

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
| `divisions` | `int` | `1` | Tenon steps per edge: `2·divisions + 1` on a short edge, `4·divisions + 1` on a long one, a tenon at every odd step (`divisions`) |
| `long_edge` | `double` | `700.0` | Length in mm above which an edge takes twice the steps (`tolerance`) |
| `short_edge` | `double` | `200.0` | Length in mm below which an edge takes no tenons (`toleranceSmall`) |

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

The face plane of a cell is the surface frame at the point nearest the length-weighted centre of its outline, Rhino's `Polyline.CenterPoint`, z into the surface. The top and bottom plates are the face plane moved by `plate_offset` ± half a plate, cut by the side loop stepped in by a wall on the sides where the cell's own walls stand: the bottom edge and the left side of a hexagon, the bottom and left of a last quad, the left side of a first quad, which the definition left unstepped under the even neighbour's wall, its only plates standing inside a wall. The walls stand on the sides `0, 1, 2, 4, 5` of an even hexagon, the two halves of each merged long side one wall, on side `3` of an odd one, on all four sides of a last quad and on side `2` of a first quad, each between the outer box faces, a plate thick inward on the cell's own sides and outward on the sides it gives to the neighbouring column. A wall ends on the mitre where it meets the next wall of its box, the plane through the seam of the two walls' outer faces halfway between them, so a hexagon's cross wall is mitred against both long walls and a last quad's four walls against each other; it stops a plate short of a side where a neighbour's wall stands inside the cell, as an odd cell's cross wall and a first quad's wall stop at the even column's outward wall; else it ends on the side's plane, so the long walls of two cells of a column meet end to end on their shared edge plane. The ground truth kept the halves apart and ran every wall to the side planes, so its walls overlap one another by a thickness at every corner and junction. An odd hexagon's plates step from one even neighbour's long wall to the next on each side, where the neighbours' merged planes do not meet; where such a step stands in the way of what passes it on its way in, the lower even cell's wall on the right side and the odd cell itself on the left, the step is cut along that mover's insertion vector instead of across, so the two slide past each other on the cut: on the right side where the upper wall's plane steps into the lower wall's slab, on the left side where the upper wall stands further out than the lower one. The other steps stay as the definition drew them. A cell's plates and walls are kept in its group `box_<cell>`.

![](templates/templates_chevron_2_shifted_plates.webp)

```cpp
loop.push_back(
    flags[k] == 0
        ? sides[k].plane
        : stepped(sides[k].plane, plate_thickness, near.origin(), flags[k] < 0)
);
```

### Joinery {#chevron_shifted_joinery}

The joints are the definition's own, its C# script's `GetZigZag`, as the solver design `top_side_annen_shifted_tenons` (id 23) cuts them. The insertion direction of a cell is the bisector of two of its own sides on its outline, `unit(e5) - unit(e0)` for an even hexagon, and its axis is the mean of those two sides' plane normals to the surface, the `dirY` of the definition's insertion planes; both go to the edges whose walls the cell slides into, so a plate's side face carries the direction and axis of the cell that owns its edge, in `insertion_vectors` and `feature_axes`. Every side the definition's outer plane loop moved out by a wall carries the type 23: all six edges of a hexagon, all four of a last quad, the long side and the top of a first quad, whose other sides the definition left in place (the left side under its neighbour's wall gets a degenerate zigzag there, so no joint). A wall is joined, wall first, to each top and bottom plate whose side on its edge has a direction; the three-valence rows `{top0, wall, top1, wall}` stay as the two plates sharing a wall. The design then draws the plate's mid-wall line from the middle of its inner corner on one face and its outer corner on the other at each end, averages it with the line of the plate across the wall and keeps the middle 90%: `4·divisions + 1` steps on a line over `long_edge`, `2·divisions + 1` under it, a tenon half a step wide around every odd step, so the plate across the wall, running the line the other way, takes the even steps; the flanks are the planes through the direction and the axis, from the wall's inner face to its outer, each tenon with its slot through the wall, and an edge under `short_edge` gets none. A plate whose side lies on both halves of a merged long side is paired with that wall once per half, and the solver gives a pair listed again its next contact, so each half keeps its own tenons along its own direction; at the junction the design bounds the outer corner by the face of the odd cell's cross wall standing across it, the plane the definition's outer loop met there. The corners of a box take Christopher's Annen fingers, `side_side_outplane_annen_fingers` (17) on the end face of the wall ending on the mitre and 10 on the start face of the wall beginning there, both inserted along the box's up and cut in `finger_divisions` parts, as the chevron's walls are. From these tables the solver cuts the tenons and slots into all 868 plates and the fingers into the 224 corners; the picture shows a 4 x 4 block at the crown, `box_65` to `box_68`, `box_93` to `box_96`, `box_119` to `box_122` and `box_147` to `box_150`, four neighbouring cells of each of the four middle columns: the even column's cells bring their walls, the odd column's plates sit between the walls their even neighbours own, those outside the block missing from the picture.

![](templates/templates_chevron_2_shifted_joints.webp)

```cpp
for (size_t k = 0; k < edges.size(); k++) {
    if (!tenon_side(_cells.masks[cell], edges[k]))
        continue;

    types[2 + k] = 23;
    const std::map<EdgeKey, Vector>::const_iterator found =
        directions.find(edge_key(static_cast<int>(cell), edges[k]));

    if (found == directions.end())
        continue;

    vectors[2 + k] = found->second;
    plate_axes[2 + k] = axes.at(found->first);
}
```

### Ground Truth {#chevron_shifted_ground_truth}

`examples/templates_chevron_2_shifted.cpp` builds the pattern on the first Annen surface, the surface internalised in the definition, and is compared, with `divisions` at the definition's 2, with `Annen2.3dm` as the definition baked it (the template's default is now 1, half the tenons, which cuts 6518 tenons instead of 13036): 431 top and bottom plates carrying its tenons and 644 walls it never cut, as meshes in metres. Every vertex of every ground-truth plate lies within 0.011 mm of our outlines after the joints, except at the cut steps of 104 odd plates, where the definition's step corner stands up to 81 mm off the cut, and on the first quads' plates, stepped in by a wall where the definition left them inside their neighbours' walls, which moves the tenons they share with the plates across their cross walls by up to 19 mm on 12 of those, and every tenon the ground truth carries, 12772 of them, has its corners within 0.011 mm of a tenon of ours, on all 431 plates but the 16 on the naked long side of column 0 and the tenon beside each cut, which the cut trims, leaning through the plate's thickness as the ground truth's do within 0.012 mm; on that naked side the definition split its wall at a plane through the midpoint of the two edge middles, a plane no wall stands on once the halves are one, so the junction now bounds the tenons square across the side through the collinear vertex and those 16 plates' tenons move by up to 1.1 mm. Every ground-truth wall vertex lies on our wall outlines before the merge, the mitres and the trims, and the ground truth lacks the top plate of the last quad of column 0, which this template builds. Ours has tenons the ground truth lacks on 56 plates: those next to the seam of a ground-truth outline, on its first or last side, always the ones nearest the seam corner, where the definition's polyline insertion dropped the tenons it had computed, and those on the two ground-truth meshes whose caps are torn (objects 844 and 957). The definition's walls are plain, so the slots the solver cuts for every tenon have no counterpart there; every slot of ours is the exact negative of its tenon, and the walls no tenon reaches, the last quads' end walls, stay uncut here as well.


## Assembly {#chevron_assembly}

`src/templates/chevron/chevron_assembly.h` takes a built shell apart box by box: `wood_chevron::Assembly` reads the boxes from the `box_<i>` groups, every box's one insertion vector from the insertion lines, and from the through-tenon joints between two boxes the order, a box after the box whose wall its tenons enter along its own vector and after the box whose tenons its wall takes along it, the nearer of the two vectors when the tenon runs along both within the surface's turn; then it slides every box in order along its vector from `reach` behind its place into the boxes already placed, every plate the exact prism its merged outlines and slot loops bound, with points just inside its faces and along every edge, and reports every plate pair that meets on the way with the depth the deepest point stood inside the other plate and the points inside it with the box in place. Plate meshes are not used for this: a loft keeps the slots as separate shells and a BRep tessellation fans the concave caps. `examples/templates_chevron_4_chevron_assembly.cpp`, `_5_shifted_assembly.cpp` and `_6_max2_assembly.cpp` run it on the three shells and write the plan of a 4 x 4 block at the crown for `tools/render_assembly_gif.py`, which draws the block's boxes sliding in one after another.

![Chevron assembly](images/templates/templates_chevron_4_chevron_assembly.gif)
![MAX2 assembly](images/templates/templates_chevron_6_max2_assembly.gif)
![Shifted assembly](images/templates/templates_chevron_5_shifted_assembly.gif)

Christopher's chevron tenons are prisms along their own box's vector, and the chevron and MAX2 shells assemble as rigid boxes: on the first Annen surface 184 boxes in order, 22 plate pairs meeting along the way, 18 of them deeper than 1 mm and the deepest 6.67 mm, all but one at the end of the way with points inside each other in place, the overlaps the turned and moved edge planes give neighbouring plates; the MAX2 arch 200 boxes, 27 pairs, the deepest 7.67 mm, 23 of them in place and four binding by up to 2.1 mm over the last 20 mm. The shifted cells do not. A plate's tenons into the wall of a neighbouring cell are cut along that cell's insertion vector, which lies in the neighbour's plane; the surface turns between the two cells, so in the plate's own plane the tenon axis is off by that turn, about 3 degrees at the feet, and a box slid in along one vector binds by a couple of millimetres across the wall. `Annen2.3dm` has exactly the same tenons, so the template reproduces them rather than correcting them. An even cell's outward long wall takes its own plates' tenons along the odd neighbour's vector, which is right for the order: that wall is in place before the odd plates slide their tenons into it. What blocked the boxes was the steps of the odd plates where the long walls of two cells of a column meet end to end: a step of up to 100 mm into the slab the lower wall sweeps through on its last 50 mm of travel, 20 mm deep, the full half thickness, at every such junction. With the steps cut along the movers' vectors and the first quads' plates stepped in under their neighbours' walls, which the definition left overlapping them by a thickness, the sweep finds 849 plate pairs meeting, 668 deeper than 1 mm, 39 deeper than 5 mm and none deeper than 8.3 mm: the tenons' lean, and the left steps of two columns the rule leaves uncut. The picture shows the order the tenons dictate.

```cpp
#include "wood_session.h"
#include "src/templates/chevron/chevron_shifted.h"

int main() {
    ChevronShifted shell;
    shell.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```
