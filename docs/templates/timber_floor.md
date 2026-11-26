# Timber Floors {#templates_timber_floor}

[TOC]

Timber floors as five structures on two beams, sized for a span and its loads by the book's tables or by a producer's own equations, all the layers on them as one (`src/templates/timber_floor/`).

<div class="wood-card-grid cols-3">
  <a class="wood-card" href="#timber_floor_structures">
    <img src="templates_timber_floor_1_structures_section_a.webp" alt="Structures">
    <span class="wood-card-title">Structures</span>
  </a>
  <a class="wood-card" href="#timber_floor_sizing">
    <img src="templates_timber_floor_2_spans_all.webp" alt="Sections from the Span">
    <span class="wood-card-title">Sections from the Span</span>
  </a>
  <a class="wood-card" href="#timber_floor_joints">
    <img src="templates_timber_floor_3_joints_edges.webp" alt="Bearings and Joints">
    <span class="wood-card-title">Bearings and Joints</span>
  </a>
  <a class="wood-card" href="#timber_floor_earth">
    <img src="templates_timber_floor_4_earth_open.webp" alt="Timber and Earth">
    <span class="wood-card-title">Timber and Earth</span>
  </a>
  <a class="wood-card" href="#timber_floor_build_ups">
    <img src="templates_timber_floor_1_structures_all.webp" alt="Structure and Layers">
    <span class="wood-card-title">Structure and Layers</span>
  </a>
  <a class="wood-card" href="#timber_floor_assembly_sequence">
    <img src="assembly_templates_timber_floor_1_structures_box.gif" alt="Assembly Sequence">
    <span class="wood-card-title">Assembly Sequence</span>
  </a>
</div>

## Parameters {#timber_floor_parameters}

| Parameter | Default | Description |
| --- | --- | --- |
| `span`, `width` | `5000.0`, `2400.0` | Between the middles of the two beams, and across |
| `rib_width`, `rib_depth`, `rib_spacing` | `0.0`, `0.0`, `600.0` | Joists or ribs along the span; a depth of `0` for none |
| `flange_width`, `flange_depth` | `0.0`, `0.0` | Flanges of an I-joist, the rib its web |
| `top`, `bottom`, `bottom_width` | `0.0`, `0.0`, `0.0` | Board on and under the ribs; a strip under every rib when it has a width |
| `panel`, `panel_width`, `edge` | `0.0`, `0.0`, `0` | Panels side by side and their edges: butt, tongue in groove, strip, board in rebates, key |
| `lamella`, `plies`, `skin`, `web_spacing` | `0.0`, `0`, `0.0`, `200.0` | Boards on edge, cross-laminated layers, or the shells and webs of a hollow panel |
| `concrete`, `connector`, `notches` | `0.0`, `1`, `3` | Concrete on the timber, held by glue, notches, crossed pins or steel locks |
| `layers` | `0.0` | All the layers on the structure as one: `LAYERS_LIGHT` 60, `LAYERS_HEAVY` 110 |
| `seat`, `beam_width`, `beam_depth` | `0`, `160.0`, `280.0` | On the beams, in their rebates or on battens; the two beams |

---

## Structures {#timber_floor_structures}

Every floor is one of five structures: joists, ribs under a panel, a box, a solid slab, timber and concrete. One bay of each for a home over 5 m, sized by the book's tables (`TimberFloorGuide::joists`, `box`, `hollow`, `dowelled`, `composite`), all the layers on it as one:

![](templates/templates_timber_floor_1_structures_all.webp)

Joists, ribbed, box, hollow, dowelled, cross-laminated, timber and concrete:

![](templates/templates_timber_floor_1_structures_section_a.webp)

![](templates/templates_timber_floor_1_structures_section_b.webp)

| Structure | Carries | Good at | Costs |
| --- | --- | --- | --- |
| Joists | one way | light, adjusted to any load, services between them | depth, time on site |
| Ribs, box | one way | little timber, shallow, ready to walk on | gluing in the shop |
| Solid: boards on edge, planks, cross-laminated | one or two ways | shallow, simple, seen from below; cross-laminated a stiff plate | timber, swelling across the boards |
| Timber and concrete | one way | stiff, quiet, long spans | weight, wet work or a crane |

### Sizes {#timber_floor_sizes}

