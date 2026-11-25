# Module {#templates_module}

[TOC]

Framed timber walls, hollow cassette shells, and spatial modules, alone, stacked and set into a skeleton, in `src/templates/module/*.h` with automatic face-contact detection.

<div class="wood-card-grid cols-4">
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
  <a class="wood-card" href="#module_types">
    <img src="templates_module_7_module_types_all.webp" alt="Module Types">
    <span class="wood-card-title">Module Types</span>
  </a>
  <a class="wood-card" href="#module_open">
    <img src="templates_module_8_open_module_all.webp" alt="Open Modules">
    <span class="wood-card-title">Open Modules</span>
  </a>
  <a class="wood-card" href="#module_stacked">
    <img src="templates_module_9_stacked_modules_all.webp" alt="Stacked Modules">
    <span class="wood-card-title">Stacked Modules</span>
  </a>
  <a class="wood-card" href="#module_rack">
    <img src="templates_module_10_module_rack_all.webp" alt="Rack">
    <span class="wood-card-title">Rack</span>
  </a>
  <a class="wood-card" href="#module_frame">
    <img src="templates_module_11_module_frame_all.webp" alt="Frame">
    <span class="wood-card-title">Frame</span>
  </a>
  <a class="wood-card" href="#module_levels">
    <img src="templates_module_12_module_levels_all.webp" alt="Levels">
    <span class="wood-card-title">Levels</span>
  </a>
  <a class="wood-card" href="#module_corridor">
    <img src="templates_module_13_module_corridor_all.webp" alt="Corridor">
    <span class="wood-card-title">Corridor</span>
  </a>
  <a class="wood-card" href="#module_wet_room">
    <img src="templates_module_14_wet_room_all.webp" alt="Wet Room">
    <span class="wood-card-title">Wet Room</span>
  </a>
  <a class="wood-card" href="#module_assembly_sequence">
    <img src="assembly_templates_module_1_wall.gif" alt="Assembly Sequence">
    <span class="wood-card-title">Assembly Sequence</span>
  </a>
</div>

---

## Framed Wall {#module_wall}

A framed wall in the `XZ`-plane with openings and two sheathing panels (`module/wall.h`, `module/framed_panel.h`, example `templates_module_1_wall`).

![](templates/templates_module_1_wall_all.webp)

### Wall Parameters

| Parameter | Default | Description |
| --- | --- | --- |
| `length`, `height` | `4800.0`, `2700.0` | Overall wall length along `x` and height along `z` (`mm`), foot at `z = 0` |
| `stud_width` | `60.0` | In-plane width (`mm`) of all rails, studs, jamb studs, headers, and sills |
| `stud_depth` | `160.0` | Frame depth (`mm`) across the wall (`y`-axis insulation cavity) |
| `stud_spacing` | `625.0` | Grid spacing (`mm`) of studs from `x = 0` (half of a `1250 mm` board) |
| `sheet_thickness` | `15.0` | Thickness (`mm`) of the two face sheathing `FacePlanarElement` panels |
| `openings` | Door at `600`, window at `2525` | `std::vector<Opening>` (`{x, width, sill, head}`; `sill == 0.0` defines a door) |

### Wall Generation Steps

1. Frame (`frame`): Bottom and top rails (bottom rail interrupted across door openings), full-height jamb studs on either side of every opening with headers and window sills between them, and grid studs every `stud_spacing` (omitting any grid stud within `0.5 * stud_spacing` of a jamb or end stud and splitting studs inside openings into top and bottom cripples):

![](templates/templates_module_1_wall_frame.webp)

```cpp
for (double x = guide.stud_spacing; x + half < guide.length - w; x += guide.stud_spacing) { ... }
```

