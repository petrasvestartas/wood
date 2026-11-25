# tools

Every docs picture is made by one recipe line: the element, joint and truss pictures drawn by the viewer's native
renderer the way the joint library draws its joints (see-through, as dark as solid, one isometric camera), the other
template pictures screenshots of the hosted viewer, cropped and nothing else. `python3 tools/check_element_pages.py` (run by `wood-docs.yml`) proves that every picture a
page shows exists, that every file under `docs/images` is shown by a page, and that a recipe makes it.

## Which command makes which folder

| Folder under `docs/` | Command |
| --- | --- |
| `images/elements/` | `python3 tools/render_element_pictures.py [example ...]` (names are the example targets, every picture there when none; drawn like the joint library) |
| `images/templates/` (shells, folding, chevron, cross, reciprocal, module, grid, vault) | `bash tools/render_template_pictures.sh [family]` |
| `images/templates/truss_*.png`, `templates_truss_1_pratt.png` | `bash tools/render_template_pictures.sh truss` (each library truss and the joinery's nodes close up, drawn by `render_element_pictures.py --scenes` / `--close` like the joint library: see-through, as dark as solid; native renderer) |
| `images/templates/truss_bow_*.webp` | `bash tools/render_template_pictures.sh bow` |
| `images/templates/*_assembly.gif` | `python tools/render_assembly_gif.py <example>` (the one tool that does not use the browser: it needs the viewer crate's native renderer for a fixed camera) |
| `images/assembly/` | `python3 tools/render_template_assembly.py <example> ...` (every template's assembly, run with `WOOD_ASSEMBLY` for its `ModuleSequence`: a GIF per kind of module and one of the whole; native renderer), then `python3 tools/assembly_docs.py` for the pages' sections |
| `images/joint_library/assembly_*.png`, `.gif` | `python3 tools/render_family_gifs.py [example ...]` (every example of the plate and beam joint pages, run with `WOOD_ASSEMBLY` for its assembly order, drawn apart and taken in one by one like the library's joints) |
| `images/joint_library/beam_*.png` | `python3 tools/render_joint_library.py [design ...]` (every record of data/joints/*.yaml on a square and an inclined corner; native renderer) |
| `images/templates/templates_truss_5_carpentry_assembly.gif` | `python3 tools/render_joint_assembly_gif.py templates_truss_5_carpentry_assembly` (an AssemblySequence's order and directions, apart, in one by one, out in reverse; native renderer) |
| `images/examples/ex_*.webp`, `images/diagrams/wood_architecture`, `wood_joint_unit_box`, `wood_boolean_cutting`, `wood_beam_joints_connectors`, `images/landing/landing_architecture` | `python tools/render_architecture_pictures.py [examples] [pipeline] [landing] [wiring] [cutting] [sheets]` |
| `images/diagrams/wood_elements_sheet_1_elements.webp` | `python tools/render_element_sheets.py` (after the element pictures) |
| `images/joint_library/` | `bash tools/joint_library_figures/make.sh` (the other native-renderer pipeline: labelled tiles, trimmed and scaled; not yet on the viewer tool) |
| `doxygen/icons.svg` (the icons, the Rhino plugin's and those drawn after them) | `python3 tools/make_icons.py` |
| `doxygen/hero.mp4`, `hero_2x.mp4`, `hero_small.mp4`, `hero.webp` (the landing page's film and its poster) | `uv run --with av --with pillow --with numpy --with manifold3d tools/render_hero.py` (`templates_floor_7_contacts_cantilevers` built; native renderer; the moves are `hero_moves.py`, `--check` proves that nothing runs into anything) |
| `templates/floor/` | the `wood-film-docs` skill |

Not yet made by a recipe (listed in `UNRECIPED` in `check_element_pages.py`): `two_plates.png`,
`templates_shells_2_custom_curves.png`, `templates_truss_4_custom_wireframe.png`, `landing_elements|templates|examples.webp`,
`hilti_reference_photo.jpg` (a photograph).

## The tools behind the recipes

- `screenshot_viewer.mjs`: one screenshot of the viewer for one scene (`SHOT_PB`, commands as arguments); `SHOT_PORT` keeps it off another Chrome.
- `crop_viewer_shot.py`: crops a shot to what it draws, to an aspect with `SHOT_ASPECT`.
- `render_template_docs.py`: runs an example, takes groups of its scene and shoots them (`--view`, `--opacity`, `--wheel`, `--name`, `--scene`, `--aspect`).
- `render_scene_picture.py`: one written scene file as one picture.
- `render_element_pictures.py`: draws a scene the way the joint library draws its joints (see-through, as dark as solid, one
  isometric camera, 960 x 640): an example's scene, scene files (`--scenes`), or one point close up (`--close`).
- `assembly_docs.py`: writes the Assembly Sequence section of every template page from the GIFs above and what the runs reported.
- `truss_docs.py`: writes the truss galleries of `docs/templates/truss.md` from the truss data and the pictures above.

## Everything else

- `regen_proto.sh`: regenerates `generated/` with the pinned protoc (36.2); CI does the same and fails on a difference. `rust/src/proto/wood_proto.rs` is regenerated by `cargo build` in `rust/`.
- `run_guarded.sh` / `run_guarded.ps1`: run a command with a time limit and a memory cap.
- `reference_2025.py`: dumps the 2025 CGAL solver's answers into `tests/golden/reference_2025`.
- `xml_to_dataset.py`: turns an XML dataset into a wood dataset.
