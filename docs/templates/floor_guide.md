# Part 1. FloorGuide: the geometry {#templates_floor_guide}

[TOC]

It computes geometry only that will be used later as a guide for elements.

The floor guide is initialized by 4 corners in the example file "templates_floor_1_floor_guide.cpp":

```cpp
    wood_floor::FloorGuide guide({
        Point(-3000.0, -3000.0, 0.0),
        Point(3000.0, -3000.0, 0.0),
        Point(3000.0, 3000.0, 0.0),
        Point(-3000.0, 3000.0, 0.0),
    });
```

The constructor is the whole computation: it takes the corners and, optionally, every parameter, then runs the steps below in order, each line one result, and draws them. Reading the constructor in "floor_guide.cpp" top to bottom is reading this page:

```cpp
    FloorGuide::FloorGuide(const std::array<Point, 4>& corners, double size_oculus = 1000.0, ...)
        : WoodSession("floor_guide"), corners(corners), size_oculus(size_oculus), ... {

        centre = ...;                      // the centre
        oculus_points[q] = ...;            // the oculus points
        _construction_planes[q] = ...;     // a plane pair per member, the column blocks sized by the rib starts
        _construction_quads[q] = ...;      // each member's footprint
        _boundary_parabolas[q] = ...;      // the rib curves
        _central_panel[q] = ...;           // the panel between the inner ribs
        _bed_top_planes[q] = ...;          // the column blocks' undersides
        _rib_bottom = ...;                 // the column cutter level
        soffit = ...;                      // the beam soffit
        draw();
    }
```

## Parameters

The oculus: `size_oculus` and `size_inner_beams`.

![The oculus parameters](floor/941_parameters_oculus.webp)

The column head in plan: the head, its chamfer, the wedges, the ribs and the t-sections.

![The column head parameters](floor/942_parameters_head.webp)

Outer rib 0 and column 0 in elevation: `bay_height`, `height`, `rise` and `column_head_depth`.

![The elevation parameters](floor/943_parameters_elevation.webp)

The two leaning planes seen edge-on: the middle wedge's fan plane and the oculus beam's bearing plane.

![The leaning planes](floor/944_parameters_angles.webp)

Every parameter is a constructor argument with its default, kept as a read-only field of `FloorGuide`. Sizes are in mm, angles in degrees.

| Parameter | Default | What it sets |
|---|---|---|
| `size_oculus` | 1000 | Distance of every oculus point from the centre along its seam |
| `size_column_head` | 220 | Side of the square column shaft and of the head polygon at the corner |
| `size_column_head_chamfer` | 120 | Where the chamfer points sit on the shaft faces; also the capitel width |
| `size_outer_ribs` | 100 | Outer rib thickness |
| `size_inner_ribs` | 60 | Inner rib thickness |
| `size_inner_beams` | 60 | Seam and oculus beam thickness; also the ring beam width at the datum |
| `size_wedge` | 240 | Side wedge block thickness |
| `middle_wedge_factor` | 1.25 | The middle wedge block in `size_wedge` thicknesses |
| `size_tsections` | 27 | Flange plane offset and bed layer thickness |
| `height` | 650 | Rib depth where the parabola starts |
| `rise` | 453 | Parabola rise from there to the seam; the seam depth `static_h()` is `height - rise` |
| `wedge_plane_angle` | -10 | How far the middle wedge's fan plane leans about its top edge |
| `oculus_plane_angle` | 5 | How far the oculus beam's bearing plane leans about its top edge |
| `column_head_depth` | 730 | Depth of the carved column head and of the capitel |
| `bay_height` | 3500 | The storey: floor top above the slab, the column top |

Code: [`FloorGuide` parameters](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L52-L66)

## The constructor, step by step

Each block of the constructor in order; open a block for its code and its picture, and its steps inside for the code of the function it calls.

<details open>
<summary><b>Centre</b></summary>

The vertex average of the four corners.

```cpp
// centre of the floor, the average of the four corners
centre = Point::centroid({corners[0], corners[1], corners[2], corners[3]});
```

![The centre](floor/950_centre.webp)

</details>

<details>
<summary><b>Oculus points</b></summary>

`size_oculus` from the centre towards each edge midpoint: the corners of the hole, each on its seam.

