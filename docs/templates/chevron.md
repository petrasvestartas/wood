# Chevron {#templates_chevron}

[TOC]

An Annen surface folded into chevron strips, each face a box of four plates: a top, a bottom and a side on each chevron edge (`chevron/chevron.h`, example `templates_chevron_1_chevron`). The template is a `WoodSession` whose constructor is its recipe, one group per step; every picture below is one group of the example's scene.

![chevron](templates/templates_chevron_1_chevron_plates.webp)

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

## Surface

One of the 23 Annen surfaces read by `wood_chevron::annen_surfaces`.

![surface](templates/templates_chevron_1_chevron_surface.webp)

```cpp
const std::vector<NurbsSurface> surfaces = wood_chevron::annen_surfaces(path.string());
add_nurbssurface(surface, add_group("surface"));
```

## Mesh

Each strip across u is cut into chevron rows that grow by `scale` from both ends and meet in the middle.

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

## Strips

Faces that share an edge 1-2 or 3-0 grow into a strip, and each face marks its two chevron edges against the next strip.

![strips](templates/templates_chevron_1_chevron_strips.webp)

```cpp
const bool flip = strip_index % 2 == 0;
const std::array<int, 2> edges = flip ? std::array<int, 2>{3, 0} : std::array<int, 2>{0, 1};
```

## Edge planes

A plane stands on every face edge; inner chevron edges turn by `edge_rotation` or move by `edge_offset`, boundary edges snap upright.

![edge planes](templates/templates_chevron_1_chevron_edge_planes.webp)

```cpp
if (j % 2 == 1) {
    plane = plane.translate_by_normal(plate_thickness * edge_offset);
} else {
    const double turn = _strips.flips[face] ? angle : -angle;
    ...
}
```

## Bisectors

Every face corner gets the plane halfway between its two edge planes, where two side plates meet.

![bisectors](templates/templates_chevron_1_chevron_bisectors.webp)

```cpp
bisectors[face][j] = dihedral_plane(_edge_planes[face][(j + 1) % 4], _edge_planes[face][j]);
```

## Plates

The face plane moved into the box gives the top and bottom plates, and each chevron edge plane cut by the box and the bisector gives a side.

![plates](templates/templates_chevron_1_chevron_plates.webp)

```cpp
for (const double level : levels)
    outlines.push_back(polygon_from_planes(face_plane.translate_by_normal(level), sides));

outlines.push_back(polygon_from_planes(edges[edge], box));
outlines.push_back(polygon_from_planes(edges[edge].translate_by_normal(t), box));
```

## Joinery

Per plate pair the insertion vectors along the bisectors, mortises in the top and bottom, tenons in the sides, and the adjacency the solver reads; the line shows each face's insertion.

![insertion](templates/templates_chevron_1_chevron_insertion.webp)

```cpp
_joints_per_face[pair + 0] = {0, 0, 20, 20, 20, 20};
_joints_per_face[pair + 1] = {0, 0, 20, 20, 20, 20};
_joints_per_face[pair + 2] = {0, 0, 10, 10, 10, 10};
_joints_per_face[pair + 3] = {0, 0, 10, 10, 10, 10};
```

<details><summary>Example code: templates_chevron_1_chevron.cpp</summary>

\include{lineno} templates_chevron_1_chevron.cpp

</details>
