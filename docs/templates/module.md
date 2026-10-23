# Module {#templates_module}

[TOC]

Timber modules from beams and panels, after the kit-of-parts buildings of AUAR, the hollow cassettes of the BUGA Wood Pavilion 2019 and the room modules of Blumer Lehmann: a module is a name in the tree, a group of `Beam` and `Plate` elements and the contacts between them, nothing more. Every member of a frame has one section, the studs stand on a grid of the stud spacing, and every opening is framed by its own jamb studs, so a member is doubled only where an opening is off the grid. Each template is a `WoodSession` whose constructor is its recipe, one group per step; every picture below is groups of the example's scene, rendered by `tools/render_template_pictures.sh`.

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

The rails, the bottom one stopping at the door; per opening a full-height jamb stud either side, the header between them and under a window a sill; then the studs on the grid. A stud stands at every multiple of the stud spacing from the wall's start, so the studs line up with the sheet edges whatever the openings are; an opening brings its own jamb studs, a grid stud that falls on a jamb is that jamb, and one beside it stays, so a member is doubled only where the opening is off the grid. A grid stud inside an opening is a cripple over its header and under its sill. Every member is a `Beam` of `profile_rectangle(stud_depth, stud_width)`; the red faces are the contacts found between them.

![frame](templates/templates_module_1_wall_frame.webp)

