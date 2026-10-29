# Templates {#templates}

[TOC]

Generators under `src/templates/`, one folder per family (`shells/`, `folding/`, `cylindrical_folding/`, `developable_folding/`, `chevron/`, `cross/`, `module/`, `reciprocal/`, `grid/`, `floor/`, `truss/`, `vault/`), that turn a surface, a mesh or a building outline into wood elements. Every template has examples under `examples/`, each a CMake target of the same name that builds it with its defaults and writes `data/output/pb/live.pb` for the viewer. Each family has its own page with a picture of every example and its code folded under it; the floor's page explains its whole construction step by step in pictures.

Click any template card below to jump directly to its chapter, geometric formulation, and C++ examples:

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="templates_shells.html">
    <img src="templates_shells_1_translation_shell_plates.webp" alt="Translation Shell">
    <span class="wood-card-title">Shells (`TranslationShell`)</span>
    <span class="wood-card-sub">Mitered, chamfered plates per swept quad &amp; lamella gridshells</span>
  </a>
  <a class="wood-card" href="templates_folding.html">
    <img src="templates_folding_1_reflex_fold_plates.webp" alt="Folding Flat">
    <span class="wood-card-title">Folding Flat</span>
    <span class="wood-card-sub">Planar plates folded along straight creases (`ReflexFold`, `DiamondMesh`, `PrismaticFold`, `CounterFold`, `FoldedFrame`)</span>
  </a>
  <a class="wood-card" href="templates_cylindrical_folding.html">
    <img src="templates_cylindrical_folding_1_reflection_lamellas.webp" alt="Folding Cylindrical">
    <span class="wood-card-title">Folding Cylindrical</span>
    <span class="wood-card-sub">Cylindrical `PlateCurved` lamellas (`ReflectionFold`, `CylinderVault`, `CurvedFoldedBeam`, `CornerFan`, `CylinderCarving`)</span>
  </a>
  <a class="wood-card" href="templates_developable_folding.html">
    <img src="templates_developable_folding_1_curved_fold_lamellas.webp" alt="Folding Developable">
    <span class="wood-card-title">Folding Developable</span>
    <span class="wood-card-sub">Cones &amp; ruled developables (`CurvedFold`, `CurvedCreaseUnfolding`, `CreaseQuadrants`)</span>
  </a>
  <a class="wood-card" href="templates_chevron.html">
    <img src="templates_chevron_1_chevron_plates.webp" alt="Chevron">
    <span class="wood-card-title">Chevron</span>
    <span class="wood-card-sub">Four-plate hollow chevron boxes on Annen NURBS surfaces (`Chevron`)</span>
  </a>
  <a class="wood-card" href="templates_cross.html">
    <img src="templates_cross_1_vda_mesh_all.webp" alt="Cross (VDA Mesh)">
    <span class="wood-card-title">Cross (`VdaMesh`)</span>
    <span class="wood-card-sub">Mitered face plates interlocked by cross-slotted edge connector plates</span>
  </a>
  <a class="wood-card" href="templates_module.html">
    <img src="templates_module_3_cassette_house_all.webp" alt="Wall, Cassette &amp; Room Modules">
    <span class="wood-card-title">Module (`Wall`, `CassetteShell`, `RoomModule`)</span>
    <span class="wood-card-sub">Stud-framed walls, hollow cassette shells, gable houses &amp; multi-storey room modules</span>
  </a>
  <a class="wood-card" href="templates_reciprocal.html">
    <img src="templates_reciprocal_1_rotation_beams.webp" alt="Reciprocal">
    <span class="wood-card-title">Reciprocal (`ReciprocalRotation`, `ReciprocalMove`)</span>
    <span class="wood-card-sub">Pinwheel-rotated &amp; bisector-shifted reciprocal beam gridshells</span>
  </a>
  <a class="wood-card" href="templates_grid.html">
    <img src="3_elements_tree_all.webp" alt="Grid">
    <span class="wood-card-title">Grid (`Building`, `Framing`)</span>
    <span class="wood-card-sub">Multi-storey timber buildings from footprints, solids &amp; lines</span>
  </a>
  <a class="wood-card" href="templates_floor.html">
    <img src="floor_viewer.webp" alt="Floor">
    <span class="wood-card-title">Floor (`FloorGuide`, `Floor`)</span>
    <span class="wood-card-sub">Vaulted timber floor bay on four carved columns with central oculus</span>
  </a>
  <a class="wood-card" href="templates_truss.html">
    <img src="templates_truss_1_pratt.png" alt="Truss">
    <span class="wood-card-title">Truss (`Truss`, `TrussGuide`)</span>
    <span class="wood-card-sub">Boundary-peeling timber trusses &amp; 80 parametric roof/bridge presets</span>
  </a>
  <a class="wood-card" href="templates_vault.html">
    <img src="templates_vault_9_gothic_ribbed_vault_tierceron.webp" alt="Vault">
    <span class="wood-card-title">Vault (`GothicRibbedVault`, `wood_vault`)</span>
    <span class="wood-card-sub">6 Gothic ribbed timber vaults (cylindrical webs), `star_vault` &amp; 7 stereotomic vaults</span>
  </a>
</div>

