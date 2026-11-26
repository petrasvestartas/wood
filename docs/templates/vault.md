# Vault {#templates_vault}

[TOC]

Parametric ribbed timber vaults and stereotomic masonry vaults in `src/templates/vault/`: every template is a session built from its guide in `vault_guide.h` (`Arch(ArchGuide)`, `Barrel(BarrelGuide)`, `Dome(DomeGuide)`, `Wall(WallGuide)`, `CrossVault(CrossVaultGuide)`, `CloisterVault(CloisterVaultGuide)`, `GothicRibbedVault(GothicRibbedVaultGuide)`), the defaults being the examples' settings. Every ribbed vault uses straight 2D plan lines rising as planar arches in 3D and simple circular barrel cylinder webs (`K = 0`) whose top and bottom edges follow the supporting beam centerlines.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#vault_quadripartite">
    <img src="templates_vault_9_gothic_ribbed_vault_quadripartite.webp" alt="Quadripartite Vault">
    <span class="wood-card-title">Quadripartite Vault</span>
  </a>
  <a class="wood-card" href="#vault_tierceron">
    <img src="templates_vault_9_gothic_ribbed_vault_tierceron.webp" alt="Tierceron Vault">
    <span class="wood-card-title">Tierceron Vault</span>
  </a>
  <a class="wood-card" href="#vault_star_stellar">
    <img src="templates_vault_9_gothic_ribbed_vault_star.webp" alt="Stellar Vault">
    <span class="wood-card-title">Stellar Vault</span>
  </a>
  <a class="wood-card" href="#vault_lierne_star">
    <img src="templates_vault_9_gothic_ribbed_vault_lierne.webp" alt="Lierne Star Vault">
    <span class="wood-card-title">Lierne Star Vault</span>
  </a>
  <a class="wood-card" href="#vault_net">
    <img src="templates_vault_9_gothic_ribbed_vault_net.webp" alt="Net Vault">
    <span class="wood-card-title">Net Vault</span>
  </a>
  <a class="wood-card" href="#vault_fan">
    <img src="templates_vault_9_gothic_ribbed_vault_fan.webp" alt="Fan Vault">
    <span class="wood-card-title">Fan Vault</span>
  </a>
  <a class="wood-card" href="#vault_arch">
    <img src="templates_vault_1_arch_all.webp" alt="Voussoir Arch">
    <span class="wood-card-title">Voussoir Arch</span>
  </a>
  <a class="wood-card" href="#vault_barrel">
    <img src="templates_vault_2_barrel_all.webp" alt="Barrel Vault">
    <span class="wood-card-title">Barrel Vault</span>
  </a>
  <a class="wood-card" href="#vault_dome">
    <img src="templates_vault_4_dome_all.webp" alt="Voussoir Dome">
    <span class="wood-card-title">Voussoir Dome</span>
  </a>
  <a class="wood-card" href="#vault_wall">
    <img src="templates_vault_5_wall_all.webp" alt="Masonry Wall">
    <span class="wood-card-title">Masonry Wall</span>
  </a>
  <a class="wood-card" href="#vault_cross">
    <img src="templates_vault_6_cross_vault_all.webp" alt="Cross Vault">
    <span class="wood-card-title">Cross Vault</span>
  </a>
  <a class="wood-card" href="#vault_cloister">
    <img src="templates_vault_7_cloister_vault_all.webp" alt="Cloister Vault">
    <span class="wood-card-title">Cloister Vault</span>
  </a>
  <a class="wood-card" href="#vault_assembly_sequence">
    <img src="assembly_templates_vault_1_arch.gif" alt="Assembly Sequence">
    <span class="wood-card-title">Assembly Sequence</span>
  </a>
</div>

---

## Quadripartite Vault {#vault_quadripartite}

Four perimeter wall arches, diagonal groin ribs, and orthogonal ridge ribs over a 10 x 10 m bay with 8 cylindrical triangular web panels and 4 corner support blocks.

<div class="wood-pair-grid">
  <img src="templates_vault_9_gothic_ribbed_vault_quadripartite.webp" alt="Quadripartite Vault 3D view">
  <img src="templates_vault_9_gothic_ribbed_vault_quadripartite_bottom.webp" alt="Quadripartite Vault bottom beam pattern">
</div>

---

## Tierceron Vault {#vault_tierceron}

Secondary tierceron ribs spring in straight plan lines from the 4 corner support blocks to mid-ridge bosses, dividing the two orthogonal barrel cylinders into 16 cylindrical web panels.

<div class="wood-pair-grid">
  <img src="templates_vault_9_gothic_ribbed_vault_tierceron.webp" alt="Tierceron Vault 3D view">
  <img src="templates_vault_9_gothic_ribbed_vault_tierceron_bottom.webp" alt="Tierceron Vault bottom beam pattern">
