---
name: wood-film-docs
description: Explain a complex 3D modelling algorithm in the wood docs as a step-by-step "film" - one rendered picture per step, colour-coded by role, labelled, with a one-sentence page per step linked to the exact code. Use for "document <algorithm> step by step", "explain this 3D script graphically", "make pictures for the docs", "like the floor film", or when redoing or fixing those pictures and pages.
argument-hint: [<topic> | redo <topic> | fix <frame numbers>]
---

# Film documentation for 3D modelling code

The floor template (`docs/templates/floor*.md`, pictures in `docs/templates/floor/`) is the reference:
11 chapter pages, ~230 pictures, every step of the algorithm in the order the code runs.
`templates/` beside this file holds the generator it was made with. Copy it, never rewrite it.

## Process

1. **Read the algorithm** end to end. List every step in the order the code runs: one step = one
   picture = one `## N. title` section. Group steps into chapters of 10-30.
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
   `python3 docs/<topic>/render.py` (all frames and the films) or `render.py 023 087` (some).
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

**Viewer**
- Arctic view (`VIEWER_AO=1`, `VIEWER_OUTLINES=1`), `VIEWER_OPACITY=1`, no grid
  (`VIEWER_NO_GRID=1`). Below opacity 1 the viewer blends every face and the scene turns
  see-through; never blend a "bare" render either.
- Every named view (top, front, right, iso) is orthographic. Hidden lines there needed a viewer
  fix (session 6f9e4e4e, `sag_terms`); if lines show through solids again, suspect the viewer's
  hidden-line slack before the drawing.

**Colour = role**, the same in picture, colour key and text:

| Role | Colour |
|---|---|
| what the step builds | blue `#2196EA` (Block Research Group primary) |
| the variable or value it introduces | pink `#E8478B` |
| a second result | yellow `#F2CC0C` |
| what it reads from earlier steps; dashed = construction helper | grey `#737373` |
| context | light grey `#DADADA`, solid |
| dimensions, mesh edges | black `#1A1A1A` |

Never orange, amber, slate, dark blue or green. Member families get their own set
(`FAMILY_COLORS`: pink, yellow, two neutral greys, pale yellow, pale blue). Colours go to the
viewer as linear light: `movie.cpp`'s `shown()` decodes sRGB first.

**Lines**
- One pen for every line: `PEN` = 2 px. Colour, not width, tells roles apart.
- Never two lines on one another: `Frame` cuts a lower-ranked line (context < input < role
  colours) where a higher one covers it on screen. Never lay a coloured line exactly on another
  colour's line or face (blue on yellow reads green): leave the shared side out.
- Dashes: one pattern everywhere, sized by the pen (dash 5 pens, gap 2 pens + 4 px), stretched so
  a line starts and ends on a full dash. The viewer has only round caps.
- No hatching. An area between lines is `frame.fill` (pale tint, no edge).
- Dimensions: `frame.dimension`, black, extension lines, 45-degree ticks, value as its label.

**Solids**
- Members are solids (`frame.solid`, `frame.element`), never wire loops; loops drawn on a solid
  only double its edges. Every solid gets black edges; context solids light grey, highlighted
  ones in their role or family colour, all opaque.
- A plane is the viewer's plane object: square, x and y axes, headed normal, unfilled (a fill
  gets an Arctic outline). A plane seen edge-on in a plan or elevation draws only its normal.

**Framing and labels**
- Plans from above at the working level, elevations along x or y, 3D steps from a fixed corner;
  one box per frame (`VIEWER_BOUNDS`), the same box for steps that belong together.
- Labels: black plates with white text in Roboto, a leader to a ring on the point, placed by
  render.py's cost-based labelling (no overlap, no two labels on one spot), drawn 4x and scaled
  down so they are smooth. Every name a plate shows is a name in the code.
- Pictures render at twice the layout size (`SCALE = 2`, 2560 x 1648, `VIEWER_THICKNESS=2`):
  label placement runs in the 1x layout, everything is drawn at 2x.
- The caption over the picture states the step in code terms, e.g.
  `faces_into(q) = pair(plane_into(q), thickness): ...`.

## Writing style

- One plain sentence per step; two or three only where a reader would otherwise miss a refusal,
  a skew rule or a sign. No tables unless the numbers are the point (at most 4 rows), no code
  blocks (the `Code:` line links the source), at most one mermaid diagram per page.
- Each step: `## N. title`, the picture, its colour-key line
  (`<span style="color:#2196EA">■ built</span> ...`), the sentence, the `Code:` line.
- `Code:` lines link to GitHub at a pinned commit, file and line range: the reader must reach the
  exact lines from every step.
- Mermaid diagrams top to bottom (`flowchart TD`, `direction TB`): drawn left to right they are
  scaled down to the page width until the text is unreadable. The footer sets Roboto 18 px.
- Doxygen: hex colours and anything with `<...>` in backticks, no bare `@word` or `\word`,
  `@subpage` lines without a colon in the title text, the `■` character not `&#9632;`.
- The overview page: what the algorithm makes, the data structures (one mermaid class diagram),
  a colour table, the member and name table, the examples, then the chapter list.

## Fonts

Roboto (text) and Roboto Mono (code), as the compas_wood docs (`theme.font` in its mkdocs.yml).
`docs/doxygen/header.html` loads them from Google Fonts and overrides doxygen-awesome's font
variables; the Mermaid init in `footer.html` uses Roboto; render.py reads
`~/.local/share/fonts/Roboto-Regular.ttf` (Roboto v3.010 from googlefonts/roboto-3-classic).
