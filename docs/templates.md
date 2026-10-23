# Templates {#templates}

[TOC]

Generators under `src/templates/`, one folder per family (`shells/`, `folding/`, `chevron/`, `cross/`, `module/`, `reciprocal/`, `grid/`, `floor/`, `truss/`), that turn a surface, a mesh or a building outline into wood elements. Every template has examples under `examples/`, each a CMake target of the same name that builds it with its defaults and writes `data/output/pb/live.pb` for the viewer. Each family has its own page with a picture of every example and its code folded under it; the floor's page explains its whole construction step by step in pictures.

| Template | Header | Example | Elements |
| --- | --- | --- | --- |
| [Translation shell](@ref templates_shells) | `shells/translation_shell.h` | `templates_shells_1_translation_shell` | `TranslationShell`: one mitered, chamfered plate per swept quad |
| [Reflex fold](@ref templates_folding) | `folding/reflex_fold.h` | `templates_folding_1_reflex_fold` | one plate per fold |
| [Chevron](@ref templates_chevron) | `chevron/chevron.h` | `templates_chevron_1_chevron` | four plates per face of an Annen surface |
| [Diamond mesh](@ref templates_folding) | `folding/diamond_mesh.h` | `templates_folding_2_diamond_mesh` | one plate per triangle of a rhombus pattern |
| [Singly folded plate](@ref templates_folding) | `folding/prismatic_fold.h` | `templates_folding_{3_prismatic_fold,4_folded_barrel}` | Engel pp. 219-221: a fold profile carried straight over the span, one plate per fold, diaphragms under the folds and a flange folded on at each free edge; a zigzag and a folded barrel approaching a cylindrical shell |
| [Counter-running fold](@ref templates_folding) | `folding/counter_fold.h` | `templates_folding_{5_counter_fold,6_elevated_fold,7_conical_fold}` | Engel pp. 222-224: fold profiles at stations along the span, counter-running, raised or deepened, every warped strip folded into two planar triangles, ridge to ridge, ridge to valley or alternate |
| [Folded frame](@ref templates_folding) | `folding/folded_frame.h` | `templates_folding_{8_two_hinged_frame,9_three_hinged_frame,10_three_hinged_arch}` | Engel pp. 225-227: a frame or arch whose section is a fold profile, its depth following the moments, the phase turning at every corner |
| [Polygon fold](@ref templates_folding) | `folding/polygon_fold.h` | `templates_folding_{11_triangle_fold,...,15_truncated_pyramid}` | Engel pp. 228, 231-233: folds over a polygon plan in concentric rings, planar trapezoids, an apex or a flat top |
| [Cross fold](@ref templates_folding) | `folding/cross_fold.h` | `templates_folding_{16_cross_fold,...,19_fold_composition}` | Engel pp. 229-230: the waterbomb fold over a square, subdivided from the corners, rising or sloping, composed in a grid |
| [Triangle fold](@ref templates_folding) | `folding/triangle_fold.h` | `templates_folding_{20_triangle_square,...,23_triangle_hexagon}` | Engel p. 232: folded triangular surfaces over square, triangular, pentagonal and hexagonal plans |
| [Fan fold](@ref templates_folding) | `folding/fan_fold.h` | `templates_folding_24_fan_fold` | Engel p. 229: fold lines fanning from the corners to the cross, ridges rising toward the centre |
| [Folding variants](@ref templates_folding) | `folding/*.h` | `templates_folding_{25_one_fold,...,36_folded_truncated_pyramid}` | Engel pp. 220-233: the remaining variants of every page, one example each |
| [Curved fold](@ref templates_folding) | `folding/curved_fold.h` | `templates_folding_37_curved_fold` | the curved folded tent roof of Knie's Zauberhut: twelve folds on a 32 m roof rising into a 26 m tower, every fold strip one curved NURBS panel |
| [VDA mesh](@ref templates_cross) | `cross/vda_mesh.h` | `templates_cross_1_vda_mesh` | one plate per face plus connector plates across every interior edge |
| [Wall module](@ref templates_module) | `module/wall.h` | `templates_module_1_wall` | `Wall`: rails, studs, jamb studs, headers, sills and cripples as beams of one section, the studs on a 625 mm grid and jamb studs at every opening, one sheathing plate on each face with the openings cut out, a face contact between every member and panel that touch |
| [Cassette shell](@ref templates_module) | `module/cassette_shell.h` | `templates_module_2_cassette_shell` | `CassetteShell`: per mesh face a hollow cassette, two thin plates and mitered edge beams cut on the bevel the cell shares with its neighbour, the contacts between cassettes detected |
| [Cassette house](@ref templates_module) | `module/cassette_shell.h` | `templates_module_3_cassette_house` | `CassetteShell::house_mesh`: a gable house of six cassettes, one per wall, gable end and roof slope, joined in T joints, the door a notch, each two panels with its windows cut out and a frame of edge beams, studs, jambs, headers and sills between them, the contacts between cassettes detected |
| [Room module](@ref templates_module) | `module/room_module.h` | `templates_module_{4_room_module,5_module_block,6_bank_pavilion}` | `RoomModule`: timber frame modules after Blumer Lehmann, 9 x 3 m with 2.7 m clear, floor and ceiling of long edge beams a wall wide over the whole length on and under the corner posts, joists across flush on both faces, the ceiling thinner with deep edge beams, the ceiling edge beam one deeper beam on the open sides, on corner posts, walls only on the closed sides, the modules of a room open to each other with a post at mid-length, one wall between rooms, side by side and stacked up to three storeys; the brochure's bank pavilion of four modules measured from its plan; every contact detected |
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
