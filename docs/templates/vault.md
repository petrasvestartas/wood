# Vault {#templates_vault}

[TOC]

`wood_vault` (`src/templates/vault/vault.h`) generates parametric **Gothic ribbed timber vaults** (`GothicRibbedVault` / `gothic_ribbed_vault`) built from single-curved glulam ribs (`wood_session::BeamCurved`), corner support blocks (`wood_session::Block`), and simple cylindrical shell web panels (`vault_panel`), together with **stellar cylindrical rib & shell vaults** (`star_vault`) and **stereotomic masonry vaults** (`arch`, `barrel`, `dome`, `wall`, `cross_vault`, `cloister_vault`) inside `WoodSession`.

Click any vault card below to jump directly to its full-size image, geometric rules, and C++ example code:

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#vault_gothic">
    <img src="templates_vault_9_gothic_ribbed_vault_all.webp" alt="Gothic Ribbed Timber Vaults">
    <span class="wood-card-title">1. Gothic Ribbed Vaults</span>
    <span class="wood-card-sub">6 historic rib-plan topologies with cylindrical web panels</span>
  </a>
  <a class="wood-card" href="#vault_quadripartite">
    <img src="templates_vault_9_gothic_ribbed_vault_quadripartite.webp" alt="Quadripartite Cross Vault">
    <span class="wood-card-title">1.1 Quadripartite Cross Vault</span>
    <span class="wood-card-sub">Bern Cathedral side aisle / Wells (`GothicPattern::quadripartite`)</span>
  </a>
  <a class="wood-card" href="#vault_tierceron">
    <img src="templates_vault_9_gothic_ribbed_vault_tierceron.webp" alt="Tierceron Star Vault">
    <span class="wood-card-title">1.2 Tierceron Star Vault</span>
    <span class="wood-card-sub">Bern Cathedral / Exeter (`GothicPattern::tierceron`)</span>
  </a>
  <a class="wood-card" href="#vault_star_stellar">
    <img src="templates_vault_9_gothic_ribbed_vault_star.webp" alt="Four-Lozenge Stellar Vault">
    <span class="wood-card-title">1.3 Four-Lozenge Stellar Vault</span>
    <span class="wood-card-sub">Bern Cathedral angel-boss star vault (`GothicPattern::star_stellar`)</span>
  </a>
  <a class="wood-card" href="#vault_lierne_star">
    <img src="templates_vault_9_gothic_ribbed_vault_lierne.webp" alt="Double-Lozenge Lierne Star Vault">
    <span class="wood-card-title">1.4 Double-Lozenge Lierne Vault</span>
    <span class="wood-card-sub">Bern Cathedral / Ely Choir (`GothicPattern::lierne_star`)</span>
  </a>
  <a class="wood-card" href="#vault_net">
    <img src="templates_vault_9_gothic_ribbed_vault_net.webp" alt="Reticulated Diamond-Lattice Net Vault">
    <span class="wood-card-title">1.5 Reticulated Net Vault</span>
    <span class="wood-card-sub">Bern Cathedral choir &amp; nave *Netzgewölbe* (`GothicPattern::net_reticulated`)</span>
  </a>
  <a class="wood-card" href="#vault_fan">
    <img src="templates_vault_9_gothic_ribbed_vault_fan.webp" alt="Radiating Fan / Conoid Vault">
    <span class="wood-card-title">1.6 Radiating Fan / Conoid Vault</span>
    <span class="wood-card-sub">Gloucester Cloisters / King's College (`GothicPattern::fan_conoid`)</span>
  </a>
  <a class="wood-card" href="#vault_star">
    <img src="templates_vault_8_star_vault_all.webp" alt="Stellar Cylindrical Rib &amp; Shell Vault">
    <span class="wood-card-title">2. Stellar Cylindrical Vault</span>
    <span class="wood-card-sub">20 planar ribs + 16 cylindrical webs (`star_vault`)</span>
  </a>
  <a class="wood-card" href="#vault_arch">
    <img src="templates_vault_1_arch_all.webp" alt="Voussoir Arch">
    <span class="wood-card-title">3. Voussoir Arch</span>
    <span class="wood-card-sub">25 radial voussoirs over 6 m span (`arch`)</span>
  </a>
  <a class="wood-card" href="#vault_barrel">
    <img src="templates_vault_2_barrel_all.webp" alt="Barrel Vault">
    <span class="wood-card-title">4. Barrel Vault</span>
    <span class="wood-card-sub">Running-bond segmental tunnel (`barrel`)</span>
  </a>
  <a class="wood-card" href="#vault_barrel_staggered">
    <img src="templates_vault_3_barrel_staggered_all.webp" alt="Closed Staggered Barrel Vault">
    <span class="wood-card-title">5. Closed Staggered Barrel</span>
    <span class="wood-card-sub">Staggered courses with aligned end rings (`barrel`, `closed = true`)</span>
  </a>
  <a class="wood-card" href="#vault_dome">
    <img src="templates_vault_4_dome_all.webp" alt="Spherical Voussoir Dome">
    <span class="wood-card-title">6. Spherical Voussoir Dome</span>
    <span class="wood-card-sub">20 staggered hoops of 40 hexagonal voussoirs (`dome`)</span>
  </a>
  <a class="wood-card" href="#vault_wall">
    <img src="templates_vault_5_wall_all.webp" alt="Running-Bond Masonry Wall">
    <span class="wood-card-title">7. Running-Bond Wall</span>
    <span class="wood-card-sub">Staggered ashlar courses with half-brick closers (`wall`)</span>
  </a>
  <a class="wood-card" href="#vault_cross">
    <img src="templates_vault_6_cross_vault_all.webp" alt="Stereotomic Cross (Groin) Vault">
    <span class="wood-card-title">8. Stereotomic Cross Vault</span>
    <span class="wood-card-sub">Intersecting barrels + L-shaped groin stones (`cross_vault`)</span>
  </a>
  <a class="wood-card" href="#vault_cloister">
    <img src="templates_vault_7_cloister_vault_all.webp" alt="Stereotomic Cloister (Pavilion) Vault">
    <span class="wood-card-title">9. Stereotomic Cloister Vault</span>
    <span class="wood-card-sub">Four cylindrical gores mitered on bay diagonals (`cloister_vault`)</span>
  </a>
