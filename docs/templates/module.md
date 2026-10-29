# Module {#templates_module}

[TOC]

Framed timber walls, hollow cassette shells, and multi-storey room modules in `src/templates/module/*.h` with automatic face-contact detection.

<div class="wood-card-grid cols-3">
  <a class="wood-card" href="#module_wall">
    <img src="templates_module_1_wall_all.webp" alt="Framed Wall">
    <span class="wood-card-title">Framed Wall</span>
  </a>
  <a class="wood-card" href="#module_cassette_shell">
    <img src="templates_module_2_cassette_shell_all.webp" alt="Cassette Shell">
    <span class="wood-card-title">Cassette Shell</span>
  </a>
  <a class="wood-card" href="#module_cassette_house">
    <img src="templates_module_3_cassette_house_all.webp" alt="Cassette House">
    <span class="wood-card-title">Cassette House</span>
  </a>
  <a class="wood-card" href="#module_room">
    <img src="templates_module_4_room_module_all.webp" alt="Room Module">
    <span class="wood-card-title">Room Module</span>
  </a>
  <a class="wood-card" href="#module_block">
    <img src="templates_module_5_module_block_all.webp" alt="Module Block">
    <span class="wood-card-title">Module Block</span>
  </a>
  <a class="wood-card" href="#module_bank">
    <img src="templates_module_6_bank_pavilion_all.webp" alt="Bank Pavilion">
    <span class="wood-card-title">Bank Pavilion</span>
  </a>
</div>

---

## 1. Framed Wall {#module_wall}

A framed wall in the `XZ`-plane with openings and two sheathing panels (`module/wall.h`, `module/framed_panel.h`, example `templates_module_1_wall`).

![](templates/templates_module_1_wall_all.webp)

### Wall Parameters

| Parameter | Default | Description |
| --- | --- | --- |
| `length`, `height` | `4800.0`, `2700.0` | Overall wall length along `x` and height along `z` (`mm`), foot at `z = 0` |
| `stud_width` | `60.0` | In-plane width (`mm`) of all rails, studs, jamb studs, headers, and sills |
| `stud_depth` | `160.0` | Frame depth (`mm`) across the wall (`y`-axis insulation cavity) |
| `stud_spacing` | `625.0` | Grid spacing (`mm`) of studs from `x = 0` (half of a `1250 mm` board) |
| `sheet_thickness` | `15.0` | Thickness (`mm`) of the two face sheathing `Plate` panels |
| `openings` | Door at `600`, window at `2525` | `std::vector<Opening>` (`{x, width, sill, head}`; `sill == 0.0` defines a door) |

### Wall Generation Steps

1. Frame (`frame`): Bottom and top rails (bottom rail interrupted across door openings), full-height jamb studs on either side of every opening with headers and window sills between them, and grid studs every `stud_spacing` (omitting any grid stud within `0.5 * stud_spacing` of a jamb or end stud and splitting studs inside openings into top and bottom cripples):

![](templates/templates_module_1_wall_frame.webp)

```cpp
for (double x = guide.stud_spacing; x + half < guide.length - w; x += guide.stud_spacing) { ... }
```

2. Sheathing (`sheathing`): One `15 mm` `Plate` on each face of the wall, with doors cut as bottom boundary notches and windows cut as interior polygon holes (`Plate(bottom, top, inner_bottoms, inner_tops)`):

![](templates/templates_module_1_wall_sheathing.webp)

```cpp
for (const std::pair<double, size_t>& entry : doors) {
    const Opening& door = guide.openings[entry.second];
    corners.push_back(Point(door.x, levels[k], 0.0));
    corners.push_back(Point(door.x, levels[k], door.head));
    corners.push_back(Point(door.x + door.width, levels[k], door.head));
    corners.push_back(Point(door.x + door.width, levels[k], 0.0));
}
```

![](templates/templates_module_1_wall_inside.webp)

3. Contacts (`contacts_frame`, `contacts_sheathing`): `add_module_contacts` (`module/module_contact.h`) detects all `26` frame-to-frame and `32` sheet-to-frame `InteractionContactFace` regions (`<target>_on_<source>` or `<target>_beside_<source>`):

```cpp
const bool beside = std::max(end_i, end_j) < 0.9;
const std::shared_ptr<Beam> source = !beside && end_i > end_j ? beam_j : beam_i;
const std::shared_ptr<Beam> target = !beside && end_i > end_j ? beam_i : beam_j;
const std::shared_ptr<InteractionContactFace> face = session.compute_face_contact(source, target);
```

