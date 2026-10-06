# Elements {#elements}

The timber elements under `src/joinery_solver/wood_elements/`, one page each: what it is, its constructors, a picture and the example that draws it. Every example is a CMake target under `examples/elements/` that writes `data/output/pb/live.pb` for the viewer.

An element is built from its own parameters only. Whatever another element does to it comes through `WoodSession::add_interaction(source, target, interaction)`: a contact, a plate joint's outline feature, or an `InteractionFeatureSolid` that adds a solid to its stock or takes one away.

| Element | Header | Example |
| --- | --- | --- |
| [Plate](@ref elements_plate) | `wood_element_plate.h` | `element_plate` |
| [Beam](@ref elements_beam) | `wood_element_beam.h` | `element_beam` |
| [BeamVariable](@ref elements_beam_variable) | `wood_element_beam_variable.h` | `element_beam_variable` |
| [Column](@ref elements_column) | `wood_element_column.h` | `element_column` |
| [Block](@ref elements_block) | `wood_element_block.h` | `element_block` |
| [Support](@ref elements_support) | `wood_element_support.h` | `element_support` |

- @subpage elements_plate
- @subpage elements_beam
- @subpage elements_beam_variable
- @subpage elements_column
- @subpage elements_block
- @subpage elements_support
