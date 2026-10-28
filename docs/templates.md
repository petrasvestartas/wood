# Templates {#templates}

[TOC]

Parametric timber structure generators under `src/templates/`. Every template subclasses `WoodSession`, stores each geometric construction stage in a named tree group, and exports `live.pb` via its `examples/templates_*.cpp` targets.

![Parametric Templates Pipeline](diagrams/wood_templates_pipeline.svg)

| Template | Header | Example Target | Elements & Joinery |
| --- | --- | --- | --- |
| [Translation shell](@ref templates_shells) | `shells/translation_shell.h` | `templates_shells_1_translation_shell` | `TranslationShell`: mitered, chamfered `Plate` per swept quad |
| [Folding flat](@ref templates_folding) | `folding/*.h` | `templates_folding_*` | Planar folded plates (`PrismaticFold`, `CounterFold`, `ConicalFold`, `FoldedFrame`, `SingleFold`, `DiagonalFold`, `FoldComposition`, `TruncatedPyramid`, `TriangleFold`, `ReflexFold`, `DiamondMesh`) with exact `offset_plates` miters |
| [Folding cylindrical](@ref templates_cylindrical_folding) | `cylindrical_folding/*.h` | `templates_cylindrical_folding_*` | Cylindrical `PlateCurved` shells (`ReflectionFold`, `CylinderVault`, `CurvedFoldedBeam`, `CornerFan`, `CylinderCarving`) |
| [Folding developable](@ref templates_developable_folding) | `developable_folding/*.h` | `templates_developable_folding_*` | Developable `PlateCurved` cones & curved-crease mechanisms (`CurvedFold`, `CurvedCreaseUnfolding`, `CreaseQuadrants`, `Development`) |
| [Chevron](@ref templates_chevron) | `chevron/chevron.h` | `templates_chevron_1_chevron` | `Chevron`: 4-plate chevron boxes on an Annen `NurbsSurface` |
| [VDA mesh](@ref templates_cross) | `cross/vda_mesh.h` | `templates_cross_1_vda_mesh` | `VdaMesh`: mitered face plates + crossing edge connector plates (`JointPlate::cross_slot`) |
| [Wall module](@ref templates_module) | `module/wall.h` | `templates_module_1_wall` | `Wall`: `625 mm` stud grid, jamb studs, headers/sills, notched/punched sheathing panels, and face contacts |
| [Cassette shell](@ref templates_module) | `module/cassette_shell.h` | `templates_module_2_cassette_shell` | `CassetteShell`: hollow top/bottom plates + beveled edge beams per polygon face |
| [Cassette house](@ref templates_module) | `module/cassette_shell.h` | `templates_module_3_cassette_house` | `CassetteShell::house_mesh`: 6-cassette gable house joined by ranked T-butts |
| [Room module](@ref templates_module) | `module/room_module.h` | `templates_module_{4..6}_*` | `RoomModule`: `9 × 3 m` framed timber modules (floor, downstand ceiling, corner/mid posts, walls) stacked into multi-storey blocks |
| [Reciprocal rotation](@ref templates_reciprocal) | `reciprocal/reciprocal_rotation.h` | `templates_reciprocal_1_rotation` | `ReciprocalRotation`: interior edges rotated into vertex pinwheels inside a mitered boundary frame |
| [Reciprocal move](@ref templates_reciprocal) | `reciprocal/reciprocal_move.h` | `templates_reciprocal_2_move` | `ReciprocalMove`: alternating face edges shifted sideways to bear on neighbour sides |
| [Grid](@ref templates_grid) | `grid/grid.h` | `3_elements_tree`, `templates_grid_{1..5}_*` | `Grid`: multi-storey columns, pyramidal/stepped heads, girders, purlins, braces, decks, and core/facade walls |
| [Floor](@ref templates_floor) | `floor/floor.h` | `templates_floor_{1..8}_*` | `FloorGuide` + `Floor`: vaulted timber slab with parabolic ribs, arched beds, carved columns, wedges, let-in plates, and pins |
| [Truss](@ref templates_truss) | `truss/truss.h` | `templates_truss_{1,2}_*` | `TrussGuide` + `Truss`: boundary-ranked chord/web/post beams with automatic node butts across 41 library trusses |

- @subpage templates_shells
- @subpage templates_folding
- @subpage templates_cylindrical_folding
- @subpage templates_developable_folding
- @subpage templates_chevron
- @subpage templates_cross
- @subpage templates_module
- @subpage templates_reciprocal
- @subpage templates_grid
- @subpage templates_floor
- @subpage templates_truss
