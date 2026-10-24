# Timber modules: framed wall panels, then hollow hex cassettes (plan, 2026-10-23)

Two new template families, each a `WoodSession` built from a guide, following the floor pattern
(`src/templates/floor`): members, then contacts, then connectors, every joint an element added with
`add` and `add_interaction`.

The templates use framed beam-and-panel modules and hollow cassettes with two plates closed by edge beams.

## 1. `src/templates/module/` — the wall module (start here)

`ModuleGuide`: a wall outline (length, height, thickness) and its openings (door and window
rectangles with sill and head heights). `WallModule : WoodSession` makes:

1. **studs**: `Beam` posts at a stud spacing (default 625 mm, half a 1250 sheet), extra studs at
   each side of every opening;
2. **plates**: bottom and top `Beam` rails along the whole length;
3. **openings**: a header over and a sill under each opening, trimmer studs carrying the header;
4. **sheathing**: `Plate` panels on one or both faces, split at the sheet width, cut out at the
   openings;
5. **contacts**: `compute_face_contact` stud to rail, stud to header, panel to every member it lies on;
6. **connectors**: butt joints stud to rail (`JointBeam` cut), `JointBeam::headed_pins` panel to
   frame along each stud, a lap or the plate joint library between neighbouring panels.

Examples: `templates_module_1_wall` (a plain wall), `templates_module_2_openings` (a door and two
windows), then a corner and a T between two modules. Docs page `docs/templates/module.md`, one
section and picture per constructor block.

## 2. `src/templates/cassette/` — the segmented hex shell

`CassetteShell : WoodSession` from a surface or a hex-dominant mesh:

1. **cells**: a hexagonal pattern on the surface (planarised, reuse the `VdaMesh` face/edge/bisector
   planes: the edge plane between two cells is already the shared cut);
2. **plates**: per cell a thin top and bottom `Plate` (e.g. 20 + 20 mm), offset along the face normal;
3. **edge beams**: per cell edge a `Beam` between the two plates, its outer face on the shared edge
   plane (the inclined cut), its ends mitered on the corner bisectors;
4. **contacts**: plate to beam inside a cassette, beam to beam across every interior edge;
5. **connectors**: plate to beam screwed (`headed_pins`), beam to beam across cells by finger joints
   from the plate library or `centred_pins`.

Examples: one cassette, then the shell. Same docs pattern.

## Decisions (2026-10-23)

- The wall module came first; the cassette shell and a gable house of cassettes followed, in `docs/templates/module.md`.
- Sheathing is one panel on each face, the door a notch and the windows holes; every frame member has one section and the studs
  between two posts are spaced evenly, so no two parallel members stand side by side.
- No connectors yet: the modules detect every contact, between members, panels and cassettes; bolts between cassettes come later.
- Cassettes meet in T joints by rank, one running through square and the other cut on its face, so the fewest cuts are inclined;
  only the hex shell keeps its miters on the bisector planes.
