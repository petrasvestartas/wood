# Column {#elements_column}

[TOC]

A column: a section swept along its axis. Anything else comes through interactions: a block glued on adds to its stock, a cutter, a joint or a connector takes a solid away, each with `add_interaction(source, column, InteractionFeatureSolid(mesh, operation))`.

## Constructors

```cpp
Column(const Line& axis, const Polyline& section, const std::string& name = "column")
static std::shared_ptr<Column> square(const Line& axis, const Plane& corner, double side, const std::string& name = "column")
WoodSession wood_floor::glued_head(const Line& axis, const Plane& corner, double side, double head_side, double head_height, const std::string& name = "column")
```

## A rectangle along an axis

![A rectangle along an axis](elements/element_column.png)

A 200 x 300 rectangle swept along a 3500 axis; the axis and the section are its features.

\include{lineno} elements/element_column.cpp

## The floor's column as a session

![The floor's column as a session](elements/element_column_session.png)

A column with glued blocks and cuts is several elements and the features they put on it, so `wood_floor::column(guide, q)` returns them as a WoodSession: the 220 shaft, two blocks glued on for the 340 head (`SolidOperation::add`, hidden once glued), the support with its joint's seat and screws, and six hidden cutter plates taking away the faces the ribs and the column blocks bear on (`SolidOperation::subtract`).

\include{lineno} elements/element_column_session.cpp
