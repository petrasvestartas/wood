# Templates {#templates}

[TOC]

Parametric timber structure generators under `src/templates/`. Every template subclasses `WoodSession`, transforms explicit primary geometric inputs into timber elements and joints across named scene-tree groups, and exports `live.pb` via its `examples/templates_*.cpp` targets.

![Parametric Templates Pipeline](diagrams/wood_templates_pipeline.svg)

| Template | Header | Primary Inputs | Elements & Joinery Generated |
| --- | --- | --- | --- |
| [Translation shell](@ref templates_shells) | `shells/translation_shell.h` | `Polyline cross_section` ($XZ$), `Polyline profile` ($YZ$), `thickness`, `chamfer` | `TranslationShell`: translated sections $\to$ planar quad mesh $\to$ mitered, chamfered `Plate` solids |
| [Folding flat](@ref templates_folding) | `folding/*.h` | Fold `profile` / `stations` / plan `Polygon` / `NurbsSurface`, `thickness` | 11 planar folded-plate templates (`PrismaticFold` .. `DiamondMesh`) with exact `offset_plates` multi-plate node miters |
| [Folding cylindrical](@ref templates_cylindrical_folding) | `cylindrical_folding/*.h` | `NurbsCurve` arcs, ruling vector `along`, `Fold{station, turn}` / `Carver` cylinders, `thickness` | Cylindrical `PlateCurved` shells (`ReflectionFold`, `CylinderVault`, `CurvedFoldedBeam`, `CornerFan`, `CylinderCarving`) + flat `Development` |
| [Folding developable](@ref templates_developable_folding) | `developable_folding/*.h` | `NurbsCurve` crease arc/catenary, opening angle $\alpha$, crease `planes` (`zigzag`, `leaning`, `fan`) | Developable `PlateCurved` cones & curved-crease mechanisms (`CurvedFold`, `CurvedCreaseUnfolding`, `CreaseQuadrants`, `Development`) |
| [Chevron](@ref templates_chevron) | `chevron/chevron.h` | Annen `NurbsSurface`, `u_divisions`, `v_division_dist`, `shift`, `scale`, `box_height`, `plate_thickness` | `Chevron`: herringbone quad strips, turned/offset edge planes, corner bisectors, and 4-plate boxes with mortise/tenon insertion vectors |
| [VDA mesh](@ref templates_cross) | `cross/vda_mesh.h` | Polygon `Mesh`, `face_thickness`, `face_positions`, `edge_divisions`, `connector` dimensions | `VdaMesh`: mitered face `Plate`s + crossing interior-edge connector `Plate`s joined by `JointPlate::cross_slot()` |
| [Wall module](@ref templates_module) | `module/wall.h` | `WallGuide(length, height, stud_width, stud_depth, stud_spacing, openings)` | `Wall` / `FramedPanel`: rails, jambs, headers/sills, `625 mm` grid studs, cripples, notched/punched sheathing, and `add_module_contacts` |
| [Cassette shell & house](@ref templates_module) | `module/cassette_shell.h` | Polygon `Mesh`, `depth`, `plate_thickness`, `beam_width`, `openings`, `stud_spacing`, `ranks` | `CassetteShell`: hollow top/bottom `Plate`s + perimeter/stud beams joined by bevel miters (`ranks={}`) or ranked T-joints (`house_ranks`) |
| [Room module](@ref templates_module) | `module/room_module.h` | `RoomModuleGuide(length, width, clear_height, rooms, storeys, layouts)` | `RoomModule`: `280 mm` floor cassette, `170 mm` ceiling with `190 × 470` downstand beams, corner/mid posts, and walls |
| [Reciprocal](@ref templates_reciprocal) | `reciprocal/*.h` | Polygon `Mesh`, `angle`/`scale` (rotation) or `shift` (move), `width`, `height`, `CornerJoint` | `ReciprocalRotation` & `ReciprocalMove`: pinwheel or bisector-shifted `Beam`s trimmed on neighbour side faces inside a boundary `frame` |
| [Grid](@ref templates_grid) | `grid/grid.h` | `Building` (`from_solid` / `from_footprint` / `from_lines`), `Pattern`, `Framing` | `Grid`: multi-storey `Column`s, pyramidal/stepped `head`s, `girder`s, `purlin`s, `deck`s, `brace`s, and `wall`/`core_wall`s |
| [Floor](@ref templates_floor) | `floor/floor.h` | `FloorGuide(corners)` (4 CCW bay corner `Point`s + rib/bed/column/oculus parameters) | `FloorGuide` + `Floor`: 4 radial rib-and-bed quarters, oculus ring, carved columns, `68` contacts, wedges, let-in plates, and `56` pins |
| [Truss](@ref templates_truss) | `truss/truss.h` | `TrussGuide(chords, webs, posts, chord_radius, web_radius)` or `80` library presets | `Truss`: outer-boundary-peeled member ranking with automatic kink miters, heel butts, let-in web cuts, and cross half-laps |

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
