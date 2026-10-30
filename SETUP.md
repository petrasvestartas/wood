# Setup

## Toolchain Requirements

| Tool | Minimum | Notes |
| --- | --- | --- |
| C++ Compiler | C++23 | GCC 13+, Clang 17+, or MSVC 19.38+ (Visual Studio 2022) |
| CMake | 3.22+ | `FetchContent` pulls `session_cpp` (`session_core`) and `manifold` (`v3.5.4`) |
| Git | 2.30+ | Used by CMake `FetchContent` |
| Doxygen | 1.15.0 (optional) | Required only for `cmake --build build --target docs` |

## Build & Test

### Linux / macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target run_tests --parallel
```

### Windows (MSVC)

```powershell
cmake -S . -B build
cmake --build build --config Release --target run_tests --parallel
```

## Build Documentation

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target docs
# Output: build/docs/html/index.html
```

## Scene Output (`data/output/pb/`)

Executables write `.pb` scenes via `pb_path("<name>")`:
- Default path: `<repo>/data/output/pb/<name>.pb`