</div>

---

## Stellar Vault {#vault_star_stellar}

Four central diamond lozenges framed by interlocking diagonal ribs, corner tiercerons, and tertiary lierne chords across 26 planar curved ribs and 20 cylindrical web panels.

<div class="wood-pair-grid">
  <img src="templates_vault_9_gothic_ribbed_vault_star.webp" alt="Stellar Vault 3D view">
  <img src="templates_vault_9_gothic_ribbed_vault_star_bottom.webp" alt="Stellar Vault bottom beam pattern">
</div>

---

## Lierne Star Vault {#vault_lierne_star}

Inner and outer star rings linked through intermediate diagonal bosses by 34 planar curved ribs, dividing the intersecting barrel cylinders into 32 cylindrical web panels.

<div class="wood-pair-grid">
  <img src="templates_vault_9_gothic_ribbed_vault_lierne.webp" alt="Lierne Star Vault 3D view">
  <img src="templates_vault_9_gothic_ribbed_vault_lierne_bottom.webp" alt="Lierne Star Vault bottom beam pattern">
</div>

---

## Net Vault {#vault_net}

Reticulated diamond-lattice vault inspired by the Bern Cathedral choir, combining 34 planar curved ribs with interlocking cross-lap chords and 32 cylindrical web panels over a 10 x 10 m bay.

<div class="wood-pair-grid">
  <img src="templates_vault_9_gothic_ribbed_vault_net.webp" alt="Net Vault 3D view">
  <img src="templates_vault_9_gothic_ribbed_vault_net_bottom.webp" alt="Net Vault bottom beam pattern">
</div>

---

## Fan Vault {#vault_fan}

Straight-in-plan radiating fan ribs and polygonal collar ribs springing from the 4 corner support blocks across 50 planar curved ribs and 48 cylindrical web panels.

<div class="wood-pair-grid">
  <img src="templates_vault_9_gothic_ribbed_vault_fan.webp" alt="Fan Vault 3D view">
  <img src="templates_vault_9_gothic_ribbed_vault_fan_bottom.webp" alt="Fan Vault bottom beam pattern">
</div>

---

## Voussoir Arch {#vault_arch}

Circular arch of 6 m span and 3 m rise divided into 25 radial wedge voussoir blocks with 24 bed-joint contacts.

![](templates/templates_vault_1_arch_all.webp)

\include{lineno} templates_vault_1_arch.cpp

---

## Barrel Vault {#vault_barrel}

Segmental circular tunnel vault extruded in running bond over 9 courses by 6 rings with half-ring closer voussoirs on odd courses.

![](templates/templates_vault_2_barrel_all.webp)

\include{lineno} templates_vault_2_barrel.cpp

---

## Voussoir Dome {#vault_dome}

Hemispherical dome of 20 staggered hoops by 40 hexagonal voussoirs tapering from 400 mm at springing to 250 mm around a 6-degree oculus.

![](templates/templates_vault_4_dome_all.webp)

\include{lineno} templates_vault_4_dome.cpp

---

## Masonry Wall {#vault_wall}

Running-bond ashlar wall of 400 x 200 mm courses with half-brick closers and automatic bed- and head-joint contact detection.

![](templates/templates_vault_5_wall_all.webp)

\include{lineno} templates_vault_5_wall.cpp

---

## Cross Vault {#vault_cross}

Two semicircular barrel vaults intersecting over a 6 x 6 m bay with monolithic L-shaped groin stones bent across the bay diagonals.

![](templates/templates_vault_6_cross_vault_all.webp)

\include{lineno} templates_vault_6_cross_vault.cpp

---

## Cloister Vault {#vault_cloister}

Four cylindrical barrel webs rising from the perimeter walls of a 6 x 6 m bay and mitered along the vertical diagonal planes.

![](templates/templates_vault_7_cloister_vault_all.webp)

\include{lineno} templates_vault_7_cloister_vault.cpp

## Gothic ribbed vaults {#vault_example_9}