```cpp
// oculus points, size_oculus from the centre towards each edge midpoint
for (size_t q = 0; q < 4; q++)
    oculus_points[q] = centre + (midpoint(q) - centre).normalized() * size_oculus;
```

![The oculus points](floor/951_oculus_points.webp)

</details>

<details>
<summary><b>Construction planes</b></summary>

A pair of planes per member, its two faces: 2 outer ribs, 3 inner beams, 2 inner ribs, 3 wedges and 6 t-sections per quarter. `pair(plane, size)` returns the plane and its copy moved by the member's size.

```cpp
class ConstructionPlanes {
public:
    std::array<std::array<Plane, 2>, 2> outer_ribs; // Along the two bay edges, the band offset inwards by outer_ribs.
    std::array<std::array<Plane, 2>, 3> inner_beams; // Along the two seams and the oculus edge; the oculus one tilted by the oculus plane angle.
    std::array<std::array<Plane, 2>, 2> inner_ribs; // From the column head chamfer to the inner beam corners.
    std::array<std::array<Plane, 2>, 3> wedges; // The column head fan: side 0, the middle one tilted by wedge_plane_angle, side 1.
    std::array<std::array<Plane, 2>, 6> tsections; // Beside the ribs, tsections thick: outer rib 0, inner rib 0 outer and central face, inner rib 1 central and outer face, outer rib 1.
};
```


```cpp
// construction planes, a pair per member; each column block as thick as its rib's start, chosen so both outer ribs of a column end at one depth
for (size_t q = 0; q < 4; q++) {
    const ConstructionPlanes planes = compute_construction_planes(q);
    _construction_planes[q] = planes;
}
```

![The construction planes of quarter 0](floor/952_construction_planes_loop.webp)

<details>
<summary>1. Outer ribs: the bay edges</summary>

Each bay edge at the corner as a vertical wall, moved to the middle of its half edge, offset `size_outer_ribs` into the bay.

```cpp
// 1. outer ribs: the bay edge's plane, normal into the bay, its origin at the quarter's half edge
const Plane edge0 = Plane::from_line(Line::from_points(corners[q], corners[(q + 1) % 4]), down);
const Plane edge1 = Plane::from_line(Line::from_points(corners[(q + 3) % 4], corners[q]), down);
cp.outer_ribs = {
    pair(edge0.moved_to(Point::mid_point(polygon[0], polygon[1])), size_outer_ribs),
    pair(edge1.moved_to(Point::mid_point(polygon[4], polygon[0])), size_outer_ribs),
};
```

![Outer ribs](floor/954_construction_planes_outer_ribs.webp)

</details>

<details>
<summary>2. Inner beams: the seams and the oculus edge</summary>

Walls on the two seams; the oculus edge's wall turned `oculus_plane_angle` about the edge.

```cpp
// 2. inner beams on the polygon's seam and oculus lines; the oculus one tilted by oculus_plane_angle about its line
const Line oculus_line = Line::from_points(polygon[2], polygon[3]);
const Plane oculus_plane = Plane::from_line(oculus_line, down);
const Plane tilted = oculus_plane.transformed(Xform::rotation_around_line(oculus_line, -oculus_plane_angle, true));
cp.inner_beams = {
    pair(Plane::from_line(Line::from_points(polygon[1], polygon[2]), down), size_inner_beams),
    {tilted, oculus_plane.translate_by_normal(size_inner_beams)},
    pair(Plane::from_line(Line::from_points(polygon[3], polygon[4]), down), size_inner_beams),
};
```

![Inner beams](floor/955_construction_planes_inner_beams.webp)

</details>

<details>
<summary>3. Inner ribs: column head to beam corners</summary>

From the column head's chamfer points to where two inner beams' far faces cross the floor.

```cpp
// 3. inner ribs from the column head's chamfer points to the inner beam corners
const Plane xy = Plane::xy_plane_at(0.0);
const Point p0 = Intersection::plane_plane_plane(xy, cp.inner_beams[0][1], cp.inner_beams[1][1]).value();
const Point p1 = Intersection::plane_plane_plane(xy, cp.inner_beams[1][1], cp.inner_beams[2][1]).value();
const Point p2 = head[2];
const Point p3 = head[3];
cp.inner_ribs = {
    pair(Plane::from_line(Line::from_points(p2, p0), down), size_inner_ribs),
    pair(Plane::from_line(Line::from_points(p3, p1), Vector::z_axis()), size_inner_ribs),
};
```

