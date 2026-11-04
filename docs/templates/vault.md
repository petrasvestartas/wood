# Vault {#templates_vault}

[TOC]

Parametric ribbed timber vaults and stereotomic masonry vaults in `src/templates/vault/vault.h`. Every ribbed vault uses straight 2D plan lines rising as planar arches in 3D and simple circular barrel cylinder webs (`K = 0`) whose top and bottom edges follow the supporting beam centerlines.

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
  <a class="wood-card" href="#vault_star">
    <img src="templates_vault_8_star_vault_all.webp" alt="Star Vault">
    <span class="wood-card-title">Star Vault</span>
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
</div>

---

## Quadripartite Vault {#vault_quadripartite}

Four perimeter wall arches, diagonal groin ribs, and orthogonal ridge ribs over a 10 x 10 m bay with 8 cylindrical triangular web panels and 4 corner support blocks.

<div class="wood-pair-grid">
  <img src="templates_vault_9_gothic_ribbed_vault_quadripartite.webp" alt="Quadripartite Vault 3D view">
  <img src="templates_vault_9_gothic_ribbed_vault_quadripartite_bottom.webp" alt="Quadripartite Vault bottom beam pattern">
</div>

```cpp
#include "wood_session.h"
#include "src/templates/vault/vault.h"

int main() {
    wood_vault::GothicRibbedVault vault(
        10000.0, 10000.0, 1000.0, 120.0, 360.0,
        wood_vault::GothicPattern::quadripartite,
        wood_vault::ArchMethod::groin_cylinders
    );
    vault.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

---

## Tierceron Vault {#vault_tierceron}

Secondary tierceron ribs spring in straight plan lines from the 4 corner support blocks to mid-ridge bosses, dividing the two orthogonal barrel cylinders into 16 cylindrical web panels.

<div class="wood-pair-grid">
  <img src="templates_vault_9_gothic_ribbed_vault_tierceron.webp" alt="Tierceron Vault 3D view">
  <img src="templates_vault_9_gothic_ribbed_vault_tierceron_bottom.webp" alt="Tierceron Vault bottom beam pattern">
</div>

```cpp
#include "wood_session.h"
#include "src/templates/vault/vault.h"

