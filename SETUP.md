# Setup

## Toolchain Requirements

| Tool | Minimum | Notes |
| --- | --- | --- |
| C++ Compiler | C++20 | GCC 11+, Clang 14+, or MSVC 19.30+ (Visual Studio 2022) |
| CMake | 3.24+ | `FetchContent` pulls `session_cpp` (`session_core`) and `minitest` |
| Git | 2.30+ | Used by CMake `FetchContent` |
| Doxygen | 1.9+ *(optional)* | Required only for `cmake --build build --target docs` |

## Build & Test

### Linux / macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target run_tests --parallel
./build/run_tests
```

### Windows (MSVC)

```powershell
cmake -S . -B build
cmake --build build --config Release --target run_tests --parallel
.\build\Release\run_tests.exe
```

## Build Documentation

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target docs
# Output: build/docs/html/index.html
```

## Scene Output (`session_data/`)

Executables write `.pb` scenes via `pb_path("<name>")`:
- Default path: `<repo>/session_data/<name>.pb`
- Override directory: `export WOOD_PB_DIR=/path/to/dir`
