# Gothic Ribbed Vault & Stereotomic Vaults (wood_vault) {#templates_vault}

[TOC]

![](diagrams/template_vault_pipeline.webp)

`wood_vault` (`src/templates/vault/vault.h`) provides parametric **Gothic ribbed vaults** built from curved glulam beams (`wood_session::BeamCurved`) with corner support blocks (`wood_session::Block`), **stellar shell-and-rib vaults** (`star_vault`), and **stereotomic masonry vaults** (`arch`, `barrel`, `dome`, `wall`, `cross_vault`, `cloister_vault`) inside `WoodSession`.

---

## 1. Gothic Ribbed Vaults (`GothicRibbedVault` / `gothic_ribbed_vault`)

`wood_vault::GothicRibbedVault` and `wood_vault::gothic_ribbed_vault` generate shallow, symmetric Gothic ribbed vaults (`1:10` rise-to-span ratio by default: `10000 × 10000 mm` bay, `1000 mm` crown rise) from continuous NURBS `BRep` curved glulam beams (`wood_session::BeamCurved`, `120 × 360 mm`, `6` lamellae), `4` polygonal corner support blocks (`wood_session::Block`), and trimmed NURBS `BRep` infill shell panels (`vault_panel`) meeting directly along the beam centerlines.

The rib topologies, tracing-floor proportions, and analytical web surfaces are grounded in terrestrial laser scanning (LiDAR) and geometric studies of medieval Gothic cathedrals:
- **Tracing the Past** (*Digitising and Modelling English Medieval Vaults*, Alexandrina Buchanan, Nicholas Webb, JR Peterson, and James Hillson, 2021 — [tracingthepast.org.uk](https://www.tracingthepast.org.uk/2021/04/09/digitising_modelling/))
- **David Wendland et al.** (2024), late-Gothic rib vault geometry and stonecutting analysis (*International Journal of Architectural Heritage*, [`10.1080/15583058.2024.2427661`](https://www.tandfonline.com/doi/epdf/10.1080/15583058.2024.2427661))
- **Eszter Jobbik & János Krähling** (2024), late-Gothic net and stellar vault system geometry (*Nexus Network Journal*, [`10.1007/s00004-024-00780-1`](https://link.springer.com/article/10.1007/s00004-024-00780-1))

![](examples/templates_vault_9_gothic_ribbed_vault_all.webp)

### Structural & Geometric Features

1. **Analytical Top Vault Surfaces (`ArchMethod`) Flush with Rib Extrados**:
   - Every curved rib's centerline is offset inward along the local surface normal by `-0.5 * rib_height * up` so its **upper (`+y`) extrados surface lies flush on a smooth analytical vault surface** `S(x, y)` — allowing curved or segmented timber infill panels to sit directly on top of the ribs:
     - `ArchMethod::spherical_sail` — Spherical dome / pendentive sail passing through the 4 bay corners `(±W/2, ±L/2, 0)` and rising to `(0, 0, rise)` with radius `R_sail = ((W/2)^2 + (L/2)^2 + rise^2) / (2 * rise)`.
     - `ArchMethod::groin_cylinders` — Two orthogonal circular barrel cylinders intersecting along the diagonal groin planes `|x|/W = |y|/L`, with identical circular boundary arches on all 4 walls.
     - `ArchMethod::cloister_cylinders` — Domical translational cylinder groin vault whose boundary arches rise to `0.65 * rise` and whose central ridges arch to `rise`.
2. **Corner Support Blocks (`support_block`, `wood_session::Block`)**:
   - At the 4 bay corners, converging ribs never overlap or self-intersect; instead, they stop at a common radius `r_corner` against a polygonal springer block (`support_block`) whose top face follows the vault surface `S(x, y)` and whose facets match the vertical stop planes of the incoming ribs.
3. **Exact Angle-Bisector Node Miter Cuts (`BeamCurved::trimmed_brep`)**:
   - At every interior boss and wall-midpoint node, incident ribs are sorted counter-clockwise by plan angle and trimmed by the two adjacent vertical angle-bisector planes (`rib->cuts`).
   - `BeamCurved::trimmed_brep()` constructs a closed, smooth NURBS `BRep` with trimmed NURBS top/bottom surfaces and untrimmed ruled side faces (no internal mesh subdivision lines).

| Parameter | Type / Default | Description |
| :--- | :--- | :--- |
| `span_x`, `span_y` | `double` (`10000.0`, `10000.0`) | Symmetric rectangular bay span (`x`, `y`) in `mm`. |
| `rise` | `double` (`1000.0`) | Shallow `1:10` crown height (`mm`) at `(0, 0, rise)`. |
| `rib_width`, `rib_height` | `double` (`120.0`, `360.0`) | Cross-section width and depth (`mm`) of `BeamCurved` glulam ribs (`6` lamellae). |
| `pattern` | `GothicPattern` | `quadripartite`, `tierceron`, `star_stellar`, `lierne_star`, `net_reticulated`, or `fan_conoid`. |
| `surface_method` | `ArchMethod` | `spherical_sail`, `groin_cylinders`, or `cloister_cylinders`. |
| `subdivisions` | `int` (`16`) | Interpolation stations per `BeamCurved` rib. |

---

### Six Historical Cathedral Topologies (`GothicPattern`)

| Pattern | Historical Prototype | Rib & Support Topology |
| :--- | :--- | :--- |
| `GothicPattern::quadripartite` | **Notre-Dame de Paris / Wells / Salisbury Nave** (Early Gothic, c. 1190–1220) | `4` corner support blocks + `16` mitered `BeamCurved` ribs (`8` perimeter wall arches, `4` diagonal ribs, `4` orthogonal ridge ribs). |
| `GothicPattern::tierceron` | **Lincoln / Exeter Cathedral Nave** (English Decorated Starcut Tierceron Vault, c. 1280–1369) | `4` corner support blocks + `28` mitered `BeamCurved` ribs (`8` wall arches, `4` diagonals, `8` segmented ridge ribs, `8` tiercerons to `±1/2` ridge bosses). |
| `GothicPattern::star_stellar` | **Burgos / German *Sterngewölbe* / *Bóveda Estrellada*** (8-Pointed Star Vault, c. 1350–1480) | `4` corner support blocks + `36` mitered `BeamCurved` ribs forming an 8-pointed central star with intermediate diagonal bosses. |
| `GothicPattern::lierne_star` | **Ely Cathedral Choir / Wells Lady Chapel** (Double-Star Tierceron + Lierne Vault, c. 1320–1350) | `4` corner support blocks + `48` mitered `BeamCurved` ribs with outer tiercerons, inner stellar liernes, and 4 petal lozenges. |
| `GothicPattern::net_reticulated` | **Parler School / Central European *Netzgewölbe*** (Late-Gothic Reticulated Diamond-Lattice Net Vault — Jobbik 2024) | `4` corner support blocks + `40` mitered `BeamCurved` ribs arranged in a non-crossing rhomboidal diamond lattice around a central Parler lozenge (*Raute*). |
| `GothicPattern::fan_conoid` | **Gloucester Cloisters / King's College Chapel** (English Perpendicular 2-Tier Fan Vault, c. 1370–1515) | `4` corner support blocks + `104` mitered `BeamCurved` ribs with `5` radiating conoid ribs per corner across `2` concentric collar tiers and a central spandrel star. |

| | | |
| :---: | :---: | :---: |
| ![](examples/templates_vault_9_gothic_ribbed_vault_quadripartite.webp)<br>`GothicPattern::quadripartite` (`groin_cylinders`) | ![](examples/templates_vault_9_gothic_ribbed_vault_tierceron.webp)<br>`GothicPattern::tierceron` (`spherical_sail`) | ![](examples/templates_vault_9_gothic_ribbed_vault_star.webp)<br>`GothicPattern::star_stellar` (`cloister_cylinders`) |
| ![](examples/templates_vault_9_gothic_ribbed_vault_lierne.webp)<br>`GothicPattern::lierne_star` (`spherical_sail`) | ![](examples/templates_vault_9_gothic_ribbed_vault_net.webp)<br>`GothicPattern::net_reticulated` (`spherical_sail`) | ![](examples/templates_vault_9_gothic_ribbed_vault_fan.webp)<br>`GothicPattern::fan_conoid` (`spherical_sail`) |

```cpp
#include "wood_session.h"
#include "src/templates/vault/vault.h"

using namespace wood_session;
using namespace wood_vault;

GothicRibbedVault vault(
    10000.0, 10000.0, 1000.0, 120.0, 360.0,
    GothicPattern::lierne_star,
    ArchMethod::spherical_sail
);
```

<details>
<summary>Example code: examples/templates_vault_9_gothic_ribbed_vault.cpp</summary>

\include{lineno} templates_vault_9_gothic_ribbed_vault.cpp

</details>

---

## 2. Star Vault with NURBS Sail Webs & `BeamCurved` Ribs (`star_vault`)

A stellar vault over an `8000 mm` square bay (`examples/templates_vault_8_star_vault.cpp`): `20` `BeamCurved` glulam ribs (4 diagonals, 8 tiercerons, 4 liernes, and 4 wall arches) supporting `12` NURBS `BRep` shell webs (`150 mm` thick) trimmed on a spherical sail surface.

![](templates/templates_vault_8_star_vault_all.webp)

<details>
<summary>Example code: examples/templates_vault_8_star_vault.cpp</summary>

\include{lineno} templates_vault_8_star_vault.cpp

</details>

---

## 3. Stereotomic Masonry Vaults (`arch`, `barrel`, `dome`, `wall`, `cross_vault`, `cloister_vault`)

Discrete `wood_session::Block` voussoir generators with automatic `compute_face_contacts(0)` bed- and head-joint contact detection:

| | | |
| :---: | :---: | :---: |
| ![](templates/templates_vault_1_arch_all.webp)<br>`arch` (`25` voussoirs, `24` contacts) | ![](templates/templates_vault_2_barrel_all.webp)<br>`barrel` (`58` voussoirs, `145` contacts) | ![](templates/templates_vault_3_barrel_staggered_all.webp)<br>`barrel` (`closed = true`, `76` voussoirs) |
| ![](templates/templates_vault_4_dome_all.webp)<br>`dome` (`800` voussoirs, `2320` contacts) | ![](templates/templates_vault_6_cross_vault_all.webp)<br>`cross_vault` (`184` voussoirs & groin stones) | ![](templates/templates_vault_7_cloister_vault_all.webp)<br>`cloister_vault` (`304` voussoirs, `584` contacts) |

<details>
<summary>Example code: examples/templates_vault_6_cross_vault.cpp</summary>

\include{lineno} templates_vault_6_cross_vault.cpp

</details>
