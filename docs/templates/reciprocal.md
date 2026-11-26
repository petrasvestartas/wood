# Reciprocal {#templates_reciprocal}

[TOC]

Reciprocal timber beam frames on polygon meshes in `src/templates/reciprocal/`. Every interior beam bears against the side face of its neighbour while naked boundary edges are enclosed by a mitered or priority-butted perimeter frame.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#reciprocal_rotation">
    <img src="templates_reciprocal_1_rotation_assembly.webp" alt="Pinwheel Rotation">
    <span class="wood-card-title">Pinwheel Rotation</span>
  </a>
  <a class="wood-card" href="#reciprocal_move">
    <img src="templates_reciprocal_2_move_assembly.webp" alt="Half-Edge Translation">
    <span class="wood-card-title">Half-Edge Translation</span>
  </a>
  <a class="wood-card" href="#reciprocal_assembly_sequence">
    <img src="assembly_templates_reciprocal_1_rotation.gif" alt="Assembly Sequence">
    <span class="wood-card-title">Assembly Sequence</span>
  </a>
</div>

## Parameters

Every input is a field of the guide in `src/templates/reciprocal/reciprocal_guide.h`: `ReciprocalRotation(ReciprocalRotationGuide)` and `ReciprocalMove(ReciprocalMoveGuide)`. `defaults(mesh)` is the guide below on a mesh; `butted(mesh, surface, vertical)` is the examples' frame: `100 x 400` beams, a butted frame, its ups tilted per side.

| Field | `ReciprocalRotationGuide` | `ReciprocalMoveGuide` | Description |
| --- | --- | --- | --- |
| `mesh` | `Mesh` | `Mesh` | Input polygon mesh |
| `angle`, `scale` | `0.35 rad`, `1.4` | — | Midpoint rotation around average face normal and axial scale |
| `extend_factor` | `5.0` | — | Beam widths a beam may run past its ends to reach the beam it bears on |
| `shift` | — | `50.0` | Sideways translation along corner bisectors in mm |
| `width`, `height` | `100.0`, `0.0` | `100.0`, `0.0` | Beam cross-section width and depth in mm, the depth twice the width when `0` |
| `cut_offset_factor` | `1.0` | `1.0` | End-cut offset from crossing neighbour axis (`1.0` = flush side face) |
| `edge_up` | — | `false` | Beam up from the faces of its edge, else from its own face |
| `frame_ups` | `{}` | `{}` | Up per naked edge, the others transported |
| `butt` | `false` | `false` | Perimeter frame butted at its corners, else mitered |
| `through_priority` | `{}` | `{}` | Per naked edge, the higher runs through a butt corner |

---

## Pinwheel Rotation {#reciprocal_rotation}

`ReciprocalRotation` scales each interior mesh edge about its midpoint and rotates it by `angle` around its average face normal so beams meeting at each vertex form a pinwheel.

### Input Mesh and Rotated Axes

![](templates/templates_reciprocal_1_rotation_mesh.webp)

![](templates/templates_reciprocal_1_rotation_axes.webp)

```cpp
const Vector dir = along.normalized().transformed(Xform::rotation(up, guide.angle));
const double half = along.magnitude() * 0.5 * guide.scale;
axes.push_back(Axis{{edge.first, edge.second}, Line::from_points(mid - dir * half, mid + dir * half), up});
```

### Perimeter Frame and Trimmed Beams

![](templates/templates_reciprocal_1_rotation_frame.webp)

![](templates/templates_reciprocal_1_rotation_beams.webp)

---

## Half-Edge Translation {#reciprocal_move}

`ReciprocalMove` 2-colors mesh half-edges by breadth-first traversal and shifts every color-`0` interior half-edge sideways along the corner bisector by `shift`.

### Input Mesh and Shifted Axes

![](templates/templates_reciprocal_2_move_mesh.webp)

![](templates/templates_reciprocal_2_move_axes.webp)

```cpp
const Vector bisector = (axes[i][(j + 1) % n].dir - axes[i][(j - 1 + n) % n].dir) * 0.5;
axes[i][j].line += bisector * guide.shift;
```

### Perimeter Frame and Trimmed Beams

![](templates/templates_reciprocal_2_move_frame.webp)

![](templates/templates_reciprocal_2_move_beams.webp)

<!-- assembly:begin -->
## Assembly Sequence {#reciprocal_assembly_sequence}

The frame beam by beam, then the beams from the frame to the middle. A beam slides in along the direction nearest to straight down that its neighbours' side faces leave it, and would turn first where no slide is left: no beam of the two needs to. Every move is a `ModuleSequence` move: the module taken apart piece by piece, each piece swept as a solid against every piece still in, the moving ones drawn brown, a GIF per kind of module and one of the whole.

| assembly | | |
|---|---|---|
| **Rotation**, 262 elements, 261 moves | | |
| ![frame, 44 pieces](assembly/assembly_templates_reciprocal_1_rotation_frame.gif) frame, 44 pieces | ![reciprocal rotation, 219 pieces](assembly/assembly_templates_reciprocal_1_rotation.gif) reciprocal rotation, 219 pieces |  |
| **Move**, 43 elements, 42 moves | | |
| ![frame, 15 pieces](assembly/assembly_templates_reciprocal_2_move_frame.gif) frame, 15 pieces | ![reciprocal move, 29 pieces](assembly/assembly_templates_reciprocal_2_move.gif) reciprocal move, 29 pieces |  |

<!-- assembly:end -->

## Examples {#reciprocal_examples}

### 1 Rotation {#reciprocal_example_1}

The reciprocal rotation template on the sinusoidal dome: every interior edge of the quad mesh becomes a 100 x 400 beam, lengthened 1.4 times about its midpoint and turned 0.35 rad about its normal, its ends cut at the side faces of the beams it meets; straight frame beams on the naked edges, butted at the corners. Groups mesh, axes, frame and beams, one per constructor step.

\include{lineno} templates_reciprocal_1_rotation.cpp

### 2 Move {#reciprocal_example_2}

The reciprocal move template on the sinusoidal dome's hexagonal mesh: every other edge of each face becomes a 100 x 400 beam moved 100 sideways so it bears on its neighbours, each end cut flush at the beam it lands on; upright frame beams on the naked edges, butted at the corners. Groups mesh, axes, frame and beams, one per constructor step.

\include{lineno} templates_reciprocal_2_move.cpp