2. Sheathing (`sheathing`): One `15 mm` `FacePlanarElement` on each face of the wall, with doors cut as bottom boundary notches and windows cut as interior polygon holes (`FacePlanarElement(bottom, top, inner_bottoms, inner_tops)`):

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
const std::shared_ptr<EdgeStraightElement> source = !beside && end_i > end_j ? beam_j : beam_i;
const std::shared_ptr<EdgeStraightElement> target = !beside && end_i > end_j ? beam_i : beam_j;
const std::shared_ptr<InteractionContactFace> face = session.compute_face_contact(source, target);
```

\include{lineno} templates_module_1_wall.cpp

---

## Cassette Shell {#module_cassette_shell}

Segmented shell of hollow timber cassettes — one per planar face of a polygon `Mesh` (`module/cassette_shell.h`, example `templates_module_2_cassette_shell`).

![](templates/templates_module_2_cassette_shell_all.webp)

### Cassette Shell Parameters

| Parameter | Default | Description |
| --- | --- | --- |
| `mesh` | `default_mesh()` (`15`-face VDA dome) | Input polygon mesh (faces may include re-entrant door notches) |
| `depth` | `200.0` | Total cassette thickness (`mm`) along the face normal, including face plates |
| `plate_thickness` | `20.0` | Top and bottom face `FacePlanarElement` thickness (`mm`) |
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

2. Hollow Cassette Lofting: Each cell consists of top and bottom face `FacePlanarElement` panels clipped by the edge planes plus perimeter beams lofted between `bottom` and `top` (`outer` edge plane to `inner = outer.translate_by_normal(-beam_width)`):

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
for (const std::shared_ptr<FacePlanarElement>& a : parts[f])
    for (const std::shared_ptr<FacePlanarElement>& b : parts[static_cast<size_t>(g)]) {
        const std::shared_ptr<InteractionContactFace> face = add_contact(a, b);
        if (face)
            contacts.between.push_back({a, b, face});
    }
```

\include{lineno} templates_module_2_cassette_shell.cpp

---

## Cassette House {#module_cassette_house}

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

\include{lineno} templates_module_3_cassette_house.cpp

---

## Room Module {#module_room}

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

\include{lineno} templates_module_4_room_module.cpp

---

## Module Block {#module_block}

Twelve modules (`rooms = {2, 2}`, `storeys = 3`) arranged as two 2-module rooms per floor over three storeys (`examples/templates_module_5_module_block.cpp`):

![](templates/templates_module_5_module_block_all.webp)

\include{lineno} templates_module_5_module_block.cpp

---

## Bank Pavilion {#module_bank}

Four modules on a `3000 × 10270 mm` grid (`rooms = {1, 2, 1}`: closed meeting room, 2-module `6000 mm` clear open office, and entrance module with a `2730 mm` inset porch, `examples/templates_module_6_bank_pavilion.cpp`):

![](templates/templates_module_6_bank_pavilion_all.webp)

![](templates/templates_module_6_bank_pavilion_plan.webp)

![](templates/templates_module_6_bank_pavilion_open.webp)

\include{lineno} templates_module_6_bank_pavilion.cpp

---

## Spatial Modules {#module_spatial}

A spatial module is a room made in the shop: a floor, walls on it, a ceiling on them (`SpatialModule`, `spatial_module.h`). `SpatialModuleGuide` sizes one, `ModuleBuildingGuide` sets many side by side, over each other or into a skeleton (`ModuleBuilding`, `module_building.h`). What follows is the modular construction of Kolb, Kolb, Müller, *Holzbau mit System*, 2024, chapter B8.

### Grid and Transport {#module_grid}

| | Usual | Also |
| --- | --- | --- |
| Width grid | 2.50 to 3.00 m, a module 50 mm narrower for its straps and wrapping | 3.80 m, to 5.50 m |
| Length | 6, 9, 12, 15 m, multiples of the width | |
| Clear height | 2.50, 2.75, 3.00 m | to 4.00 m |
| Made in the shop | 70 to 80 % | 90 % |

| Transport width | Permit | Escort |
| --- | --- | --- |
| to 2.55 m | none, to 16.50 m long | none |
| to 3.00 m | a yearly one | none |
| over 3.00 m | one for each journey | a vehicle from 3.50 m, a certified escort from 3.80 m |
| 4.50 to 5.50 m | one for each journey, on motorways at night | two vehicles |

To 4.00 m high and 40 t without a permit of its own; Switzerland, 2023. The straps and the wrapping count.

### Module Parameters {#module_spatial_parameters}

