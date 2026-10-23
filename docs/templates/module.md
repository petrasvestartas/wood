# Module {#templates_module}

[TOC]

Timber modules from beams and panels, after the kit-of-parts buildings of AUAR and the hollow cassettes of the BUGA Wood Pavilion 2019: a module is a name in the tree, a group of `Beam` and `Plate` elements and the contacts between them, nothing more. Each template is a `WoodSession` whose constructor is its recipe, one group per step; every picture below is groups of the example's scene.

## Wall

A framed wall with a door and a window, sheathed on both faces (`module/wall.h`, example `templates_module_1_wall`).

![wall](templates/templates_module_1_wall_all.webp)

```cpp
wood_module::WallGuide guide;  // length 4800, height 2700, a door and a window
wood_module::Wall wall(guide);
```

| Parameter | Default | Controls |
| --- | --- | --- |
| `length`, `height` | 4800, 2700 | the wall's size, foot at z 0 |
| `stud_width`, `stud_depth` | 60, 160 | stud and rail section; the depth is the insulation space |
| `stud_spacing` | 625 | stud centres, half a sheet |
| `header_depth` | 200 | the header over every opening |
| `sheet_thickness` | 15 | one sheathing panel on each face |
| `openings` | a door at 600, a window at 2525 | `{x, width, sill, head}`, a sill of 0 a door |

### Frame

The rails, the bottom one stopping at the door, a stud at every spacing no opening takes, and per opening two king studs, a header, a sill under a window and the cripples above and below. Every member is a `Beam` of `profile_rectangle(stud_depth, size)`; the red faces are the contacts found between them.

![frame](templates/templates_module_1_wall_frame.webp)

```cpp
// rails: the bottom rail, split at every door, and the top rail along the whole wall
add_rails();

// studs: a stud at every stud spacing that no opening takes
add_studs();

// openings: per opening its king studs, header, sill and the cripples above and below it
add_openings();
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

`compute_face_contact` between every two frame members and between every panel and frame member. The member whose axis runs square to the contact ends on it and is the target, the one it stands on the source. Each contact is named `<target>_on_<source>`, stored with `add_interaction` and its polygon drawn as a red mesh in `contacts_frame` or `contacts_sheathing`: 28 in the frame, 34 between the two panels and the 17 frame members. No connector is made.

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
| `mesh` | `default_mesh()`, the fifteen-face VDA dome | one cassette per face |
| `depth` | 200 | cassette depth along the face normal, plates included |
| `plate_thickness` | 20 | top and bottom plate |
| `beam_width` | 60 | edge beam width square to its edge plane, and every stud's and frame member's |
| `openings` | none | per face its `{x, width, sill, head}` in the face frame: x along its first edge, y up the face |
| `stud_spacing` | 0, none | studs along every face's first edge between the edge beams |
| `ranks` | none, miters | per face its rank at a T joint, the lower running through square and the other cut on its face |

### Planes

Per face edge one plane through the edge, leaning on the average normal of the two cells and pointing out of the face; both cassettes on an edge cut their sides on it, so cells at an angle meet on a bevel. Per corner the plane halfway between its two edge planes miters the beams, square to the edge where the two edges run in line.

```cpp
Vector out = (end - start).cross(lean);
out.normalize_self();

if ((_face_planes[f].origin() - middle).dot(out) > 0.0)
    out = out * -1.0;