Single spans in mm, deflection with creep up to L / 500, C24 or GL24h, a light build-up 0.8 kN/m², a heavy one 1.8; for a first size. Source: Kolb, Kolb, Müller, *Holzbau mit System*, 2024, tables 8 to 10, p. 311 to 312.

| Use kN/m² | Span m | Build-up | Joists at 600 | Box ribs at 500 | Dowelled | Hollow | Timber + concrete |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Homes, 2 | 4 | 0.8 | 120 x 220 | 80 x 100 | 130 | 140 | 100 + 80 |
|  | 4 | 1.8 | 120 x 240 | 80 x 120 | 150 | 160 | 100 + 80 |
|  | 5 | 0.8 | 140 x 260 | 80 x 160 | 170 | 180 | 100 + 80 |
|  | 5 | 1.8 | 160 x 280 | 80 x 180 | 180 | 200 | 100 + 80 |
|  | 6 | 0.8 | 120 x 320 | 80 x 200 | 200 | 220 | 120 + 100 |
|  | 6 | 1.8 | 120 x 360 | 80 x 240 | 220 | 240 | 120 + 100 |
| Offices, 3 | 5 | 0.8 | 140 x 280 | 80 x 180 | 180 | 220 | 100 + 80 |
|  | 5 | 1.8 | 180 x 280 | 80 x 200 | 200 | 220 | 100 + 80 |
|  | 6 | 0.8 | 160 x 320 | 80 x 240 | 220 | 280 | 120 + 100 |
|  | 6 | 1.8 | 160 x 360 | 80 x 260 | 240 | 280 | 120 + 100 |
|  | 7.5 | 0.8 | 160 x 400 | 80 x 320 | 280 | 320 | 140 + 140 |
|  | 7.5 | 1.8 | 160 x 440 | 80 x 360 | 320 | 360 | 160 + 140 |
| Assembly rooms, 4 | 5 | 0.8 | 200 x 280 | 80 x 200 | 200 | 220 | 100 + 80 |
|  | 5 | 1.8 | 160 x 320 | 80 x 220 | 200 | 240 | 100 + 80 |
|  | 6 | 0.8 | 160 x 360 | 80 x 260 | 240 | 280 | 120 + 100 |
|  | 6 | 1.8 | 200 x 360 | 80 x 280 | 250 | 280 | 120 + 100 |
|  | 7.5 | 0.8 | 160 x 440 | 80 x 360 | 320 | 360 | 160 + 140 |
|  | 7.5 | 1.8 | 200 x 440 | 80 x 400 | 320 | 360 | 160 + 140 |

\include{lineno} templates_timber_floor_1_structures.cpp

---

## Sections from the Span {#timber_floor_sizing}

Producers' tables and apps pick the least section that carries a load over a span. `SizingRule` (`floor_sizing.h`) holds each one's material, factors and limits; `TimberFloorGuide::baubuche`, `vmg_joists`, `kerto_ripa`, `clt_ribs`, `clt_box`, `novatop` and `lignatur` give the floor for a span. Over 4 and 6 m:

![](templates/templates_timber_floor_2_spans_all.webp)

Over 6 m: BauBuche, VMG, Kerto-Ripa, Kerto-Ripa box, Hasslacher, CLT BOX, Novatop, Lignatur:

![](templates/templates_timber_floor_2_spans_section_a.webp)

![](templates/templates_timber_floor_2_spans_section_b.webp)

### Equations {#timber_floor_equations}

A single span `l`, loads per beam: `g` permanent with its own weight, `q` imposed, `p = g + q`, `E I` the stiffness and `G A` the shear stiffness over 1.2.

| Check | Equation |
| --- | --- |
| Deflection | `w(p) = 5 p l^4 / (384 E I) + p l^2 / (8 G A)` |
| at once | `w(g + q) <= l / 300` to `l / 600` |
| with creep | `w(g) (1 + kdef) + w(q) (1 + psi2 kdef) <= l / 250` to `l / 500` |
| lasting | `w(g + psi2 q) (1 + kdef) <= l / 300` |
| Bending | `(gG g + gQ q) l^2 / 8 <= kmod / gM k_h f_m b h^2 / 6`, `k_h = (h0 / h)^s` |
| Shear | `(gG g + gQ q) l / 2 <= kmod / gM f_v b h / 1.5` |
| Bearing | `(gG g + gQ q) l / 2 <= kmod / gM f_c90 b (support + 15)` |
| First frequency | `pi / (2 l^2) sqrt(E I / m) >= 8 Hz`, `m` the swinging mass |
| 1 kN in the middle | `w = min(F l^2 / (42 k E I'), F l^3 / (48 E I)) <= 0.5 mm` and more under 6 m, `k = (E I across / E I')^0.25`, `E I'` per width |
| Glued ribs and boxes | `E I = sum(E b h^3 / 12 + E b h a^2)`, a panel as wide as its rib and a tenth of the span |
| Layers over cross layers | the outer `E A a^2` times `1 / (1 + pi^2 E A d / (l^2 b G_R))`, `d` the cross layer, `G_R` 50 N/mm² |

