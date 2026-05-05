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
| `ElementPlate` | `src/element.h` | Input: bottom + top polyline pair |
| `WoodElement` | `wood_element.h` | Plate with planes, thickness, side faces |
| `WoodJoint` | `wood_element.h` | One connection: type, area, tooth profiles |
| `TranslationShell` | `src/templates/translation_shell.cpp` | Swept quad mesh + per-face plates |

---

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

Wood types use JSON. Session types support JSON and protobuf.

```cpp
TranslationShell ts;
ts.file_json_dump("shell.json");
auto ts2 = TranslationShell::file_json_load("shell.json");

session.pb_dump("session.pb");
```

---

## Dependencies

- CMake ≥ 3.22, MSVC / GCC / Clang with C++23
- `session_cpp` at `C:/pc/3_code/code_rust/session/session_cpp` (hardcoded in `CMakeLists.txt`)
