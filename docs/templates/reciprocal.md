# Reciprocal {#templates_reciprocal}

[TOC]

Reciprocal timber beam frames on free-form polygon meshes (`src/templates/reciprocal/*.h`, examples `templates_reciprocal_1_rotation` and `templates_reciprocal_2_move`). Both `ReciprocalRotation` and `ReciprocalMove` subclass `WoodSession` and execute a 4-stage geometric pipeline (`mesh` → `axes` → `frame` → `beams`) where every interior member bears against the side face of its neighbour and naked boundary edges are enclosed by a mitered or priority-butted perimeter frame.

<div class="wood-card-grid cols-3">
  <a class="wood-card" href="#reciprocal_rotation">
    <img src="templates_reciprocal_1_rotation_beams.webp" alt="Pinwheel Rotation">
    <span class="wood-card-title">1. Pinwheel Rotation (`ReciprocalRotation`)</span>
    <span class="wood-card-sub">Interior edges rotated by `angle` &amp; scaled by `scale` (`templates_reciprocal_1_rotation`)</span>
  </a>
  <a class="wood-card" href="#reciprocal_move">
    <img src="templates_reciprocal_2_move_beams.webp" alt="Half-Edge Translation">
    <span class="wood-card-title">2. Half-Edge Translation (`ReciprocalMove`)</span>
    <span class="wood-card-sub">2-colored half-edges shifted along corner bisectors (`templates_reciprocal_2_move`)</span>
  </a>
</div>

![](diagrams/template_reciprocal_pipeline.webp)

## Parameters

| Parameter | `ReciprocalRotation` | `ReciprocalMove` | Description |
| --- | --- | --- | --- |
| `mesh` | `const Mesh&` | `const Mesh&` | Input polygon mesh (quad, hex, or dual-hex on dome, hypar, Annen, or Scherk surface) |
| `angle` / `scale` | `0.35 rad`, `1.4` | — | Midpoint rotation around average face normal and axial scale factor before trimming |
| `shift` | — | `50.0` (`100.0` in example) | Sideways translation (`mm`, `≥ width / 2`) of color-`0` half-edges along corner bisectors |
| `width`, `height` | `100.0`, `400.0` | `100.0`, `400.0` | Cross-section width across the beam and depth along `up` (`0` → `2 * width`) |
| `extend_factor` | `5.0` | — | Multiples of `width` an axis extends past its endpoints before neighbour cut clipping |
| `cut_offset_factor` | `1.0` | `1.0` | End-cut offset from crossing neighbour's axis (`1.0` = flush with neighbour's side face) |
| `frame_ups` | `map<edge_key, Vector>` | `map<edge_key, Vector>` | Section `up` vectors on naked boundary edges (transported across miter corners when omitted) |
| `beam_up` | `EdgeAverage` | `OwningFace` / `EdgeAverage` | Normal source for interior beam cross-sections |
| `corner_joint` | `Miter` / `Butt` | `Miter` / `Butt` | Perimeter frame corner joint rule (`through_priority` selects through-beam at butts) |

---

## 1. Pinwheel Rotation {#reciprocal_rotation}

Every interior mesh edge is scaled about its midpoint and rotated by `angle` around its average face normal so the beams meeting at each vertex form a self-supporting pinwheel (`ReciprocalRotation`):

![](templates/templates_reciprocal_1_rotation_beams.webp)

### 1.1 Input Mesh

`wood_reciprocal::MeshFaces::of(mesh)` extracts face vertex rings, Newell normals, and edge-to-face ownership:

![](templates/templates_reciprocal_1_rotation_mesh.webp)

```cpp
_faces = wood_reciprocal::MeshFaces::of(mesh);
add_mesh(mesh, group_named("mesh"));
```

### 1.2 Rotated and Scaled Axes

Each interior edge (`owners.size() >= 2`) is scaled by `scale` about its midpoint and rotated by `angle` around the normalized sum of its two adjacent face normals:

![](templates/templates_reciprocal_1_rotation_axes.webp)

```cpp
const Vector dir = along.normalized().transformed(Xform::rotation(up, angle));
const double half = along.magnitude() * 0.5 * scale;
axes.push_back(Axis{{edge.first, edge.second}, Line::from_points(mid - dir * half, mid + dir * half), up});
```

### 1.3 Perimeter Boundary Frame

`boundary_frame()` walks naked boundary loops, parallel-transports `frame_ups` across corner miter planes, and trims corner joints by `CornerJoint::Miter` or priority `CornerJoint::Butt` (`through_priority`):

![](templates/templates_reciprocal_1_rotation_frame.webp)

```cpp
_frame = wood_reciprocal::boundary_frame(
    _faces.faces, _faces.owners, _faces.points, _faces.normals,
    this->width, this->height, frame_ups, corner_joint, through_priority
);
_frame_beams = wood_reciprocal::frame_beams(
    _frame, _faces.faces, _faces.points, this->width, this->height
);
```

### 1.4 Boxed and End-Trimmed Beams

Each axis is extended by `width * extend_factor` and clipped by `cut_beam` against the side planes (`±0.5 * width * cut_offset_factor`) of all beams sharing its start and end mesh vertices (or against the perimeter `frame`):

![](templates/templates_reciprocal_1_rotation_beams.webp)

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

<details>
<summary>Example code: examples/templates_reciprocal_1_rotation.cpp</summary>

\include{lineno} templates_reciprocal_1_rotation.cpp

</details>

---

## 2. Half-Edge Translation {#reciprocal_move}

Instead of rotating edges, `ReciprocalMove` 2-colors the half-edges of the mesh via breadth-first traversal and shifts every color-`0` interior half-edge sideways along the bisector of its two face-neighbour edges by `shift`:

![](templates/templates_reciprocal_2_move_beams.webp)

### 2.1 Input Mesh

Newell face normals are oriented upward (`n_z >= 0`) and half-edge adjacency across shared edges is indexed:

![](templates/templates_reciprocal_2_move_mesh.webp)

```cpp
_faces = compute_faces();
add_mesh(mesh, group_named("mesh"));
```

### 2.2 Bisector-Shifted Half-Edge Axes

Half-edges are 2-colored so adjacent edges of a face alternate colors (`0` vs `1`). Every interior half-edge of color `0` is translated along the bisector `0.5 * (dir_next - dir_prev)` by `shift` and trimmed to its neighbour lines:

![](templates/templates_reciprocal_2_move_axes.webp)

```cpp
const Vector bisector = (axes[i][(j + 1) % n].dir - axes[i][(j - 1 + n) % n].dir) * 0.5;
axes[i][j].line += bisector * shift;
```

### 2.3 Perimeter Boundary Frame

Naked boundary edges receive straight perimeter `frame_<k>` beams mitered or butted at corners:

![](templates/templates_reciprocal_2_move_frame.webp)

```cpp
_frame_beams = wood_reciprocal::frame_beams(
    _frame, _faces.faces, _faces.points, this->width, this->height
);
```

### 2.4 Boxed and Flush-Cut Beams

Each shifted axis is boxed into a `width × height` beam and cut flush at both ends against the side plane of the beam across its neighbouring face edge (`compute_cut_plane`) or the boundary frame:

![](templates/templates_reciprocal_2_move_beams.webp)

```cpp
if (crossing) {
    cuts[side] = wood_reciprocal::unbounded(compute_cut_plane(i, j, *crossing, ends[side]));
    continue;
}

cuts[side] = wood_reciprocal::frame_cuts(_frame, vertices[side], axis.dir);
```

<details>
<summary>Example code: examples/templates_reciprocal_2_move.cpp</summary>

\include{lineno} templates_reciprocal_2_move.cpp

</details>
