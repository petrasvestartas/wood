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
| `sheet_width`, `sheet_thickness` | 1250, 15 | sheathing panels on both faces |
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

A `Plate` on each face, split at the sheet width and at every opening edge, a sheet break near an opening edge dropped so no sliver is left; over an opening a panel above the head and one below the sill.

![sheathing](templates/templates_module_1_wall_sheathing.webp)

```cpp
for (const std::array<double, 2>& span : spans) {
    const Polyline bottom = Polyline::rectangle(Point(strip[0], inner, span[0]), along, up, width, height);
    const Polyline top = Polyline::rectangle(Point(strip[0], outer, span[0]), along, up, width, height);
    add(std::make_shared<Plate>(bottom, top, fmt::format("sheathing_{}_{}", side, count)), group);
}
```

### Contacts

`compute_face_contact` between every two frame members and between every panel and frame member. The member whose axis runs square to the contact ends on it and is the target, the one it stands on the source. Each contact is named `<target>_on_<source>`, stored with `add_interaction` and its polygon drawn as a red mesh in `contacts_frame` or `contacts_sheathing`: 28 in the frame, 52 between the panels and the frame. No connector is made.

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
wood_module::CassetteShell shell;  // or CassetteShell(mesh, depth, plate_thickness, beam_width)
```

| Parameter | Default | Controls |
| --- | --- | --- |
| `mesh` | `default_mesh()`, the fifteen-face VDA dome | one cassette per face |
| `depth` | 200 | cassette depth along the face normal, plates included |
| `plate_thickness` | 20 | top and bottom plate |
| `beam_width` | 60 | edge beam width square to its edge plane |

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

Every contact between cassettes drawn in `contacts` as a red mesh, one per interior edge of the mesh:

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

Many cassettes joined in space: a 6000 x 4000 gable house, walls 2700 high and the ridge at 4500, from `CassetteShell::house_mesh` (example `templates_module_3_cassette_house`). The four walls are split into panels on a grid with a door and a window in the front, a window in the back and one in each gable; each opening is a cell left out of the mesh, so the cassettes around it close it with their edge beams. A triangle stands over each end wall and two roof slopes run to the ridge.

![cassette house](templates/templates_module_3_cassette_house_all.webp)

```cpp
wood_module::CassetteShell house(wood_module::CassetteShell::house_mesh(), 160.0, 20.0, 60.0, "cassette_house");
```

Every cassette is cut on the plane it shares with each neighbour: square where two panels of one wall meet in a plane, beveled at the wall corners, the eaves and the ridge. A gable triangle runs along the panels under it in a straight line; its plates skip those straight corners and its beams meet there square to the edge. Every contact between cassettes as a red mesh, one per shared mesh edge:

![contacts](templates/templates_module_3_cassette_house_contacts.webp)

```cpp
// an edge plane parallel to the one before it continues that side: no corner between them
for (const Plane& plane : _edge_planes[f])
    if (sides.empty() || plane.z_axis().angle(sides.back().z_axis(), false, true) > 0.57)
        sides.push_back(plane);
```

<details><summary>Example code: templates_module_3_cassette_house.cpp</summary>

\include{lineno} templates_module_3_cassette_house.cpp

</details>
