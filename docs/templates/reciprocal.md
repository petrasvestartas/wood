# Reciprocal {#templates_reciprocal}

[TOC]

Reciprocal frames on a mesh: every beam bears on its neighbours, framed by straight beams on the naked edges. Both templates are a `WoodSession` whose constructor runs four steps, each into its own group: `mesh`, `axes`, `frame`, `beams`.

## ReciprocalRotation

Every interior edge becomes a beam turned about its midpoint, so the beams round each vertex form a pinwheel.

![reciprocal rotation](templates/templates_reciprocal_1_rotation_beams.webp)

```cpp
ReciprocalRotation rotation(
    mesh,
    0.35,   // angle, radians about the edge's up
    1.4,    // scale, lengthening about the midpoint
    100.0,  // width
    400.0,  // height
    5.0,    // extend_factor, widths an end may run on to its neighbour
    1.0,    // cut_offset_factor, 1 is flush with the neighbour's side
    frame_ups,
    wood_reciprocal::CornerJoint::Butt,
    through
);
```

### Mesh

The input mesh, its faces, normals and edge owners.

![mesh](templates/templates_reciprocal_1_rotation_mesh.webp)

```cpp
_faces = wood_reciprocal::MeshFaces::of(mesh);
add_mesh(mesh, group_named("mesh"));
```

### Axes

Every interior edge scaled about its midpoint and turned about the average normal of its two faces.

![axes](templates/templates_reciprocal_1_rotation_axes.webp)

```cpp
const Vector dir = along.normalized().transformed(Xform::rotation(up, angle));
const double half = along.magnitude() * 0.5 * scale;
axes.push_back(Axis{{edge.first, edge.second}, Line::from_points(mid - dir * half, mid + dir * half), up});
```

### Frame

A straight beam on every naked edge, its section mirrored across every mitre, butted at the corners.

![frame](templates/templates_reciprocal_1_rotation_frame.webp)

```cpp
_frame = wood_reciprocal::boundary_frame(
    _faces.faces,
    _faces.owners,
    _faces.points,
    _faces.normals,
    this->width,
    this->height,
    frame_ups,
    corner_joint,
    through_priority
);
_frame_beams = wood_reciprocal::frame_beams(
    _frame,
    _faces.faces,
    _faces.points,
    this->width,
    this->height
);
```

### Beams

Every axis boxed, each end stopped at the first side face of a beam at its vertex, or at the frame.

![beams](templates/templates_reciprocal_1_rotation_beams.webp)

```cpp
const wood_reciprocal::ReciprocalBeam beam = wood_reciprocal::cut_beam(
    line.start() - dir * extend,
    line.end() + dir * extend,
    dir,
    _axes[i].up,
    width,
    height,
    cuts_start,
    cuts_end
);
```

<details><summary>Example code: templates_reciprocal_1_rotation.cpp</summary>

\include{lineno} templates_reciprocal_1_rotation.cpp

</details>

## ReciprocalMove

Every other edge of each face becomes a beam moved sideways, so each end lands on the side of its neighbour.

![reciprocal move](templates/templates_reciprocal_2_move_beams.webp)

```cpp
ReciprocalMove move(
    mesh,
    100.0,  // shift, at least half the width
    100.0,  // width
    400.0,  // height
    1.0,    // cut_offset_factor
    frame_ups,
    ReciprocalMove::BeamUp::EdgeAverage,
    wood_reciprocal::CornerJoint::Butt,
    through
);
```

### Mesh

The input mesh in vertex index order, its Newell normals turned up.

![mesh](templates/templates_reciprocal_2_move_mesh.webp)

```cpp
_faces = compute_faces();
add_mesh(mesh, group_named("mesh"));
```

### Axes

The half-edges two-coloured, colour 0 moved along the bisector of its neighbours by shift and trimmed to them.

![axes](templates/templates_reciprocal_2_move_axes.webp)

```cpp
const Vector bisector = (axes[i][(j + 1) % n].dir - axes[i][(j - 1 + n) % n].dir) * 0.5;
axes[i][j].line += bisector * shift;
```

### Frame

The same straight frame as the rotation, on the move's upward normals.

![frame](templates/templates_reciprocal_2_move_frame.webp)

```cpp
_frame_beams = wood_reciprocal::frame_beams(
    _frame,
    _faces.faces,
    _faces.points,
    this->width,
    this->height
);
```

### Beams

Every axis boxed, each end cut flush at the side of the beam across its neighbouring edge, or at the frame.

![beams](templates/templates_reciprocal_2_move_beams.webp)

```cpp
if (crossing) {
    cuts[side] = wood_reciprocal::unbounded(compute_cut_plane(i, j, *crossing, ends[side]));
    continue;
}

cuts[side] = wood_reciprocal::frame_cuts(_frame, vertices[side], axis.dir);
```

<details><summary>Example code: templates_reciprocal_2_move.cpp</summary>

\include{lineno} templates_reciprocal_2_move.cpp

</details>