</div>

![](diagrams/template_vault_pipeline.webp)

---

## 1. Gothic Ribbed Timber Vaults {#vault_gothic}

`wood_vault::GothicRibbedVault` and `wood_vault::gothic_ribbed_vault` generate shallow, symmetric Gothic ribbed vaults (`1:10` rise-to-span ratio by default: `10000 × 10000 mm` bay, `1000 mm` crown rise) from single-curved glulam ribs (`wood_session::BeamCurved`, `120 × 360 mm`, `6` lamellae), `4` polygonal corner support blocks (`wood_session::Block`, `support_block`), and simple cylindrical NURBS `BRep` web panels (`vault_panel`).

### Core Geometric Rules

1. **Straight 2D Plan Lines Rising as Planar Arches in 3D**:
   Every rib (`wall_arch`, `diagonal_rib`, `ridge_rib`, `tierceron_rib`, `lierne_rib`, `net_rib`, `fan_rib`, `collar_rib`) is defined on the horizontal bay domain `[-span_x/2, span_x/2] × [-span_y/2, span_y/2]` as a **straight 2D line segment** `p(t) = (1 - t)*p_0 + t*p_1`. In 3D, each straight plan segment rises as a **planar arch curve** inside the vertical plane over that segment. Every `BeamCurved` cross-section `up` vector lies strictly in the rib's vertical plane, so every glulam rib is single-curved with **zero sideways twist**.