planes[f].push_back(Plane::from_point_normal(middle, out));
```

### Cassette

A top and a bottom plate cut by the edge planes, and per edge a beam between them: its outer face on the edge plane, its inner face `beam_width` in, its ends on the miter planes, each a `Plate` lofted between its loops at the two plate levels, so its outer face is cut at the angle between the two cells.

![cassette](templates/templates_module_2_cassette_shell_cassette_4.webp)

```cpp
const Plane& outer = _edge_planes[f][j];
const Plane inner = outer.translate_by_normal(-beam_width);
const std::vector<Plane> sides = {outer, _miter_planes[f][j], inner, _miter_planes[f][(j + n - 1) % n]};
const Polyline bottom_loop = Polyline::from_planes(sides, bottom);
const Polyline top_loop = Polyline::from_planes(sides, top);
add(std::make_shared<Plate>(bottom_loop, top_loop, fmt::format("edge_beam_{}_{}", f, j)), group);
```

### Contacts

`compute_face_contact` inside every cassette, each plate on each beam and every beam on the next at its miter, and between cassettes the two beams on every interior edge, face to face on the bevel they share; a pair that should touch and does not throws. Two neighbouring cassettes, the contact red on its host beam:

![two cassettes](templates/templates_module_2_cassette_shell_cassette_8.webp)

Every contact between cassettes drawn in `contacts` as a red mesh: on each of the 29 interior edges the two plates and the two beams meet on the bisector, 104 contacts in all:

![contacts](templates/templates_module_2_cassette_shell_contacts.webp)

```cpp
const std::shared_ptr<Plate> beam = get_element_by_name<Plate>(fmt::format("edge_beam_{}_{}", f, j));
const std::shared_ptr<Plate> other = get_element_by_name<Plate>(fmt::format("edge_beam_{}_{}", g, i));
const std::shared_ptr<InteractionContactFace> face = add_contact(beam, other);

if (!face)
    throw std::runtime_error(fmt::format("cassettes {} and {} do not touch across their edge", f, g));
```

No connector is made yet: the contacts are where bolts or joints between cassettes will go.

<details><summary>Example code: templates_module_2_cassette_shell.cpp</summary>

\include{lineno} templates_module_2_cassette_shell.cpp

</details>

## Cassette house

Cassettes joined in space: a 6000 x 4000 gable house, walls 2700 high and the ridge at 4500, one cassette per whole face (example `templates_module_3_cassette_house`). `CassetteShell::house_mesh` gives the six faces, the front, the two gable ends as pentagons, the back and the two roof slopes, and `CassetteShell::house_openings` the door and windows in each face's frame.

![cassette house](templates/templates_module_3_cassette_house_all.webp)

```cpp
wood_module::CassetteShell house(
    wood_module::CassetteShell::house_mesh(),
    160.0,
    20.0,
    60.0,
    wood_module::CassetteShell::house_openings(),
    625.0,
    wood_module::CassetteShell::house_ranks(),
    "cassette_house"
);
```

Each cassette is a big panel on either side with its openings cut out, and between them the edge beams on its boundary, studs every 625 mm, rafters on the roof, and around every opening two jambs, a header and under a window a sill; a door stands on the bottom beam.

The cassettes meet in T joints, not miters, so the fewest cuts are inclined: `house_ranks` puts the roof slopes over everything, the front slope over the back one at the ridge, and the long walls past the gable ends. The cassette of lower rank runs through with a square end, out past the other's far side; the other is cut on its face. Wall and gable meet square, and so do gable and roof, whose plane holds the gable's normal; only the wall tops under the slope and the back slope at the ridge are cut at an angle. Inside a cassette the edge beams butt the same way, the more level one running through. Only the hex shell keeps its miters.

```cpp
// T, cut: on the side of the neighbour's cassette that faces this one
if (!through) {
    const double side = (face.origin() - other.origin()).dot(other.z_axis()) > 0.0 ? 1.0 : -1.0;
    const Point surface = other.origin() + other.z_axis() * (side * 0.5 * depth);
    planes[f].push_back(Plane::from_point_normal(surface, other.z_axis() * -side));
    continue;
}
```

Seen through, at opacity 0.95:

![cassette house seen through](templates/templates_module_3_cassette_house_inside.webp)

One gable end, square on every side: its sides stop on the long walls, its top under the roof; its studs cut on the inner faces of the sloped beams, the window framed:

![gable end](templates/templates_module_3_cassette_house_gable.webp)

```cpp
// runs: the whole height; around an opening it stands in, above the header and below the sill; none beside the jambs
std::vector<std::array<Plane, 2>> runs = {{inner_planes[low_side], inner_planes[high_side]}};
```

The roof over the front wall and the left gable end, both running under it with the roof overhanging them square:

![gable and roof](templates/templates_module_3_cassette_house_gable_roof.webp)

Every contact between cassettes as a red mesh, a cut cassette's plates and beams on the face of the one running through, 43 in all:

![contacts](templates/templates_module_3_cassette_house_contacts.webp)

<details><summary>Example code: templates_module_3_cassette_house.cpp</summary>

\include{lineno} templates_module_3_cassette_house.cpp

</details>
