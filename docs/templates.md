# Templates {#templates}

[TOC]

Generators under `src/templates/`, one folder per family (`shells/`, `folding/`, `cross/`, `reciprocal/`, `grid/`, `floor/`), that turn a surface, a mesh or a building outline into wood elements. Every template has examples under `examples/`, each a CMake target of the same name that builds it with its defaults and writes `data/output/pb/live.pb` for the viewer. Each family has its own page with a picture of every example and its code folded under it; the floor's page explains its whole construction step by step in pictures.

| Template | Header | Example | Elements |
| --- | --- | --- | --- |
| [Translation shell](@ref templates_shells) | `shells/translation_shell.h` | `templates_shells_1_translation_shell` | `TranslationShell`: one mitred, chamfered plate per swept quad |
| [Reflex fold](@ref templates_folding) | `folding/reflex_fold.h` | `templates_folding_1_reflex_fold` | one plate per fold |
| [Chevron](@ref templates_folding) | `folding/chevron.h` | `templates_folding_2_chevron` | four plates per face of an Annen surface |
| [Diamond mesh](@ref templates_folding) | `folding/diamond_mesh.h` | `templates_folding_3_diamond_mesh` | one plate per triangle of a rhombus pattern |
| [VDA mesh](@ref templates_cross) | `cross/vda_mesh.h` | `templates_cross_1_vda_mesh` | one plate per face plus connector plates across every interior edge |
| [Reciprocal move](@ref templates_reciprocal) | `reciprocal/reciprocal_move.h` | `templates_reciprocal_2_move` | a beam on every other edge of each face, moved past its neighbours, inside a straight frame |
| [Reciprocal rotation](@ref templates_reciprocal) | `reciprocal/reciprocal_rotation.h` | `templates_reciprocal_1_rotation` | a beam on every interior edge, turned about its midpoint, inside a straight frame |
| [Lamella gridshell](@ref templates_shells) | `shells/lamella_gridshell.h` | `templates_shells_{2_gridshell,3_gridshell_iso}` | `LamellaGridshell`: two upright boards per lamella on iso or asymptotic curves, in two layers, a hexagonal stud at every crossing |
| [Grid](@ref templates_grid) | `grid/grid.h` | `3_elements_tree`, `templates_grid_{1..5}_*` | a `Grid` session per building: columns, heads, girders, beams, purlins, braces, decks and walls level by level |
| [Floor](@ref templates_floor) | `floor/floor.h` | `templates_floor_{1..8}_*` | the vaulted timber floor bay of compas_tf: outer and inner ribs, seam and oculus beams, wedge blocks, t-sections, beds, the oculus ring, columns on supports and their connectors |

- @subpage templates_shells
- @subpage templates_folding
- @subpage templates_cross
- @subpage templates_reciprocal
- @subpage templates_grid
- @subpage templates_floor
