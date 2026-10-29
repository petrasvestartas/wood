# Shells {#templates_shells}

[TOC]

Translational plate shells in `src/templates/shells/translation_shell.h`. `TranslationShell` sweeps a generator cross-section along a directrix profile without rotation, producing planar quad faces and mitered, corner-chamfered `Plate` solids.

## Parameters

| Parameter | Type | Default | Role |
| --- | --- | --- | --- |
| cross_section | Polyline | default_cross_section() | Generator arch curve in the XZ plane |
| profile | Polyline | default_profile() | Directrix trajectory in the YZ plane |
| thickness | double | 10.0 | Plate thickness in mm |
| chamfer | double | 1.0 | Corner relief setback in mm |
| chamfer_angle | double | 180.0 | Interior corner angle threshold in degrees |

## Input Curves {#shells_curves}

Generator `cross_section` and directrix `profile` polylines stored in the `curves` group.

![](templates/templates_shells_1_translation_shell_curves.webp)

```cpp
const std::shared_ptr<TreeNode> curves = add_group("curves");
add_polyline(cross_section, curves);
add_polyline(profile, curves);
```

## Translated Sections {#shells_sections}

`compute_sections()` translates `cross_section` by `profile[i] - profile[0]` at each profile vertex.

![](templates/templates_shells_1_translation_shell_sections.webp)

```cpp
for (size_t i = 0; i < profile.point_count(); i++) {
    const Vector step = profile[i] - profile[0];
    sections.push_back(cross_section.translated(step));
}
```

## Planar Quad Mesh {#shells_mesh}

Because every section is a pure translation, each quad spans two parallel displacement vectors and is strictly planar.

![](templates/templates_shells_1_translation_shell_mesh.webp)

```cpp
for (size_t i = 1; i < sections.size(); i++)
    for (size_t j = 0; j + 1 < count; j++) {
        const size_t row = i * count + j;
        const size_t previous = row - count;
        faces.push_back({row, previous, previous + 1, row + 1});
    }

return Mesh::from_vertices_and_faces(points, faces);
```

## Mitered Plates {#shells_plates}

`mitered_plates` offsets each quad by `±thickness / 2`, trims shared edges on dihedral bisector planes, and chamfers sharp corners.

![](templates/templates_shells_1_translation_shell_plates.webp)

```cpp
const std::vector<std::shared_ptr<Plate>> plates = mitered_plates(
    mesh, thickness, chamfer, chamfer, chamfer_angle
);

for (const std::shared_ptr<Plate>& plate : plates)
    add(plate, plates_group);
```

## Example

```cpp
#include "wood_session.h"
#include "src/templates/shells/translation_shell.h"

using namespace session_cpp;
using namespace wood_session;

int main() {
    const Polyline cross_section({
        {1200.0, 0.0,   0.0},
        { 900.0, 0.0, 260.0},
        { 600.0, 0.0, 360.0},
        { 300.0, 0.0, 260.0},
        {   0.0, 0.0,   0.0},
    });
    const Polyline profile({
        {0.0,     0.0,   0.0},
        {0.0,  -500.0, 240.0},
        {0.0, -1000.0, 340.0},
        {0.0, -1500.0, 240.0},
        {0.0, -2000.0,   0.0},
    });

    TranslationShell shell(cross_section, profile, 12.0, 1.5, 180.0, "custom_translation_shell");
    shell.pb_dump(pb_path("live"));
    return 0;
}
```
