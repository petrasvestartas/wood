# Delorme {#templates_delorme}

[TOC]

Philibert de l'Orme's 1561 small-timber vault and dome construction system (*charpente à petits bois*, `DelormeGuide` and `DelormeSystem` in `src/templates/delorme/delorme.h`), after *Nouvelles inventions pour bien bastir et à petits fraiz* (Paris, 1561) and Legrand, Molinos and André-Jacob Roubo's 39 m hemispherical cupola at the *Halle au Blé* (Paris, 1782–1783). Short, thin flat boards (~4 French *pieds*, 1.1–1.3 m long) stand on edge in two or three side-by-side plies with running-bond (*quinconce*) stagger, keyed end-to-end by central stub-tenons (*tenon et entaille*), pegged across overlapped plies by wooden *chevilles*, and cinched laterally without iron nails by straight transverse *liernes* and twin opposing tapered wedges (*clavettes*) driven tight against both cheeks of every rib.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#delorme_arch_detail">
    <img src="templates_delorme_1_arch_detail_arch_bay.webp" alt="1561 Three-Rib Bay">
    <span class="wood-card-title">1561 Three-Rib Bay</span>
  </a>
  <a class="wood-card" href="#delorme_barrel_vault">
    <img src="templates_delorme_2_barrel_vault_semicircular.webp" alt="Three-Ply Barrel Vault">
    <span class="wood-card-title">Three-Ply Barrel Vault</span>
  </a>
  <a class="wood-card" href="#delorme_imperiale">
    <img src="templates_delorme_2_barrel_vault_pointed_imperiale.webp" alt="Pointed Impériale Roof">
    <span class="wood-card-title">Pointed Impériale Roof</span>
  </a>
  <a class="wood-card" href="#delorme_dome">
    <img src="templates_delorme_3_dome_dome_extrados.webp" alt="Halle au Blé Cupola">
    <span class="wood-card-title">Halle au Blé Cupola</span>
  </a>
  <a class="wood-card" href="#delorme_dome">
    <img src="templates_delorme_3_dome_dome_intrados.webp" alt="Cupola Intrados">
    <span class="wood-card-title">Cupola Intrados</span>
  </a>
  <a class="wood-card" href="#delorme_crossed_vault">
    <img src="templates_delorme_4_crossed_vault_groin_vault.webp" alt="Crossed Groin Pavilion">
    <span class="wood-card-title">Crossed Groin Pavilion</span>
  </a>
  <a class="wood-card" href="#delorme_funicular">
    <img src="templates_delorme_4_crossed_vault_funicular_catenary.webp" alt="Catenary Funicular Vault">
    <span class="wood-card-title">Catenary Funicular Vault</span>
  </a>
  <a class="wood-card" href="#delorme_assembly_sequence">
    <img src="assembly_templates_delorme_1_arch_detail.gif" alt="Assembly Sequence">
    <span class="wood-card-title">Assembly Sequence</span>
  </a>
</div>

## Parameters

| Parameter | Default | Description |
| --- | --- | --- |
| `kind` | `SemicircularBarrel` | `SemicircularBarrel`, `PointedImperiale`, `HemisphericalDome`, `CrossedGroin` or `CustomRibs` |
| `span`, `rise`, `length` | `4800`, `2400`, `3200` | Clear span (or dome diameter), crown height above `z = 0`, and bay depth in mm |
| `n_ribs`, `n_plies` | `4`, `2` | Number of arch / meridian ribs and side-by-side overlapped board plies (`2` or `3`) |
| `plank_length` | `1200.0` | Target chord length of each short flat plank (~4 French *pieds*) in mm |
| `rib_depth`, `plank_thickness` | `220.0`, `34.0` | Radial board depth (~8–9 *pouces*) and board thickness (~1.25 *pouces*) in mm |
| `stub_tenon_length`, `stub_tenon_ratio` | `26.0`, `0.34` | Central edgewise butt stub-tenon length and depth ratio (*tenon et entaille*) |
| `lierne_spacing`, `lierne_width`, `lierne_height` | `950`, `34`, `68` | Arc pitch and rectangular section of transverse or chordal binding *liernes* in mm |
| `clavette_width`, `clavette_length`, `clavette_taper` | `16`, `148`, `3` | Dimensions of the twin opposing tapered wooden wedges clamping each rib's outer cheeks |
| `cheville_radius` | `8.0` | Radius of cylindrical wooden alignment pegs bored across staggered overlaps in mm |
| `oculus_radius`, `flat_springing` | `580.0`, `true` | Compression oculus curb radius on domes and horizontal springing trim on `z = 0` |

---

## 1561 Treatise Three-Rib Vault Bay {#delorme_arch_detail}

Reconstructs the canonical woodcut of Philibert de l'Orme's *Nouvelles inventions pour bien bastir et à petits fraiz* (1561). Each curved rib comprises two side-by-side plies of short flat boards sawn to curved extrados and intrados facets, staggered by half a board length so every radial butt seam (`tenon et entaille`) is bridged at mid-plank by its companion ply. Continuous rectangular *liernes* pass perpendicularly through aligned mid-depth mortises across all three ribs and lock every rib from both sides with tapered *clavettes*.

<div class="wood-pair-grid">
  <img src="templates_delorme_1_arch_detail_arch_bay.webp" alt="Delorme 1561 three-rib vaulted bay with liernes, clavettes and chevilles">
  <img src="templates_delorme_1_arch_detail_arch_intrados.webp" alt="Intrados soffit view showing side-by-side staggered planks and wedged lierne keys">
</div>

\include{lineno} templates_delorme_1_arch_detail.cpp