2. **Simple Cylindrical Web Surfaces (`K = 0`, Zero Doubly-Curved Patches)**:
   No vault web patch is doubly curved. The bay is divided by its two diagonal groin planes `|x|/h_x = |y|/h_y` into four quadrants, each roofed by a **simple circular barrel cylinder** (`K = 0`, developable surface):
   - **North & South webs** (`|y|/h_y >= |x|/h_x`): a circular barrel cylinder whose straight rulings run parallel to the North–South (`Y`) axis and whose arch section in `X` rises from `z = 0` at `x = ±h_x` to the crown height `rise`.
   - **East & West webs** (`|x|/h_x > |y|/h_y`): an orthogonal circular barrel cylinder whose straight rulings run parallel to the East–West (`X`) axis and whose arch section in `Y` rises from `z = 0` at `y = ±h_y` to `rise`.
   Along the diagonal groins `|x|/h_x = |y|/h_y`, the two orthogonal cylinders meet along a single continuous 3D groin arch. Each rib's centerline is offset downward by `-0.5 * rib_height` so its **top extrados face lies flush along the cylindrical web surface**.
3. **Web Panel Top & Bottom Edges Directly Follow the Supporting Beam Centerlines**:
   Each polygonal cell of the 2D planar rib graph produces a closed `BRep` shell panel (`vault_panel`) whose intrados is trimmed on the quadrant's barrel cylinder and whose extrados is shifted vertically by `panel_thickness`. Because every rib lies in the vertical plane over its straight 2D plan segment and the panel side faces are vertical ruled surfaces over those same plan segments, **both the bottom edge and the top edge of every web panel lie directly above the supporting beam centerline**, and every corner cell stops flush against the facets of the 4 polygonal corner support blocks (`support_block`).

![](examples/templates_vault_9_gothic_ribbed_vault_all.webp)

### Constructor & Function Signatures

```cpp
enum class GothicPattern {
    quadripartite,   // Classic quadripartite cross vault (Bern Cathedral / Wells / Salisbury / Amiens Nave)
    tierceron,       // Starcut tierceron star vault (Bern Cathedral / Lincoln / Exeter Nave)
    star_stellar,    // 4-lozenge stellar star vault (Bern Cathedral / Sterngewoelbe / Boveda Estrellada)
    lierne_star,     // Double-star lierne vault (Bern Cathedral / Ely Choir / Wells Lady Chapel)
    net_reticulated, // Late Gothic reticulated diamond-lattice net vault (Bern Cathedral Choir / Parler / Netzgewoelbe)
    fan_conoid,      // Perpendicular radiating fan / conoid vault (Gloucester Cloisters / King's College)
};

std::vector<std::shared_ptr<Element>> gothic_ribbed_vault(
    double span_x = 10000.0,
    double span_y = 10000.0,
    double rise = 1000.0,
    double rib_width = 120.0,
    double rib_height = 360.0,
    GothicPattern pattern = GothicPattern::lierne_star,
    ArchMethod surface_method = ArchMethod::groin_cylinders,
    int subdivisions = 16
);

GothicRibbedVault(
    double span_x = 10000.0,
    double span_y = 10000.0,
    double rise = 1000.0,
    double rib_width = 120.0,
    double rib_height = 360.0,
    GothicPattern pattern = GothicPattern::lierne_star,
    ArchMethod arch_method = ArchMethod::groin_cylinders,
    int subdivisions = 16,
    const std::string& name = "gothic_ribbed_vault"
);
```

| Parameter | Type / Default | Description |
| :--- | :--- | :--- |
| `span_x`, `span_y` | `double` (`10000.0`, `10000.0`) | Clear rectangular bay span (`x`, `y`) in `mm` (`[-span_x/2, span_x/2] × [-span_y/2, span_y/2]`). |
| `rise` | `double` (`1000.0`) | Crown height (`mm`) of the intersecting barrel cylinders at `(0, 0, rise)`. |
| `rib_width`, `rib_height` | `double` (`120.0`, `360.0`) | Cross-section width and depth (`mm`) of the planar-arch `BeamCurved` glulam ribs (`6` lamellae). |
| `pattern` | `GothicPattern` | `quadripartite`, `tierceron`, `star_stellar`, `lierne_star`, `net_reticulated`, or `fan_conoid`. |
| `surface_method` | `ArchMethod` (`groin_cylinders`) | Cylindrical groin barrel surface selector. |
| `subdivisions` | `int` (`16`) | Interpolation stations per `BeamCurved` rib and cylindrical `vault_panel` edge. |

