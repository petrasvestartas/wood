# wood — Agent Instructions

## House style

Apply `../.claude/agents/session-reviewer.md` to all handwritten wood code. The user's wood scope
overrides that reviewer's default exclusion of wood. Kernel parity and kernel CI
steps apply only when changing the kernels; wood remains a C++ consumer.

Use explicit types, one concept per file, normal multiline function bodies, short
functions and the standard section banners. Keep one header/source pair for Joint, JointPlate and JointBeam. Plate factories,
parameters, Annen and Vidy stay in the JointPlate pair; use sections and small
functions instead of files per factory or family. Expose library designs by their
actual names, such as `JointPlate::ts_e_p_3`, with their own parameters.
Preserve the user-owned TODO checklist in `examples/1_elements.cpp` and mark its
completion accurately. Generated protobuf files follow the generator's format.

## Kernel first

- Before writing a geometry or scene helper in wood, search the kernel headers
  (`../session/session_cpp/src/*.h`: Point, Vector, Line, Plane, Polyline, Mesh, Xform,
  Intersection, Closest, ConvexHull, BooleanPolyline, Session). If it exists, call it
  directly; never re-implement its math and never wrap it only to rename it.
- If the operation is missing and is not timber-specific, propose it for the kernel
  (cpp, py and rust together) instead of writing it in wood.
- A helper two files need lives once in the module's internal header, never as two
  `static` copies.

