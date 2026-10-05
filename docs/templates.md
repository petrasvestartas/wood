# Templates {#templates}

Generators under `src/templates/`, one folder per family (`shells/`, `folding/`, `cross/`, `reciprocal/`, `grid/`, `floor/`), that turn a surface, a mesh or a building outline into wood elements. Every template has examples under `examples/`, each a CMake target of the same name that builds it with its defaults and writes `data/output/pb/live.pb` for the viewer. Each family has its own page with a picture of every example and its code folded under it; the floor's page explains its whole construction step by step in pictures.

| Template | Header | Example | Elements |
| --- | --- | --- | --- |
| [Translation shell](@ref templates_shells) | `shells/translation_shell.h` | `templates_shells_1_translation_shell` | one chamfered plate per swept quad |
| [Reflex fold](@ref templates_folding) | `folding/reflex_fold.h` | `templates_folding_1_reflex_fold` | one plate per fold |
| [Chevron](@ref templates_folding) | `shells/chevron.h` | `templates_folding_2_chevron` | four plates per face of an Annen surface |
| [Diamond mesh](@ref templates_folding) | `folding/diamond_mesh.h` | `templates_folding_3_diamond_mesh` | one plate per triangle of a rhombus pattern |
| [VDA mesh](@ref templates_cross) | `cross/vda_mesh.h` | `templates_cross_1_vda_mesh` | one plate per face plus connector plates across every interior edge |
| [Reciprocal move](@ref templates_reciprocal) | `reciprocal/reciprocal_move.h` | `templates_reciprocal_2_move` | one beam plate per mesh edge, shifted past its neighbours |
| [Reciprocal rotation](@ref templates_reciprocal) | `reciprocal/reciprocal_rotation.h` | `templates_reciprocal_1_rotation` | one beam plate per mesh edge, rotated about its midpoint |
| [Lamella gridshell](@ref templates_shells) | `shells/lamella_gridshell.h` | `templates_shells_2_gridshell` | two upright boards per lamella on iso or asymptotic curves, in two layers, a hexagonal stud at every crossing |
| [Grid](@ref templates_grid) | `grid/grid.h` | `1_elements_*`, `templates_grid_{footprint,solid,lines,reference,framing}` | columns, heads, girders, beams, purlins, braces, decks and walls of a multistorey building |
| [Floor](@ref templates_floor) | `floor/floor.h` | `templates_floor_{1..8}_*` | the vaulted timber floor bay of compas_tf: outer and inner ribs, seam and oculus beams, wedge blocks, t-sections, beds, the oculus ring, columns on supports and their connectors |

- @subpage templates_shells
- @subpage templates_folding
- @subpage templates_cross
- @subpage templates_reciprocal
- @subpage templates_grid
- @subpage templates_floor