```cpp
#include "wood_session.h"
#include "src/templates/module/wall.h"

using namespace session_cpp;
using namespace wood_session;

/// A framed wall module with a door and a window.
int main() {

    wood_module::Wall wall;
    std::cout << wall << std::endl;

    const size_t frame = wall.get_branch("contacts_frame").objects.meshes->size();
    const size_t sheathing = wall.get_branch("contacts_sheathing").objects.meshes->size();
    std::cout << fmt::format("wall: {} frame contacts, {} sheathing contacts", frame, sheathing)
              << std::endl;
    wall.pb_dump(pb_path("live"));
    return 0;
}
```

---

## 2. Cassette Shell {#module_cassette_shell}

Segmented shell of hollow timber cassettes — one per planar face of a polygon `Mesh` (`module/cassette_shell.h`, example `templates_module_2_cassette_shell`).

![](templates/templates_module_2_cassette_shell_all.webp)

### Cassette Shell Parameters

| Parameter | Default | Description |
| --- | --- | --- |
| `mesh` | `default_mesh()` (`15`-face VDA dome) | Input polygon mesh (faces may include re-entrant door notches) |
| `depth` | `200.0` | Total cassette thickness (`mm`) along the face normal, including face plates |
| `plate_thickness` | `20.0` | Top and bottom face `Plate` thickness (`mm`) |
| `beam_width` | `60.0` | In-plane width (`mm`) of perimeter edge beams, jambs, headers, sills, and grid studs |
| `openings` | `{}` | Per-face `Opening{x, width, sill, head}` list in the local face frame |
| `stud_spacing` | `0.0` (none) | Internal stud spacing (`mm`) along each face's first edge |
| `ranks` | `{}` (bevel miters) | Per-face priority rank at T-joints (`ranks.empty()` → angle-bisector bevel seams; ranked → lower rank runs through square) |

### Cassette Shell Generation Steps

1. Edge and Miter Planes: When `ranks` is empty, each shared edge receives a plane leaning on the average normal of its two adjacent cells (`face.z_axis() + other.z_axis()`), and each corner receives the dihedral bisector of its two meeting edge planes:

```cpp
if (ranks.empty()) {
    Vector lean = (end - start).cross(face.z_axis() + other.z_axis()) * winding;
    lean.normalize_self();
    planes[f].push_back(Plane::from_point_normal(middle, lean));
    continue;
}
```

2. Hollow Cassette Lofting: Each cell consists of top and bottom face `Plate` panels clipped by the edge planes plus perimeter beams lofted between `bottom` and `top` (`outer` edge plane to `inner = outer.translate_by_normal(-beam_width)`):

![](templates/templates_module_2_cassette_shell_cassette_4.webp)

```cpp
const Plane& outer = _edge_planes[f][j];
const Plane inner = outer.translate_by_normal(-beam_width);
const std::vector<Plane> sides = {outer, end_plane, inner, start_plane};
const Polyline bottom_loop = Polyline::from_planes(sides, bottom);
const Polyline top_loop = Polyline::from_planes(sides, top);
```

3. Intra- and Inter-Cassette Contacts: `compute_face_contact` verifies every internal part-to-part contact and all `104` inter-cassette contacts across the `29` interior edges (throwing if any shared edge lacks contact):

![](templates/templates_module_2_cassette_shell_two_cassettes.webp)

```cpp
for (const std::shared_ptr<Plate>& a : parts[f])
    for (const std::shared_ptr<Plate>& b : parts[static_cast<size_t>(g)]) {
        const std::shared_ptr<InteractionContactFace> face = add_contact(a, b);
        if (face)
            contacts.between.push_back({a, b, face});
    }
```

```cpp
#include "wood_session.h"
#include "src/templates/module/cassette_shell.h"

using namespace session_cpp;
using namespace wood_session;

/// A segmented shell of hollow cassettes on the VDA dome.
int main() {

    wood_module::CassetteShell shell;
    std::cout << shell << std::endl;

    const size_t contacts = shell.get_branch("contacts").objects.meshes->size();
    std::cout << fmt::format(
                     "cassette shell: {} cassettes, {} contacts between them",
                     shell.mesh.number_of_faces(),
                     contacts
                 )
              << std::endl;
    shell.pb_dump(pb_path("live"));
    return 0;
}
```

---

## 3. Cassette House {#module_cassette_house}

A `6000 × 4000 mm` gable house (`2700 mm` eaves, `4500 mm` ridge, `6` cassettes, `43` inter-cassette contacts, example `templates_module_3_cassette_house`) combining `CassetteShell::house_mesh()`, `house_windows()`, `stud_spacing = 625.0`, and `house_ranks()`.

![](templates/templates_module_3_cassette_house_all.webp)

With `house_ranks()`, adjacent cassettes meet in ranked T-joints rather than bevels: roof cassettes run over walls and gables, and long walls run past gable ends:

