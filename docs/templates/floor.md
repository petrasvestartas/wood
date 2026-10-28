# Floor {#templates_floor}

[TOC]

Vaulted timber floor slab supported on four carved-head columns: four radial rib-and-bed quarters interlocked around a central oculus ring (`68` face contacts, wedges, let-in column plates, centred dowels, and headed pins).

![Vaulted Timber Floor System](diagrams/wood_floor_system.svg)

- @subpage templates_floor_guide — `wood_floor::FloorGuide` (`src/templates/floor/floor_guide.h`): Steps 1–6 (bay split, radial fan planes, datum loops, parabolic soffits, bed rails, column cutters)
- @subpage templates_floor_model — `wood_floor::Floor` (`src/templates/floor/floor_h`): Steps 7–11 (timber members, carved columns & supports, `68` face contacts, connectors, pins, exact `BRep` export)

![The finished floor: templates_floor_7_contacts_cantilevers in the viewer](floor/floor_viewer.webp)

## Examples

| Example | Scene Built |
| --- | --- |
| [templates_floor_1_floorguide](https://github.com/petrasvestartas/wood/blob/main/examples/templates_floor_1_floorguide.cpp) | `FloorGuide`: quarter `0` plan, member quads, fan planes, and parabolic soffits (Steps 1–6) |
| [templates_floor_2_column_model](https://github.com/petrasvestartas/wood/blob/main/examples/templates_floor_2_column_model.cpp) | Single corner column on its `Support`, glued `head_blocks`, and 6-cutter carved head (`get_branch("column_0")`) |
| [templates_floor_3_contacts](https://github.com/petrasvestartas/wood/blob/main/examples/templates_floor_3_contacts.cpp) | `68` `InteractionContactFace` polygons (`17` per quarter: `1` `seam_wedge`, `1` `oculus_wedge`, `2` `column_plate`, `6` `block_pins`, `6` butt joints, `1` `pins_ring_corner`) |
| [templates_floor_7_contacts_cantilevers](https://github.com/petrasvestartas/wood/blob/main/examples/templates_floor_7_contacts_cantilevers.cpp) | Complete `6000 × 6000 mm` bay with columns, connectors, pins, and exact cylindrical NURBS `BRep` bores |
| [templates_floor_8_rectangle](https://github.com/petrasvestartas/wood/blob/main/examples/templates_floor_8_rectangle.cpp) | Rectangular `6000 × 4800 mm` bay with exact `BRep` members and connectors |