### What each one checks {#timber_floor_rules}

| Rule | Material N/mm² | Limits | Compared with the printed values |
| --- | --- | --- | --- |
| `baubuche_beam`, GL75 | E 16800, f_m 75 (600 / h)^0.1, f_v 4.5 (600 / h)^0.13 | at once l / 300, bending, shear; kmod 0.8, gM 1.3, 1.4 on the whole load, no creep | 5712 of 5712 spans, one and two spans |
| `baubuche_board`, S | f_m 80 (300 / h)^0.12 to 1, f_v 8 | the same, also l / 500 | 2160 of 2160 |
| `baubuche_joist` | E 16800 | 8 Hz under g alone, joists 120 every 850 | 7 of 7 with g from 2.9 to 3.2, which is not printed |
| `kerto_beam`, Kerto-S | E 13800, G 600, f_v 4.1 | l / 400, with creep l / 300 (kdef 0.8), shear; 1.15 g + 1.5 q | 43 of 45 within 15 mm, worst 25 |
| `kerto_panel`, Kerto-Q | E 10500, G 120 | l / 400, with creep l / 300 | 108 of 108 from 27 thick; 21 thick up to 10 mm long |
| `lvl_joist` | E 13800, G 600 | 1 kN, 8 Hz with g + 30 kg/m² | 9 joists, worst 0.17 m |
| `kerto_ripa`, LVL ribs | panel E 10500, strip 13800 | 8 Hz with g + 30 kg/m² | 14 ribs and open boxes of the LVL Handbook, worst 0.14 m |
| `vmg_beam` | E 13800, G 600, f_c90 6 (48 P) | with creep l / 250, 1 kN 1.5 mm, 8 Hz with g + 30 kg/m², bearing on 45; a metre of floor each | 153 of 155 (48 P) |
| `i_joist`, VMG joists | flanges 13800, web 2550, deck 2500 | with creep l / 250 (shear kdef 2.25), 8 Hz with g + 0.3 q and the deck glued on | 137 of 154 within 20 mm, 11 within 0.1 m, 6 capped by a rule not found |
| `lignatur` | E 11000 | l / 475, light floors l / 600, a roof l / 500 | 40 of 40 of the calculator within 1 mm |
| `clt_box` | E 11550, ribs 11500 | lasting l / 300 (kdef 0.6, psi2 0.3), rib shear | 53 of 59 depths and ribs |
| `novatop` | panels 7800, ribs 5300, G 600 | l / 300 at once | 38 of 45 depths, the rest one step |
| `clt_rib`, Hasslacher | plate 11600, ribs 11500 | 8 Hz under g alone | 25 of 30 depths, the rest one step |
| `book_joist`, Kolb | E 11000 | with creep l / 500 (kdef 0.6, psi2 0.3, assembly 0.6) | joists 54 of 54, boxes 18 of 18, dowelled 17 of 18, hollow 18 of 18 within 2.5 % |

The rules are read back from the tables where a producer does not print them; the last column is this code against the printed values. They are for a first size. An engineer checks fire, sound, openings, supports and the country's annex.

### Section by span {#timber_floor_spanning}