```cpp
const double side = (face.origin() - other.origin()).dot(other.z_axis()) > 0.0 ? 1.0 : -1.0;
const double reach = through ? -side : side;
const Point surface = other.origin() + other.z_axis() * (reach * 0.5 * depth);
planes[f].push_back(Plane::from_point_normal(surface, other.z_axis() * -side));
```

![](templates/templates_module_3_cassette_house_inside.webp)

![](templates/templates_module_3_cassette_house_front.webp)

```cpp
for (double u = stud_spacing; u < length; u += stud_spacing) { ... }
```

![](templates/templates_module_3_cassette_house_gable.webp)

![](templates/templates_module_3_cassette_house_section.webp)

![](templates/templates_module_3_cassette_house_eave.webp) ![](templates/templates_module_3_cassette_house_ridge.webp)

```cpp
#include "wood_session.h"
#include "src/templates/module/cassette_shell.h"

using namespace session_cpp;
using namespace wood_session;

/// A gable house of six cassettes.
int main() {

    wood_module::CassetteShell house(
        wood_module::CassetteShell::house_mesh(),
        160.0,
        20.0,
        60.0,
        wood_module::CassetteShell::house_windows(),
        625.0,
        wood_module::CassetteShell::house_ranks(),
        "cassette_house"
    );
    std::cout << house << std::endl;

    const size_t contacts = house.get_branch("contacts").objects.meshes->size();
    std::cout << fmt::format(
                     "cassette house: {} cassettes, {} contacts between them",
                     house.mesh.number_of_faces(),
                     contacts
                 )
              << std::endl;
    house.pb_dump(pb_path("live"));
    return 0;
}
```

---

## 4. Room Module {#module_room}

Volumetric timber room modules (`module/room_module.h`, examples `templates_module_4_room_module`, `templates_module_5_module_block`, and `templates_module_6_bank_pavilion`).

![](templates/templates_module_4_room_module_all.webp)

### Room Module Parameters

| Parameter | Default | Description |
| --- | --- | --- |
| `length`, `width`, `clear_height` | `9000.0`, `3000.0`, `2700.0` | Single module plan (`mm`) and interior clear height (`3150 mm` total height) |
| `stud_width`, `stud_depth`, `stud_spacing` | `60.0`, `160.0`, `625.0` | Wall framing section (`mm`) and stud grid spacing |
| `sheet_thickness` | `15.0` | Wall sheathing thickness (`mm`); corner posts match total wall thickness (`190 mm`) |
| `floor_joist_width`, `floor_joist_depth`, `deck_thickness` | `80.0`, `240.0`, `25.0` | `280 mm` floor cassette: `190 × 280` long edge beams + `16` transverse `80 × 240` joists between `15 mm` bottom board and `25 mm` top deck |
| `ceiling_joist_width`, `ceiling_joist_depth`, `board_thickness` | `60.0`, `140.0`, `15.0` | `170 mm` ceiling cassette: `16` transverse `60 × 140` joists between two `15 mm` boards |
| `downstand_depth` | `300.0` | Downstand drop of the two `190 × 470` long ceiling edge beams (`2400 mm` clear under downstand) |
| `side_posts` | `1` | Intermediate posts along open long sides between modules of the same room |
| `layouts` | Window at facade end, door at corridor end | Per-module wall openings (`{front, back, start, end}`) and entrance `porch` setback |
| `rooms`, `storeys` | `{2}`, `1` | Modules grouped side-by-side into open rooms (`rooms = {2, 2}`) and stacked vertically (`storeys = 3`) |

### Room Module Generation Steps

1. Open-Sided Module Logic: Long sides between modules belonging to the same room are left open (supported by corner posts and `side_posts` at mid-span); between adjacent rooms, a single partition wall is placed inside the smaller room's module:

![](templates/templates_module_4_room_module_single.webp)

```cpp
const bool front_inside = column > 0 && previous[0] == room[0];
const bool back_inside = column + 1 < columns() && next[0] == room[0];
const bool front_wall = column == 0 || (!front_inside && room[1] < previous[1]);
const bool back_wall = column + 1 == columns() || (!back_inside && room[1] <= next[1]);
```

2. Floor, Ceiling, and Wall Assemblies: Vertical load paths align in a `190 mm` strip along each long side (ceiling `190 × 470` edge beam → `190 × 190` corner and mid posts and `2400 mm` long walls → floor `190 × 280` edge beam):

![](templates/templates_module_4_room_module_floor.webp)

![](templates/templates_module_4_room_module_ceiling.webp)

![](templates/templates_module_4_room_module_section.webp)

![](templates/templates_module_4_room_module_wall.webp)

![](templates/templates_module_4_room_module_inside.webp)

3. Contacts (`contacts_frame`, `contacts_sheathing`): `add_module_contacts` registers all `333` frame-to-frame and `378` sheet-to-frame face contacts across both modules.