---

### 1.1 Quadripartite Cross Vault {#vault_quadripartite}

**Historical prototype**: Bern Cathedral (*Berner Münster*) side aisles / Wells & Salisbury Cathedral naves (`23` elements: `4` corner `support_block`s + `11` `BeamCurved` ribs + `8` cylindrical `vault_panel`s).

The four-part cross vault is formed by **11 continuous `BeamCurved` ribs** in vertical planes over straight 2D plan lines: `4` unbroken full-span perimeter wall arches (`wall_arch`), `3` diagonal ribs (`diagonal_rib`: `1` unbroken primary SW–NE diagonal ogive `{0, 8, 2}` and `2` secondary SE/NW half-diagonals `{1, 8}`, `{3, 8}` T-butting at the central crown boss `(0, 0)`), and `4` orthogonal ridge ribs (`ridge_rib`) linking the wall midpoints to the crown. In plan, these straight rib segments divide the bay into **8 cylindrical triangular web panels (`vault_panel`)**—two per cardinal barrel cylinder (`surf_ns` and `surf_ew`)—whose top and bottom edges follow the supporting beam centerlines and stop flush against the 4 corner `support_block` plinths.

![](examples/templates_vault_9_gothic_ribbed_vault_quadripartite.webp)

```cpp
wood_vault::GothicRibbedVault vault(
    10000.0, 10000.0, 1000.0, 120.0, 360.0,
    wood_vault::GothicPattern::quadripartite,
    wood_vault::ArchMethod::groin_cylinders
);
```

---

### 1.2 Tierceron Star Vault {#vault_tierceron}

**Historical prototype**: Bern Cathedral 4-pointed star / tierceron vault & Lincoln / Exeter Cathedral naves (`39` elements: `4` corner `support_block`s + `19` `BeamCurved` ribs + `16` cylindrical `vault_panel`s).

The Bern Cathedral 4-pointed starcut tierceron vault is formed by **19 continuous `BeamCurved` ribs**: `4` full-span perimeter wall arches (`wall_arch`), `3` diagonal ribs (`diagonal_rib`: `1` unbroken SW–NE diagonal and `2` SE/NW half-diagonals), `4` orthogonal ridge ribs (`ridge_rib`), and **8 straight-in-plan secondary tierceron ribs (`tierceron_rib`)** springing from the 4 corner support blocks to the mid-ridge star bosses at `(0, ±0.5 * hy)` and `(±0.5 * hx, 0)`. Every tierceron rises in its vertical plane to meet the cylindrical barrel vault surface and trims flush against the continuous ridge rib and corner block, partitioning the North/South and East/West barrel cylinders into **16 cylindrical triangular web panels (`vault_panel`)** whose top and bottom edges follow the rib centerlines directly.

![](examples/templates_vault_9_gothic_ribbed_vault_tierceron.webp)

```cpp
wood_vault::GothicRibbedVault vault(
    10000.0, 10000.0, 1000.0, 120.0, 360.0,
    wood_vault::GothicPattern::tierceron,
    wood_vault::ArchMethod::groin_cylinders
);
```

---

### 1.3 Four-Lozenge Stellar Vault {#vault_star_stellar}

**Historical prototype**: Bern Cathedral 4-lozenge Greek-cross stellar vault (*Sterngewölbe*) / Burgos *Bóveda Estrellada* (`51` elements: `4` corner `support_block`s + `27` `BeamCurved` ribs + `20` cylindrical `vault_panel`s).

The Bern Cathedral 4-lozenge Greek-cross stellar vault places intermediate bosses at `(±0.36 * hx, ±0.36 * hy)` along the diagonal ogives and `(0, ±0.62 * hy)`, `(±0.62 * hx, 0)` along the cardinal axes, built from **27 continuous `BeamCurved` ribs** (`4` full-span `wall_arch`es, `3` `diagonal_rib`s, `4` outer `ridge_rib`s, `8` corner `tierceron_rib`s, and `8` tertiary `lierne_rib` chords). The liernes and inner diagonal segments frame **4 central diamond lozenges meeting at the crown `(0, 0)`** surrounded by `16` outer triangular cells—a total of **20 cylindrical web panels (`vault_panel`)** on `surf_ns` and `surf_ew` whose top and bottom edges follow the beam centerlines directly.

