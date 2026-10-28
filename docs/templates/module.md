# Module {#templates_module}

[TOC]

Framed timber panels, hollow cassette shells, and multi-storey room modules (`src/templates/module/*.h`) built from `Beam` and `Plate` elements with automatic `add_module_contacts` face-contact detection.

## Wall

A framed wall with a door and a window, one panel on each face (`module/wall.h`, example `templates_module_1_wall`): one `FramedPanel` standing in the xz plane (`module/framed_panel.h`), the framing every module's walls, floors and ceilings share.

![wall](templates/templates_module_1_wall_all.webp)

```cpp
wood_module::WallGuide guide;  // length 4800, height 2700, a door and a window
wood_module::Wall wall(guide);
```

| Parameter | Default | Controls |
| --- | --- | --- |
| `length`, `height` | 4800, 2700 | the wall's size, foot at z 0 |
| `stud_width` | 60 | every member's size in the plane of the wall: studs, rails, jamb studs, headers and sills |
| `stud_depth` | 160 | frame depth across the wall, the insulation space |
| `stud_spacing` | 625 | stud centres from the wall's start, half a 1250 sheet; 600 for 1200 sheets, 400 for a stiffer wall |
| `sheet_thickness` | 15 | one sheathing panel on each face |
| `openings` | a door at 600, a window at 2525 | `{x, width, sill, head}`, a sill of 0 a door |

### Frame

Bottom/top rails (interrupted at doors), full-height jamb studs + headers/sills around openings, and grid studs every `stud_spacing` (dropping any grid stud within `0.5 * stud_spacing` of a jamb/edge stud and splitting interior studs into cripples).

![frame](templates/templates_module_1_wall_frame.webp)

```cpp
// studs: on the grid of the stud spacing from the panel's start, where no edge or jamb stud stands and none stands closer than half
// the spacing, the bay beside it already narrower; inside an opening cripples over its header and under its sill
for (double x = guide.stud_spacing; x + half < guide.length - w; x += guide.stud_spacing) {
```

### Sheathing

One `Plate` on each face over the whole wall: every door a notch up from the foot of its outline, every window a hole through both faces of the panel.

![sheathing](templates/templates_module_1_wall_sheathing.webp)

```cpp
for (const std::pair<double, size_t>& entry : doors) {
    const Opening& door = guide.openings[entry.second];
    corners.push_back(Point(door.x, levels[k], 0.0));
    corners.push_back(Point(door.x, levels[k], door.head));
    corners.push_back(Point(door.x + door.width, levels[k], door.head));
    corners.push_back(Point(door.x + door.width, levels[k], 0.0));
}
```

Seen through, at opacity 0.95, the frame between the two panels:

![wall seen through](templates/templates_module_1_wall_inside.webp)

### Contacts

`add_module_contacts` (`module/module_contact.h`) detects all frame-to-frame (`26`) and sheet-to-frame (`32`) face contacts (`<target>_on_<source>` or `<target>_beside_<source>`).

![contacts](templates/templates_module_1_wall_contacts.webp)

```cpp
// side by side, as a jamb and a stud next to it: neither ends on the other
const bool beside = std::max(end_i, end_j) < 0.9;
const std::shared_ptr<Beam> source = !beside && end_i > end_j ? beam_j : beam_i;
const std::shared_ptr<Beam> target = !beside && end_i > end_j ? beam_i : beam_j;
const std::shared_ptr<InteractionContactFace> face = session.compute_face_contact(source, target);
```

<details><summary>Example code: templates_module_1_wall.cpp</summary>

\include{lineno} templates_module_1_wall.cpp

</details>

## Cassette shell

A segmented shell of hollow cassettes, one per face of a mesh of planar hexagons and pentagons (`module/cassette_shell.h`, example `templates_module_2_cassette_shell`). Instead of one thick panel each cell is two thin plates with edge beams between them.