int main() {
    wood_vault::GothicRibbedVault vault(
        10000.0, 10000.0, 1000.0, 120.0, 360.0,
        wood_vault::GothicPattern::tierceron,
        wood_vault::ArchMethod::groin_cylinders
    );
    vault.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

---

## Stellar Vault {#vault_star_stellar}

Four central diamond lozenges framed by interlocking diagonal ribs, corner tiercerons, and tertiary lierne chords across 26 planar curved ribs and 20 cylindrical web panels.

<div class="wood-pair-grid">
  <img src="templates_vault_9_gothic_ribbed_vault_star.webp" alt="Stellar Vault 3D view">
  <img src="templates_vault_9_gothic_ribbed_vault_star_bottom.webp" alt="Stellar Vault bottom beam pattern">
</div>

```cpp
#include "wood_session.h"
#include "src/templates/vault/vault.h"

int main() {
    wood_vault::GothicRibbedVault vault(
        10000.0, 10000.0, 1000.0, 120.0, 360.0,
        wood_vault::GothicPattern::star_stellar,
        wood_vault::ArchMethod::groin_cylinders
    );
    vault.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

---

## Lierne Star Vault {#vault_lierne_star}

Inner and outer star rings linked through intermediate diagonal bosses by 34 planar curved ribs, dividing the intersecting barrel cylinders into 32 cylindrical web panels.

<div class="wood-pair-grid">
  <img src="templates_vault_9_gothic_ribbed_vault_lierne.webp" alt="Lierne Star Vault 3D view">
  <img src="templates_vault_9_gothic_ribbed_vault_lierne_bottom.webp" alt="Lierne Star Vault bottom beam pattern">
</div>

```cpp
#include "wood_session.h"
#include "src/templates/vault/vault.h"

int main() {
    wood_vault::GothicRibbedVault vault(
        10000.0, 10000.0, 1000.0, 120.0, 360.0,
        wood_vault::GothicPattern::lierne_star,
        wood_vault::ArchMethod::groin_cylinders
    );
    vault.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

---

## Net Vault {#vault_net}

Reticulated diamond-lattice vault inspired by the Bern Cathedral choir, combining 34 planar curved ribs with interlocking cross-lap chords and 32 cylindrical web panels over a 10 x 10 m bay.

<div class="wood-pair-grid">
  <img src="templates_vault_9_gothic_ribbed_vault_net.webp" alt="Net Vault 3D view">
  <img src="templates_vault_9_gothic_ribbed_vault_net_bottom.webp" alt="Net Vault bottom beam pattern">
</div>

```cpp
#include "wood_session.h"
#include "src/templates/vault/vault.h"

int main() {
    wood_vault::GothicRibbedVault vault(
        10000.0, 10000.0, 1000.0, 120.0, 360.0,
        wood_vault::GothicPattern::net_reticulated,
        wood_vault::ArchMethod::groin_cylinders
    );
    vault.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

---

## Fan Vault {#vault_fan}

Straight-in-plan radiating fan ribs and polygonal collar ribs springing from the 4 corner support blocks across 50 planar curved ribs and 48 cylindrical web panels.

<div class="wood-pair-grid">
  <img src="templates_vault_9_gothic_ribbed_vault_fan.webp" alt="Fan Vault 3D view">
  <img src="templates_vault_9_gothic_ribbed_vault_fan_bottom.webp" alt="Fan Vault bottom beam pattern">
</div>

```cpp
#include "wood_session.h"
#include "src/templates/vault/vault.h"

int main() {
    wood_vault::GothicRibbedVault vault(
        10000.0, 10000.0, 1000.0, 120.0, 360.0,
        wood_vault::GothicPattern::fan_conoid,
        wood_vault::ArchMethod::groin_cylinders
    );
    vault.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

---

## Star Vault {#vault_star}

Stellar rib-and-shell vault over an 8 x 8 m bay with 18 planar curved ribs, 4 corner support blocks, and 16 cylindrical BRep web shells with face-to-face contact detection.

<div class="wood-pair-grid">
  <img src="templates_vault_8_star_vault_all.webp" alt="Star Vault 3D view">
  <img src="templates_vault_8_star_vault_bottom.webp" alt="Star Vault bottom beam pattern">
</div>

```cpp
#include "wood_session.h"
#include "src/templates/vault/vault.h"

int main() {
    wood_session::WoodSession session("vault_star_vault");
    for (const auto& element : wood_vault::star_vault(8000.0, 1000.0, 150.0, 0.5, 200.0, 8))
        session.add(element);
    session.compute_face_contacts(0);
    session.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

---

## Voussoir Arch {#vault_arch}

Circular arch of 6 m span and 3 m rise divided into 25 radial wedge voussoir blocks with 24 bed-joint contacts.

![](templates/templates_vault_1_arch_all.webp)

```cpp
#include "wood_session.h"
#include "src/templates/vault/vault.h"

int main() {
    wood_session::WoodSession session("vault_arch");
    for (const auto& element : wood_vault::arch(6000.0, 3000.0, 500.0, 500.0, 25))
        session.add(element);
    session.compute_face_contacts(0);
    session.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

---

## Barrel Vault {#vault_barrel}

Segmental circular tunnel vault extruded in running bond over 9 courses by 6 rings with half-ring closer voussoirs on odd courses.

![](templates/templates_vault_2_barrel_all.webp)

```cpp
#include "wood_session.h"
#include "src/templates/vault/vault.h"

int main() {
    wood_session::WoodSession session("vault_barrel");
    for (const auto& element : wood_vault::barrel(6000.0, 2000.0, 250.0, 6000.0, 9, 6, 0.5))
        session.add(element);
    session.compute_face_contacts(0);
    session.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

---

## Voussoir Dome {#vault_dome}

Hemispherical dome of 20 staggered hoops by 40 hexagonal voussoirs tapering from 400 mm at springing to 250 mm around a 6-degree oculus.

![](templates/templates_vault_4_dome_all.webp)

```cpp
#include "wood_session.h"
#include "src/templates/vault/vault.h"

int main() {
    wood_session::WoodSession session("vault_dome");
    for (const auto& element : wood_vault::dome(4000.0, 400.0, 250.0, 40, 20, 6.0, 90.0))
        session.add(element);
    session.compute_face_contacts(0);
    session.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

---

## Masonry Wall {#vault_wall}

Running-bond ashlar wall of 400 x 200 mm courses with half-brick closers and automatic bed- and head-joint contact detection.

![](templates/templates_vault_5_wall_all.webp)

```cpp
#include "wood_session.h"
#include "src/templates/vault/vault.h"

int main() {
    wood_session::WoodSession session("vault_wall");
    for (const auto& element : wood_vault::wall(4000.0, 2000.0, 200.0, 400.0, 200.0, 0.5))
        session.add(element);
    session.compute_face_contacts(0);
    session.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

---

## Cross Vault {#vault_cross}

Two semicircular barrel vaults intersecting over a 6 x 6 m bay with monolithic L-shaped groin stones bent across the bay diagonals.

![](templates/templates_vault_6_cross_vault_all.webp)

```cpp
#include "wood_session.h"
#include "src/templates/vault/vault.h"

int main() {
    wood_session::WoodSession session("vault_cross_vault");
    for (const auto& element : wood_vault::cross_vault(6000.0, 3000.0, 480.0, 10, 12))
        session.add(element);
    session.compute_face_contacts(0);
    session.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

---

## Cloister Vault {#vault_cloister}

Four cylindrical barrel webs rising from the perimeter walls of a 6 x 6 m bay and mitered along the vertical diagonal planes.

![](templates/templates_vault_7_cloister_vault_all.webp)

```cpp
#include "wood_session.h"
#include "src/templates/vault/vault.h"

int main() {
    wood_session::WoodSession session("vault_cloister_vault");
    for (const auto& element : wood_vault::cloister_vault(6000.0, 3000.0, 300.0, 8, 12))
        session.add(element);
    session.compute_face_contacts(0);
    session.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

## Examples {#vault_examples}

### 1 Arch {#vault_example_1}

A semicircular arch of 25 voussoirs over 6 m, 500 thick and 500 deep; compute_face_contacts(0) finds a contact between every two neighbours, 24 in all.

\include{lineno} templates_vault_1_arch.cpp

### 2 Barrel {#vault_example_2}

A segmental barrel vault over 6 m, 6 m long, nine courses across and six rings along it, every odd course shifted half a ring with part voussoirs at the ends; compute_face_contacts(0) pairs the courses along the bed joints and the rings along the head joints.

\include{lineno} templates_vault_2_barrel.cpp

### 4 Dome {#vault_example_4}

A hemispherical dome of 4 m intrados radius with an oculus, 20 hoops of 40 voussoirs, every odd hoop shifted half a voussoir and every voussoir a hexagon bending at the joints above and below so the hoops close without gaps, 400 thick at the springing tapering to 250 at the oculus; compute_face_contacts(0) pairs the voussoirs along their meridian joints and with the two voussoirs above and below.

\include{lineno} templates_vault_4_dome.cpp

### 5 Wall {#vault_example_5}

A running bond wall 4 m long and 2 m high of 400 by 200 bricks, half bricks closing every odd course; compute_face_contacts(0) pairs the bricks along the bed and head joints.

\include{lineno} templates_vault_5_wall.cpp

### 6 Cross Vault {#vault_example_6}

A cross (groin) vault over a 6 m square bay: two semicircular barrels crossing, every web ten courses per half arch laid in running bond over twelve rings, the course at the crown shifted half a ring, and in every course one groin stone bent along the diagonal through both webs out to the first joint past the groin, the lowest a springer: 144 voussoirs and 40 groin stones, 184 blocks like the reference.

\include{lineno} templates_vault_6_cross_vault.cpp

### 7 Cloister Vault {#vault_example_7}

A cloister (pavilion) vault over a 6 m square bay: four webs rising from the four walls, each eight courses up to the crown by twelve rings along its wall, cut on the bay diagonals where the webs meet.

\include{lineno} templates_vault_7_cloister_vault.cpp

### 8 Star Vault {#vault_example_8}

A star (stellar) vault over an 8 m square bay (1 m rise) on intersecting circular barrel cylinders: eighteen single-curved planar ribs in vertical planes - two interlocking continuous diagonals, eight tiercerons from the corners to the star points, four continuous ridge ribs from the wall crowns through the star bosses to the central boss, and four wall arches - four corner support blocks, and the sixteen cylindrical webs between them, each 150 thick: a BRep solid whose intrados is a circular barrel cylinder trimmed along straight plan lines at the beam centers, whose extrados is shifted vertically by thickness and whose sides are vertical ruled surfaces neighbouring webs share.

\include{lineno} templates_vault_8_star_vault.cpp

### 9 Gothic Ribbed Vault {#vault_example_9}

Six symmetric shallow (1:10 rise-to-span, 10 m x 10 m span, 1 m rise) Gothic cathedral ribbed vault bays built from single-curved planar glulam beams (BeamCurved, 120 x 360 mm, 6 lamellae) in vertical planes over straight 2D plan lines with exact angle-bisector node miter cuts (rib-\>cuts), 4 polygonal corner support blocks (Block) where ribs and web panels terminate cleanly, and simple cylindrical barrel web panels (BRep, K = 0 groin_cylinders) whose bottom and top edges lie directly above the supporting beam centerlines (Bern Cathedral / Berner Muenster; compas_tno; Tracing the Past LiDAR survey, Buchanan, Webb & Peterson 2021; Wendland et al. 2024; Jobbik 2024):
1. quadripartite_wells    — Classic quadripartite cross vault on intersecting barrel cylinders (Bern Cathedral / Wells / Salisbury, 4 blocks + 10 ribs + 8 cylindrical web panels = 22 elements)
2. tierceron_exeter       — 4-pointed starcut tierceron vault on intersecting barrel cylinders (Bern Cathedral / Lincoln / Exeter, 4 blocks + 18 ribs + 16 cylindrical web panels = 38 elements)
3. star_stellar_burgos    — 4-lozenge Greek-cross stellar vault (Sterngewoelbe / Boveda Estrellada) on intersecting barrel cylinders (Bern Cathedral, 4 blocks + 26 ribs + 20 cylindrical web panels = 50 elements)
4. lierne_star_ely        — Double-star tierceron-lierne vault on intersecting barrel cylinders (Bern Cathedral / Ely Choir / Wells Lady Chapel, 4 blocks + 34 ribs + 32 cylindrical web panels = 70 elements)
5. net_reticulated_parler — Reticulated diamond-lattice net vault (Netzgewoelbe) on intersecting barrel cylinders (Bern Cathedral Choir / Parler, 4 blocks + 34 ribs + 32 cylindrical web panels = 70 elements)
6. fan_conoid_gloucester  — Radiating fan vault with straight horizontal collar ribs along cylinder rulings (compas_tno / Gloucester, 4 blocks + 50 ribs + 48 cylindrical web panels = 102 elements)

\include{lineno} templates_vault_9_gothic_ribbed_vault.cpp
