# Column {#elements_column}

A column: a section swept along its axis, trimmed by its cut planes. Anything else comes through interactions: a block glued on adds to its stock, a cutter, a joint or a connector takes a solid away, each with `add_interaction(source, column, InteractionFeatureSolid(mesh, operation))`. The column draws its stock less every subtract.

```cpp
Column(const Line& axis, const Polyline& section, const std::string& name = "column")
static std::shared_ptr<Column> square(const Line& axis, const Plane& corner, double side, const std::string& name = "column")
static wood_floor::ColumnSession wood_floor::ColumnSession::glued_head(const Line& axis, const Plane& corner, double side, double head_side, double head_height, const std::string& name = "column")
```

![Column](elements/element_column.png)

A 200 x 300 rectangle swept along a 3500 axis; the lines are its features, the axis and the section.

\include{lineno} elements/element_column.cpp

## The floor's column as a session

A column with glued blocks and cuts is several elements and the features they put on it, so `ColumnSession::glued_head` and the floor's `ColumnSession(guide, q)` return a session: the 220 shaft, two blocks glued on for the 340 head (`SolidOperation::add`, hidden once glued), the support with its joint's seat and screws, and six hidden cutter plates taking away the faces the ribs and the column blocks bear on (`SolidOperation::subtract`).

![Column session](elements/element_column_session.png)

\include{lineno} elements/element_column_session.cpp