| Parameter | Default | Description |
| --- | --- | --- |
| `length`, `width`, `clear_height` | `6000.0`, `2950.0`, `2500.0` | Outside in plan, clear inside; a `3000` grid less the joint |
| `solid_walls`, `solid_floor`, `solid_ceiling` | `false` | One cross-laminated plate each, else framed |
| `wall_panel`, `floor_panel`, `ceiling_panel` | `100.0`, `140.0`, `80.0` | Thickness of the solid ones; the ceiling carries only itself |
| `stud_width`, `stud_depth`, `wall_sheet` | `60.0`, `120.0`, `15.0` | Wall frame and the sheet on each face |
| `rib_width`, `floor_rib`, `floor_sheet` | `80.0`, `160.0`, `27.0` | Floor box: ribs across the module, a board on each face |
| `ceiling_rib`, `ceiling_sheet` | `120.0`, `15.0` | Ceiling box |
| `spacing` | `625.0` | Studs and ribs, half a `1250` board |
| `closed`, `openings`, `shafts` | all four, none, none | Which sides have walls, their openings, holes through floor and ceiling |
| `downstand`, `side_posts` | `240.0`, `0` | Beam and posts of an open long side |
| `brace` | `0.0` | Crossed transport boards in an open side |
| `pin_spacing` | `0.0` | Pins along the edges of solid plates |

### Module Types {#module_types}

Timber frame, solid plates, and a timber frame under a solid ceiling. The walls stand on the floor and carry the ceiling along their whole length.

![](templates/templates_module_7_module_types_all.webp)

| framed | solid | mixed |
| --- | --- | --- |
| ![](templates/templates_module_7_module_types_framed_bones.webp) | ![](templates/templates_module_7_module_types_solid_inside.webp) | ![](templates/templates_module_7_module_types_mixed.webp) |

```cpp
// floor: one plate, or ribs across between two edge ribs, a board on each face
add_floor();

// walls: on the floor, the long ones over the whole length, the ends between them
add_walls();

// frames: posts and a downstand beam along every open long side
add_frames();

// ceiling: on the walls and the downstand beams
add_ceiling();
```

The solid module's plates are pinned along every edge, from the far face of one into the edge of the other.

\include{lineno} templates_module_7_module_types.cpp

### Open Modules {#module_open}

Several open modules make one large room. Their loads come down at points: posts at the corners, and between them where the use allows. Open on two sides, a module is braced until it stands.

![](templates/templates_module_8_open_module_all.webp)

\include{lineno} templates_module_8_open_module.cpp

---

## Stacked Modules {#module_stacked}

Modules that carry each other: the usual case, built to eight storeys, every module made for the load on the lowest. Every wall, floor and ceiling between two rooms is double.

![](templates/templates_module_9_stacked_modules_all.webp)

### Building Parameters {#module_building_parameters}

| Parameter | Default | Description |
| --- | --- | --- |
| `modules`, `rooms`, `storeys` | one guide, `{1, 1, 1}`, `3` | Modules across, cycled; how many make each room; how many stand on each other |
| `joint` | `50.0` | Between two modules, filled with mineral wool |
| `batten`, `strip`, `bearing` | `40.0`, `12.0`, `80.0` | Batten under a module, elastic strip on it, the width of both |
| `coupler`, `coupler_width` | `0.0`, `400.0` | Plates over the joints at the start ends |
| `post`, `beam`, `deck`, `ledger` | `0.0`, `320.0`, `0.0`, `80.0` | Skeleton; a `post` of `0` for none, a `deck` of `0` for ledgers |
| `bay`, `level`, `clearance` | `1`, `1`, `20.0` | Modules between two rows of posts, modules on one level, the gap to the skeleton |
| `corridor`, `corridor_panel` | `0.0`, `160.0` | Between two rows of modules |
| `pod`, `pod_guide`, `pod_at` | `false`, a wet room | A module set into every module |

### Bearings {#module_bearings}

A module stands on two battens along its long walls with an elastic strip on each, which keeps sound from passing; under an open side on short ones, a post's width each.

![](templates/templates_module_9_stacked_modules_bearings.webp)

### Couplers {#module_couplers}

A plate over every joint at the modules' ends, pinned into the two ceilings and the two floors that meet there, holds the storeys together against wind and lift and comes off again.

![](templates/templates_module_9_stacked_modules_front.webp)

```cpp
const std::shared_ptr<JointBeamElement> pins = JointBeamElement::headed_pins(
    *plate, *member, *seat, PinLayout::corners, 4, 20.0, 0.0, 4.0, guide.coupler + 80.0
);
```

\include{lineno} templates_module_9_stacked_modules.cpp

---

## Modules in a Skeleton {#module_skeleton}

The skeleton carries, the modules less or only themselves, so they are alike and light.

### Rack {#module_rack}

`deck = 0`: a post in every joint, a beam per storey, ledgers on its faces; a module slides in lengthwise onto two of them.

