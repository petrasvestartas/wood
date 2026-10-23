# Templates {#templates}

[TOC]

Generators under `src/templates/`, one folder per family (`shells/`, `folding/`, `chevron/`, `cross/`, `module/`, `reciprocal/`, `grid/`, `floor/`, `truss/`), that turn a surface, a mesh or a building outline into wood elements. Every template has examples under `examples/`, each a CMake target of the same name that builds it with its defaults and writes `data/output/pb/live.pb` for the viewer. Each family has its own page with a picture of every example and its code folded under it; the floor's page explains its whole construction step by step in pictures.

| Template | Header | Example | Elements |
| --- | --- | --- | --- |
| [Translation shell](@ref templates_shells) | `shells/translation_shell.h` | `templates_shells_1_translation_shell` | `TranslationShell`: one mitered, chamfered plate per swept quad |
| [Reflex fold](@ref templates_folding) | `folding/reflex_fold.h` | `templates_folding_1_reflex_fold` | one plate per fold |
| [Chevron](@ref templates_chevron) | `chevron/chevron.h` | `templates_chevron_1_chevron` | four plates per face of an Annen surface |
| [Diamond mesh](@ref templates_folding) | `folding/diamond_mesh.h` | `templates_folding_2_diamond_mesh` | one plate per triangle of a rhombus pattern |
| [VDA mesh](@ref templates_cross) | `cross/vda_mesh.h` | `templates_cross_1_vda_mesh` | one plate per face plus connector plates across every interior edge |
| [Wall module](@ref templates_module) | `module/wall.h` | `templates_module_1_wall` | `Wall`: rails, studs, jamb studs, headers, sills and cripples as beams of one section, the studs on a 625 mm grid and jamb studs at every opening, one sheathing plate on each face with the openings cut out, a face contact between every member and panel that touch |
| [Cassette shell](@ref templates_module) | `module/cassette_shell.h` | `templates_module_2_cassette_shell` | `CassetteShell`: per mesh face a hollow cassette, two thin plates and mitered edge beams cut on the bevel the cell shares with its neighbour, the contacts between cassettes detected |
| [Cassette house](@ref templates_module) | `module/cassette_shell.h` | `templates_module_3_cassette_house` | `CassetteShell::house_mesh`: a gable house of six cassettes, one per wall, gable end and roof slope, joined in T joints, the door a notch, each two panels with its windows cut out and a frame of edge beams, studs, jambs, headers and sills between them, the contacts between cassettes detected |
| [Room module](@ref templates_module) | `module/room_module.h` | `templates_module_{4_room_module,5_module_block}` | `RoomModule`: timber frame modules after Blumer Lehmann, 9 x 3 m with 2.7 m clear, a framed floor and ceiling on four corner posts, walls only on the closed sides, the modules of a room open to each other, side by side and stacked up to three storeys, every contact detected |
| [Reciprocal move](@ref templates_reciprocal) | `reciprocal/reciprocal_move.h` | `templates_reciprocal_2_move` | a beam on every other edge of each face, moved past its neighbours, inside a straight frame |
| [Reciprocal rotation](@ref templates_reciprocal) | `reciprocal/reciprocal_rotation.h` | `templates_reciprocal_1_rotation` | a beam on every interior edge, turned about its midpoint, inside a straight frame |
| [Grid](@ref templates_grid) | `grid/grid.h` | `3_elements_tree`, `templates_grid_{1..5}_*` | a `Grid` session per building: columns, heads, girders, beams, purlins, braces, decks and walls level by level |
| [Floor](@ref templates_floor) | `floor/floor.h` | `templates_floor_{1..8}_*` | the vaulted timber floor bay of compas_tf: outer and inner ribs, seam and oculus beams, wedge blocks, t-sections, beds, the oculus ring, columns on supports and their connectors |
| [Truss](@ref templates_truss) | `truss/truss.h` | `templates_truss_{1,2}_*` | a `Truss` session from a `TrussGuide` of chord polylines, web and post lines: a beam per chord run, web and post, every butt read from the nodes; 21 roof, bridge and graphic-statics trusses in the library |

- @subpage templates_shells
- @subpage templates_folding
- @subpage templates_chevron
- @subpage templates_cross
- @subpage templates_module
- @subpage templates_reciprocal
- @subpage templates_grid
- @subpage templates_floor
- @subpage templates_truss
