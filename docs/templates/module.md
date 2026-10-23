# Module {#templates_module}

[TOC]

Timber modules from beams and panels, after the kit-of-parts buildings of AUAR and the hollow cassettes of the BUGA Wood Pavilion 2019: a module is a name in the tree, a group of `Beam` and `Plate` elements and the contacts between them, nothing more. Every member of a frame has one section, no two parallel members stand side by side, and the studs between two posts are spaced evenly. Each template is a `WoodSession` whose constructor is its recipe, one group per step; every picture below is groups of the example's scene, rendered by `tools/render_template_pictures.sh`.

## Wall

A framed wall with a door and a window, one panel on each face (`module/wall.h`, example `templates_module_1_wall`).

![wall](templates/templates_module_1_wall_all.webp)

```cpp
wood_module::WallGuide guide;  // length 4800, height 2700, a door and a window
wood_module::Wall wall(guide);
```

| Parameter | Default | Controls |
| --- | --- | --- |
| `length`, `height` | 4800, 2700 | the wall's size, foot at z 0 |
| `stud_width` | 60 | every member's size in the plane of the wall: studs, rails, king studs, headers and sills |
| `stud_depth` | 160 | frame depth across the wall, the insulation space |
| `stud_spacing` | 625 | the largest distance between stud centres; the studs between two posts are spaced evenly |
| `sheet_thickness` | 15 | one sheathing panel on each face |
| `openings` | a door at 600, a window at 2525 | `{x, width, sill, head}`, a sill of 0 a door |

### Frame

The rails, the bottom one stopping at the door; per opening two king studs, a header and under a window a sill; then the studs. The posts are the two end studs and every king stud; between every two of them as few studs as keep the stud spacing stand evenly apart, and one that falls inside an opening is a cripple over its header and under its sill. Every member is a `Beam` of `profile_rectangle(stud_depth, stud_width)`; the red faces are the contacts found between them.

![frame](templates/templates_module_1_wall_frame.webp)

```cpp
// between every two posts as few studs as keep the stud spacing, evenly apart; inside an opening cripples over its header and under its sill
for (size_t p = 0; p + 1 < posts.size(); p++) {
    const double span = posts[p + 1] - posts[p];
    const int between = static_cast<int>(std::ceil(span / guide.stud_spacing - 1e-9)) - 1;

    for (int k = 1; k <= between; k++) {
        const double x = posts[p] + span * k / (between + 1);
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

`compute_face_contact` between every two frame members and between every panel and frame member. The member whose axis runs square to the contact ends on it and is the target, the one it stands on the source. Each contact is named `<target>_on_<source>`, stored with `add_interaction` and its polygon drawn as a red mesh in `contacts_frame` or `contacts_sheathing`: 32 in the frame, 38 between the two panels and the 19 frame members. No connector is made.

![contacts](templates/templates_module_1_wall_contacts.webp)

```cpp
const std::shared_ptr<Beam> source = end_i > end_j ? members[j] : members[i];
const std::shared_ptr<Beam> target = end_i > end_j ? members[i] : members[j];
const std::shared_ptr<InteractionContactFace> face = compute_face_contact(source, target);
face->name = fmt::format("{}_on_{}", target->name, source->name);
add_interaction(source, target, face);
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
| `stud_spacing` | 0, none | the largest distance between stud centres; the studs between two posts are spaced evenly |
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

Each cassette is one panel on either side with its windows cut out, and between them the edge beams on its boundary, the jambs, header and sill of every window, and the studs, rafters on the roof. The posts are the edge beams that run up the face, the door's among them, and the jambs; between every two of them as few studs as keep 625 mm stand evenly apart, their ends cut on the inner faces of the beams over and under them, so a stud over the door or under a gable slope is a cripple, and one inside a window is split over its header and under its sill. Every member is 60 wide.

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

The front alone: the door a notch from the ground and the window a hole through both panels; in the whole house the openings show the inside of the walls behind them. Its frame, the door with a cripple over it, the window with cripples over and under it and the studs evenly between the posts, is in the picture seen through above.

![front](templates/templates_module_3_cassette_house_front.webp)

```cpp
// between every two posts as few studs as keep the stud spacing, evenly apart
for (size_t p = 0; p + 1 < posts.size(); p++) {
    const double first = posts[p][1] - 0.5 * w;
    const double last = posts[p + 1][0] + 0.5 * w;
    const double span = last - first;
    const int between = static_cast<int>(std::ceil(span / stud_spacing - 1e-9)) - 1;
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