![Inner ribs](floor/956_construction_planes_inner_ribs.webp)

</details>

<details>
<summary>4. Wedges: the column head fan, sized by the rib starts</summary>

The chamfer plane turned `wedge_plane_angle`, the side planes leaning with it; each block as thick as its rib's start, the middle one `middle_wedge_factor` times their mean.

```cpp
// 4. wedges: the chamfer plane tilted by wedge_plane_angle about its top edge, the two side planes leaning with it along the inner ribs
const Line side0 = Line::from_points(head[1], head[2]);
const Line side1 = Line::from_points(head[2], head[3]);
const Line side2 = Line::from_points(head[3], head[4]);
const Plane chamfer = Plane::from_line(side1, Vector::z_axis()).transformed(Xform::rotation_around_line(side1, wedge_plane_angle, true));
const Line line0 = Intersection::plane_plane(chamfer, cp.inner_ribs[0][1]).value();
const Line line1 = Intersection::plane_plane(chamfer, cp.inner_ribs[1][1]).value();
const Plane wedge0 = Plane::from_line(side0, -line0.to_direction());
const Plane wedge2 = Plane::from_line(side2, line1.to_direction());

// the near faces first, as the rib starts read them; each side block then as thick as its rib's start, the middle one middle_wedge_factor times their mean
cp.wedges = {pair(wedge0, 0.0), pair(chamfer, 0.0), pair(wedge2, 0.0)};
const std::array<double, 2> starts = compute_rib_starts(cp);
cp.wedges = {pair(wedge0, starts[0]), pair(chamfer, middle_wedge_factor * (0.5 * (starts[0] + starts[1]))), pair(wedge2, starts[1])};
```

![Wedges](floor/957_construction_planes_wedges.webp)

![The blocks at their thickness](floor/961_section_block_planes.webp)

</details>

<details>
<summary>4a. Rib starts: both outer ribs of a column end at one depth</summary>

Each outer rib's parabola starts this far from the column; the shallower end of the two at `size_wedge` is the level, and the other rib's start is solved (secant) so its end lands on it.

```cpp
std::array<double, 2> FloorGuide::compute_rib_starts(const ConstructionPlanes& cp) const {

    const std::array<Line, 2> axes = {outer_rib_axis(cp, 0), outer_rib_axis(cp, 1)};
    const std::array<Plane, 2> fans = {cp.wedges[0][0], cp.wedges[2][0]};
    const std::array<Plane, 2> seams = {cp.inner_beams[0][0], cp.inner_beams[2][0]};
    const double level = std::max(fan_end(axes[0], size_wedge, fans[0], seams[0]), fan_end(axes[1], size_wedge, fans[1], seams[1]));

    return {rib_start_at_level(axes[0], fans[0], seams[0], level), rib_start_at_level(axes[1], fans[1], seams[1], level)};
}
double FloorGuide::rib_start_at_level(const Line& axis, const Plane& fan, const Plane& seam, double level) const {

    const double length = axis.length();
    double x0 = size_wedge;
    double f0 = fan_end(axis, x0, fan, seam) - level;

    if (std::abs(f0) <= RIB_START_TOLERANCE)
        return x0;

    double x1 = x0 + 1.0;
    double f1 = fan_end(axis, x1, fan, seam) - level;

    for (size_t i = 0; i < RIB_START_STEPS; i++) {
        if (std::abs(f1) <= RIB_START_TOLERANCE)
            return x1;

        const double x2 = x1 - f1 * (x1 - x0) / (f1 - f0);

        if (x2 <= 0.0 || x2 >= length)
            throw std::runtime_error(fmt::format("an outer rib's start for the column level {:.3f} leaves its axis: {:.3f} of {:.3f} mm", level, x2, length));

        x0 = x1;
        f0 = f1;
        x1 = x2;
        f1 = fan_end(axis, x1, fan, seam) - level;
    }

    throw std::runtime_error(fmt::format("an outer rib's start for the column level {:.3f} did not converge: {:.3e} mm off", level, f1));
}
```

