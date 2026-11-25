# Plaited {#templates_plaited}

[TOC]

Interlocking plaited stereotomy (`PlaitedStereotomy`) and Leonardo da Vinci reciprocal arch framing (`PlaitedReciprocal`) in `src/templates/plaited/plaited.h`, after Richard Etlin, Giuseppe Fallacara and Luc Tamborero (*Plaited Stereotomy: Stone Vaults for the Modern World*, 2008). A single geometric guide (`PlaitedGuide`) decomposes flat floor bays, single-curved arches, and doubly curved shells into four-sided extrados cells whose opposite joint planes alternate inward (`-bevel`) and outward (`+bevel`) slopes for full face-to-face contact, open hot-wire reciprocal bar vaults (`opening > 0`), or pin-connected reciprocal timber arches.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#plaited_flat_floor">
    <img src="templates_plaited_1_flat_floor_intrados.webp" alt="Flat Vault">
    <span class="wood-card-title">Flat Vault</span>
  </a>
  <a class="wood-card" href="#plaited_arch">
    <img src="templates_plaited_2_arch_blocks.webp" alt="Single-Curved Arch">
    <span class="wood-card-title">Single-Curved Arch</span>
  </a>
  <a class="wood-card" href="#plaited_dome">
    <img src="templates_plaited_3_dome_intrados.webp" alt="Doubly Curved Dome">
    <span class="wood-card-title">Doubly Curved Dome</span>
  </a>
  <a class="wood-card" href="#plaited_hotwire">
    <img src="templates_plaited_4_hotwire_blocks_plan.webp" alt="Hot-Wire Abeille Vault">
    <span class="wood-card-title">Hot-Wire Abeille Vault</span>
  </a>
  <a class="wood-card" href="#plaited_hotwire_sphera">
    <img src="templates_plaited_4_hotwire_blocks_sphera_open.webp" alt="Hot-Wire Sphera Dome">
    <span class="wood-card-title">Hot-Wire Sphera Dome</span>
  </a>
  <a class="wood-card" href="#plaited_reciprocal_arch">
    <img src="templates_plaited_5_reciprocal_arch_arch_pinned.webp" alt="Pinned Reciprocal Arch">
    <span class="wood-card-title">Pinned Reciprocal Arch</span>
  </a>
  <a class="wood-card" href="#plaited_assembly_sequence">
    <img src="assembly_templates_plaited_1_flat_floor.gif" alt="Assembly Sequence">
    <span class="wood-card-title">Assembly Sequence</span>
  </a>
</div>

## Parameters

| Parameter | `PlaitedGuide` / `PlaitedStereotomy` | `PlaitedReciprocal` | Description |
| --- | --- | --- | --- |
| `mesh` | `const Mesh&` | via `guide` | Extrados polygon mesh (flat, single-curved or doubly curved) |
| `pattern` | `Abeille`, `Sphera`, `LeonardoStar` | via `guide` | 4-way quad checkerboard, 3-way hexagonal rhombille, or universal `n`-kite star |
| `thickness` | `180.0` | `beam_height` | Structural depth along `-normal` in mm |
| `bevel` | `0.45 rad` | — | Signed joint tilt angle around each interior seam or trapezoidal bar cheek |
| `opening` | `0.0` (`[0, 0.85]`) | — | Open-lattice fraction narrowing each block into a 6-plane hot-wire reciprocal bar |
| `flat_base` | `false` | — | Terminates springing blocks of curved arches and domes flush on the horizontal ground plane |
| `beam_width`, `beam_height` | — | `80.0`, `thickness` | Timber bar width and radial depth in mm |
| `pin_diameter` | — | `0.0` | Cylindrical pin diameter bored across butt contacts when positive |

---

## Flat Surface: Abeille Vault {#plaited_flat_floor}

Joseph Abeille's 1699 *volta piana* translates Leonardo da Vinci's four-bar reciprocal system into closed stereotomy. `PlaitedGuide` 2-colors the quad grid so even and odd cells alternate which pair of opposite edges tilts inward (`-bevel`, supporting cheeks) and which pair tilts outward (`+bevel`, overhanging ends). Naked outer boundary seams stay vertical for a flush square perimeter while the underside forms recessed woven coffers.

<div class="wood-pair-grid">
  <img src="templates_plaited_1_flat_floor_blocks.webp" alt="Abeille flat vault extrados view">
  <img src="templates_plaited_1_flat_floor_intrados.webp" alt="Abeille flat vault woven intrados coffers">
</div>

\include{lineno} templates_plaited_1_flat_floor.cpp

---

## Single-Curved Surface: Plaited Barrel Arch {#plaited_arch}

Mapping `PlaitedPattern::Abeille` onto a circular barrel mesh (`arch_quad_mesh`) produces Fallacara's plaited arch (*Arco di Abeille*). Full-depth shared bevel planes (`relief_depth = 0.0`) maintain continuous face-to-face contact across all four sides of every voussoir from extrados to intrados, while `flat_base = true` terminates both springing courses on the horizontal ground plane.

<div class="wood-pair-grid">
  <img src="templates_plaited_2_arch_blocks.webp" alt="Single-curved plaited barrel arch">
  <img src="templates_plaited_2_arch_intrados.webp" alt="Woven intrados coffers of the barrel arch with full side contact">