| Floor, loads kN/m² | 4 m | 5 m | 6 m | 7 m | 8 m | 9 m | 10 m |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Joists C24, GL24 every 600, g 1.8, q 2 | 120 x 240 | 80 x 360 | 80 x 440 | 80 x 480 | 80 x 560 | 100 x 600 | 140 x 600 |
| BauBuche joists 120 every 850, g 3, q 3 | 120 x 240 | 120 x 320 | 120 x 400 | 120 x 520 | 120 x 600 | 120 x 720 | 120 x 840 |
| LVL ribs 51 every 612 under 25, g 1.9, q 2 | 225 | 285 | 385 | 475 | 625 |  |  |
| LVL open box, strips 300 x 43, g 1.9, q 2 | 268 | 268 | 288 | 368 | 468 | 668 | 668 |
| VMG LVL 48P beams a metre apart, g 1, q 2 | 36 x 290 | 45 x 360 | 63 x 400 |  |  |  |  |
| VMG joists every 300, g 1, q 2 | 45 / 200 | 90 / 220 | 90 / 300 | 90 / 360 |  |  |  |
| VMG joists every 600, g 1, q 2 | 90 / 220 | 90 / 300 | 90 / 360 |  |  |  |  |
| Lignatur LFE, g 1, q 2.8 | 140 | 180 | 220 | 280 | 320 | 360 |  |
| CLT BOX, g 2.5, q 3 | 220 / 80 | 220 / 80 | 220 / 80 | 260 / 80 | 300 / 80 | 340 / 80 | 380 / 80 |
| Novatop Element A2, g 2, q 3 | 160 | 200 | 260 | 300 | 360 |  |  |
| Hasslacher ribs 160 every 625 under 120, g 1.5 | 160 | 160 | 160 | 240 | 320 | 400 | 480 |

Depths, or width x depth, `flange / depth` of a VMG joist, `depth / rib` of a CLT BOX, in mm. The loads are each producer's own, so rows do not compare; the own weight is added, for VMG as its tables leave it out.

### Products {#timber_floor_products}

