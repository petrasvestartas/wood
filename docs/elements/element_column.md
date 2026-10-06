# Column {#elements_column}

A column: a section swept along its axis, trimmed by its cut planes. Anything else comes through interactions: a block glued on adds to its stock, a cutter, a joint or a connector takes a solid away, each with `add_interaction(source, column, InteractionFeatureSolid(mesh, operation))`. The column draws its stock less every subtract.

```cpp
Column(const Line& axis, const Polyline& section, const std::string& name = "column")
static std::shared_ptr<Column> square(const Line& axis, const Plane& corner, double side, const std::string& name = "column")
static wood_floor::ColumnSession wood_floor::ColumnSession::glued_head(const Line& axis, const Plane& corner, double side, double head_side, double head_height, const std::string& name = "column")
```

![Column](elements/element_column.png)

Left to right: an axis and a 200 x 300 section; `Column::square` with a notch a hidden block takes away (`SolidOperation::subtract`); `ColumnSession::glued_head`, the 220 shaft and two blocks glued on (`SolidOperation::add`) for a 340 head over its top 730; the floor's column, the glued head on its support with six hidden cutter plates taking away the faces the ribs and the column blocks bear on. A column with features is several elements and their interactions, so `glued_head` returns a session, not a column.

\include{lineno} elements/element_column.cpp
