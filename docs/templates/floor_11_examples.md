# Floor 11: BReps and the examples {#templates_floor_11_examples}

[TOC]

<em>Step 11 of @ref templates_floor_model · previous: @ref templates_floor_10_pins</em>

A finished floor is written with exact solids.
`pb_dump` (through `pb_dumps`) runs `compute_breps` first, so every cut member and every round part is a BRep.
The examples under `examples/` show the guide, its contacts, one column and the whole floor.
[templates_floor_1_floorguide.cpp](https://github.com/petrasvestartas/wood/blob/main/examples/templates_floor_1_floorguide.cpp) shows quarter 0 of the guide alone.
[templates_floor_3_contacts.cpp](https://github.com/petrasvestartas/wood/blob/main/examples/templates_floor_3_contacts.cpp) shows the 68 contacts of page 8.
`tests/floor_elements.cpp` checks what each step must hold, from the contact counts to the pin directions.

## 351. compute_breps

![](floor/351_compute_breps.webp)

<span style="color:#2196EA">■ built</span> `inner_beams_0_0` as a BRep

`WoodSession::compute_breps` is called first by `pb_dump` (through `pb_dumps`).
It writes every cut member, connector part, pin and support as its BRep instead of its mesh, the bores exact cylinders.
Here the seam beam shows its pin holes.

Code: `WoodSession::compute_breps`, `WoodSession::pb_dumps`, [wood_session.cpp](https://github.com/petrasvestartas/wood/blob/main/src/joinery_solver/wood_session.cpp).

## 352. Example: one column

![](floor/352_example_column.webp)

<span style="color:#2196EA">■ built</span> `column_0` and `support_0`

[templates_floor_2_column_model.cpp](https://github.com/petrasvestartas/wood/blob/main/examples/templates_floor_2_column_model.cpp) builds `Floor(guide)`.
It grafts `floor.get_branch("column_0")` into a new `WoodSession("column_0")`.
That is one column on its support with its glued and carved head, read back from the floor.

```cpp
const wood_floor::Floor floor(guide);
WoodSession column("column_0");
column.graft(floor.get_branch("column_0"), nullptr);
column.pb_dump(pb_path("live"));
```

Code: [templates_floor_2_column_model.cpp](https://github.com/petrasvestartas/wood/blob/main/examples/templates_floor_2_column_model.cpp).

## 353. Example: the square floor

![](floor/357_example_square.webp)

<span style="color:#2196EA">■ built</span> the connector parts   <span style="color:#A3A3A3">■ context</span> the members, cut

[templates_floor_7_contacts_cantilevers.cpp](https://github.com/petrasvestartas/wood/blob/main/examples/templates_floor_7_contacts_cantilevers.cpp): `Floor(guide)` on the 6000 x 6000 bay, its pins among the connectors, written as BReps.

Code: [templates_floor_7_contacts_cantilevers.cpp](https://github.com/petrasvestartas/wood/blob/main/examples/templates_floor_7_contacts_cantilevers.cpp).

## 354. Example: a rectangular bay

![](floor/358_example_rectangle.webp)

<span style="color:#2196EA">■ built</span> the connector parts   <span style="color:#A3A3A3">■ context</span> the members, cut

[templates_floor_8_rectangle.cpp](https://github.com/petrasvestartas/wood/blob/main/examples/templates_floor_8_rectangle.cpp): the same on a 6000 x 4800 bay, where each quarter has its own shape.

Code: [templates_floor_8_rectangle.cpp](https://github.com/petrasvestartas/wood/blob/main/examples/templates_floor_8_rectangle.cpp).