![cassette shell](templates/templates_module_2_cassette_shell_all.webp)

```cpp
wood_module::CassetteShell shell;  // or CassetteShell(mesh, depth, plate_thickness, beam_width, openings, stud_spacing, ranks)
```

| Parameter | Default | Controls |
| --- | --- | --- |
| `mesh` | `default_mesh()`, the fifteen-face VDA dome | one cassette per face, a face may be notched |
| `depth` | 200 | cassette depth along the face normal, plates included |
| `plate_thickness` | 20 | top and bottom plate |
| `beam_width` | 60 | every edge beam's, stud's and frame member's width, one section for all |
| `openings` | none | per face its `{x, width, sill, head}` in the face frame: x along its first edge, y up the face |
| `stud_spacing` | 0, none | stud centres from every face's first corner along its first edge; every window is framed by its own jamb studs |
| `ranks` | none, miters | per face its rank at a T joint, the lower running through square and the other cut on its face |

### Planes

Without ranks, per face edge one plane through the edge, leaning on the average normal of the two cells; both cassettes on an edge cut their sides on it, so cells at an angle meet on a bevel. It points out of the face by the winding of its corners, so a notched face works too. Per corner the plane halfway between its two edge planes miters the beams, square to the edge where the two edges run in line.

```cpp
// miter: the plane through the edge leaning on the average normal of the two faces
if (ranks.empty()) {
    Vector lean = (end - start).cross(face.z_axis() + other.z_axis()) * winding;
    lean.normalize_self();
    planes[f].push_back(Plane::from_point_normal(middle, lean));
    continue;
}
```

### Cassette

A top and a bottom plate cut by the edge planes, and per edge a beam between them: its outer face on the edge plane, its inner face `beam_width` in, its ends on the miter planes, each a `Plate` lofted between its loops at the two plate levels, so its outer face is cut at the angle between the two cells.

![cassette](templates/templates_module_2_cassette_shell_cassette_4.webp)

```cpp
const Plane& outer = _edge_planes[f][j];
const Plane inner = outer.translate_by_normal(-beam_width);
const std::vector<Plane> sides = {outer, end_plane, inner, start_plane};
const Polyline bottom_loop = Polyline::from_planes(sides, bottom);
const Polyline top_loop = Polyline::from_planes(sides, top);
```

### Contacts

`compute_face_contact` between every two parts of a cassette that touch, and between every two parts of two neighbouring cassettes; two cassettes that share an edge and do not touch throw. Two neighbouring cassettes, the contact red on its host beam:

![two cassettes](templates/templates_module_2_cassette_shell_two_cassettes.webp)

Every contact between cassettes drawn in `contacts` as a red mesh: on each of the 29 interior edges the two plates and the two beams meet on the bisector, 104 contacts in all:

![contacts](templates/templates_module_2_cassette_shell_contacts.webp)

```cpp
for (const std::shared_ptr<Plate>& a : parts[f])
    for (const std::shared_ptr<Plate>& b : parts[static_cast<size_t>(g)]) {
        const std::shared_ptr<InteractionContactFace> face = add_contact(a, b);

        if (face)
            contacts.between.push_back({a, b, face});
    }

if (contacts.between.size() == found)
    throw std::runtime_error(fmt::format("cassettes {} and {} do not touch across their edge", f, g));
```

No connector is made yet: the contacts are where bolts or joints between cassettes will go.

<details><summary>Example code: templates_module_2_cassette_shell.cpp</summary>

\include{lineno} templates_module_2_cassette_shell.cpp

</details>

## Cassette house

Cassettes joined in space: a 6000 x 4000 gable house, walls 2700 high and the ridge at 4500, one cassette per whole face (example `templates_module_3_cassette_house`). `CassetteShell::house_mesh` gives the six faces: the front with its door a notch from the ground, the two gable ends as pentagons, the back and the two roof slopes; `CassetteShell::house_windows` puts a window in the front, the back and each gable end, in each face's frame.

