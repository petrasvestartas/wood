# wood — timber joint pipeline

Two plates of wood touch. This figures out **where** they touch, **which teeth** fit that spot, and **carves** them into both plates.

---

## Build

```bash
cmake -B build
cmake --build build --config Release --target main_translation_shell
```

First build downloads protobuf + abseil (needs internet, takes a few minutes). Subsequent builds are fast.

---

## Run an example

```bash
.\build\Release\main_translation_shell.exe
```

Output writes `translation_shell.json` to the working directory.

---

## All targets

| Target | Source |
|---|---|
| `main_hello` | `examples/main_hello.cpp` |
| `main_annen_corner` | `examples/main_annen_corner.cpp` |
| `main_translation_shell` | `examples/templates/main_translation_shell.cpp` |
| `main_fold_reflex` | `src/templates/main_fold_reflex.cpp` |
| `main_fold_translation` | `src/templates/main_fold_translation.cpp` |
| `main_chevron_test` | `src/templates/main_chevron_test.cpp` |
| `main_json_session` | `examples/main_json_session.cpp` |

```bash
cmake --build build --config Release --target <target>
```

---

## Key types

| Type | File | What it is |
|---|---|---|
| `WoodElement` | `src/joinery_solver/wood_element.h` | Plate: bottom + top outline pair, side faces, planes, thickness. Owns a `session_cpp::Element` |
| `BlockElement` | `src/joinery_solver/wood_element.h` | Any closed loops, one plane each - contact detection only. Owns a `session_cpp::Element` |
| `WoodJoint` | `src/joinery_solver/wood_element.h` | One connection: type, area, lines, volumes, cut outlines. Owns two `session_cpp::ElementFeature` (one per element) |
| `FaceContact` | `src/joinery_solver/wood_face_to_face.h` | One touching face pair and its overlap polygon |
| `TranslationShell` | `src/templates/translation_shell.cpp` | Swept quad mesh + per-face plates |

The wood types are composed over the kernel rather than derived from it: `to_element()` /
`from_element()` move between a `WoodElement` and the `session_cpp::Element` a Session stores
(`element_type = "WoodElement"`), and `WoodJoint::to_features()` is the joint as each host
element carries it. `examples/main_element_mapping_check.cpp` checks the mapping both ways.

## Contact detection

`wood_face_to_face.h`: `adjacency_search` (oriented box per element, BVH, SAT) → candidate
pairs; `faces_coplanar` → touching back-to-back faces; `face_overlap_area` → the overlap polygon,
computed by Clipper2 on int64 coordinates (`CLIPPER_SCALE`, 1e-6 mm). `face_contacts` runs the
whole thing for any element type. `examples/main_face_to_face.cpp` checks it on plates, on
loose loops, and on rotated block grids with a known number of contacts, and exits non-zero
if any check fails.

## Joint type codes

| Code | Meaning |
|---|---|
| 11 | side–side, teeth out |
| 12 | side–side, teeth in |
| 20 | top–side |
| 30 | cross (scissors) |
| 40 | top–top |

---

## Serialization

Every wood type serializes through the kernel: `jsondump` / `jsonload`, `file_json_dump(s)` /
`file_json_load(s)`, and for elements `pb_dumps` / `pb_loads`, `pb_dump` / `pb_load`.

```cpp
WoodElement plate(bottom, top);
std::string bytes = plate.pb_dumps();                 // a session_proto.Element, element_type "WoodElement"
WoodElement back  = WoodElement::pb_loads(bytes);     // outlines, planes, thickness, joint types restored

session.add_element(plate.to_element());              // into a Session, with the joints as features
session.pb_dump("session.pb");

WoodJoint joint = joints[0];
joint.file_json_dump("joint.json");                   // solver fields + its two ElementFeatures
```
