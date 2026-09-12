---
name: wood-kernel
description: Architecture map of the wood timber-joinery kernel - the 9-step detection pipeline, joint type table, joint library conventions, data flow and file navigator. Use for "how does wood detect/construct joints", add-joint, migrate, debug-pipeline and pipeline questions.
argument-hint: [add-joint | migrate <feature> | debug <symptom> | pipeline | <question>]
---

# Wood Kernel Architecture Guide

**Usage:** `/wood-kernel [add-joint | migrate <feature> | debug <symptom> | pipeline | <question>]`

---

## Instructions for Claude

First read `memory/wood_kernel.md`. It documents the legacy CGAL tree under `cmake/src/wood/include/`; the current port is `src/joinery_solver/` with the same file names. Cite the current file unless the question is about the migration.

Then dispatch on the argument:

---

### Bare invocation (`/wood-kernel`)

Produce a formatted architecture map with these 5 sections:

**1. Entry Point**
- `cmake/main.cpp` — arg parsing, test/shape selector
- Key global flags: `OUTPUT_GEOMETRY_TYPE`, `DATA_SET_INPUT_FOLDER`

**2. Detection Pipeline (9 steps)**
- get_elements → rtree_search → adjacency_search → pair_search → face_to_face/plane_to_face/border_to_face → three_valence_joint_addition_vidy → construct_joint_by_index → get_joints_geometry → write_xml
- Types assigned: 12=SS_InPlane, 11=SS_OutOfPlane, 13=SS_Rotate, 20=TS, 30=Cross, 40=TT, 60=Boundary

**3. Joint Type Table**
Show the 7-group table (detected type | group | slot range | category | example functions).

**4. Data Flow**
XML → polyline_pairs → elements → joint detection → unit-box geometry → orient → XML/protobuf output

**5. File Navigator**
Key files (current tree; the legacy copies sit under `cmake/src/wood/include/`):
- Detection: `src/joinery_solver/wood_main.cpp`
- Joint library: `src/joinery_solver/wood_joint_lib.h`
- Data types: `src/joinery_solver/wood_element.h`, `wood_joint.h`, `wood_cut.h`
- Shapes: `src/shapes/shapes.cpp`
- session_cpp: `ext/session_cpp/src/`

---

### `add-joint`

Show the 7-step protocol with a minimal code template:

**7-Step Protocol:**
1. Pick category (detect type 11/12/13/20/30/40/60) + choose unused slot index N
2. Declare: `void <category>_N(joint& j)` in `wood_joint_lib.h`
3. Implement: fill `j.m[0]`, `j.m[1]`, `j.f[0]`, `j.f[1]` in unit-box space; fill `j.m_boolean_type`, `j.f_boolean_type`; set `j.name = __func__`; last polyline = boundary rectangle
4. If joint scales with thickness: `j.unit_scale = true`
5. Register: `joint_names[N] = "<category>_N"` in `construct_joint_by_index()`
6. Add dispatch: `case N: <category>_N(j); break;`
7. Add XML dataset + test in `wood_test.cpp`

**Minimal code template:**
```cpp
// wood_joint_lib.h
void ss_e_ip_N(wood::joint& j);

// wood_joint_lib.cpp
void ss_e_ip_N(wood::joint& j) {
    j.name = __func__;
    j.unit_scale = true;

    // Unit-box space: x=[-0.5,0.5], y=edge, z=thickness
    // Fill m[0] (male top), m[1] (male bottom)
    // Fill f[0] (female top), f[1] (female bottom)
    // Last polyline in each = boundary rectangle

    j.m_boolean_type = { cut_type::mill, cut_type::edge_insertion };
    j.f_boolean_type = { cut_type::mill, cut_type::edge_insertion };
}
```

---

### `migrate <feature>`

Steps:
1. Grep `cmake/src/wood/include/` for CGAL uses in the named feature: `IK::`, `CGAL::`, `CGAL_Polyline`, `cgal_*_util`
2. Group results by: type predicates | geometric algorithms | data structures
3. Find session_cpp equivalent in `cmake/ext/session_cpp/src/`
4. Produce before/after pattern showing the replacement
5. Flag: precision differences, linking implications (proto_objects), typedef conflicts

---

### `debug <symptom>`

Match symptom against this table and cite the fix with file:function:

| Symptom | Root cause | Fix |
|---|---|---|
| mesh normals inconsistent | `unify_winding` before `weld` | Call `weld(1.0)` first; BFS needs shared edges |
| hexshell mesh broken | weld tolerance too small | Use `weld(1.0)` — vertex gaps are ~0.93 units |
| wrong dihedral at edges | single face normal used | `compute_face_edge_planes`: sum+normalise adjacent normals |
| face offsets wrong shape | vertex projection used | Use `outline_from_planes(...)` plane intersection |
| joint not found at runtime | slot 0 used | Slots start at 1; slot 0 triggers continue |
| joint geometry wrong size | unit_scale not set | Set `j.unit_scale = true` |
| build succeeds but cmake exits 1 | absl/protoc post-build | Normal; confirm by `wood.exe` line in output |
| output truncated on crash | stdout buffering | Use `std::cerr` for debug |
| ambiguous Mesh type | typedef in stdafx.h | Qualify as `session_cpp::Mesh` |
| female/male swapped | JOINTS_TYPES sign | Negative = female; positive = male |

For any symptom not in the table, search `memory/wood_kernel.md` Section 10 and answer with a file:line citation.

---

### `pipeline`

Produce the annotated 9-step trace. For each step show:
- Step name
- File: `cmake/src/wood/include/wood_main.cpp`
- Function signature
- Input → Output
- Key algorithms used
- Joint types assigned (where applicable)

---

### Any other text

Answer using `memory/wood_kernel.md` as primary source. Always cite `file:line` or `Section N` from the reference. Never guess — if the answer is not in memory, say so and suggest which source file to read.
