# Column {#elements_column}

[TOC]

A column swept from a closed cross-section along a straight axis and modified by incoming `InteractionFeatureSolid` operations (glued head blocks, support seats, and rib pockets).

## Constructors

```cpp
Column(const Line& axis, const Polyline& section, const std::string& name = "column");
static std::shared_ptr<Column> square(const Line& axis, const Plane& corner, double side, const std::string& name = "column");
std::vector<std::shared_ptr<Block>> head_blocks(double head_side, double head_height) const;
```

## Parameters

| Parameter | Default | Role |
| --- | --- | --- |
| `axis` | — | Base-to-head centreline defining length and inclination |
| `section` | — | Closed cross-section swept along `axis` |
| `corner`, `side` (`square`) | — | Corner plane and side length for a square column |
| `rotation` (profile constructor) | `0` | Rotation angle in degrees around `axis` |
| `head_side`, `head_height` (`head_blocks`) | — | Glued head blocks widening the top of the column to `head_side` over `head_height` |

## A rectangle along an axis

A 200 × 300 mm rectangular section swept along a 3500 mm vertical axis.

![](elements/element_column.png)

```cpp
const Polyline section = Polyline::rectangle({-100.0, -150.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 200.0, 300.0);
scene.add(std::make_shared<Column>(Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 3500.0}), section, "column"));
```

<details><summary>Example code: elements/element_column.cpp</summary>

\include{lineno} elements/element_column.cpp

</details>

## Floor column assembly

A floor corner column assembly combining the 220 mm shaft, two glued head blocks (`SolidOperation::add`), a steel `Support`, and six pocket/bore cutters (`SolidOperation::subtract`).

![](elements/element_column_session.png)

```cpp
const wood_floor::Floor floor(guide);
scene.graft(floor.get_branch("column_0"), nullptr);
```

<details><summary>Example code: elements/element_column_session.cpp</summary>

\include{lineno} elements/element_column_session.cpp

</details>