```cpp
#include "wood_session.h"
#include "src/templates/module/room_module.h"

using namespace session_cpp;
using namespace wood_session;

/// One room of two timber room modules open between them: framed floors and ceilings on corner posts, walls on the closed sides.
int main() {

    wood_module::RoomModule module;
    std::cout << module << std::endl;

    const size_t frame = module.get_branch("contacts_frame").objects.meshes->size();
    const size_t sheathing = module.get_branch("contacts_sheathing").objects.meshes->size();
    std::cout << fmt::format(
                     "room module: {} modules of {} x {} x {} mm, {} frame contacts, {} sheathing "
                     "contacts",
                     module.columns(),
                     module.guide.length,
                     module.guide.width,
                     module.module_height(),
                     frame,
                     sheathing
                 )
              << std::endl;
    module.pb_dump(pb_path("live"));
    return 0;
}
```

---

### Multi-Storey Module Block {#module_block}

Twelve modules (`rooms = {2, 2}`, `storeys = 3`) arranged as two 2-module rooms per floor over three storeys (`examples/templates_module_5_module_block.cpp`):

![](templates/templates_module_5_module_block_all.webp)

```cpp
#include "wood_session.h"
#include "src/templates/module/room_module.h"

using namespace session_cpp;
using namespace wood_session;

/// A block of timber room modules like the ZM10 school pavilions: two rooms of two modules per storey, three storeys stacked.
int main() {

    wood_module::RoomModuleGuide guide;
    guide.rooms = {2, 2};
    guide.storeys = 3;
    wood_module::RoomModule block(guide, "module_block");
    std::cout << block << std::endl;

    const size_t frame = block.get_branch("contacts_frame").objects.meshes->size();
    const size_t sheathing = block.get_branch("contacts_sheathing").objects.meshes->size();
    std::cout << fmt::format(
                     "module block: {} modules, {} frame contacts, {} sheathing contacts",
                     block.columns() * guide.storeys,
                     frame,
                     sheathing
                 )
              << std::endl;
    block.pb_dump(pb_path("live"));
    return 0;
}
```

---

### Bank Pavilion {#module_bank}

Four modules on a `3000 × 10270 mm` grid (`rooms = {1, 2, 1}`: closed meeting room, 2-module `6000 mm` clear open office, and entrance module with a `2730 mm` inset porch, `examples/templates_module_6_bank_pavilion.cpp`):

![](templates/templates_module_6_bank_pavilion_all.webp)

![](templates/templates_module_6_bank_pavilion_plan.webp)

![](templates/templates_module_6_bank_pavilion_open.webp)

```cpp
#include "wood_session.h"
#include "src/templates/module/room_module.h"

using namespace session_cpp;
using namespace wood_session;

/// The bank pavilion of four modules.
int main() {

    wood_module::RoomModuleGuide guide;
    guide.length = 10270.0;
    guide.stud_depth = 180.0;
    guide.rooms = {1, 2, 1};
    guide.layouts = {
        {{{{},
           {{4825.0, 1590.0, 0.0, 2100.0}},
           {{330.0, 1620.0, 600.0, 2500.0}},
           {{330.0, 1620.0, 600.0, 2500.0}}}},
         0.0},
        {{{{}, {}, {{330.0, 1620.0, 600.0, 2500.0}}, {{330.0, 1620.0, 600.0, 2500.0}}}}, 0.0},
        {{{{}, {}, {{330.0, 1620.0, 600.0, 2500.0}}, {{330.0, 1620.0, 600.0, 2500.0}}}}, 0.0},
        {{{{{395.0, 1550.0, 0.0, 2100.0}, {5295.0, 890.0, 0.0, 2100.0}, {7557.0, 890.0, 0.0, 2100.0}
           },
           {{1268.0, 720.0, 1500.0, 2100.0}},
           {{120.0, 910.0, 600.0, 2500.0}, {1295.0, 910.0, 600.0, 2500.0}},
           {{323.0, 1020.0, 600.0, 2500.0}}}},
         2730.0},
    };
    wood_module::RoomModule pavilion(guide, "bank_pavilion");
    std::cout << pavilion << std::endl;

    const size_t frame = pavilion.get_branch("contacts_frame").objects.meshes->size();
    const size_t sheathing = pavilion.get_branch("contacts_sheathing").objects.meshes->size();
    std::cout
        << fmt::format(
               "bank pavilion: {} modules of {} x {} mm, {} frame contacts, {} sheathing contacts",
               pavilion.columns(),
               guide.length,
               guide.width,
               frame,
               sheathing
           )
        << std::endl;
    pavilion.pb_dump(pb_path("live"));
    return 0;
}
```