```cpp
// studs: on the grid of the stud spacing from the wall's start, where no edge or jamb stud stands, so a stud doubles a jamb only
// where the opening is off the grid; inside an opening cripples over its header and under its sill
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

`add_module_contacts` (`module/module_contact.h`) searches the face contact between every two frame members and between every sheet and frame member whose boxes touch. The member whose axis runs square to the contact ends on it and is the target, the one it stands on the source; two members side by side, a jamb and a grid stud next to it, are named `<target>_beside_<source>`. Each contact is named `<target>_on_<source>`, stored with `add_interaction` and its polygon drawn as a red mesh in `contacts_frame` or `contacts_sheathing`: 28 in the frame, 34 between the two panels and the 17 frame members. No connector is made.

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

A segmented shell of hollow cassettes after the BUGA Wood Pavilion 2019, one per face of a mesh of planar hexagons and pentagons (`module/cassette_shell.h`, example `templates_module_2_cassette_shell`). Instead of one thick panel each cell is two thin plates with edge beams between them.

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

Each cassette is one panel on either side with its windows cut out, and between them the edge beams on its boundary, the jamb studs, header and sill of every window, and the studs, rafters on the roof. The studs stand on the 625 mm grid from each face's first corner, the same logic as the wall: a grid stud that falls on an edge beam running up the face or on a jamb is that member, one beside it stays, and one inside a window is split over its header and under its sill. Their ends are cut on the inner faces of the beams over and under them, so a stud over the door or under a gable slope is a cripple. Every member is 60 wide.

The cassettes meet in T joints, not miters, so the fewest cuts are inclined: `house_ranks` puts the roof slopes over everything, the front slope over the back one at the ridge, and the long walls past the gable ends. The cassette of lower rank runs through and ends flush with the other's outer face; the other is cut on the through cassette's inner face. Wall and gable meet square, and so do gable and roof, whose plane holds the gable's normal; at the eaves the roof ends in a plumb cut flush with the wall's outer face, the wall top is cut to the roof's underside, and at the ridge the front slope ends flush with the top of the back slope: the only cuts at an angle. Inside a cassette the edge beams butt the same way, the more level one running through. Only the hex shell keeps its miters.

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

Timber room modules after the modules of Blumer Lehmann, a room of two and a block of them (`module/room_module.h`, examples `templates_module_4_room_module` and `templates_module_5_module_block`).

![room of two modules](templates/templates_module_4_room_module_all.webp)

What a module is, from Blumer Lehmann's pages, reference sheets and project photos:

| | Blumer Lehmann | the default here |
| --- | --- | --- |
| structure | timber frame (Holzrahmenbau), up to 90 % prefabricated on a production line | framed floor, ceiling and walls |
| frame | the photo of a ZM10 module craned onto Friesenberg school: a thick floor and ceiling box, a post at each end of an open long side, walls only on the closed sides | four corner posts, a wall thick, between the floor and the ceiling; a framed wall between the posts on every closed side |
| rooms | a ZM10 classroom of 68 m2 spans more than one module, a post at the joint inside; in Luxembourg two modules form one room of twice the area | `modules_per_room` modules side by side form one room, the long sides between them open; rooms keep a wall each between them |
| facade | ZM10 pavilions: every module end a bay of about 3 m with a window, three storeys, a corridor through the other ends with cased doors | the end at x 0 the facade with a window, the end at x length the corridor with a door |
| plan | 3 x 9 m outside (Chäserrugg modules; the Berlin school DFK 1.0: 28 modules, 42 x 18 x 3 m) | 9000 x 3000 |
| height | 2.7 m clear inside (Permapack office module) | 2700 clear, 3200 outside |
| stacking | side by side freely, stacked up to three storeys | `columns` side by side, `storeys` stacked |
| road | a Swiss standing permit carries 3 m wide, 4 m high, 30 m long, 44 t | 3000 wide, 3200 high |

```cpp
wood_module::RoomModuleGuide guide;  // two modules of 9000 x 3000 forming one room, 2700 clear
wood_module::RoomModule room(guide);
```

| Parameter | Default | Controls |
| --- | --- | --- |
| `length`, `width`, `clear_height` | 9000, 3000, 2700 | a module's plan and the clear height inside |
| `stud_width` | 60 | every frame member's size in the plane of its element |
| `stud_depth`, `joist_depth` | 160, 200 | the wall frame and the floor and ceiling frame depth |
| `stud_spacing` | 625 | studs and joists from each element's start |
| `sheet_thickness`, `deck_thickness` | 15, 25 | a sheet on each face of a wall, a deck on each face of a floor and a ceiling; a post is as wide as the wall |
| `openings` | a window at the facade end, a door at the corridor end | per wall in its own frame |
| `columns`, `modules_per_room`, `storeys` | 2, 2, 1 | modules side by side, how many form one room, modules stacked |

### Module

A frame like the one in the photo: the floor and the ceiling over the whole module, a post at each corner, a wall between the posts on the closed sides, and the long side towards the other module of its room open, the floor and ceiling spanning between the end posts:

![open-sided module](templates/templates_module_4_room_module_single.webp)

```cpp
// long sides: a wall between the posts, or open towards a neighbour of the same room
const bool front_open = column % guide.modules_per_room != 0;
const bool back_open = (column + 1) % guide.modules_per_room != 0 && column + 1 < guide.columns;
```

### Elements

Every element is a `FramedPanel`, the wall's framing in a plane: rails along its foot and top, an edge stud at both ends, studs on the grid, a jamb stud either side of every opening with the header and sill between them, and a sheet on each face. The floor and the ceiling lie flat over the whole module, their rails the rim joists and their studs the joists, 625 apart across it:

![floor](templates/templates_module_4_room_module_floor.webp)

The facade end wall between the posts, its window framed by jamb studs:

![facade end wall](templates/templates_module_4_room_module_wall.webp)

Seen through, at opacity 0.95:

![room seen through](templates/templates_module_4_room_module_inside.webp)

### Contacts

`add_module_contacts`, shared with the wall: every two members and sheets whose boxes touch get a face contact, the posts and walls on the floor deck and under the ceiling deck, the sheets on the members under them; between modules the floors, ceilings and posts of a room along their open sides, the walls of two rooms back to back, and a ceiling deck under the next floor. The room of two modules has 312 contacts in its frames and 398 with its sheets:

![contacts](templates/templates_module_4_room_module_contacts.webp)

### Block

Four modules side by side and three storeys like the ZM10 school pavilions, two rooms of two modules per storey; each module end a bay of the facade with its window:

![module block](templates/templates_module_5_module_block_all.webp)

```cpp
wood_module::RoomModuleGuide guide;
guide.columns = 4;
guide.storeys = 3;
wood_module::RoomModule block(guide, "module_block");
```

Sources: [Blumer Lehmann, Holzmodulbau](https://www.blumer-lehmann.com/bauprojekte-realisieren/bauen/holzmodulbau.html), with the photo of a ZM10 module placed at Friesenberg; [ZM10 Zurich](https://www.blumer-lehmann.com/referenzen/referenzholz-temporaerer-schulraum-zm10-zuerich.html); [Permapack office module](https://blumer-lehmann.com/dam/Holzbau/Downloads/Dokumente/verkaufsdoku-occasionsmodul-permapack-geisser-blumer-lehmann.pdf); [DBZ 2019, from mountain café to office](https://www.blumer-lehmann.com/en/dam/jcr:e6a4a58b-e1a0-4898-87d2-8c7f182b7488/2019-12-dbz-modulbau-modul-recycling-vom-bergcafe-zum-buero.pdf); [Ernst & Sohn 2019, timber modules for schools](https://www.blumer-lehmann.com/en/dam/jcr:8ad14105-9d05-4ec2-acd1-a55a9b9ca526/2019-09-ernst-und-sohn-special-kindertagesstaetten-mehr-spielraum-mit-holzmodulbauten-in-kinderbetreuung-und-schule.pdf); [Kanton St. Gallen, special transport permits](https://www.sg.ch/verkehr/strassenverkehr/sonderbewilligungen/ausnahmefahrzeuge-und--transporte.html).

<details><summary>Example code: templates_module_4_room_module.cpp</summary>

\include{lineno} templates_module_4_room_module.cpp

</details>

<details><summary>Example code: templates_module_5_module_block.cpp</summary>

\include{lineno} templates_module_5_module_block.cpp

</details>