![](examples/templates_vault_9_gothic_ribbed_vault_star.webp)

```cpp
wood_vault::GothicRibbedVault vault(
    10000.0, 10000.0, 1000.0, 120.0, 360.0,
    wood_vault::GothicPattern::star_stellar,
    wood_vault::ArchMethod::groin_cylinders
);
```

---

### 1.4 Double-Lozenge Lierne Star Vault {#vault_lierne_star}

**Historical prototype**: Bern Cathedral / Ely Cathedral Choir & Wells Lady Chapel double-star tierceron-lierne vault (`71` elements: `4` corner `support_block`s + `35` `BeamCurved` ribs + `32` cylindrical `vault_panel`s).

The double-star tierceron-lierne vault combines an inner star ring (`±0.34` along the ridges) and an outer star ring (`±0.68` along the ridges) connected through diagonal bosses (`±0.48`) by **35 continuous `BeamCurved` ribs** (`4` full-span `wall_arch`es, `3` `diagonal_rib`s, `4` continuous `ridge_rib`s, `8` corner `tierceron_rib`s, and `16` `lierne_rib` chords). These straight plan segments partition the orthogonal barrel cylinders into **32 cylindrical triangular web panels (`vault_panel`)** (`8` per barrel cylinder quadrant) whose top and bottom boundary curves follow the centerlines of their bounding ribs.

![](examples/templates_vault_9_gothic_ribbed_vault_lierne.webp)

```cpp
wood_vault::GothicRibbedVault vault(
    10000.0, 10000.0, 1000.0, 120.0, 360.0,
    wood_vault::GothicPattern::lierne_star,
    wood_vault::ArchMethod::groin_cylinders
);
```

---

### 1.5 Reticulated Net Vault {#vault_net}

**Historical prototype**: Bern Cathedral (*Berner Münster*) Choir reticulated net vault (*Netzgewölbe*, Matthäus Ensinger & Erhard Küng) / Parler school (`79` elements: `4` corner `support_block`s + `43` `BeamCurved` ribs + `32` cylindrical `vault_panel`s).

The Bern Cathedral Choir reticulated net vault (*Netzgewölbe*) weaves **43 continuous `BeamCurved` ribs** (`4` full-span `wall_arch`es, `3` `diagonal_rib`s through `±0.52` and `±0.24`, `4` outer `ridge_rib`s to `±0.62`, `8` corner `tierceron_rib`s, and `24` `net_rib` lattice chords through inner ridge nodes at `±0.30`). Every cell—including the central and secondary diamond lozenges (*Rauten*) and side lunettes—lies strictly within a single circular barrel cylinder quadrant (`surf_ns` or `surf_ew`), producing **32 cylindrical `vault_panel`s** whose top and bottom edges follow the supporting beam centerlines.

![](examples/templates_vault_9_gothic_ribbed_vault_net.webp)

```cpp
wood_vault::GothicRibbedVault vault(
    10000.0, 10000.0, 1000.0, 120.0, 360.0,
    wood_vault::GothicPattern::net_reticulated,
    wood_vault::ArchMethod::groin_cylinders
);
```

---

### 1.6 Radiating Fan Vault {#vault_fan}

**Historical prototype**: `compas_tno` fan form diagram on intersecting circular barrel cylinders / Gloucester Cathedral Cloisters (`103` elements: `4` corner `support_block`s + `51` `BeamCurved` ribs + `48` cylindrical `vault_panel`s).