![cassette house](templates/templates_module_3_cassette_house_all.webp)

```cpp
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
```

Each face cassette combines two window-punched sheathing `Plate`s with `60 mm` perimeter beams, window jambs/headers/sills, and `625 mm` grid studs/rafters. `house_ranks()` resolves inter-cassette seams as T-joints (roof slopes over walls/gables; long walls past gable ends).

```cpp
// T: this cassette is cut on the near side of the neighbour where it stops, or ends flush with its far side where it runs through
const double side = (face.origin() - other.origin()).dot(other.z_axis()) > 0.0 ? 1.0 : -1.0;
const double reach = through ? -side : side;
const Point surface = other.origin() + other.z_axis() * (reach * 0.5 * depth);
planes[f].push_back(Plane::from_point_normal(surface, other.z_axis() * -side));
```

Seen through, at opacity 0.95:

![cassette house seen through](templates/templates_module_3_cassette_house_inside.webp)

The front alone: the door a notch from the ground and the window a hole through both panels; in the whole house the openings show the inside of the walls behind them. Its frame, the door with a cripple over it, the window with its jamb studs and cripples over and under it and the studs on the grid, is in the picture seen through above.

![front](templates/templates_module_3_cassette_house_front.webp)

```cpp
// uprights: the jamb studs either side of every window, then a stud on every multiple of the stud spacing from the face's first
// corner that no post takes, so a stud doubles a jamb only where the window is off the grid; and whether each is a jamb
for (double u = stud_spacing; u < length; u += stud_spacing) {
```

One gable end, square on every side: its sides stop on the long walls, its top under the roof; its studs cut on the inner faces of the sloped beams, the window framed:

![gable end](templates/templates_module_3_cassette_house_gable.webp)

Looking along the ridge, both eaves are clean: each slope ends in a plumb cut flush with its wall's outer face and each wall's top is cut along the slope's underside; at the ridge the front slope runs through, its end flush with the top of the back slope:

![section along the ridge](templates/templates_module_3_cassette_house_section.webp)

An eave and the ridge close up:

![eave](templates/templates_module_3_cassette_house_eave.webp) ![ridge](templates/templates_module_3_cassette_house_ridge.webp)

Every contact between cassettes as a red mesh, a cut cassette's plates and beams on the face of the one running through, 43 in all:

![contacts](templates/templates_module_3_cassette_house_contacts.webp)

<details><summary>Example code: templates_module_3_cassette_house.cpp</summary>

\include{lineno} templates_module_3_cassette_house.cpp

</details>

## Room module

Timber room modules, a room of two, a block of them and a bank pavilion (`module/room_module.h`, examples `templates_module_4_room_module`, `templates_module_5_module_block` and `templates_module_6_bank_pavilion`).

![room of two modules](templates/templates_module_4_room_module_all.webp)

Module geometry and default dimensions:

| Part | Default geometry |
| --- | --- |
| structure | framed floor, ceiling and walls |
| frame | four corner posts, a wall thick, on the floor's long edge beams under the ceiling's, which run the whole length over every post; a framed wall between them on every closed side |
| floor and ceiling | an edge beam a wall wide along each long side over the whole length, joists across between them, the first and last at the module's ends, the same size, a board under and a board over the joists, flush with the edge beams on both faces; the floor 280, 190 x 280 edge beams, sixteen 80 x 240 joists between a 15 board and a 25 deck; the ceiling 170, sixteen 60 x 140 joists between a 15 lining and a 15 top board; joists at most 625 apart |
| ceiling edge beams | both long edge beams of every ceiling 190 x 470, reaching 300 below it, on the posts and the long walls, 2400 high under them |
| end walls | an end wall between floor and ceiling and between the corner posts, 2700 high, on the floor's deck over its end joist |
| rooms | `rooms`: the modules side by side forming each room, the long sides between them open, a post at mid-length on each |
| walls between rooms | one wall, in the module of the smaller room, the lower column on a tie |
| facade | the end at x 0 the facade with a window, the end at x length the corridor with a door |
| plan | 9000 x 3000, side by side without a gap |
| height | 2700 clear, 3150 outside |
| stacking | `rooms` side by side, `storeys` stacked |
| road | 3000 wide, 3150 high |