| whole | skeleton |
| --- | --- |
| ![](templates/templates_module_10_module_rack_all.webp) | ![](templates/templates_module_10_module_rack_skeleton.webp) |

\include{lineno} templates_module_10_module_rack.cpp

### Frame {#module_frame}

`bay = 2`, `level = 1`: a deck per storey, two modules open to each other on it, as when an old frame is used again.

| whole | skeleton |
| --- | --- |
| ![](templates/templates_module_11_module_frame_all.webp) | ![](templates/templates_module_11_module_frame_skeleton.webp) |

\include{lineno} templates_module_11_module_frame.cpp

### Levels {#module_levels}

`bay = 2`, `level = 2`: modules carry each other between two decks, the skeleton takes the load of every second storey.

| whole | skeleton |
| --- | --- |
| ![](templates/templates_module_12_module_levels_all.webp) | ![](templates/templates_module_12_module_levels_skeleton.webp) |

\include{lineno} templates_module_12_module_levels.cpp

---

## Corridor {#module_corridor}

Two rows of modules face each other over a corridor of planar panels on ledgers on their ends: modules where rooms repeat, panels where they do not.

| whole | one storey |
| --- | --- |
| ![](templates/templates_module_13_module_corridor_all.webp) | ![](templates/templates_module_13_module_corridor_plan.webp) |

\include{lineno} templates_module_13_module_corridor.cpp

---

## Wet Room {#module_wet_room}

A module set into a module: the rooms with most services are made on their own and set on the floor before the ceiling goes on.

![](templates/templates_module_14_wet_room_all.webp)

\include{lineno} templates_module_14_wet_room.cpp

<!-- assembly:begin -->
## Assembly Sequence {#module_assembly_sequence}

Panels, cassettes and rooms as they leave the shop: every frame member by member, the sheets on it, then the panels into a module and the modules side by side and storey on storey. Every move is a `ModuleSequence` move: the module taken apart piece by piece, each piece swept as a solid against every piece still in, the moving ones drawn brown, a GIF per kind of module and one of the whole.