Six symmetric shallow (1:10 rise-to-span, 10 m x 10 m span, 1 m rise) Gothic cathedral ribbed vault bays built from single-curved planar glulam beams (BeamCurved, 120 x 360 mm, 6 lamellae) in vertical planes over straight 2D plan lines with exact angle-bisector node miter cuts (rib-\>cuts), 4 polygonal corner support blocks (Block) where ribs and web panels terminate cleanly, and simple cylindrical barrel web panels (BRep, K = 0 groin_cylinders) whose bottom and top edges lie directly above the supporting beam centerlines (Bern Cathedral / Berner Muenster; compas_tno; Tracing the Past LiDAR survey, Buchanan, Webb & Peterson 2021; Wendland et al. 2024; Jobbik 2024):
1. quadripartite_wells    — Classic quadripartite cross vault on intersecting barrel cylinders (Bern Cathedral / Wells / Salisbury, 4 blocks + 10 ribs + 8 cylindrical web panels = 22 elements)
2. tierceron_exeter       — 4-pointed starcut tierceron vault on intersecting barrel cylinders (Bern Cathedral / Lincoln / Exeter, 4 blocks + 18 ribs + 16 cylindrical web panels = 38 elements)
3. star_stellar_burgos    — 4-lozenge Greek-cross stellar vault (Sterngewoelbe / Boveda Estrellada) on intersecting barrel cylinders (Bern Cathedral, 4 blocks + 26 ribs + 20 cylindrical web panels = 50 elements)
4. lierne_star_ely        — Double-star tierceron-lierne vault on intersecting barrel cylinders (Bern Cathedral / Ely Choir / Wells Lady Chapel, 4 blocks + 34 ribs + 32 cylindrical web panels = 70 elements)
5. net_reticulated_parler — Reticulated diamond-lattice net vault (Netzgewoelbe) on intersecting barrel cylinders (Bern Cathedral Choir / Parler, 4 blocks + 34 ribs + 32 cylindrical web panels = 70 elements)
6. fan_conoid_gloucester  — Radiating fan vault with straight horizontal collar ribs along cylinder rulings (compas_tno / Gloucester, 4 blocks + 50 ribs + 48 cylindrical web panels = 102 elements)

\include{lineno} templates_vault_9_gothic_ribbed_vault.cpp

<!-- assembly:begin -->
## Assembly Sequence {#vault_assembly_sequence}

Block by block from the springings to the crown; the Gothic bays their support blocks, the ribs on them, the web panels on the ribs. A block is let down along the direction nearest to straight down its neighbours leave it. The web panels, trimmed surfaces, are let down on the ribs unswept. Every move is a `ModuleSequence` move: the module taken apart piece by piece, each piece swept as a solid against every piece still in, the moving ones drawn brown, a GIF per kind of module and one of the whole.

| assembly | | |
|---|---|---|
| **Arch**, 25 elements, 24 moves | | |
| ![vault arch, 25 pieces](assembly/assembly_templates_vault_1_arch.gif) vault arch, 25 pieces |  |  |
| **Barrel**, 58 elements, 57 moves | | |
| ![vault barrel, 58 pieces](assembly/assembly_templates_vault_2_barrel.gif) vault barrel, 58 pieces |  |  |
| **Dome**, 800 elements, 799 moves | | |
| ![vault dome, 800 pieces](assembly/assembly_templates_vault_4_dome.gif) vault dome, 800 pieces |  |  |
| **Wall**, 105 elements, 104 moves | | |
| ![vault wall, 105 pieces](assembly/assembly_templates_vault_5_wall.gif) vault wall, 105 pieces |  |  |
| **Cross vault**, 184 elements, 183 moves | | |
| ![vault cross vault, 184 pieces](assembly/assembly_templates_vault_6_cross_vault.gif) vault cross vault, 184 pieces |  |  |
| **Cloister vault**, 304 elements, 303 moves | | |
| ![vault cloister vault, 304 pieces](assembly/assembly_templates_vault_7_cloister_vault.gif) vault cloister vault, 304 pieces |  |  |
| **Gothic ribbed vaults**, 352 elements, 346 moves | | |
| ![fan conoid gloucester, 102 pieces](assembly/assembly_templates_vault_9_gothic_ribbed_vault_fan_conoid_gloucester.gif) fan conoid gloucester, 102 pieces | ![lierne star ely, 70 pieces](assembly/assembly_templates_vault_9_gothic_ribbed_vault_lierne_star_ely.gif) lierne star ely, 70 pieces | ![net reticulated parler, 70 pieces](assembly/assembly_templates_vault_9_gothic_ribbed_vault_net_reticulated_parler.gif) net reticulated parler, 70 pieces |
| ![quadripartite wells, 22 pieces](assembly/assembly_templates_vault_9_gothic_ribbed_vault_quadripartite_wells.gif) quadripartite wells, 22 pieces | ![star stellar burgos, 50 pieces](assembly/assembly_templates_vault_9_gothic_ribbed_vault_star_stellar_burgos.gif) star stellar burgos, 50 pieces | ![tierceron exeter, 38 pieces](assembly/assembly_templates_vault_9_gothic_ribbed_vault_tierceron_exeter.gif) tierceron exeter, 38 pieces |

<!-- assembly:end -->
