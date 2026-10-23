# Module {#templates_module}

[TOC]

Timber modules from beams and panels, after the kit-of-parts buildings of AUAR: a module is a name in the tree, a group of `Beam` and `Plate` elements, their contacts and the pins on them, nothing more. Each template is a `WoodSession` whose constructor is its recipe, one group per step; every picture below is groups of the example's scene.

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

`compute_face_contact` between every two frame members and between every panel and frame member. The member whose axis runs square to the contact ends on it and is the target; the pins come through the other. Each contact is named `<target>_on_<source>` and stored with `add_interaction`.

```cpp
const std::shared_ptr<Beam> source = end_i > end_j ? members[j] : members[i];
const std::shared_ptr<Beam> target = end_i > end_j ? members[i] : members[j];
const std::shared_ptr<InteractionContactFace> face = compute_face_contact(source, target);
face->name = fmt::format("{}_on_{}", target->name, source->name);
add_interaction(source, target, face);
```

### Connectors

`JointBeam::headed_pins` on every contact: two through a rail, header or king stud into the end of the member on it, and a column of pins through a panel into every stud and a row into every rail, one per 150 mm. Each is added and passed to both members.

![connectors](templates/templates_module_1_wall_connectors.webp)

```cpp
for (size_t k = 0; k < connectors.frame.size(); k++) {
    const std::shared_ptr<JointBeam>& pins = connectors.frame[k];
    add(pins, group);
    add_interaction(pins, contacts.frame[k].a, pins->interaction(0));
    add_interaction(pins, contacts.frame[k].b, pins->interaction(1));
}
```

<details><summary>Example code: templates_module_1_wall.cpp</summary>

\include{lineno} templates_module_1_wall.cpp

</details>
