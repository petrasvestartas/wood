---
name: wood-film-docs
description: Explain a class that encodes a 3D modelling algorithm in the wood docs step by step - one page that follows its constructor block by block, each block an expandable section with one line of text, its code and a static rendered picture. Use for "document <algorithm> step by step", "explain this 3D script graphically", "make pictures for the docs", "like the floor film", or when redoing or fixing those pictures and pages.
argument-hint: [<topic> | redo <topic> | fix <frame numbers>]
---

# Step-by-step documentation for 3D modelling code

`docs/templates/floor_guide.md` (pictures in `docs/templates/floor/`) is the reference: one page that
follows `FloorGuide`'s constructor block by block (wood `CLAUDE.md`, "Class constructors"). The full
picture rules are `wood-agent` Protocol 5. `templates/` beside this file holds the generator. Copy it,
never rewrite it.

## Process

1. **Read the constructor** end to end: one constructor block = one expandable section, in its
   order; each `compute_*` step that builds something distinct = one nested section with its own
   picture.
2. **Set up the generator, local only.** It is documentation tooling, never committed:
   - copy `templates/movie.h`, `movie.cpp`, `main.cpp`, `render.py` to `docs/<topic>/`; keep the
     `Look`, `Frame` and `write` parts, replace the floor helpers, `Context` and chapter list;
   - `git apply templates/cmake_local_block.patch` in wood (adjust the glob to `docs/<topic>`);
   - `git -C ../session/session_viewer apply templates/viewer_render_options.patch`, then build the
     renderer: `cargo build -j8 --release --target x86_64-unknown-linux-gnu --example selftest`.
3. **Write one function per frame** (`templates/chapter_example.cpp` shows the idioms): a `Frame`
   with number, slug, caption, view and box; draw only what the step reads (input/context) and
   what it builds (role colours); label every named thing; `frame.write(context.dir)`.
4. **Generate and render:** build and run `docs_<topic>_movie`, then
   `python3 docs/<topic>/render.py` (all frames) or `render.py 023 087` (some). Static images only.
   Run both under `timeout 10m systemd-run --user --scope -q -p MemoryMax=8G`.
5. **QA every frame**, not a sample: tile 9 per contact sheet and look at all of them. Machine
   checks: `label problems: 0` from render.py; no green pixels (`g > r+20 and g > b+15`, see
   below); no edge visible through a solid.
6. **Write the pages** (rules below), pin the `Code:` links with `templates/permalinks.py`, and
   after code changes carry them over with `templates/remap.py <wood sha> <session_cpp sha>`
   (it prints every citation whose lines changed: reread and fix those by hand).
7. **Build the docs** (`cmake --build build --target docs`) and look at `build/docs/html`, which is
   what the user reviews. Zero Doxygen warnings from the pages.
8. **Commit** only `docs/templates/<topic>*.md` and `docs/templates/<topic>/` (and code fixes);
   never `docs/<topic>/`, the CMake block or the viewer patch. Push, wait for CI green.
9. **Remove the generator** (`rm -rf docs/<topic>`, revert the CMake block and the viewer files);
   keep a tarball in the session scratchpad while more rounds may come.

## Picture style (each rule came from a rejected picture)

- Static images, never films or composites; every text readable at full size; no caption band (the
  text lives in the page).
- White background; members grey solids with black edges; lines black: colour carries no meaning,
  except a plane's axes (x pink, y yellow-green, normal blue) and the thick pink dot of a computed
  point. No dashed lines.
- Labels: black text on white plates, solid black leader dot, never anchored on a line's midpoint;
  every name is a name in the code.
- A plane is a grey shaded square without grid, with its axes, both planes of a pair; never a line.
- 3D quarter views: iso, orbit `-60,70`, a plan of the same scene in the top-right (40 %, white only
  inside its outline), the 3D view shifted left. Close-ups get their own box; slopes read in profile.
- Plans in one sequence keep one scale.

## Writing style

- The page follows the constructor: the parameters (one code snippet of the constructor's arguments
  with their defaults, and as few pictures as show them all), an expandable Tables block with the
  class definitions as code, then one `<details>` section per constructor block in order.
- Each section: one line of text, the constructor block's code, its picture; nested `<details>`
  with the `compute_*` code of each sub-step and its picture. Snippets are cut from the source, so
  they match it exactly; a result is named and fixed-size on its own line before it is stored.
- Text extremely short: the code and the picture explain, the sentence only names the step.
- Doxygen: hex colours and anything with `<...>` in backticks, no bare `@word` or `\word`.

## Fonts

Roboto (text) and Roboto Mono (code), as the compas_wood docs (`theme.font` in its mkdocs.yml).
`docs/doxygen/header.html` loads them from Google Fonts and overrides doxygen-awesome's font
variables; the Mermaid init in `footer.html` uses Roboto; render.py reads
`~/.local/share/fonts/Roboto-Regular.ttf` (Roboto v3.010 from googlefonts/roboto-3-classic).
