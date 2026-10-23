# Shells {#templates_shells}

[TOC]

Plates on a surface. Each template is a `WoodSession`: its constructor is the recipe, one block per modelling step, and each step's result sits in a tree group named after it, so every picture below is that group of the example's scene.

## TranslationShell

A shell of mitered plates, a cross section swept along a profile (`src/templates/shells/translation_shell.h`).

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

One plate per quad, its sides mitered at every fold by `Mesh::miter_contours` and its sharp corners chamfered, named `plate_<i>`.

![plates](templates/templates_shells_1_translation_shell_plates.webp)

```cpp
const std::vector<std::shared_ptr<Plate>> plates = mitered_plates(
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