```cpp
wood_module::RoomModuleGuide guide;  // two modules of 9000 x 3000 forming one room, 2700 clear
wood_module::RoomModule room(guide);
```

| Parameter | Default | Controls |
| --- | --- | --- |
| `length`, `width`, `clear_height` | 9000, 3000, 2700 | a module's plan and the clear height inside |
| `stud_width`, `stud_depth`, `stud_spacing` | 60, 160, 625 | the wall frame: its members, its depth and the stud grid from each wall's start |
| `sheet_thickness` | 15 | a sheet on each face of a wall; a post is as wide as the wall |
| `floor_joist_width`, `floor_joist_depth` | 80, 240 | the floor's joists, the board under and the deck over them: the floor and its edge beams 280 |
| `deck_thickness` | 25 | the floor's deck between its edge beams, flush with their tops |
| `ceiling_joist_width`, `ceiling_joist_depth` | 60, 140 | the ceiling's joists, a board on each face: the ceiling 170 thick, thinner, it carries only itself |
| `board_thickness`, `joist_spacing` | 15, 625 | the board under every floor's and ceiling's joists and the ceiling's top board; the largest joist spacing |
| `downstand_depth` | 300 | how far the ceiling's long edge beams reach below it |
| `side_posts` | 1 | posts along a long side without a wall, evenly between the corner posts |
| `layouts` | a window 2100 x 1900 at the facade end, sill 600, head 2500, a door at the corridor end | per module across, cycled: per wall its openings in its own frame, and a porch, the start wall set back |
| `rooms`, `storeys` | {2}, 1 | the modules side by side forming each room, modules stacked |

### Module

A frame like the ones in the photos: the floor and the ceiling, a post at each corner on the floor's long edge beam under the ceiling's, a wall between floor and ceiling on the closed sides, and the long side towards the other module of its room open, a post at mid-length beside the neighbour's:

![open-sided module](templates/templates_module_4_room_module_single.webp)

```cpp
// long sides: open towards a neighbour of the same room; between two rooms one wall, in the module of the smaller room
const bool front_inside = column > 0 && previous[0] == room[0];
const bool back_inside = column + 1 < columns() && next[0] == room[0];
const bool front_wall = column == 0 || (!front_inside && room[1] < previous[1]);
const bool back_wall = column + 1 == columns() || (!back_inside && room[1] <= next[1]);
```

### Elements

The floor and the ceiling each have an edge beam a wall wide along each long side over the whole length, the main beams: every post stands on the floor's and carries the ceiling's, so the posts, the walls and the edge beams stand in one strip along each side and storey over storey, edge beam on edge beam, the load going straight down. The joists run across between them, the first and the last at the module's ends, all the same size, between a board under and a board over them, flush with the edge beams on both faces. The floor is 280 thick, its edge beams 190 x 280, its sixteen 80 x 240 joists dividing the 9000 into fifteen bays of 595 between a 15 board and a 25 deck:

![floor](templates/templates_module_4_room_module_floor.webp)

The ceiling is thinner, 170, since it carries only itself: sixteen 60 x 140 joists between its 15 lining and its 15 top board. Its two long edge beams are 190 x 470, reaching 300 below it, on the corner posts, the post at mid-length of an open side and the long walls, all 2400 high under them:

![ceiling](templates/templates_module_4_room_module_ceiling.webp)

From the end, the two modules of the room side by side: the floor flush on both faces, the end walls between floor and ceiling, and the ceiling's deep edge beams over the corner posts on all four long sides:

![end view](templates/templates_module_4_room_module_section.webp)