![Where each outer rib's curve starts](floor/960_section_rib_starts.webp)

</details>

<details>
<summary>5. T-sections beside the ribs</summary>

Each rib face moved `size_tsections` towards the bed it carries.

```cpp
// 5. t-sections beside the ribs
cp.tsections = {
    pair(cp.outer_ribs[0][1], size_tsections),
    pair(cp.inner_ribs[0][0], -size_tsections),
    pair(cp.inner_ribs[0][1], size_tsections),
    pair(cp.inner_ribs[1][1], size_tsections),
    pair(cp.inner_ribs[1][0], -size_tsections),
    pair(cp.outer_ribs[1][1], size_tsections),
};
```

![T-sections](floor/958_construction_planes_tsections.webp)

</details>

</details>

<details>
<summary><b>Construction quads</b></summary>

Each member's footprint on the floor, where four of its planes cross: its two faces and the planes it starts and ends on.

```cpp
// construction quads, each member's footprint on the floor where four of its planes cross
for (size_t q = 0; q < 4; q++) {
    const ConstructionQuads quads = compute_construction_quads(_construction_planes[q]);
    _construction_quads[q] = quads;
}
```

![The construction quads](floor/962_section_construction_quads.webp)

<details>
<summary>Four planes per member</summary>

`quad_of` crosses each pair of neighbouring planes with the floor; here the outer ribs.

```cpp
ConstructionQuads FloorGuide::compute_construction_quads(const ConstructionPlanes& cp) const {

    // the four planes of each member's quad, its corners 3-0, 0-1, 1-2 and 2-3 on the datum
    const auto quad_of = [](const std::array<Plane, 4>& planes) {
        return Polyline::from_planes({planes[3], planes[0], planes[1], planes[2]}, Plane::xy_plane_at(0.0));
    };

    ConstructionQuads quad;
    quad.outer_ribs = {
        quad_of({cp.outer_ribs[0][0], cp.inner_beams[0][0], cp.outer_ribs[0][1], cp.wedges[0][0]}),
        quad_of({cp.outer_ribs[1][0], cp.inner_beams[2][0], cp.outer_ribs[1][1], cp.wedges[2][0]}),
    };
    ...
}
```


</details>

</details>

<details>
<summary><b>Boundary parabolas</b></summary>

The curved underside of each rib and its two layers 27 and 54 above; the inner ribs' are the outer ones projected onto them.

```cpp
// boundary parabolas, the curved underside of each rib and its two layers 27 and 54 above
for (size_t q = 0; q < 4; q++) {
    const std::array<std::array<Polyline, 3>, 4> parabolas = compute_boundary_parabolas(q);
    _boundary_parabolas[q] = parabolas;
}
```

![The boundary parabolas](floor/963_section_boundary_parabolas.webp)

<details>
<summary>The parabola over a rib axis</summary>

From `-height` at the rib start to `-static_h()` at the seam.

```cpp
Polyline FloorGuide::outer_parabola(const Line& axis, double distance) const {

    const Point start = axis.start();
    const Point end = axis.end();
    const Point trimmed = start + (end - start).normalized() * distance;
    const Point middle = Point::mid_point(trimmed, end);

    return Polyline::quadratic_points(trimmed + Vector(0.0, 0.0, -height), middle + Vector(0.0, 0.0, -static_h()), end + Vector(0.0, 0.0, -static_h()));
}
```


</details>

<details>
<summary>Outer and inner parabolas</summary>

The outer ribs' parabolas with their layers, then projected along the outer rib normal onto the inner ribs.

```cpp
std::array<std::array<Polyline, 3>, 4> FloorGuide::compute_boundary_parabolas(size_t q) const {

    const ConstructionPlanes& cp = _construction_planes[q];
    const std::array<double, 2> starts = compute_rib_starts(cp);
    std::array<std::array<Polyline, 3>, 4> parabolas;

    for (size_t k = 0; k < 2; k++) {
        const Polyline parabola = outer_parabola(outer_rib_axis(cp, k), starts[k]);
        parabolas[k] = {parabola, parabola.offset_toward(size_tsections, Vector::z_axis()), parabola.offset_toward(2.0 * size_tsections, Vector::z_axis())};
    }

    // the inner parabolas are projections of the outer ones onto the inner ribs' outer faces
    for (size_t i = 0; i < 2; i++) {
        const Xform projection = Xform::project_to_plane_by_axis(cp.inner_ribs[i][0], cp.outer_ribs[i][0].z_axis());
        const std::array<Polyline, 3>& outer = parabolas[i];
        parabolas[2 + i] = {outer[0].transformed(projection), outer[1].transformed(projection), outer[2].transformed(projection)};
    }

    return parabolas;
}
```


</details>

</details>

<details>
<summary><b>Central panel</b></summary>

The ruled surface between the two inner ribs (rule A): one sweep for both ribs, so the central beds stay flat quads.

```cpp
// central panel, the ruled surface between the two inner ribs and its traces on their faces
for (size_t q = 0; q < 4; q++) {
    const CentralPanel panel = compute_central_panel(q);
    _central_panel[q] = panel;
}
```

![The central panel](floor/964_section_central_panel.webp)

<details>
<summary>Sweep, ruling and traces</summary>

The sweep that makes the two swept traces parallel, the ruling between them, and the three layers traced on both central faces.

```cpp
CentralPanel FloorGuide::compute_central_panel(size_t q) const {

    const ConstructionPlanes& cp = _construction_planes[q];
    const std::array<Plane, 2> faces = {cp.inner_ribs[0][1], cp.inner_ribs[1][1]};
    const std::array<Vector, 2> normals = {faces[0].z_axis(), faces[1].z_axis()};
    const std::array<Polyline, 2> shadows = {_boundary_parabolas[q][2][0], _boundary_parabolas[q][3][0]};
    const Vector reference = (normals[0] - normals[1]).flattened().normalized();

    CentralPanel panel;
    panel.rib_sweep = rib_sweep(shadows, normals, size_inner_ribs, reference);
    const std::array<Polyline, 2> soffits = {shadows[0].transformed(Xform::project_to_plane_by_axis(faces[0], panel.rib_sweep)), shadows[1].transformed(Xform::project_to_plane_by_axis(faces[1], panel.rib_sweep))};
    panel.ruling = (soffits[1].get_point(0) - soffits[0].get_point(0)).flattened().normalized();

    // the layers: the soffit's offsets by size_tsections and twice that in the panel's own cross-section, projected along the ruling onto both central faces
    const Polyline section = soffits[0].transformed(Xform::project_to_plane_by_axis(Plane::from_point_normal(soffits[0].get_point(0), panel.ruling), panel.ruling));
    const Polyline layer1 = section.offset_toward(size_tsections, Vector::z_axis());
    const Polyline layer2 = section.offset_toward(2.0 * size_tsections, Vector::z_axis());

    for (size_t k = 0; k < 2; k++)
        panel.traces[k] = {soffits[k], layer1.transformed(Xform::project_to_plane_by_axis(faces[k], panel.ruling)), layer2.transformed(Xform::project_to_plane_by_axis(faces[k], panel.ruling))};

    return panel;
}
```


</details>

<details>
<summary>The closure the sweep solves</summary>

The cross product of the start chord and the end chord between the swept traces, zero when they are parallel.

```cpp
double FloorGuide::closure(const std::array<Polyline, 2>& shadows, const std::array<Vector, 2>& normals, double thickness, const Vector& r) {

    // each rib's outer face trace moves thickness / (n . r) along r to reach its central face
    const std::array<double, 2> shift = {thickness / normals[0].dot(r), thickness / normals[1].dot(r)};
    const size_t n = shadows[0].point_count() - 1;
    const Vector start = ((shadows[1].get_point(0) + r * shift[1]) - (shadows[0].get_point(0) + r * shift[0])).flattened();
    const Vector vertex = ((shadows[1].get_point(n) + r * shift[1]) - (shadows[0].get_point(n) + r * shift[0])).flattened();

    return start.cross(vertex)[2] / (start.magnitude() * vertex.magnitude());
}
```


</details>

</details>

<details>
<summary><b>Bed top planes</b></summary>

The underside of each column block: the plane through the four lowest points of its bed row, one per row.

```cpp
// bed top planes, the underside of each column block where it sits on its bed row: beside rib 0, in the central panel, beside rib 1
for (size_t q = 0; q < 4; q++) {
    const std::array<Plane, 3> bed_planes = compute_bed_top_planes(q);
    _bed_top_planes[q] = bed_planes;
}
```

![The bed top planes](floor/965_section_bed_top_planes.webp)

<details>
<summary>Fitted per row</summary>

Each row's bed top curve on its two side faces, trimmed between its beam and its block; the plane through the four lowest points, normal up.

```cpp
std::array<Plane, 3> FloorGuide::compute_bed_top_planes(size_t q) const {

    const ConstructionPlanes& cp = _construction_planes[q];
    const std::array<std::array<Polyline, 3>, 4>& parabolas = _boundary_parabolas[q];
    const CentralPanel& panel = _central_panel[q];

    // the plane through a panel's deepest quad, its top layer on the two side planes trimmed by the panel planes, normal up
    const auto fitted = [](const std::array<Polyline, 2>& faces, const Plane& cut_plane0, const Plane& cut_plane1) {
        std::array<std::vector<Point>, 2> pts = {faces[0].trimmed(cut_plane0, cut_plane1, EXTENSION).get_points(), faces[1].trimmed(cut_plane0, cut_plane1, EXTENSION).get_points()};

        if (pts[0].front()[2] > pts[0].back()[2]) {
            std::reverse(pts[0].begin(), pts[0].end());
            std::reverse(pts[1].begin(), pts[1].end());
        }

        const Plane plane = Plane::from_points_pca({pts[0][0], pts[0][1], pts[1][0], pts[1][1]});

        return Plane::from_point_normal(plane.origin(), plane.z_axis()[2] < 0.0 ? -plane.z_axis() : plane.z_axis());
    };

    const Xform side00 = Xform::project_to_plane_by_axis(cp.inner_ribs[0][0], cp.outer_ribs[0][0].z_axis());
    const Xform side01 = Xform::project_to_plane_by_axis(cp.outer_ribs[0][1], cp.outer_ribs[0][0].z_axis());
    const Xform side20 = Xform::project_to_plane_by_axis(cp.inner_ribs[1][0], cp.outer_ribs[1][0].z_axis());
    const Xform side21 = Xform::project_to_plane_by_axis(cp.outer_ribs[1][1], cp.outer_ribs[1][0].z_axis());

    return {
        fitted({parabolas[0][2].transformed(side00), parabolas[0][2].transformed(side01)}, cp.inner_beams[0][1], cp.wedges[0][0]),
        fitted({panel.traces[0][2], panel.traces[1][2]}, cp.inner_beams[1][1], cp.wedges[1][0]),
        fitted({parabolas[1][2].transformed(side20), parabolas[1][2].transformed(side21)}, cp.inner_beams[2][1], cp.wedges[2][0]),
    };
}
```


</details>

</details>

<details>
<summary><b>Column cutter level</b></summary>

One level for every column: the deepest outer rib bottom corner on a fan plane.

```cpp
// the middle cutter level, one for every column: the deepest outer rib bottom corner on a fan plane
for (size_t q = 0; q < 4; q++)
    for (const std::array<Polyline, 2>& rib : _outer_ribs[q])
        _rib_bottom = std::min({_rib_bottom, rib[0].get_point(2)[2], rib[1].get_point(2)[2]});
```

![The column cutter level](floor/966_section_rib_bottom.webp)

</details>

<details>
<summary><b>Soffit</b></summary>

The one level every inner and ring beam's underside sits at: the deepest rib end on a beam.

```cpp
// soffit, the one level every inner and ring beam's underside sits at: the deepest rib end on a beam, so every rib meets its beam in full
soffit = -static_h();

for (size_t q = 0; q < 4; q++)
    for (size_t k = 0; k < 2; k++)
        soffit = std::min({soffit, end_level(_outer_ribs[q][k], rib_seam_ends(q)[k]), end_level(_inner_ribs[q][k], _construction_planes[q].inner_beams[1][1])});
```

![The soffit](floor/967_section_soffit.webp)

</details>

## Tables

### ConstructionPlanes

![ConstructionPlanes](floor/931_table_construction_planes.webp)

<span style="color:#2196EA">■ [0] base face</span> <span style="color:#F2CC0C">■ [1] offset face</span> <span style="color:#DADADA">■ the member between them</span>

One plane pair per member of a quarter, from `construction_planes(q)`: `[0]` is the member's base face, `[1]` the face offset from it by the member's size. The picture shows where each plane meets the floor datum.

| Field | Type | Members | Where |
|---|---|---|---|
| `outer_ribs` | `std::array<std::array<Plane, 2>, 2>` | 2 | Along the two bay edges, offset inwards by `size_outer_ribs` |
| `inner_beams` | `std::array<std::array<Plane, 2>, 3>` | 3 | Seam 0, the oculus edge (tilted by `oculus_plane_angle`), seam 1 |
| `inner_ribs` | `std::array<std::array<Plane, 2>, 2>` | 2 | From the column head chamfer to the inner beam corners |
| `wedges` | `std::array<std::array<Plane, 2>, 3>` | 3 | The column head fan: side 0, the middle one tilted by `wedge_plane_angle`, side 1 |
| `tsections` | `std::array<std::array<Plane, 2>, 6>` | 6 | Beside the ribs, `size_tsections` thick: outer rib 0, inner rib 0 outer and central face, inner rib 1 central and outer face, outer rib 1 |

Code: [`ConstructionPlanes`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L13-L21)

### ConstructionQuads

![ConstructionQuads](floor/932_table_construction_quads.webp)

<span style="color:#E8478B">■ outer_ribs</span> <span style="color:#7C7C7C">■ inner_beams</span> <span style="color:#F2CC0C">■ inner_ribs</span> <span style="color:#A8A8A8">■ wedges</span> <span style="color:#F5D890">■ tsections</span>

One plan quad per member at the floor datum, from `construction_quads(q)`: index `i` is the footprint of member `i` of its family, bounded by its own plane pair and its neighbours' planes.

| Field | Type | Quads |
|---|---|---|
| `outer_ribs` | `std::array<Polyline, 2>` | 2 |
| `inner_beams` | `std::array<Polyline, 3>` | 3: seam 0, oculus edge, seam 1 |
| `inner_ribs` | `std::array<Polyline, 2>` | 2 |
| `wedges` | `std::array<Polyline, 3>` | 3 |
| `tsections` | `std::array<Polyline, 6>` | 6 |

Code: [`ConstructionQuads`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L23-L31)

### CentralPanel

![CentralPanel](floor/933_table_central_panel.webp)

<span style="color:#2196EA">■ soffit traces</span> <span style="color:#F2CC0C">■ +t and +2t traces</span> <span style="color:#E8478B">■ ruling u and rib_sweep r</span> <span style="color:#737373">■ inner rib 0, as its loops</span>

The panel between a quarter's two inner ribs, from `central_panel(q)`, found by rule A: one horizontal ruling joins the two ribs and one sweep serves both, so the central beds stay flat quads.

| Field | Type | What it is |
|---|---|---|
| `ruling` | `Vector` | u: the panel's horizontal ruling, along which rib 0's central trace projects onto rib 1's |
| `rib_sweep` | `Vector` | r: the horizontal direction both inner ribs are swept along from their outer to their central face |
| `traces` | `std::array<std::array<Polyline, 3>, 2>` | Per inner rib `k`, on its central face: `traces[k][0]` the soffit, `[1]` +t and `[2]` +2t, t being `size_tsections` |

Code: [`CentralPanel`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L33-L39)

### FloorGuide

![FloorGuide](floor/901_floor_guide.webp)

<span style="color:#2196EA">■ corners, oculus points</span> <span style="color:#737373">■ seams, dashed</span>

The guide is made from four corners and the parameters (`size_outer_ribs`, `size_wedge`, `height`, `rise` ...); the constructor runs every step for each quarter and draws the result.

Code: [`FloorGuide`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L50)

### quarter_polygon

![quarter_polygon](floor/902_quarter_polygon.webp)

<span style="color:#2196EA">■ quarter 0</span> <span style="color:#A3A3A3">■ the other quarters</span>

The bay is split into four quarters; quarter q is the one at `corners[q]`. Every FloorGuide method takes q and computes that quarter alone, so a rectangular bay gets four different quarters; the pictures below show q = 0. The quarter's five lines carry all its planes: the bay edges, the two seams, the oculus edge.

Code: [`quarter_polygon`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L96)

### quarter_column_polygon

![quarter_column_polygon](floor/903_quarter_column_polygon.webp)

<span style="color:#2196EA">■ the column head</span> <span style="color:#737373">■ column_frame</span>

The column head at the quarter's corner, where the ribs start; `column_frame` gives its axes.

Code: [`quarter_column_polygon`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L99)

### construction_planes

![construction_planes](floor/904_construction_planes.webp)

<span style="color:#2196EA">■ base face</span> <span style="color:#F2CC0C">■ offset face</span> <span style="color:#A3A3A3">■ the member's footprint</span>

A plane pair for every member: its base face on one of the polygon's lines, and the face offset by the member's size. Every member is cut from these planes.

Code: [`construction_planes`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L112)

### construction_quads

![construction_quads](floor/905_construction_quads.webp)

family colours

Where each member's four planes meet the floor datum: its footprint in plan.

Code: [`construction_quads`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L115)

### boundary_parabolas

![boundary_parabolas](floor/906_boundary_parabolas.webp)

<span style="color:#2196EA">■ the parabolas</span> <span style="color:#F2CC0C">■ their +t and +2t layers</span> <span style="color:#737373">■ rib quads</span>

A parabola under each rib axis, from `-height` at the column to `-static_h` at the seam. The inner ones are the outer ones projected onto the inner ribs.

Code: [`boundary_parabolas`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L125)

### central_panel

![central_panel](floor/907_central_panel.webp)

<span style="color:#2196EA">■ soffit traces</span> <span style="color:#F2CC0C">■ layers</span> <span style="color:#E8478B">■ ruling</span>

Between the two inner ribs, one ruling crosses the panel and one sweep serves both ribs (rule A), so the central beds stay flat quads.

Code: [`central_panel`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L128)

### Two face loops

![Two face loops](floor/908_loops.webp)

<span style="color:#2196EA">■ [0]: top</span> <span style="color:#F2CC0C">■ [1]: bottom</span>

Every member method below returns each member as `std::array<Polyline, 2>`: its two face loops, lofted by `loft`. The guide stops here; the Floor turns loops into elements.

Code: [`loft`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L177)

### outer_ribs, inner_ribs

![outer_ribs, inner_ribs](floor/909_ribs.webp)

<span style="color:#2196EA">■ the ribs</span> <span style="color:#A3A3A3">■ the rest of the quarter</span>

Each rib's parabola trimmed by the planes it ends on, on both of its faces.

Code: [`outer_ribs`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L153)

### tsections

![tsections](floor/910_tsections.webp)

<span style="color:#2196EA">■ the t-sections</span> <span style="color:#A3A3A3">■ ribs and beams</span>

Flange strips beside the rib faces; the beds rest on them.

Code: [`tsections`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L150)

### beds

![beds](floor/911_beds.webp)

<span style="color:#2196EA">■ the beds</span>

Three rows of bed plates between the ribs, each row trimmed alike so every plate stays a quad.

Code: [`beds`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L147)

### wedges, inner_beams

![wedges, inner_beams](floor/912_wedges_and_beams.webp)

<span style="color:#2196EA">■ column blocks and inner beams</span>

The three column blocks at the head, and the three beams on the seams and the oculus edge.

Code: [`wedges`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L159)

### oculus()

![oculus()](floor/913_oculus.webp)

<span style="color:#2196EA">■ ring beams</span> <span style="color:#F2CC0C">■ the quarters' oculus beams</span> <span style="color:#A3A3A3">■ bottom wedges and central plate</span>

Four ring beams around the hole, one per oculus edge, each meeting its quarter's oculus beam on the tilted plane.

Code: [`oculus`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L165)

### column_cutters

![column_cutters](floor/914_column_cutters.webp)

<span style="color:#2196EA">■ the cutters</span> <span style="color:#A3A3A3">■ the column</span>

Six plates that carve the column head so the ribs and the column blocks sit on it.

Code: [`column_cutters`](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/templates/floor/floor_guide.h#L171)