| assembly | | |
|---|---|---|
| **Framed wall: rails, studs, headers and sills, a sheet on each face**, 18 elements, 17 moves | | |
| ![wall, 18 pieces](assembly/assembly_templates_module_1_wall.gif) wall, 18 pieces |  |  |
| **Cassette shell: bottom plate, edge beams, top plate**, 107 elements, 106 moves | | |
| ![cassette, 7 pieces](assembly/assembly_templates_module_2_cassette_shell_cassette.gif) cassette, 7 pieces | ![cassette shell, 15 pieces](assembly/assembly_templates_module_2_cassette_shell.gif) cassette shell, 15 pieces |  |
| **Cassette house**, 109 elements, 108 moves | | |
| ![cassette, 23 pieces](assembly/assembly_templates_module_3_cassette_house_cassette.gif) cassette, 23 pieces | ![cassette house, 6 pieces](assembly/assembly_templates_module_3_cassette_house.gif) cassette house, 6 pieces |  |
| **Room module: floor, walls, posts, ceiling**, 182 elements, 181 moves | | |
| ![module floor, 20 pieces](assembly/assembly_templates_module_4_room_module_module_floor.gif) module floor, 20 pieces | ![module wall front, 19 pieces](assembly/assembly_templates_module_4_room_module_module_wall_front.gif) module wall front, 19 pieces | ![module wall start, 16 pieces](assembly/assembly_templates_module_4_room_module_module_wall_start.gif) module wall start, 16 pieces |
| ![module ceiling, 20 pieces](assembly/assembly_templates_module_4_room_module_module_ceiling.gif) module ceiling, 20 pieces | ![module, 10 pieces](assembly/assembly_templates_module_4_room_module_module.gif) module, 10 pieces | ![room module, 2 pieces](assembly/assembly_templates_module_4_room_module.gif) room module, 2 pieces |
| **Module block**, 1038 elements, 1037 moves | | |
| ![module, 10 pieces](assembly/assembly_templates_module_5_module_block_module.gif) module, 10 pieces | ![module block, 12 pieces](assembly/assembly_templates_module_5_module_block.gif) module block, 12 pieces |  |
| **Bank pavilion**, 421 elements, 420 moves | | |
| ![module, 10 pieces](assembly/assembly_templates_module_6_bank_pavilion_module.gif) module, 10 pieces | ![bank pavilion, 4 pieces](assembly/assembly_templates_module_6_bank_pavilion.gif) bank pavilion, 4 pieces |  |
| **Module types: framed, solid, mixed**, 279 elements, 195 moves | | |
| ![framed floor, 15 pieces](assembly/assembly_templates_module_7_module_types_framed_framed_floor.gif) framed floor, 15 pieces | ![framed wall front, 25 pieces](assembly/assembly_templates_module_7_module_types_framed_framed_wall_front.gif) framed wall front, 25 pieces | ![framed ceiling, 15 pieces](assembly/assembly_templates_module_7_module_types_framed_framed_ceiling.gif) framed ceiling, 15 pieces |
| ![framed, 6 pieces](assembly/assembly_templates_module_7_module_types_framed.gif) framed, 6 pieces | ![mixed, 6 pieces](assembly/assembly_templates_module_7_module_types_mixed.gif) mixed, 6 pieces | ![solid, 18 pieces](assembly/assembly_templates_module_7_module_types_solid.gif) solid, 18 pieces |
| **Open modules: frame, braced**, 106 elements, 104 moves | | |
| ![braced frame, 3 pieces](assembly/assembly_templates_module_8_open_module_braced_braced_frame.gif) braced frame, 3 pieces | ![braced, 7 pieces](assembly/assembly_templates_module_8_open_module_braced.gif) braced, 7 pieces | ![frame floor, 19 pieces](assembly/assembly_templates_module_8_open_module_frame_frame_floor.gif) frame floor, 19 pieces |
| ![frame frame, 4 pieces](assembly/assembly_templates_module_8_open_module_frame_frame_frame.gif) frame frame, 4 pieces | ![frame ceiling, 19 pieces](assembly/assembly_templates_module_8_open_module_frame_frame_ceiling.gif) frame ceiling, 19 pieces | ![frame, 6 pieces](assembly/assembly_templates_module_8_open_module_frame.gif) frame, 6 pieces |
| **Stacked modules: bearings, modules, couplers**, 899 elements, 763 moves | | |
| ![module, 6 pieces](assembly/assembly_templates_module_9_stacked_modules_module.gif) module, 6 pieces | ![stacked modules, 56 pieces](assembly/assembly_templates_module_9_stacked_modules.gif) stacked modules, 56 pieces |  |
| **Rack: the skeleton, the modules slid in**, 410 elements, 376 moves | | |
| ![skeleton, 62 pieces](assembly/assembly_templates_module_10_module_rack_skeleton.gif) skeleton, 62 pieces | ![module, 6 pieces](assembly/assembly_templates_module_10_module_rack_module.gif) module, 6 pieces | ![module rack, 19 pieces](assembly/assembly_templates_module_10_module_rack.gif) module rack, 19 pieces |
| **Frame: a deck per storey, the modules on it**, 621 elements, 518 moves | | |
| ![skeleton, 75 pieces](assembly/assembly_templates_module_11_module_frame_skeleton.gif) skeleton, 75 pieces | ![module, 6 pieces](assembly/assembly_templates_module_11_module_frame_module.gif) module, 6 pieces | ![module frame, 25 pieces](assembly/assembly_templates_module_11_module_frame.gif) module frame, 25 pieces |
| **Levels: a deck every second storey**, 1604 elements, 1479 moves | | |
| ![skeleton, 52 pieces](assembly/assembly_templates_module_12_module_levels_skeleton.gif) skeleton, 52 pieces | ![module, 6 pieces](assembly/assembly_templates_module_12_module_levels_module.gif) module, 6 pieces | ![module levels, 53 pieces](assembly/assembly_templates_module_12_module_levels.gif) module levels, 53 pieces |
| **Corridor between two rows**, 1628 elements, 1415 moves | | |
| ![module, 6 pieces](assembly/assembly_templates_module_13_module_corridor_module.gif) module, 6 pieces | ![module corridor, 128 pieces](assembly/assembly_templates_module_13_module_corridor.gif) module corridor, 128 pieces |  |
| **Wet room set in before the ceiling**, 200 elements, 193 moves | | |
| ![module pod, 6 pieces](assembly/assembly_templates_module_14_wet_room_module_pod.gif) module pod, 6 pieces | ![module, 7 pieces](assembly/assembly_templates_module_14_wet_room_module.gif) module, 7 pieces | ![wet room, 4 pieces](assembly/assembly_templates_module_14_wet_room.gif) wet room, 4 pieces |

<!-- assembly:end -->
