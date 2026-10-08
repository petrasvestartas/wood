# Floor 11: BReps and the examples {#templates_floor_11_examples}

[TOC]

<em>Step 11 of @ref templates_floor_model · previous: @ref templates_floor_10_screws</em>

A finished floor can be written with exact solids: `compute_breps` turns every cut member and every round part into a BRep. The examples under `examples/` build the floor a part at a time, each a few calls on a `Floor`; `tests/floor_elements.cpp` checks what each step must hold, from the contact counts to the screw levels and the bays too narrow for the corner screws.

## 351. compute_breps

![](floor/351_compute_breps.webp)

<span style="color:#2196EA">■ built</span> `inner_beams_0_0` as a BRep

`WoodSession::compute_breps` writes every cut member, connector part, dowel and support as its BRep instead of its mesh, the bores exact cylinders; here the seam beam with its wedge pocket and its dowel and screw holes.

Code: `WoodSession::compute_breps`, [wood_session.cpp:1487-1496](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/src/joinery_solver/wood_session.cpp#L1487-L1496).

## 352. Example: one column

![](floor/352_example_column.webp)

<span style="color:#2196EA">■ built</span> `column_0` and `support_0`

[templates_floor_2_column_model.cpp](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_2_column_model.cpp): `Floor(guide)` and `add_column(0)`, one column on its support with its glued and carved head.

Code: [templates_floor_2_column_model.cpp:8-21](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_2_column_model.cpp#L8-L21).

## 353. Example: the four columns

![](floor/353_example_columns.webp)

<span style="color:#2196EA">■ built</span> the four columns and their supports

[templates_floor_3_columns_model.cpp](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_3_columns_model.cpp): `add_columns`, a column at every corner of the bay.

Code: [templates_floor_3_columns_model.cpp:8-21](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_3_columns_model.cpp#L8-L21).

## 354. Example: the quarters

![](floor/354_example_quarters.webp)

<span style="color:#E8478B">■ outer_ribs</span>   <span style="color:#F2CC0C">■ inner_ribs</span>   <span style="color:#7C7C7C">■ inner_beams</span>   <span style="color:#A8A8A8">■ wedges</span>   <span style="color:#F5D890">■ tsections</span>   <span style="color:#A6D3F6">■ beds</span>

[templates_floor_4_quarters.cpp](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_4_quarters.cpp): `add_quarters`, the members of the four quarters.

Code: [templates_floor_4_quarters.cpp:8-21](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_4_quarters.cpp#L8-L21).

## 355. Example: the oculus

![](floor/355_example_oculus.webp)

<span style="color:#2196EA">■ built</span> the ring beams, the bottom wedges and the central plate

[templates_floor_5_oculus.cpp](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_5_oculus.cpp): `add_oculus` alone.

Code: [templates_floor_5_oculus.cpp:8-21](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_5_oculus.cpp#L8-L21).

## 356. Example: the wedges

![](floor/356_example_wedges.webp)

<span style="color:#2196EA">■ built</span> the wedge parts   <span style="color:#A3A3A3">■ context</span> the members, cut

[templates_floor_6_contacts_floor.cpp](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_6_contacts_floor.cpp): `add_quarters`, `add_oculus`, `add_contacts` and `add_connectors` of the seam and oculus wedges only.

Code: [templates_floor_6_contacts_floor.cpp:8-24](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_6_contacts_floor.cpp#L8-L24).

## 357. Example: the square floor

![](floor/357_example_square.webp)

<span style="color:#2196EA">■ built</span> the connector parts   <span style="color:#A3A3A3">■ context</span> the members, cut

[templates_floor_7_contacts_cantilevers.cpp](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_7_contacts_cantilevers.cpp): `add_members`, `add_connectors` and `add_screws` on the 6000 x 6000 bay, then `compute_breps`.

Code: [templates_floor_7_contacts_cantilevers.cpp:10-29](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_7_contacts_cantilevers.cpp#L10-L29).

## 358. Example: a rectangular bay

![](floor/358_example_rectangle.webp)

<span style="color:#2196EA">■ built</span> the connector parts   <span style="color:#A3A3A3">■ context</span> the members, cut

[templates_floor_8_rectangle.cpp](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_8_rectangle.cpp): the same on a 6000 x 4800 bay, where each quarter has its own shape.

Code: [templates_floor_8_rectangle.cpp:12-31](https://github.com/petrasvestartas/wood/blob/5f7f317df711a163eda3c416cb426e5cd8663dd6/examples/templates_floor_8_rectangle.cpp#L12-L31).