---

## Semicircular Three-Ply Barrel Vault & Pointed Impériale Roof {#delorme_barrel_vault}

`DelormeGuide::barrel` with `n_plies = 3` offsets consecutive side-by-side plies by one-third of a plank length (`quinconce` 3-ply stagger) for long-span halls where bending continuity across radial butt seams must retain two-thirds of the composite section at every joint.

![](templates/templates_delorme_2_barrel_vault_semicircular.webp)

### Pointed Comble à l'Impériale (*Carène de Navire*) {#delorme_imperiale}

`DelormeGuide::pointed_imperiale` forms Delorme's inverted ship-hull roof from two circular arcs springing vertically from level wall plates and meeting at a pointed ridge miter, partitioning each slope independently so no plank spans across the crown kink.

![](templates/templates_delorme_2_barrel_vault_pointed_imperiale.webp)

\include{lineno} templates_delorme_2_barrel_vault.cpp

---

## Halle au Blé Hemispherical Cupola {#delorme_dome}

`DelormeGuide::dome` arranges `n_ribs` radial two-ply meridian ribs around a segmented compression oculus ring (`curbs`), following Legrand, Molinos and Roubo's 1783 timber dome over the Paris *Halle au Blé*. Straight chordal *liernes* alternate between even and odd sector rings along the meridian arc so adjacent latitude keyholes and *clavettes* stay clear of one another while cinching every meridian rib into a rigid shell.

<div class="wood-pair-grid">
  <img src="templates_delorme_3_dome_dome_extrados.webp" alt="Halle au Ble hemispherical Delorme dome extrados with oculus curb and staggered chordal liernes">
  <img src="templates_delorme_3_dome_dome_intrados.webp" alt="Hemispherical Delorme cupola intrados view looking up into the compression oculus">
</div>

\include{lineno} templates_delorme_3_dome.cpp

---

## Crossed Groin Pavilion & Catenary Funicular Vault {#delorme_crossed_vault}

`DelormeGuide::crossed_vault` frames a square groin pavilion from four facade portal arches and four diagonal groin ribs converging on an octagonal crown curb, linked across their haunches by diagonal chordal *liernes* and *clavettes*.

![](templates/templates_delorme_4_crossed_vault_groin_vault.webp)

### Custom Funicular Catenary Ribs {#delorme_funicular}

Passing user-defined `DelormeRibCurve` profiles directly to `DelormeGuide(params, custom_ribs)` applies the same running-bond plank lamination, stub-tenon butts, *chevilles*, *liernes* and *clavettes* to inverted catenary funicular vaults and free-form planar rib families.

![](templates/templates_delorme_4_crossed_vault_funicular_catenary.webp)

\include{lineno} templates_delorme_4_crossed_vault.cpp

<!-- assembly:begin -->
## Assembly Sequence {#delorme_assembly_sequence}

First the compression oculus curb where there is one, then every multi-ply rib plank by plank in running-bond stagger along its stub-tenon butt tangent, then the wooden chevilles across the overlapped plies, then the straight liernes threaded endwise through the aligned rib mortises, and last the twin opposing tapered clavettes driven through the lierne keyholes against both outer cheeks of every rib. Every move is a `ModuleSequence` move: the module taken apart piece by piece, each piece swept as a solid against every piece still in, the moving ones drawn brown, a GIF per kind of module and one of the whole.

| assembly | | |
|---|---|---|
| **1561 three-rib bay**, 86 elements, 85 moves | | |
| ![ribs, 33 pieces](assembly/assembly_templates_delorme_1_arch_detail_ribs.gif) ribs, 33 pieces | ![templates delorme 1 arch detail, 54 pieces](assembly/assembly_templates_delorme_1_arch_detail.gif) templates delorme 1 arch detail, 54 pieces |  |
| **Barrel and pointed imperiale**, 363 elements, 361 moves | | |
| ![ribs, 72 pieces](assembly/assembly_templates_delorme_2_barrel_vault_pointed_imperiale_ribs.gif) ribs, 72 pieces | ![pointed imperiale, 105 pieces](assembly/assembly_templates_delorme_2_barrel_vault_pointed_imperiale.gif) pointed imperiale, 105 pieces | ![ribs, 80 pieces](assembly/assembly_templates_delorme_2_barrel_vault_semicircular_ribs.gif) ribs, 80 pieces |
| ![semicircular, 108 pieces](assembly/assembly_templates_delorme_2_barrel_vault_semicircular.gif) semicircular, 108 pieces |  |  |
| **Halle au Ble cupola**, 444 elements, 443 moves | | |
| ![ribs, 120 pieces](assembly/assembly_templates_delorme_3_dome_ribs.gif) ribs, 120 pieces | ![templates delorme 3 dome, 325 pieces](assembly/assembly_templates_delorme_3_dome.gif) templates delorme 3 dome, 325 pieces |  |
| **Crossed groin and funicular**, 298 elements, 296 moves | | |
| ![ribs, 52 pieces](assembly/assembly_templates_delorme_4_crossed_vault_funicular_catenary_ribs.gif) ribs, 52 pieces | ![funicular catenary, 79 pieces](assembly/assembly_templates_delorme_4_crossed_vault_funicular_catenary.gif) funicular catenary, 79 pieces | ![ribs, 80 pieces](assembly/assembly_templates_delorme_4_crossed_vault_groin_vault_ribs.gif) ribs, 80 pieces |
| ![groin vault, 89 pieces](assembly/assembly_templates_delorme_4_crossed_vault_groin_vault.gif) groin vault, 89 pieces |  |  |

<!-- assembly:end -->