| Template | Header | Example | Elements |
| --- | --- | --- | --- |
| [Translation shell](@ref templates_shells) | `shells/translation_shell.h` | `templates_shells_1_translation_shell` | `TranslationShell`: one mitered, chamfered plate per swept quad |
| [Folding flat](@ref templates_folding) | `folding/*.h` | `templates_folding_*` | planar plates folded along straight creases: reflex, diamond, prismatic, counter-running, conical, single, diagonal and triangle folds, folded frames and truncated pyramids, every plate offset exactly with flush mitres |
| [Folding cylindrical](@ref templates_cylindrical_folding) | `cylindrical_folding/*.h` | `templates_cylindrical_folding_*` | curved plates that are cylinders, bent from flat panels and folded on curved creases: reflection folds, cylinder vaults, the curved-folded beam, the corner fan, cylinder carvings |
| [Folding developable](@ref templates_developable_folding) | `developable_folding/*.h` | `templates_developable_folding_*` | curved plates that are cones and other developables: curved folds, curved-crease unfoldings, crease quadrants |
| [Chevron](@ref templates_chevron) | `chevron/chevron.h` | `templates_chevron_1_chevron` | four plates per face of an Annen surface |
| [VDA mesh](@ref templates_cross) | `cross/vda_mesh.h` | `templates_cross_1_vda_mesh` | one plate per face plus connector plates across every interior edge |
| [Wall module](@ref templates_module) | `module/wall.h` | `templates_module_1_wall` | `Wall`: rails, studs, jamb studs, headers, sills and cripples as beams of one section, the studs on a 625 mm grid and jamb studs at every opening, one sheathing plate on each face with the openings cut out, a face contact between every member and panel that touch |
| [Cassette shell](@ref templates_module) | `module/cassette_shell.h` | `templates_module_2_cassette_shell` | `CassetteShell`: per mesh face a hollow cassette, two thin plates and mitered edge beams cut on the bevel the cell shares with its neighbour, the contacts between cassettes detected |
| [Cassette house](@ref templates_module) | `module/cassette_shell.h` | `templates_module_3_cassette_house` | `CassetteShell::house_mesh`: a gable house of six cassettes, one per wall, gable end and roof slope, joined in T joints, the door a notch, each two panels with its windows cut out and a frame of edge beams, studs, jambs, headers and sills between them, the contacts between cassettes detected |
| [Room module](@ref templates_module) | `module/room_module.h` | `templates_module_{4_room_module,5_module_block,6_bank_pavilion}` | `RoomModule`: timber frame modules 9 x 3 m with 2.7 m clear, floor and ceiling of long edge beams a wall wide over the whole length on and under the corner posts, joists across flush on both faces, the ceiling thinner with deep edge beams, the ceiling edge beam one deeper beam on the open sides, on corner posts, walls only on the closed sides, the modules of a room open to each other with a post at mid-length, one wall between rooms, side by side and stacked up to three storeys; a bank pavilion of four modules; every contact detected |
| [Reciprocal move](@ref templates_reciprocal) | `reciprocal/reciprocal_move.h` | `templates_reciprocal_2_move` | a beam on every other edge of each face, moved past its neighbours, inside a straight frame |
| [Reciprocal rotation](@ref templates_reciprocal) | `reciprocal/reciprocal_rotation.h` | `templates_reciprocal_1_rotation` | a beam on every interior edge, turned about its midpoint, inside a straight frame |
| [Lamella gridshell](@ref templates_shells) | `shells/lamella_gridshell.h` | `templates_shells_{2_gridshell,3_gridshell_iso}` | `LamellaGridshell`: two upright boards per lamella on iso or asymptotic curves, in two layers, a hexagonal stud at every crossing |
| [Grid](@ref templates_grid) | `grid/grid.h` | `3_elements_tree`, `templates_grid_{1.5}_*` | a `Grid` session per building: columns, heads, girders, beams, purlins, braces, decks and walls level by level |
| [Floor](@ref templates_floor) | `floor/floor.h` | `templates_floor_{1.8}_*` | the vaulted timber floor bay of compas_tf: outer and inner ribs, seam and oculus beams, wedge blocks, t-sections, beds, the oculus ring, columns on supports and their connectors |
| [Truss](@ref templates_truss) | `truss/truss.h` | `templates_truss_{1,2}_*` | a `Truss` session from a `TrussGuide` of chord polylines, web and post lines: a beam per chord run, web and post, every butt read from the nodes; 21 roof, bridge and graphic-statics trusses in the library |
| [Vault](@ref templates_vault) | `vault/vault.h` | `templates_vault_{1..9}_*` | `GothicRibbedVault`: six historical Gothic cathedral bays (`quadripartite`, `tierceron`, `star_stellar`, `lierne_star`, `net_reticulated`, `fan_conoid`) with curved glulam ribs (`BeamCurved`), corner support blocks (`Block`), and simple cylindrical web panels (`vault_panel`), plus `star_vault` and stereotomic masonry vaults |

<div style="display:none">
![](templates/templates_shells_1_translation_shell_plates.webp)
![](templates/templates_folding_1_reflex_fold_plates.webp)
![](templates/templates_cylindrical_folding_1_reflection_lamellas.webp)
![](templates/templates_developable_folding_1_curved_fold_lamellas.webp)
![](templates/templates_chevron_1_chevron_plates.webp)
![](templates/templates_cross_1_vda_mesh_all.webp)
![](templates/templates_module_3_cassette_house_all.webp)
![](templates/templates_reciprocal_1_rotation_beams.webp)
![](templates/3_elements_tree_all.webp)
![](floor/floor_viewer.webp)
![](templates/templates_truss_1_pratt.png)
![](examples/templates_vault_9_gothic_ribbed_vault_tierceron.webp)
</div>

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
- @subpage templates_vault
