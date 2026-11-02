# Reciprocal {#templates_reciprocal}

[TOC]

Reciprocal timber beam frames on polygon meshes in `src/templates/reciprocal/`. Every interior beam bears against the side face of its neighbour while naked boundary edges are enclosed by a mitered or priority-butted perimeter frame.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#reciprocal_rotation">
    <img src="templates_reciprocal_1_rotation_beams.webp" alt="Pinwheel Rotation">
    <span class="wood-card-title">Pinwheel Rotation</span>
  </a>
  <a class="wood-card" href="#reciprocal_move">
    <img src="templates_reciprocal_2_move_beams.webp" alt="Half-Edge Translation">
    <span class="wood-card-title">Half-Edge Translation</span>
  </a>
</div>

## Parameters

| Parameter | Rotation | Translation | Description |
| --- | --- | --- | --- |
| `mesh` | `const Mesh&` | `const Mesh&` | Input polygon mesh |
| `angle`, `scale` | `0.35 rad`, `1.4` | — | Midpoint rotation around average face normal and axial scale |
| `shift` | — | `50.0` | Sideways translation along corner bisectors in mm |
| `width`, `height` | `100.0`, `400.0` | `100.0`, `400.0` | Beam cross-section width and depth in mm |
| `cut_offset_factor` | `1.0` | `1.0` | End-cut offset from crossing neighbour axis (`1.0` = flush side face) |
| `corner_joint` | `Miter` / `Butt` | `Miter` / `Butt` | Perimeter frame corner joint rule |

---

## Pinwheel Rotation {#reciprocal_rotation}

`ReciprocalRotation` scales each interior mesh edge about its midpoint and rotates it by `angle` around its average face normal so beams meeting at each vertex form a pinwheel.

### Input Mesh and Rotated Axes

![](templates/templates_reciprocal_1_rotation_mesh.webp)

![](templates/templates_reciprocal_1_rotation_axes.webp)

```cpp
const Vector dir = along.normalized().transformed(Xform::rotation(up, angle));
const double half = along.magnitude() * 0.5 * scale;
axes.push_back(Axis{{edge.first, edge.second}, Line::from_points(mid - dir * half, mid + dir * half), up});
```

### Perimeter Frame and Trimmed Beams

![](templates/templates_reciprocal_1_rotation_frame.webp)

![](templates/templates_reciprocal_1_rotation_beams.webp)

```cpp
#include "wood_session.h"
#include "src/templates/reciprocal/reciprocal_rotation.h"

int main() {
    ReciprocalRotation frame(ReciprocalRotation::default_mesh(), 0.35, 1.4, 100.0, 400.0);
    frame.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

---

## Half-Edge Translation {#reciprocal_move}

`ReciprocalMove` 2-colors mesh half-edges by breadth-first traversal and shifts every color-`0` interior half-edge sideways along the corner bisector by `shift`.

### Input Mesh and Shifted Axes

![](templates/templates_reciprocal_2_move_mesh.webp)

![](templates/templates_reciprocal_2_move_axes.webp)

```cpp
const Vector bisector = (axes[i][(j + 1) % n].dir - axes[i][(j - 1 + n) % n].dir) * 0.5;
axes[i][j].line += bisector * shift;
```

### Perimeter Frame and Trimmed Beams

![](templates/templates_reciprocal_2_move_frame.webp)

![](templates/templates_reciprocal_2_move_beams.webp)

```cpp
#include "wood_session.h"
#include "src/templates/reciprocal/reciprocal_move.h"

int main() {
    ReciprocalMove frame(ReciprocalMove::default_mesh(), 100.0, 100.0, 400.0);
    frame.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

## Examples {#reciprocal_examples}

### 1 Rotation {#reciprocal_example_1}

The reciprocal rotation template on the sinusoidal dome: every interior edge of the quad mesh becomes a 100 x 400 beam, lengthened 1.4 times about its midpoint and turned 0.35 rad about its normal, its ends cut at the side faces of the beams it meets; straight frame beams on the naked edges, butted at the corners. Groups mesh, axes, frame and beams, one per constructor step.

\include{lineno} templates_reciprocal_1_rotation.cpp

### 2 Move {#reciprocal_example_2}

The reciprocal move template on the sinusoidal dome's hexagonal mesh: every other edge of each face becomes a 100 x 400 beam moved 100 sideways so it bears on its neighbours, each end cut flush at the beam it lands on; upright frame beams on the naked edges, butted at the corners. Groups mesh, axes, frame and beams, one per constructor step.

\include{lineno} templates_reciprocal_2_move.cpp