</div>

\include{lineno} templates_plaited_2_arch.cpp

---

## Doubly Curved Surface: Geodesic LeonardoStar Dome {#plaited_dome}

`geodesic_dome_mesh` builds an exact tangent-plane Goldberg polyhedron of planar pentagons and hexagons clipped horizontally to a level circular base ring on `z = 0`. `PlaitedPattern::LeonardoStar` subdivides each `n`-gon from its centroid to its edge midpoints into `n` ortho-kite voussoirs whose bevel signs oppose across every interior spoke and polygon edge, forming five- and six-ray woven star coffers inside a continuous flat masonry base ring.

<div class="wood-pair-grid">
  <img src="templates_plaited_3_dome_blocks.webp" alt="Doubly curved Goldberg LeonardoStar dome extrados">
  <img src="templates_plaited_3_dome_intrados.webp" alt="LeonardoStar dome intrados star weave and circular horizontal base ring">
</div>

\include{lineno} templates_plaited_3_dome.cpp

---

## Open Hot-Wire Reciprocal Block Stereotomy {#plaited_hotwire}

Setting `opening > 0` realizes the open-lattice reciprocal block vaults of Fallacara's Stage 5 progression and Thibault Schwartz / HAL Robotics robotic hot-wire polystyrene stereotomy. Interior cheeks inset to `(1 - opening)` of the cell width with upward-facing trapezoidal slopes while perimeter cheeks anchor flush on the naked mesh boundary, producing a straight outer rim and open square or triangular pinwheel voids.

<div class="wood-pair-grid">
  <img src="templates_plaited_4_hotwire_blocks_abeille_open.webp" alt="Open Abeille hot-wire reciprocal block vault">
  <img src="templates_plaited_4_hotwire_blocks_plan.webp" alt="Top plan view showing flush square boundary and open pinwheel T-junctions">
</div>

### Circular Hot-Wire Sphera Dome {#plaited_hotwire_sphera}

Passing `circular_rim = true` to `spherical_cap_hex_mesh` with `flat_base = true` extends the hexagonal rhombille seamlessly through a circular base ring and slices every springing bar horizontally flush on `z = 0`.

![](templates/templates_plaited_4_hotwire_blocks_sphera_open.webp)

\include{lineno} templates_plaited_4_hotwire_blocks.cpp

---

## Pinned Leonardo Reciprocal Barrel Arch {#plaited_reciprocal_arch}

`PlaitedReciprocal` turns each plaited cell axis into a straight timber bar enclosed by continuous straight springing sills and radially aligned arch ribs, cutting every bar end face-to-face against its crossing neighbours and boring exact cylindrical BRep pins (`pin_diameter > 0`) that lean through every bar's top into the cheek its end stands on (`JointBeamElement::skewed_pins`), driven once the bars stand.

<div class="wood-pair-grid">
  <img src="templates_plaited_5_reciprocal_arch_arch_pinned.webp" alt="Finely subdivided pinned Leonardo reciprocal barrel arch">
  <img src="templates_plaited_5_reciprocal_arch_pins_detail.webp" alt="Close-up of exact BRep cylindrical pins and bores across T-butt contacts">
</div>

\include{lineno} templates_plaited_5_reciprocal_arch.cpp

<!-- assembly:begin -->
## Assembly Sequence {#plaited_assembly_sequence}

The blocks interlock, so every one goes in along a direction of its own, leaning with its bevels, and some with a turn; the reciprocal arch its frame and its bars, then every joint's two pins, leaning, down through the bar's top. Every move is a `ModuleSequence` move: the module taken apart piece by piece, each piece swept as a solid against every piece still in, the moving ones drawn brown, a GIF per kind of module and one of the whole.

| assembly | | |
|---|---|---|
| **Flat floor**, 36 elements, 35 moves | | |
| ![plaited stereotomy, 36 pieces](assembly/assembly_templates_plaited_1_flat_floor.gif) plaited stereotomy, 36 pieces |  |  |
| **Arch**, 72 elements, 71 moves | | |
| ![plaited stereotomy, 72 pieces](assembly/assembly_templates_plaited_2_arch.gif) plaited stereotomy, 72 pieces |  |  |
| **Dome**, 255 elements, 254 moves | | |
| ![plaited stereotomy, 255 pieces](assembly/assembly_templates_plaited_3_dome.gif) plaited stereotomy, 255 pieces |  |  |
| **Hot-wire blocks**, 180 elements, 178 moves | | |
| ![abeille open, 36 pieces](assembly/assembly_templates_plaited_4_hotwire_blocks_abeille_open.gif) abeille open, 36 pieces | ![sphera open, 144 pieces](assembly/assembly_templates_plaited_4_hotwire_blocks_sphera_open.gif) sphera open, 144 pieces |  |
| **Reciprocal arch**, 514 elements, 321 moves | | |
| ![bars, 130 pieces](assembly/assembly_templates_plaited_5_reciprocal_arch_bars.gif) bars, 130 pieces | ![plaited reciprocal, 193 pieces](assembly/assembly_templates_plaited_5_reciprocal_arch.gif) plaited reciprocal, 193 pieces |  |

<!-- assembly:end -->