Following the `compas_tno` straight-in-plan fan vault topology on intersecting circular barrel cylinders, each of the 4 corner support blocks radiates straight-in-plan fan ribs (`16` `fan_rib`s to ridge nodes at `±0.33` and `±0.66`) between `4` unbroken perimeter `wall_arch`es, `3` `diagonal_rib`s, and `4` continuous `ridge_rib`s, crossed at radial distance `r_col = 0.68 * min(h_x, h_y)` from each corner by **24 straight polygonal collar ribs (`collar_rib`)** (`51` `BeamCurved` ribs in total). These straight plan segments bound **48 cylindrical triangular and trapezoidal `vault_panel` cells** (`12` per barrel cylinder quadrant) whose top and bottom borders trace the beam centerlines and corner block facets directly.

![](examples/templates_vault_9_gothic_ribbed_vault_fan.webp)

```cpp
wood_vault::GothicRibbedVault vault(
    10000.0, 10000.0, 1000.0, 120.0, 360.0,
    wood_vault::GothicPattern::fan_conoid,
    wood_vault::ArchMethod::groin_cylinders
);
```

<details><summary>Example code: examples/templates_vault_9_gothic_ribbed_vault.cpp</summary>

\include{lineno} templates_vault_9_gothic_ribbed_vault.cpp

</details>

---

## 2. Stellar Cylindrical Vault {#vault_star}

`wood_vault::star_vault(span, rise, thickness, star, rib, subdivisions)` builds a stellar rib-and-shell vault over an `8000 × 8000 mm` square bay (`1000 mm` crown rise) on two orthogonal intersecting circular barrel cylinders (`40` elements, `25` contacts):
- **20 single-curved planar ribs (`BeamCurved`)** in vertical planes over straight 2D plan segments (4 diagonal ribs, 8 corner-to-star tierceron ribs, 4 full-span ridge ribs from wall crowns through the star bosses to the central boss, and 4 perimeter wall arches), plus **4 corner support blocks (`support_block`)**.
- **16 cylindrical `BRep` shell webs (`web`, `150 mm` thick)** whose intrados is a circular barrel cylinder trimmed along the straight plan lines at the beam centerlines, whose extrados is shifted vertically by `thickness`, and whose vertical ruled side faces meet directly along the beam centerlines so `compute_face_contacts(0)` detects all `25` inter-element contacts.

![](templates/templates_vault_8_star_vault_all.webp)

```cpp
std::vector<std::shared_ptr<Element>>
star_vault(double span, double rise, double thickness, double star, double rib, int subdivisions);
```

<details><summary>Example code: examples/templates_vault_8_star_vault.cpp</summary>

\include{lineno} templates_vault_8_star_vault.cpp

</details>

---

## 3. Voussoir Arch {#vault_arch}

`wood_vault::arch(span, rise, thickness, depth, voussoirs)` divides a circular arc of clear `span` (`6000 mm`) and `rise` (`3000 mm`) into `25` wedge-shaped `wood_session::Block` voussoirs (`500 mm` radial thickness, `500 mm` depth along `y`). `compute_face_contacts(0)` detects all `24` radial bed-joint contacts between consecutive voussoirs.

![](templates/templates_vault_1_arch_all.webp)

```cpp
std::vector<std::shared_ptr<Element>>
arch(double span, double rise, double thickness, double depth, int voussoirs);
```

<details><summary>Example code: examples/templates_vault_1_arch.cpp</summary>

\include{lineno} templates_vault_1_arch.cpp

</details>

---

## 4. Barrel Vault {#vault_barrel}

`wood_vault::barrel(span, rise, thickness, length, courses, rings, stagger, closed)` extrudes a segmental circular arch (`6000 mm` span, `2000 mm` rise, `250 mm` thickness) over `length = 6000 mm` in `9` arch courses by `6` longitudinal rings (`58` voussoirs, `145` contacts). With `stagger = 0.5` and `closed = false`, odd courses shift by half a ring and terminate with half-ring closer voussoirs at both ends.

![](templates/templates_vault_2_barrel_all.webp)

```cpp
std::vector<std::shared_ptr<Element>> barrel(
    double span,
    double rise,
    double thickness,
    double length,
    int courses,
    int rings,
    double stagger = 0.5,
    bool closed = false
);
```

<details><summary>Example code: examples/templates_vault_2_barrel.cpp</summary>

