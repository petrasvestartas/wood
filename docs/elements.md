# Elements {#elements}

[TOC]

The timber elements under `src/joinery_solver/wood_elements/`, one page each: what it is, its constructors, and one picture and example per case. Every example is a CMake target under `examples/elements/` that writes `data/output/pb/live.pb` for the viewer; the pictures are drawn by `tools/render_element_docs.py` at opacity 0.75 with the element features and a layer panel of the scene.

An element is built from its own parameters only. Whatever another element does to it comes through `WoodSession::add_interaction(source, target, interaction)`: a contact, a plate joint's outline feature, an `InteractionFeatureSolid` that adds a solid to its stock or takes one away, or an `InteractionFeaturePlane` that cuts it by a plane.

| Element | Header | Examples |
| --- | --- | --- |
| [Plate](@ref elements_plate) | `wood_element_plate.h` | `element_plate`, `element_plate_holes`, `element_plate_session` |
| [Beam](@ref elements_beam) | `wood_element_beam.h` | `element_beam` |
| [BeamVariable](@ref elements_beam_variable) | `wood_element_beam_variable.h` | `element_beam_variable`, `element_beam_variable_cut` |
| [Column](@ref elements_column) | `wood_element_column.h` | `element_column`, `element_column_session` |
| [Block](@ref elements_block) | `wood_element_block.h` | `element_block` |
| [Support](@ref elements_support) | `wood_element_support.h` | `element_support` |
| [CutPlane](@ref elements_cut_plane) | `wood_element_cut_plane.h` | `element_beam_variable_cut` |

- @subpage elements_plate
- @subpage elements_beam
- @subpage elements_beam_variable
- @subpage elements_column
- @subpage elements_block
- @subpage elements_support
- @subpage elements_cut_plane
