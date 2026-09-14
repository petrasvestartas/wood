# Development environment — wood

`wood` is the **C++ core** (timber joint solver). It has no `pyproject.toml`, no
`requirements.txt`, and no Python package — building and running it needs only
CMake and a C++23 compiler, **not** a virtualenv.

A venv is set up here anyway, purely for the handful of standalone Python helper
scripts in the repo. If you only build C++, you can ignore it entirely.

## Building the C++ (no venv involved)

```bash
cd wood
cmake -B build
cmake --build build --config Release --target main_translation_shell
./build/Release/main_translation_shell     # writes translation_shell.json to cwd
```

The first configure downloads protobuf + abseil (needs internet, several
minutes). See `README.md` for the full target list.

The kernel is not a submodule of this repo: `CMakeLists.txt` resolves it as
`../session/session_cpp` (or `-DSESSION_CPP_LOCAL=<dir>`), see `README.md`.

## VS Code / IntelliSense

Include resolution comes entirely from the CMake compile database — the repo
sets `CMAKE_EXPORT_COMPILE_COMMANDS ON`, so `cmake -B build` writes
`build/compile_commands.json`. Both editors are already pointed at it:

- `.vscode/c_cpp_properties.json` — sets `compileCommands`.
- `.clangd` — points clangd at `build/`.
- `compile_commands.json` in the repo root is a symlink into `build/`, for
  tools that only look there. It is gitignored; recreate with
  `ln -sfn build/compile_commands.json compile_commands.json`.

**Squiggles on `#include` almost always mean the database is missing or
stale.** It does not exist until the first configure, and a newly added source
file is absent until the next one — so run `cmake -B build`, then
*C/C++: Reset IntelliSense Database* (or restart clangd).

Do not hand-maintain `includePath`. Paths like `"../src/session.h"` resolve
through the `-I.../session_cpp/src` entry as `src/../src/session.h`, never
relative to the including file, and that is easy to get wrong by hand.

## The Python venv (helper scripts only)

There is no `.python-version` here, so the version is chosen explicitly:

```bash
cd wood
uv venv --python 3.13
uv pip install numpy rhino3dm session_py
source .venv/bin/activate          # or prefix commands with `uv run`
```

That covers every third-party import in the repo's scripts:

| Script | Needs |
|---|---|
| `src/templates/temp/pb_to_3dm.py` | `rhino3dm`, `session_py` — converts a `.pb` scene into a Rhino `.3dm` |
| `src/templates/temp/chevron_ref.py` | `numpy` |

Run one with:

```bash
uv run python src/templates/temp/pb_to_3dm.py
```

## Not Python

- `bash/cpp.sh` — shell driver for the C++ build.
- `tools/run_guarded.sh` / `.ps1` — the run guard every solver run goes through (`CLAUDE.md`).

## Relationship to the other two repos

```
wood          C++ core            <- compiled directly by wood_nano
wood_nano     nanobind bindings   <- imports the C++, exposes it to Python
compas_wood   COMPAS wrapper      <- imports wood_nano
```

`wood_nano/CMakeLists.txt` looks for this repo at `../wood` and compiles **this
working copy** when found. So a C++ edit here reaches Python only after
rebuilding `wood_nano` (`uv pip install --no-build-isolation -e .` in that repo).

The kernel is resolved the same way in both repos: `-DSESSION_CPP_LOCAL` / the
`SESSION_CPP_LOCAL` environment variable, then the sibling `../session_cpp`, then
`../session/session_cpp` (the wood_research layout, where the kernel lives inside the
`session` monorepo submodule), then a clone of GitHub `main`. `../README.md` has the
one-command update and the per-repo commands.

## Notes

- `.venv/` self-ignores (uv writes a `.gitignore` containing `*` inside it), so it
  never appears in `git status`. `build/` is gitignored too.