\include{lineno} templates_vault_2_barrel.cpp

</details>

---

## 5. Closed Staggered Barrel Vault {#vault_barrel_staggered}

Calling `wood_vault::barrel` with `stagger = 0.2, closed = true` shifts odd courses by one-fifth of a ring and adds an aligned closing ring at both ends (`76` voussoirs, `199` bed- and head-joint contacts).

![](templates/templates_vault_3_barrel_staggered_all.webp)

<details><summary>Example code: examples/templates_vault_3_barrel_staggered.cpp</summary>

\include{lineno} templates_vault_3_barrel_staggered.cpp

</details>

---

## 6. Spherical Voussoir Dome {#vault_dome}

`wood_vault::dome(radius, bottom, top, meridians, hoops, oculus, springing)` builds a hemispherical dome (`4000 mm` intrados radius, `20` hoops of `40` voussoirs = `800` blocks, `2320` contacts) tapering from `400 mm` thickness at `springing = 90°` to `250 mm` at `oculus = 6°`. Odd hoops shift by half a meridian step, and each voussoir is lofted as a 6-corner hexagonal block that bends at the mid-meridian joints of the hoops above and below so all faces meet without gaps.

![](templates/templates_vault_4_dome_all.webp)

```cpp
std::vector<std::shared_ptr<Element>> dome(
    double radius,
    double bottom,
    double top,
    int meridians,
    int hoops,
    double oculus,
    double springing
);
```

<details><summary>Example code: examples/templates_vault_4_dome.cpp</summary>

\include{lineno} templates_vault_4_dome.cpp

</details>

---

## 7. Running-Bond Masonry Wall {#vault_wall}

`wood_vault::wall(length, height, thickness, brick, course, stagger)` lays a running-bond ashlar wall (`4000 × 2000 × 200 mm`) of `400 × 200 mm` courses with `stagger = 0.5` and half-brick closers at the ends of odd courses (`105` bricks, `285` bed- and head-joint contacts).

![](templates/templates_vault_5_wall_all.webp)

```cpp
std::vector<std::shared_ptr<Element>> wall(
    double length,
    double height,
    double thickness,
    double brick,
    double course,
    double stagger = 0.5
);
```

<details><summary>Example code: examples/templates_vault_5_wall.cpp</summary>

\include{lineno} templates_vault_5_wall.cpp

</details>

---

## 8. Stereotomic Cross Vault {#vault_cross}

`wood_vault::cross_vault(span, rise, thickness, courses, rings)` intersects two semicircular barrel vaults over a `6000 × 6000 mm` square bay (`3000 mm` rise, `480 mm` thickness, `10` courses per half-arch across `12` rings). Each course places a monolithic L-shaped `groin` stone bent across the diagonal intersection of both cylindrical webs (`144` regular voussoirs + `40` groin stones = `184` blocks).

![](templates/templates_vault_6_cross_vault_all.webp)

```cpp
std::vector<std::shared_ptr<Element>>
cross_vault(double span, double rise, double thickness, int courses, int rings);
```

<details><summary>Example code: examples/templates_vault_6_cross_vault.cpp</summary>

\include{lineno} templates_vault_6_cross_vault.cpp

</details>

---

## 9. Stereotomic Cloister Vault {#vault_cloister}

`wood_vault::cloister_vault(span, rise, thickness, courses, rings)` raises four cylindrical barrel webs inward from the four walls of a `6000 × 6000 mm` square bay (`3000 mm` rise, `300 mm` thickness, `8` courses by `12` rings) and trims each voussoir along the two vertical diagonal planes `x = ±y` (`block->cuts`) where the four cylindrical gores meet (`304` voussoirs, `584` contacts).

![](templates/templates_vault_7_cloister_vault_all.webp)

```cpp
std::vector<std::shared_ptr<Element>>
cloister_vault(double span, double rise, double thickness, int courses, int rings);
```

<details><summary>Example code: examples/templates_vault_7_cloister_vault.cpp</summary>

\include{lineno} templates_vault_7_cloister_vault.cpp

</details>
