# Shells {#templates_shells}

[TOC]

Plates and boards on a surface. Each template is a `WoodSession`: its constructor is the recipe, one block per modelling step, and each step's result sits in a tree group named after it, so every picture below is that group of the example's scene.

## TranslationShell

A shell of mitred plates, a cross section swept along a profile (`src/templates/shells/translation_shell.h`).

![translation shell](templates/templates_shells_1_translation_shell_plates.webp)

```cpp
TranslationShell shell;  // or TranslationShell(cross_section, profile, thickness, chamfer, chamfer_angle)
```

| Parameter | Default | Controls |
| --- | --- | --- |
| `cross_section` | `default_cross_section()`, an arch in xz | the curve that is swept |
| `profile` | `default_profile()`, an arch in yz | the path it is swept along |
| `thickness` | 10 | plate thickness |
| `chamfer` | 1 | how far a sharp corner is cut back |
| `chamfer_angle` | 180 | corners sharper than this, in degrees, are chamfered |

### curves

The cross section and the profile it is swept along.

![curves](templates/templates_shells_1_translation_shell_curves.webp)

```cpp
const std::shared_ptr<TreeNode> curves = add_group("curves");
add_polyline(cross_section, curves);
add_polyline(profile, curves);
```

### sections

The cross section moved to every profile point.

![sections](templates/templates_shells_1_translation_shell_sections.webp)

```cpp
for (size_t i = 0; i < profile.point_count(); i++) {
    const Vector step = profile[i] - profile[0];
    sections.push_back(cross_section.translated(step));
}
```

### mesh

A quad between every two neighbouring points of every two neighbouring sections.

![mesh](templates/templates_shells_1_translation_shell_mesh.webp)

```cpp
for (size_t i = 1; i < sections.size(); i++)
    for (size_t j = 0; j + 1 < count; j++) {
        const size_t row = i * count + j;
        const size_t previous = row - count;
        faces.push_back({row, previous, previous + 1, row + 1});
    }

return Mesh::from_vertices_and_faces(points, faces);
```

### plates

One plate per quad, its sides mitred at every fold by `Mesh::miter_contours` and its sharp corners chamfered, named `plate_<i>`.

![plates](templates/templates_shells_1_translation_shell_plates.webp)

```cpp
const std::vector<std::shared_ptr<Plate>> plates = mitred_plates(
    mesh,
    thickness,
    chamfer,
    chamfer,
    chamfer_angle
);

for (const std::shared_ptr<Plate>& plate : plates)
    add(plate, plates_group);
```

<details><summary>Example code: templates_shells_1_translation_shell.cpp</summary>

\include{lineno} templates_shells_1_translation_shell.cpp

</details>

## LamellaGridshell

A two-layer lamella gridshell on a NURBS surface: two upright boards per lamella and a hexagonal stud at every crossing, after the asymptotic gridshells of Eike Schling (`src/templates/shells/lamella_gridshell.h`).

![lamella gridshell on asymptotic curves](templates/templates_shells_2_gridshell_model.webp)

```cpp
wood_gridshell::LamellaGridshell gridshell;  // or LamellaGridshell(surface, curves, count_top, count_bottom, lamella)
```

| Parameter | Default | Controls |
| --- | --- | --- |
| `surface` | `default_surface(10000)`, a cubic saddle | the surface the lamellas are traced on |
| `curves` | 1 | 1 the asymptotic curves, 0 the u and v iso-curves |
| `count_top`, `count_bottom` | 9, 9 | lamellas of the first and the second family |
| `lamella.height`, `.thickness` | 140, 20 | board depth along the normal and across |
| `lamella.gap` | 60 | clear distance between the two boards, the stud width |
| `lamella.spacing` | 180 | normal distance between the two layers |
| `lamella.overrun` | 20 | stud length past each outer face |
| `lamella.step` | 100 | tracing step along a lamella |

On the asymptotic curves the normal curvature is zero, so a board bends only about its weak axis and unrolls straight; on the iso-curves (`curves = 0`, `templates_shells_3_gridshell_iso`) it does not.

![lamella gridshell on iso-curves](templates/templates_shells_3_gridshell_iso_model.webp)

### surface

The NURBS surface the lamellas are traced on.

![surface](templates/templates_shells_2_gridshell_surface.webp)

```cpp
add_nurbssurface(surface, add_group("surface"));
```

### lamellas

Each family traced by RK4 steps along its direction field from seeds evenly spaced along a spine of the other family.

![lamellas](templates/templates_shells_2_gridshell_lamellas.webp)

```cpp
const std::vector<std::vector<Point>> tops = compute_family(0, count_top);
const std::vector<std::vector<Point>> bottoms = compute_family(1, count_bottom);
```

```cpp
const Vector k1 = compute_slope(uv, direction) * lamella.step;
const Vector k2 = compute_slope(uv + k1 * 0.5, direction) * lamella.step;
const Vector k3 = compute_slope(uv + k2 * 0.5, direction) * lamella.step;
const Vector k4 = compute_slope(uv + k3, direction) * lamella.step;
const Vector delta = (k1 + k2 * 2.0 + k3 * 2.0 + k4) / 6.0;
```

### crossings

Every top lamella crossed with every bottom lamella in (u, v), with the frame of each there.

![crossings](templates/templates_shells_2_gridshell_crossings.webp)

```cpp
const std::vector<Crossing> crossings = compute_crossings(tops, bottoms);
```

### stations

Frames along every lamella, x along it and z the surface normal, three gap apart on the tangent at each crossing so the boards run straight past the stud.

![stations](templates/templates_shells_2_gridshell_stations.webp)

```cpp
for (size_t i = 0; i < tops.size(); i++)
    _stations.push_back(compute_stations(tops[i], top_marks[i]));

for (size_t j = 0; j < bottoms.size(); j++)
    _stations.push_back(compute_stations(bottoms[j], bottom_marks[j]));
```

### boards

Two `Beam` boards per lamella, gap apart, the first family `spacing / 2` up the normal and the second as far down, named `board_<top|bottom>_<i>_<side>`.

![boards](templates/templates_shells_2_gridshell_boards.webp)

```cpp
const std::shared_ptr<Beam> board = compute_board(
    _stations[i],
    upper ? lift : -lift,
    side == 0 ? shift : -shift,
    fmt::format("board_{}_{}_{}", layer, index, side)
);
add(board, group);
```

### studs

A hexagonal `Column` stud along the normal through both gaps at every crossing, its flats against the four boards, named `stud_<i>_<j>`.

![studs](templates/templates_shells_2_gridshell_studs.webp)

```cpp
for (const Crossing& crossing : crossings)
    add(compute_stud(crossing), studs);
```

### contacts

A face contact between every stud and each of the four boards it touches.

```cpp
compute_face_contacts();
```

`tests/shells_gridshell.cpp` checks both shells: every stud touches four boards, nothing overlaps, the asymptotic lamellas unroll within 0.1 mm of straight and the iso ones over 100 mm off.

<details><summary>Example code: templates_shells_2_gridshell.cpp</summary>

\include{lineno} templates_shells_2_gridshell.cpp

</details>

<details><summary>Example code: templates_shells_3_gridshell_iso.cpp</summary>

\include{lineno} templates_shells_3_gridshell_iso.cpp

</details>
