# Shells {#templates_shells}

[TOC]

Translational plate shells (`src/templates/shells/translation_shell.h`). `TranslationShell` subclasses `WoodSession` and transforms two input polylines (`cross_section` and `profile`) into mitered, corner-chamfered `Plate` solids across four scene-tree groups (`curves` → `sections` → `mesh` → `plates`).

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#shells_curves">
    <img src="templates_shells_1_translation_shell_curves.webp" alt="Input Curves">
    <span class="wood-card-title">1. Input Curves</span>
    <span class="wood-card-sub">Generator `cross_section` (XZ) &amp; directrix `profile` (YZ)</span>
  </a>
  <a class="wood-card" href="#shells_sections">
    <img src="templates_shells_1_translation_shell_sections.webp" alt="Translated Sections">
    <span class="wood-card-title">2. Translated Sections</span>
    <span class="wood-card-sub">`cross_section` translated along `profile` vertices</span>
  </a>
  <a class="wood-card" href="#shells_mesh">
    <img src="templates_shells_1_translation_shell_mesh.webp" alt="Quad Mesh">
    <span class="wood-card-title">3. Planar Quad Mesh</span>
    <span class="wood-card-sub">Guaranteed-planar translational quad faces</span>
  </a>
  <a class="wood-card" href="#shells_plates">
    <img src="templates_shells_1_translation_shell_plates.webp" alt="Mitered Plates">
    <span class="wood-card-title">4. Mitered &amp; Chamfered Plates</span>
    <span class="wood-card-sub">`mitered_plates` (`plate_0 .. plate_109`)</span>
  </a>
</div>

![](diagrams/template_shells_pipeline.webp)

## Primary Inputs & Parameters

| Input / Parameter | Type | Default | Geometric Role |
| --- | --- | --- | --- |
| `cross_section` | `Polyline` | `default_cross_section()` (`11` pts in `xz`) | Generator arch curve swept without rotation |
| `profile` | `Polyline` | `default_profile()` (`12` pts in `yz`) | Directrix trajectory along which `cross_section` is translated |
| `thickness` | `double` | `10.0` | Plate thickness (`mm`), offset by `±thickness / 2` along each quad normal |
| `chamfer` | `double` | `1.0` | Corner relief setback (`mm`) at sharp polygon vertices |
| `chamfer_angle` | `double` | `180.0` | Interior corner angle threshold (degrees) below which corners are chamfered |

---

## Geometry Generation Pipeline

### 1. Input Curves {#shells_curves}

The generator `cross_section` (`xz`-plane arch) and directrix `profile` (`yz`-plane arch) are stored in the `curves` group:

![](templates/templates_shells_1_translation_shell_curves.webp)

```cpp
const std::shared_ptr<TreeNode> curves = add_group("curves");
add_polyline(cross_section, curves);
add_polyline(profile, curves);
```

### 2. Translated Sections {#shells_sections}

`compute_sections()` translates `cross_section` by the displacement vector `profile[i] - profile[0]` at each profile vertex:

![](templates/templates_shells_1_translation_shell_sections.webp)

```cpp
for (size_t i = 0; i < profile.point_count(); i++) {
    const Vector step = profile[i] - profile[0];
    sections.push_back(cross_section.translated(step));
}
```

### 3. Quad Mesh {#shells_mesh}

Because every section is a pure translation, each quad between two adjacent sections spans two parallel displacement vectors and is **guaranteed planar**:

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

### 4. Mitered Plates {#shells_plates}

`mitered_plates` (`src/templates/template_chamfer.h`) calls `Mesh::miter_contours` to offset each quad by `±thickness / 2` and trim its edges on the dihedral bisector planes shared with adjacent quads, then applies `chamfer_contours` to relieve sharp corners before instantiating `plate_0 .. plate_109`:

![](templates/templates_shells_1_translation_shell_plates.webp)

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
        {0.0,     0.0,   0.0},
        {0.0,  -500.0, 240.0},
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

<details><summary>Example code: templates_shells_1_translation_shell.cpp</summary>

\include{lineno} templates_shells_1_translation_shell.cpp

</details>