Every wall is a `FramedPanel`, the wall's framing in a plane: rails along its foot and top, an edge stud at both ends, studs on the grid, a jamb stud either side of every opening with the header and sill between them, and a sheet on each face. The facade end wall between floor and ceiling, its window framed by jamb studs:

![facade end wall](templates/templates_module_4_room_module_wall.webp)

Seen through, at opacity 0.95:

![room seen through](templates/templates_module_4_room_module_inside.webp)

### Contacts

`add_module_contacts`, shared with the wall: every two members and sheets whose boxes touch get a face contact, the posts and walls on the floor deck and under the ceiling deck, the sheets on the members under them; between modules the edge beams, posts and walls side by side and a ceiling's top board under the next floor, post on post. The room of two modules has 333 contacts in its frames and 378 with its sheets:

![contacts](templates/templates_module_4_room_module_contacts.webp)

### Block

Four modules side by side and three storeys like the ZM10 school pavilions, two rooms of two modules per storey; each module end a bay of the facade with its window:

![module block](templates/templates_module_5_module_block_all.webp)

```cpp
wood_module::RoomModuleGuide guide;
guide.rooms = {2, 2};
guide.storeys = 3;
wood_module::RoomModule block(guide, "module_block");
```

### Bank pavilion

A temporary bank pavilion of four modules, with a 3.0 m module axis grid and doors 0.70, 0.89 and 1.00 m wide.

![bank pavilion](templates/templates_module_6_bank_pavilion_all.webp)

| | measured on the plan | built |
| --- | --- | --- |
| modules | 4 on a 3.0 m axis grid, a meeting room, an open office of two, a module with the entrance porch | `rooms = {1, 2, 1}`, 3000 wide |
| length | 9850 clear between the end walls, the axes along 9630 | 10270 with its 210 end walls, 9850 clear |
| office | 6000 clear between the walls of modules 1 and 4, open between modules 2 and 3 | 6000 clear |
| walls between rooms | one, 210, inside module 1 and inside module 4 | one, in the module of the smaller room |
| posts | a pair at mid-length of the open joint, 210 each; at the corners as wide as the wall | `side_posts = 1`, a wall thick, on every long side without a wall, also against the walls of modules 1 and 4 |
| windows | 1620 wide, 330 from the inner corner, at both ends of modules 1 to 3; 1020 at the end of module 4; 720 for the WC | the same |
| doors | module 1: a 1590 passage to the office; module 4: the 1.0 m entrance with a sidelight from the porch, two 0.9 m doors | the same |
| porch | the start wall of module 4 set back 2730, open to the outside, a post at its corner | `porch = 2730` |
| not drawn | sills, heads, heights; the exterior walls drawn 336 with insulation and cladding | end windows 600 to 2500, the WC window 1500 to 2100, doors to 2100, 2700 clear; walls 210; partitions inside the modules and cladding not built |

The modules without their ceilings, from above, turned a quarter, and from the porch:

![pavilion plan](templates/templates_module_6_bank_pavilion_plan.webp)

![pavilion open](templates/templates_module_6_bank_pavilion_open.webp)

```cpp
wood_module::RoomModuleGuide guide;
guide.length = 10270.0;
guide.stud_depth = 180.0;
guide.rooms = {1, 2, 1};
guide.layouts = { /* per module: the openings of its walls and its porch, measured on the plan */ };
wood_module::RoomModule pavilion(guide, "bank_pavilion");
```


<details><summary>Example code: templates_module_4_room_module.cpp</summary>

\include{lineno} templates_module_4_room_module.cpp

</details>

<details><summary>Example code: templates_module_5_module_block.cpp</summary>

\include{lineno} templates_module_5_module_block.cpp

</details>

<details><summary>Example code: templates_module_6_bank_pavilion.cpp</summary>

\include{lineno} templates_module_6_bank_pavilion.cpp

</details>
