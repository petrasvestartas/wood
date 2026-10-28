# Shells {#templates_shells}

[TOC]

Translational plate shells (`src/templates/shells/translation_shell.h`). `TranslationShell` subclasses `WoodSession` and transforms two primary input polylines (`cross_section` and `profile`) into mitered, corner-chamfered `Plate` solids across four scene-tree groups (`curves` $\to$ `sections` $\to$ `mesh` $\to$ `plates`).

![TranslationShell Primary Inputs and Geometry Generation Pipeline](diagrams/template_shells_pipeline.webp)

## Primary Inputs & Parameters

| Input / Parameter | Type | Default | Geometric Role |
| --- | --- | --- | --- |
| `cross_section` | `Polyline` | `default_cross_section()` (`11` pts in $XZ$) | Generator arch curve swept without rotation |
| `profile` | `Polyline` | `default_profile()` (`12` pts in $YZ$) | Directrix trajectory along which `cross_section` is translated |
| `thickness` | `double` | `10.0` | Plate thickness $t$ (`mm`) offset by $\pm t/2$ along each quad normal |
| `chamfer` | `double` | `1.0` | Corner relief setback (`mm`) at sharp polygon vertices |
| `chamfer_angle` | `double` | `180.0` | Interior corner angle threshold (degrees) below which corners are chamfered |

---

## Geometry Generation Pipeline

### 1. Primary Input Curves (`curves`)

The generator `cross_section` ($XZ$-plane arch) and directrix `profile` ($YZ$-plane arch) are stored in the `curves` group:

![curves](templates/templates_shells_1_translation_shell_curves.webp)

```cpp
const std::shared_ptr<TreeNode> curves = add_group("curves");
add_polyline(cross_section, curves);
add_polyline(profile, curves);
```

### 2. Translated Sections (`sections`)

`compute_sections()` translates `cross_section` by the displacement vector $\Delta_i = \text{profile}[i] - \text{profile}[0]$ at each of the $N_p$ profile vertices:

![sections](templates/templates_shells_1_translation_shell_sections.webp)

```cpp
for (size_t i = 0; i < profile.point_count(); i++) {
    const Vector step = profile[i] - profile[0];
    sections.push_back(cross_section.translated(step));
}
```

### 3. Translational Quad Mesh (`mesh`)

Because every section is a pure translation $\Delta_i$, each quad $\{v_{i,j},\, v_{i-1,j},\, v_{i-1,j+1},\, v_{i,j+1}\}$ spans two parallel displacement vectors and is **guaranteed planar**:

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

### 4. Mitered & Chamfered Plates (`plates`)

`mitered_plates` (`src/templates/template_chamfer.h`) calls `Mesh::miter_contours` to offset each quad by $\pm t/2$ and trim its edges on the dihedral bisector planes shared with adjacent quads, then applies `chamfer_contours` to relieve sharp corners before instantiating `plate_0 .. plate_109`:

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

---

## Full C++ Examples

### Custom Cross-Section & Profile Polylines

```cpp
#include "wood_session.h"
#include "src/templates/shells/translation_shell.h"

using namespace session_cpp;
using namespace wood_session;

int main() {
    // 1. Primary inputs: cross_section arch in XZ and profile arch in YZ
    const Polyline cross_section({
        {1200.0, 0.0,   0.0},
        { 900.0, 0.0, 260.0},
        { 600.0, 0.0, 360.0},
        { 300.0, 0.0, 260.0},
        {   0.0, 0.0,   0.0},
    });
    const Polyline profile({
        {0.0,    0.0,   0.0},
        {0.0, -500.0, 240.0},
        {0.0, -1000.0, 340.0},
        {0.0, -1500.0, 240.0},
        {0.0, -2000.0,   0.0},
    });

    // 2. Generate shell (curves -> sections -> mesh -> mitered & chamfered plates)
    TranslationShell shell(cross_section, profile, 12.0, 1.5, 180.0, "custom_translation_shell");

    const std::vector<std::shared_ptr<Plate>> plates = shell.get_elements_numbered<Plate>("plate");
    std::cout << fmt::format("translation shell: {} plates\n", plates.size());
    shell.pb_dump(pb_path("live"));
    return 0;
}
```

### Default Translation Shell (`examples/templates_shells_1_translation_shell.cpp`)

\include{lineno} templates_shells_1_translation_shell.cpp