| Product | Structure | Depths mm | Width m | Length m | Source |
| --- | --- | --- | --- | --- | --- |
| Joists, solid timber or glulam | joists | to 240 or 280, glulam deeper | at 500 to 700 | 13 | Kolb, p. 160 and 317 |
| BauBuche joists, Pollmeier | joists | 120 x 120 to 600, every 850 |  | to 18 | [Pollmeier 13](https://www.pollmeier.com/wp-content/uploads/2023/03/Pollmeier-BauBuche-13-Holzbalkendecken.pdf) |
| VMG Lignum I-joists | joists | 200 to 400; flanges 45, 60, 90 x 39 |  | to 18 | [VMG joist guide](https://www.vmg-lignum.eu/wp-content/uploads/2024/03/VMG_LIGNUM_JOIST_2025_08_23.pdf) |
| Kerto-Ripa, Metsä; Stora Enso LVL rib | ribs, box | ribs 39 to 75 x 100 to 1150 under 22 to 73 | 1.80 to 2.50 | 6 to 20 | [Metsä](https://www.metsagroup.com/metsawood/explore-wood/kerto-lvl-applications/floor-elements/kerto-ripa-floor-elements/), [ETA-18/1132](https://storaenso.com/-/media/documents/download-center/certificates/wood-products-approvals-and-certificates/eta/eta-sylva-lvl-rib-by-stora-enso---may-2023.pdf) |
| Rib panels, Hasslacher, KLH | ribs | plate 80 to 240 on ribs 80 to 280 x 80 to 1280 | 1.25 to 3.20 | to 24 | [Hasslacher](https://www.hasslacher.com/data/_dateimanager/broschuere/Rippenplattendecke-DE.pdf), [KLH](https://www.klh.at/wp-content/uploads/2019/09/klh-rib-elements-en.pdf) |
| CLT BOX, Schneider; Derix, Woodtec | box | 220 to 600; flanges 27 to 120, ribs 60 to 180 | 1.00 to 3.00 | to 16 | [ETA-21/0336](https://www.schneider-holz.com/site/assets/files/6878/best-wood-schneider_eta_clt-box_de.pdf), [catalogue 2026](https://www.schneider-holz.com/de/service/downloads/download/produktuebersicht-holz-schweiz/), [Derix](https://derix.de/media/freie-dokumente/XXL-Deckensysteme-aus-Holz.pdf) |
| Novatop Element | box | 160 to 400; panels and ribs 27 | 1.03, 2.09, 2.45 | to 12 | [Element](https://novatop-system.com/wp-content/uploads/TD_NOVATOP_ELEMENT_EN.pdf), [ETA-11/0310](https://novatop-system.com/wp-content/uploads/NOVATOP_Element_ETA_EN.pdf) |
| Lignatur LFE, LKE | box, hollow | 120 to 360 | 1.00, 0.20 | to 16 | [ETA-11/0137](https://www.lignatur.ch/fileadmin/ablage/downloads/das-element/eta-11-0137-lignatur-de.pdf), [weights](https://www.lignatur.ch/produkt/statik/kennwerte), [spans](https://www.lignatur.ch/produkt/statik/vorstatik) |
| Lignotrend Q3 | box | 150 to 450 of boards on each other | 0.625 | to 18 | [Z-9.1-555](https://www.dibt.de/pdf_storage/2025/Z-9.1-555%281.9.1-38!22%29.pdf), [Rippe](https://lignotrend.com/_Resources/Persistent/5/d/e/d/5ded1af7816ec8419aab71ca8183ca7a4db2b885/2024-08-14_TD%20LIGNO%20Rippe-x_D-k.pdf), [Block](https://lignotrend.com/_Resources/Persistent/e/1/5/0/e150836a8beb14c43a7b97cae60426baaf4b709d/2026-02-18_TD%20LIGNO%20Block-x_DE_k.pdf) |
| Dowel-laminated, Holz100 | solid | 80 to 400, boards 20 to 60 | 0.60 to 3.50 | to 14 | Kolb, p. 220 and 323, [Sohm](https://werbewind-login.com/files/?f=ETA+DiagonalD%C3%BCbelholz_DE.pdf&cnt=document&cntid=C90735F7-9409-BC88-4A84-1433DF69A68C), [ETA-13/0785](https://www.dibt.de/pdf_storage/2018/ETA-13!0785%288.03.04-92!17%29.pdf) |
| Glulam planks | solid | 80 to 280 | 0.20 to 1.20 | 4 to 18 | [Mayr-Melnhof](https://www.mm-holz.com/produkte/brettschichtholz-decke/) |
| Cross-laminated | solid | 100 to 300; TS3 180 to 300 on columns | 3.00, 3.50 | to 16 | [CLT Handbook](https://www.swedishwood.com/siteassets/5-publikationer/pdfer/clt-handbook-2019-eng-m-svensk-standard-2019.pdf), table 5.1, [TS3](https://www.ts3.biz/media/docs/downloads/en/2024.06_Factsheet_Architektur_en.pdf) |
| Timber and concrete, ZECH, Cree | composite | L / 25, concrete 0.4 of it, at least 60; ribs 240 x 280 under 100 | 2.60 to 2.80 | 6 to 8 | [Z-9.1-911](https://www.dibt.de/pdf_storage/2024/Z-9.1-911%281.9.1-15!24%29.pdf) |

| Product | Sizes mm | Length m | Source |
| --- | --- | --- | --- |
| BauBuche GL75 beam | b 100, 120, 160, 200, 240, 280, 300; h 80 to 1360 by 40 | 12, 13.5, 16, 18 | [Pollmeier 02](https://www.pollmeier.com/wp-content/uploads/2023/04/2025-11_EN_BauBuche_02_Produktuebersicht_web.pdf), tables [04](https://www.pollmeier.com/wp-content/uploads/2023/03/Pollmeier-BauBuche-04-Characteristic-static-strength-value.pdf) |
| BauBuche S, Q board | 40, also 60 (S), 20 (Q); width 100 to 1820 | to 18 | as above |
| Kerto-S beam | 27 to 75 thick, h 200 to 600 | to 25 | [Metsä manual](https://www.metsagroup.com/globalassets/metsa-wood/attachments/kerto-lvl-manual/en/metsa_kertolvl_mechanical_properties.pdf) |
| Kerto-Q panel | 21, 24, 27, 33, 39, 45, 51, 57, 63, 69, 75 | | [Kerto-Q](https://www.metsagroup.com/globalassets/metsa-wood/attachments/kerto-lvl-manual/en/kerto-manual-lvl-q-panel.pdf) |
| VMG Lignum LVL | 36, 39, 45, 63, 75 x 90 to 400, along and crossed | to 18 | [VMG guide](https://www.vmg-lignum.eu/wp-content/uploads/2024/03/VMG_LIGNUM_LVL_2025_08_23.pdf) |
| VMG Lignum joist | flanges 45, 60, 90 x 39, web 10, h 200 to 400 | to 18 | [VMG joist guide](https://www.vmg-lignum.eu/wp-content/uploads/2024/03/VMG_LIGNUM_JOIST_2025_08_23.pdf) |

VMG Lignum publishes its spans as tables only; no calculator or app of theirs was found. The [LVL Handbook Europe](https://puumarket.ee/wp-content/uploads/lvl-handbook.pdf) prints the method the veneer lumber makers share. The sections of the timber itself are on [Edge Straight](../elements/element_edge_straight.md) and [Face Planar](../elements/element_face_planar.md).

\include{lineno} templates_timber_floor_2_spans.cpp

---

## Bearings and Joints {#timber_floor_joints}

On the beams, in their rebates and on battens glued to their faces; a rebate pulls the beam apart across its grain and is checked or reinforced:

![](templates/templates_timber_floor_3_joints_bearings.webp)

Panels joined by a tongue in a groove, a strip in two grooves, a board in two rebates with its pins, a key across; the board and the key make the panels one diaphragm:

![](templates/templates_timber_floor_3_joints_edges.webp)

Concrete held to the timber by glue, by notches, by crossed pins over joists and by flat steel locks across boards on edge, the concrete left out:

![](templates/templates_timber_floor_3_joints_concrete.webp)

\include{lineno} templates_timber_floor_3_joints.cpp

---

## Timber and Earth {#timber_floor_earth}

The prefabricated floor of [Rematter](https://www.rematter.earth): solid beams without glue, earth rammed between them by robots, a three-layer panel pinned on both (`earth_floor.h`). The beams are wider at the foot, so the earth rests on their flanks; it weighs the floor down against sound, keeps the fire from the timber and stores heat. Everything is dry and comes apart.

![](templates/templates_timber_floor_4_earth_open.webp)

Arch, straight, flush and concealed:

![](templates/templates_timber_floor_4_earth_section.webp)

```cpp
EarthFloors floors({
    {"arch", EarthFloorGuide::arch()},
    {"straight", EarthFloorGuide::straight()},
    {"flush", EarthFloorGuide::flush()},
    {"concealed", EarthFloorGuide::concealed()},
});
```

### Earth floor parameters {#timber_floor_earth_parameters}

| Parameter | Default | Description |
| --- | --- | --- |
| `length`, `width`, `pitch` | `4200.0`, `2100.0`, `700.0` | The element, and from beam to beam; `500` clear |
| `depth`, `panel` | `200.0`, `27.0` | Beams and panel, `227` in all |
| `half_top`, `half_bottom`, `flank` | `75.0`, `100.0`, `120.0` | Half a beam at its top and foot, the height it widens over |
| `rise`, `chords` | `18.0`, `24` | The arch under the earth and its sides |
| `lining` | `0.0` | Board that hides the earth |
| `pin_spacing` | `300.0` | Pins of the panel along every half beam |
| `ties` | `0` | Rods across all beams that press the earth, as the patent has them |
| `bearing_width`, `bearing_depth` | `160.0`, `280.0` | The two beams under the element |

### What it carries {#timber_floor_earth_values}

| | Value |
| --- | --- |
| Load | to 5 kN/m², build-up to 0.75 kN/m² |
| Fire | REI 60, REI 90 on request |
| Sound | R'w 50 dB bare; with the build-up Ln,w 42 dB |
| Heat stored | 235 kJ/m²K |
| Greenhouse gas, grey energy | 0.27 kg CO2-eq, 1.16 kWh per m² and year |
| Build-up | 40 soft fibreboard, oil paper, 30 granulate or sand, 20 floorboards |
| Earth in the patent | 50 to 200 deep, rammed at 1 to 9 N/mm², the element upside down |

| Element | Depth mm | Earth depth mm | Earth m³/m² | Timber m³/m² | Weight kg/m² |
| --- | --- | --- | --- | --- | --- |
| `arch` | 227 | 120 at the beams, 102 at the crown | 0.081 | 0.080 | 199 |
| `straight` | 227 | 120 | 0.090 | 0.080 | 216 |
| `flush` | 227 | 200 | 0.139 | 0.088 | 318 |
| `concealed` | 247 | 200 | 0.139 | 0.108 | 327 |

Element sizes, `227` and the values: Rematter, [factsheet](https://www.rematter.earth/user/data/flex-objects/documents/8859462c7d5425fbaf729ef8c0f285c4/RematterAG_FactsheetDE_EN_2603.pdf), 2026; the earth: [EP4015730B1](https://patents.google.com/patent/EP4015730B1). The beams' and the earth's widths are measured off the factsheet's drawing of the four elements; the last three columns are computed from the model, with 2000 kg/m³ of earth and 450 of timber. Spans and beams are Rematter's engineers' to size.

\include{lineno} templates_timber_floor_4_earth.cpp

---

## Structure and Layers {#timber_floor_build_ups}

A floor is drawn as its structure and one layer on it as thick as all the layers together: `layers`, 60 for a light build-up (`LAYERS_LIGHT`, 0.8 kN/m²), 110 for a heavy one (`LAYERS_HEAVY`, 1.8 kN/m²).

| All layers on the structure | mm | middle | R'w dB | L'n,w dB | Floors |
| --- | --- | --- | --- | --- | --- |
| Dry: boards on insulation | 35 to 90 | 42.5 | 40 to 67 | 47 to 75 | 14 |
| Dry with a fill: boards, insulation, sand, slabs or honeycomb | 60 to 110 | 88 | 49 to 73 | 44 to 69 | 14 |
| Screed on insulation | 60 to 135 | 105 | 46 to 78 | 37 to 70 | 15 |
| Screed, insulation and a fill | 110 to 170 | 130 | 56 to 72 | 44 to 58 | 8 |

| Structure | mm | All layers on it, mm | Depth mm | kg/m² | R'w dB | L'n,w dB |
| --- | --- | --- | --- | --- | --- | --- |
| Joists 140 x 200, boarding 21 | 221 | 88 dry on battens | 309 | 52 | 40 | 73 |
| Joists 140 x 200, boarding 21 | 221 | 75 screed, insulation | 296 | 150 | 46 | 70 |
| Joists 140 x 220, boarding 22 | 242 | 110 screed, insulation, sand | 352 | 203 | 56 | 58 |
| Cross-laminated on ribs 250 | 250 | 105 screed, insulation | 355 | 136 | 66 | 55 |
| Lignatur box 200 | 200 | 58 dry, insulation | 258 | 68 | 49 | 67 |
| Lignatur surface 240 | 240 | 160 screed, insulation, grit | 400 | 296 | 72 | 45 |
| Novatop Element 240 | 240 | 88 dry, honeycomb | 328 | 151 | 59 | 60 |
| Boards on edge 140, concrete 120 | 260 | 120 screed, insulation | 380 | 537 | 62 | 44 |
| Planks 120 | 120 | 35 dry, insulation | 155 | 74 | 49 | 71 |
| Planks 140 | 140 | 95 dry, sand | 235 | 165 | 55 | 58 |
| Boards on edge 120 | 120 | 80 screed, insulation | 200 | 176 | 53 | 57 |
| Boards on edge 140 | 140 | 120 screed, insulation, slabs | 260 | 285 | 61 | 52 |

A bare structure weighs 29 to 72 kg/m², with composite concrete 204 to 247, and has R'w 26 to 44 and L'n,w 82 to 90. Linings under a floor are 42.5 to 55 more. The book prints 342 for the third. Source: Kolb, Kolb, Müller, *Holzbau mit System*, 2024, tables 11 to 24, p. 414 to 426. Not counted: a roof terrace, a hollow floor and a hardboard alone.

---

<!-- assembly:begin -->
## Assembly Sequence {#timber_floor_assembly_sequence}

The beams, then the ribs and the boards or the panels in their order, the concrete, what joins them last, all the layers on them as one; the earth rammed between its beams. Every move is a `ModuleSequence` move: the module taken apart piece by piece, each piece swept as a solid against every piece still in, the moving ones drawn brown, a GIF per kind of module and one of the whole.

| assembly | | |
|---|---|---|
| **Structures**, 132 elements, 125 moves | | |
| ![box, 11 pieces](assembly/assembly_templates_timber_floor_1_structures_box.gif) box, 11 pieces |  |  |
| **Sections from the span**, 200 elements, 184 moves | | |
| ![lignatur 4, 17 pieces](assembly/assembly_templates_timber_floor_2_spans_lignatur_4.gif) lignatur 4, 17 pieces |  |  |
| **Bearings and joints**, 106 elements, 95 moves | | |
| ![keys, 4 pieces](assembly/assembly_templates_timber_floor_3_joints_keys.gif) keys, 4 pieces |  |  |
| **Timber and earth**, 409 elements, 69 moves | | |
| ![arch, 18 pieces](assembly/assembly_templates_timber_floor_4_earth_arch.gif) arch, 18 pieces |  |  |

<!-- assembly:end -->
