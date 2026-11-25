# Timber Floors {#templates_timber_floor}

[TOC]

One bay of every timber floor system on two beams, sized for a use, a span and a build-up (`src/templates/timber_floor/`), and the dimensions the systems are made in.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#timber_floor_systems">
    <img src="templates_timber_floor_1_systems_joists.webp" alt="Joists">
    <span class="wood-card-title">Joists</span>
  </a>
  <a class="wood-card" href="#timber_floor_systems">
    <img src="templates_timber_floor_1_systems_ribbed.webp" alt="Ribbed">
    <span class="wood-card-title">Ribbed</span>
  </a>
  <a class="wood-card" href="#timber_floor_systems">
    <img src="templates_timber_floor_1_systems_box.webp" alt="Box">
    <span class="wood-card-title">Box</span>
  </a>
  <a class="wood-card" href="#timber_floor_systems">
    <img src="templates_timber_floor_1_systems_hollow.webp" alt="Hollow Box">
    <span class="wood-card-title">Hollow Box</span>
  </a>
  <a class="wood-card" href="#timber_floor_systems">
    <img src="templates_timber_floor_1_systems_planks.webp" alt="Glulam Planks">
    <span class="wood-card-title">Glulam Planks</span>
  </a>
  <a class="wood-card" href="#timber_floor_systems">
    <img src="templates_timber_floor_1_systems_dowelled.webp" alt="Dowel-Laminated">
    <span class="wood-card-title">Dowel-Laminated</span>
  </a>
  <a class="wood-card" href="#timber_floor_systems">
    <img src="templates_timber_floor_1_systems_cross_laminated.webp" alt="Cross-Laminated">
    <span class="wood-card-title">Cross-Laminated</span>
  </a>
  <a class="wood-card" href="#timber_floor_systems">
    <img src="templates_timber_floor_1_systems_composite.webp" alt="Timber and Concrete">
    <span class="wood-card-title">Timber and Concrete</span>
  </a>
  <a class="wood-card" href="#timber_floor_bearings">
    <img src="templates_timber_floor_2_bearings_all.webp" alt="Bearings">
    <span class="wood-card-title">Bearings</span>
  </a>
  <a class="wood-card" href="#timber_floor_joints">
    <img src="templates_timber_floor_3_joints_all.webp" alt="Joints">
    <span class="wood-card-title">Joints</span>
  </a>
  <a class="wood-card" href="#timber_floor_earth">
    <img src="templates_timber_floor_4_earth_arch.webp" alt="Timber and Earth">
    <span class="wood-card-title">Timber and Earth</span>
  </a>
  <a class="wood-card" href="#timber_floor_book">
    <img src="templates_timber_floor_5_systems_beams_comb.webp" alt="Floors of the Book">
    <span class="wood-card-title">Floors of the Book</span>
  </a>
  <a class="wood-card" href="#timber_floor_producers">
    <img src="templates_timber_floor_6_producers_kielsteg.webp" alt="Producers">
    <span class="wood-card-title">Producers</span>
  </a>
  <a class="wood-card" href="#timber_floor_shear">
    <img src="templates_timber_floor_7_shear_concrete_wave_open.webp" alt="Joints and Concrete">
    <span class="wood-card-title">Joints and Concrete</span>
  </a>
  <a class="wood-card" href="#timber_floor_build_ups">
    <img src="templates_timber_floor_8_build_ups_all.webp" alt="Build-ups">
    <span class="wood-card-title">Build-ups</span>
  </a>
  <a class="wood-card" href="#timber_floor_assembly_sequence">
    <img src="assembly_templates_timber_floor_1_systems_ribbed.gif" alt="Assembly Sequence">
    <span class="wood-card-title">Assembly Sequence</span>
  </a>
</div>

## Parameters {#timber_floor_parameters}

| Parameter | Default | Description |
| --- | --- | --- |
| `span`, `width` | `5000.0`, `2400.0` | Between the middles of the two beams, and across |
| `rib_width`, `rib_depth`, `rib_spacing` | `0.0`, `0.0`, `600.0` | Joists or ribs along the span; a depth of `0` for none |
| `top`, `bottom` | `0.0`, `0.0` | Board on and under the ribs |
| `panel`, `panel_width` | `0.0`, `0.0` | Thickness of the panels and their width; a width of `0` for one |
| `edge` | `0` | Panel edges: butt, tongue in groove, strip in two grooves, board in two rebates, key |
| `lamella`, `dowel`, `dowel_spacing` | `0.0`, `20.0`, `600.0` | Boards on edge a panel is made of, the dowels through them |
| `skin` | `0.0` | Shells and webs of a hollow panel |
| `concrete`, `notch`, `notches` | `0.0`, `20.0`, `3` | Concrete on the panel, the notches it fills in each half of the span |
| `seat`, `seat_length` | `0`, `60.0` | On the beams, in their rebates, or on battens glued to their faces |
| `beam_width`, `beam_depth` | `160.0`, `280.0` | The two beams |

---

## Systems {#timber_floor_systems}

Eight floors of a home over 5 m under a heavy build-up. `joists`, `box`, `dowelled`, `composite` and `hollow` read their sizes from the [tables](#timber_floor_sizes):

```cpp
const TimberFloorGuide joists = TimberFloorGuide::joists(use, span, heavy, spacing);
const TimberFloorGuide box = TimberFloorGuide::box(use, span, heavy);
const TimberFloorGuide dowelled = TimberFloorGuide::dowelled(use, span, heavy);
const TimberFloorGuide composite = TimberFloorGuide::composite(use, span, heavy);
const TimberFloorGuide hollow = TimberFloorGuide::hollow(use, span, heavy);
```

| | | |
| --- | --- | --- |
| ![](templates/templates_timber_floor_1_systems_joists.webp) Joists 160 x 280 at 600, 27 board | ![](templates/templates_timber_floor_1_systems_ribbed.webp) Ribs 100 x 240 at 500 glued under a 27 board | ![](templates/templates_timber_floor_1_systems_box.webp) Ribs 80 x 180 at 500 between two 27 boards |
| ![](templates/templates_timber_floor_1_systems_hollow.webp) Hollow box panels 200 deep, 31 shells | ![](templates/templates_timber_floor_1_systems_planks.webp) Glulam planks 180 x 200, tongue in groove | ![](templates/templates_timber_floor_1_systems_dowelled.webp) 180 of 40 boards on edge, 20 dowels |
| ![](templates/templates_timber_floor_1_systems_cross_laminated.webp) Cross-laminated 180, 27 x 160 cover board | ![](templates/templates_timber_floor_1_systems_composite.webp) Timber 100 notched into concrete 80 | |

From one side, all at one scale:

![](templates/templates_timber_floor_1_systems_section.webp)

| System | Carries | Good at | Costs |
| --- | --- | --- | --- |
| Joists | one way | light, adjusted to any load, services between the joists | depth, time on site |
| Ribbed, box | one way | little timber, shallow, ready to walk on | gluing in the shop, planning of the elements |
| Glulam planks, dowel-laminated | one way | shallow, simple, seen from below | timber, swelling across the boards |
| Cross-laminated | two ways | large plates, stable, stiff in their plane | timber |
| Timber and concrete | one way | stiff, quiet, long spans | weight, wet work or a crane |

\include{lineno} templates_timber_floor_1_systems.cpp

---

## Sizes {#timber_floor_sizes}

Single spans, sections b x h in mm, deflection up to L / 500, C24 or GL24h: for a first size only, strength and vibration are to be checked. A light build-up is 0.8 kN/m², a heavy one 1.8. Source: Kolb, Kolb, Müller, *Holzbau mit System*, 2024, p. 311; `FLOOR_SIZES` in `timber_floor_guide.h` is these rows.

### Homes, 2 kN/m² {#timber_floor_sizes_0}

| Span m | Build-up kN/m² | Joists at 500 | at 600 | at 700 | Box ribs at 500 | Dowelled | Timber + concrete | Hollow | Concrete slab |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 4 | 0.8 | 100 x 220 | 120 x 220 | 120 x 220 | 80 x 100, 154 deep | 130 | 100 + 80 | 140 | 200 |
| 4 | 1.8 | 140 x 220 | 120 x 240 | 140 x 240 | 80 x 120, 174 deep | 150 | 100 + 80 | 160 | 200 |
| 5 | 0.8 | 140 x 240 | 140 x 260 | 120 x 280 | 80 x 160, 214 deep | 170 | 100 + 80 | 180 | 200 |
| 5 | 1.8 | 120 x 280 | 160 x 280 | 180 x 280 | 80 x 180, 234 deep | 180 | 100 + 80 | 200 | 200 |
| 6 | 0.8 | 100 x 320 | 120 x 320 | 140 x 320 | 80 x 200, 254 deep | 200 | 120 + 100 | 220 | 220 |
| 6 | 1.8 | 100 x 360 | 120 x 360 | 140 x 360 | 80 x 240, 294 deep | 220 | 120 + 100 | 240 | 220 |

### Offices, 3 kN/m² {#timber_floor_sizes_1}

| Span m | Build-up kN/m² | Joists at 500 | at 600 | at 700 | Box ribs at 500 | Dowelled | Timber + concrete | Hollow | Concrete slab |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 5 | 0.8 | 140 x 260 | 140 x 280 | 160 x 280 | 80 x 180, 234 deep | 180 | 100 + 80 | 220 | 200 |
| 5 | 1.8 | 160 x 280 | 180 x 280 | 140 x 320 | 80 x 200, 254 deep | 200 | 100 + 80 | 220 | 220 |
| 6 | 0.8 | 200 x 280 | 160 x 320 | 200 x 320 | 80 x 240, 294 deep | 220 | 120 + 100 | 280 | 220 |
| 6 | 1.8 | 180 x 320 | 160 x 360 | 180 x 360 | 80 x 260, 314 deep | 240 | 120 + 100 | 280 | 250 |
| 7.5 | 0.8 | 180 x 360 | 160 x 400 | 180 x 400 | 80 x 320, 374 deep | 280 | 140 + 140 | 320 | 250 |
| 7.5 | 1.8 | 160 x 400 | 160 x 440 | 180 x 440 | 80 x 360, 414 deep | 320 | 160 + 140 | 360 | 250 |

### Assembly rooms, 4 kN/m² {#timber_floor_sizes_2}

| Span m | Build-up kN/m² | Joists at 500 | at 600 | at 700 | Box ribs at 500 | Dowelled | Timber + concrete | Hollow | Concrete slab |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 5 | 0.8 | 160 x 280 | 200 x 280 | 160 x 320 | 80 x 200, 254 deep | 200 | 100 + 80 | 220 | 200 |
| 5 | 1.8 | 200 x 280 | 160 x 320 | 180 x 320 | 80 x 220, 274 deep | 200 | 100 + 80 | 240 | 200 |
| 6 | 0.8 | 200 x 320 | 160 x 360 | 180 x 360 | 80 x 260, 314 deep | 240 | 120 + 100 | 280 | 220 |
| 6 | 1.8 | 160 x 360 | 200 x 360 | 160 x 400 | 80 x 280, 334 deep | 250 | 120 + 100 | 280 | 250 |
| 7.5 | 0.8 | 180 x 400 | 160 x 440 | 180 x 440 | 80 x 360, 414 deep | 320 | 160 + 140 | 360 | 250 |
| 7.5 | 1.8 | 160 x 440 | 200 x 440 | 180 x 480 | 80 x 400, 454 deep | 320 | 160 + 140 | 360 | 250 |

The dowelled floor is 1 m elements of boards on edge, the hollow one glued box elements, the concrete slab beside them for comparison.

### Timber in the floor {#timber_floor_volume}

m³ of timber per m² of floor of a home, computed from the rows above:

| Span m | Build-up kN/m² | Joists at 600 and 27 board | Box | Dowelled | Timber of the composite |
| --- | --- | --- | --- | --- | --- |
| 4 | 0.8 | 0.071 | 0.070 | 0.130 | 0.100 |
| 4 | 1.8 | 0.075 | 0.073 | 0.150 | 0.100 |
| 5 | 0.8 | 0.088 | 0.080 | 0.170 | 0.100 |
| 5 | 1.8 | 0.102 | 0.083 | 0.180 | 0.100 |
| 6 | 0.8 | 0.091 | 0.086 | 0.200 | 0.120 |
| 6 | 1.8 | 0.099 | 0.092 | 0.220 | 0.120 |

### Spans and depths {#timber_floor_spans}

| System | Depth mm | Element width m | Length m | Span m | Source |
| --- | --- | --- | --- | --- | --- |
| Joists, solid timber | to 240 or 280, else glulam | at 500 to 700 | 13 | to 5 | Kolb, p. 160 and 317 |
| Hollow box, Lignatur LFE | 120 to 240 by 20, 280, 320, 360 | 1.00 | to 16 | 3.4 to 8.8 in homes | [Lignatur](https://www.lignatur.ch/produkt/das-element), [sizes](https://www.lignatur.ch/produkt/statik/vorstatik) |
| Hollow box, Lignatur LKE | 120 to 320 | 0.20 | to 12 | 3.8 to 7.8 | [Lignatur](https://www.lignatur.ch/produkt/das-element) |
| Box of solid wood panels, Novatop Element | 160 to 320, to 400 | 1.03, 2.09, 2.45 | 6, to 12 | | [Novatop](https://novatop-system.com/product/element/) |
| Ribs under a panel, Novatop Open | 227, 247, 267 | 1.03, 2.09, 2.45 | 6, to 12 | | [Novatop](https://novatop-system.com/product/open/) |
| Corrugated box, Kielsteg | 280 to 800 | 1.20 | 6 to 35 | 6 to 13, roofs to 27 | [Kielsteg](https://www.kielsteg.com/elements-guide/) |
| Dowel-laminated | 80 to 240, boards 20 to 60 | 0.60, nailed to 3.50 | 14 | 3 to 7 | Kolb, p. 220 and 323, [Sohm](https://werbewind-login.com/files/?f=ETA+DiagonalD%C3%BCbelholz_DE.pdf&cnt=document&cntid=C90735F7-9409-BC88-4A84-1433DF69A68C) |
| Glulam planks, MM profideck | 80 to 260 by 20 | 0.63, to 1.20 | 4 to 16 | | [Mayr-Melnhof](https://www.mm-holz.com/produkte/brettschichtholz-decke/) |
| Glulam slab, Schilliger MHP | 100 to 280 by 20 | 0.20 to 1.00 | 4 to 18 | | [Schilliger](https://www.schilliger.ch/wp-content/uploads/2025/04/SchilligerHolz-Produktefolder-DE-2024-03-Deckensysteme-Leimholz-MHP.pdf) |
| Cross-laminated 100, 140, 160 | 100, 140, 160 | 3.00, 3.50 | 16 | 2.8, 3.2, 4.1 at L / 600 | [CLT Handbook](https://www.swedishwood.com/siteassets/5-publikationer/pdfer/clt-handbook-2019-eng-m-svensk-standard-2019.pdf), table 5.1 |
| Cross-laminated plate on glulam ribs, Hasslacher | 260 to 760: plate 100 to 120, ribs 120 x 160 to 200 x 640 at 625 | 1.25 to 3.20 | to 24 | 6 to 11 | [Hasslacher](https://www.hasslacher.com/data/_dateimanager/broschuere/Rippenplattendecke-DE.pdf) |
| Cross-laminated box, Derix | to 600, ribs 60 to 180 x 100 to 480 | 3.00 | 10 | from 6 | [Derix](https://derix.de/media/freie-dokumente/XXL-Deckensysteme-aus-Holz.pdf) |
| Timber and concrete | L / 25, concrete 0.4 of it, at least 60 | | | 6 to 12 | [CLT Handbook](https://www.swedishwood.com/siteassets/5-publikationer/pdfer/clt-handbook-2019-eng-m-svensk-standard-2019.pdf), 5.1.3 |

The sections and formats of the timber itself are on [Edge Straight](../elements/element_edge_straight.md) and [Face Planar](../elements/element_face_planar.md). Offices are sized for 3 kN/m², assembly rooms for 3 to 5, homes for 2; vibration asks for a natural frequency over 8 Hz.

### Beams under the floor {#timber_floor_beams}

Glulam b x h in mm under a home's 3.0 to 3.5 kN/m², joists in between every 875 to 1167 (750 to 1125 on the largest grid).

| Grid m | Beam | Spruce GL24h | Beech GL40c | Beech veneer GL75 |
| --- | --- | --- | --- | --- |
| 3.5 x 4.5 | edge | 120 x 260 | 120 x 240 | 120 x 220 |
| | middle | 180 x 280 | 180 x 260 | 180 x 240 |
| 3.5 x 3.5 | edge | 140 x 220 | 140 x 220 | 140 x 200 |
| | middle | 180 x 260 | 180 x 240 | 180 x 240 |
| 4.5 x 6.0 | edge | 160 x 360 | 160 x 340 | 160 x 320 |
| | middle | 200 x 380 | 200 x 360 | 200 x 340 |

Source: Kolb, p. 173, single-span joists. Grids in use: 4.5 x 4.5, 5 x 5, 6.25 x 6.25, 7.5 x 7.5, 2.5 x 7.5, 5 x 7.5 m; homes are cheapest at 4 to 6 m.

---

## Bearings {#timber_floor_bearings}

On the beams, in their rebates, and on battens glued to their faces. A rebate pulls the beam apart across its grain: it is checked or reinforced.

![](templates/templates_timber_floor_2_bearings_all.webp)

| on the beam | in the rebate | on the batten |
| --- | --- | --- |
| ![](templates/templates_timber_floor_2_bearings_resting_section.webp) | ![](templates/templates_timber_floor_2_bearings_rebated_section.webp) | ![](templates/templates_timber_floor_2_bearings_battens_section.webp) |

\include{lineno} templates_timber_floor_2_bearings.cpp

---

## Joints {#timber_floor_joints}

Along the joint of two panels: a tongue in a groove, a strip in two grooves, a board in two rebates with its pins, a key across. The board and the key make the panels one diaphragm.

![](templates/templates_timber_floor_3_joints_all.webp)

| tongue | strip | board |
| --- | --- | --- |
| ![](templates/templates_timber_floor_3_joints_tongue_section.webp) | ![](templates/templates_timber_floor_3_joints_strip_section.webp) | ![](templates/templates_timber_floor_3_joints_board_section.webp) |

\include{lineno} templates_timber_floor_3_joints.cpp

---

## Timber and Earth {#timber_floor_earth}

The prefabricated floor of [Rematter](https://www.rematter.earth): solid beams without glue, earth rammed between them by robots, a three-layer panel pinned on both (`earth_floor.h`). The beams are wider at the foot, so the earth rests on their flanks; it weighs the floor down against sound, keeps the fire from the timber and stores heat. Everything is dry and comes apart.

| arch | straight |
| --- | --- |
| ![](templates/templates_timber_floor_4_earth_arch_section.webp) | ![](templates/templates_timber_floor_4_earth_straight_section.webp) |
| **flush** | **concealed** |
| ![](templates/templates_timber_floor_4_earth_flush_section.webp) | ![](templates/templates_timber_floor_4_earth_concealed_section.webp) |

| the element | without its panel |
| --- | --- |
| ![](templates/templates_timber_floor_4_earth_arch.webp) | ![](templates/templates_timber_floor_4_earth_open.webp) |

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

## Floors of the Book {#timber_floor_book}

Every floor of the floor chapter that no one producer makes, samples 2.4 m long (`FloorProduct`, `floor_product.h`): a product is its straight pieces, each a section and a run, what is let into what, and its pins.

|  |  |  |  |
| --- | --- | --- | --- |
| ![](templates/templates_timber_floor_5_systems_joists_exposed.webp) Joists, exposed | ![](templates/templates_timber_floor_5_systems_joists_partly_exposed.webp) Joists, boards between them | ![](templates/templates_timber_floor_5_systems_joists_clad.webp) Joists, clad on battens | ![](templates/templates_timber_floor_5_systems_joists_trimmed.webp) Joists trimmed round an opening |
| ![](templates/templates_timber_floor_5_systems_ribbed_above.webp) Ribs under a panel, glued | ![](templates/templates_timber_floor_5_systems_ribbed_below.webp) Ribs on a panel, glued | ![](templates/templates_timber_floor_5_systems_box.webp) Ribs between two panels, glued | ![](templates/templates_timber_floor_5_systems_beams_butted.webp) Solid beams, a board on them |
| ![](templates/templates_timber_floor_5_systems_beams_comb.webp) Solid beams, a comb of two tongues | ![](templates/templates_timber_floor_5_systems_glulam_elements.webp) Glulam elements on edge, a strip in the joint | ![](templates/templates_timber_floor_5_systems_boards_dowelled.webp) Boards on edge, dowelled | ![](templates/templates_timber_floor_5_systems_boards_nailed.webp) Boards on edge, nailed |
| ![](templates/templates_timber_floor_5_systems_cross_laminated_3.webp) Cross-laminated 40 40 40 | ![](templates/templates_timber_floor_5_systems_cross_laminated_5.webp) Cross-laminated 40 20 40 20 40 | ![](templates/templates_timber_floor_5_systems_cross_laminated_7.webp) Cross-laminated 30 40 30 40 30 40 30 | ![](templates/templates_timber_floor_5_systems_veneer.webp) Veneer lumber 75 |

```cpp
/// One straight piece of a floor product, in mm from its corner at its underside.
struct FloorPart {
    std::string name;
    FloorSection section;
    double start = 0.0; // Where it starts along its run.
    double end = 0.0; // Where it ends.
    size_t run = 0; // 0 along the span, 1 across it, 2 upright.
    std::vector<std::string> pockets; // The parts it is let into.
};
```

\include{lineno} templates_timber_floor_5_systems.cpp

---

## Producers {#timber_floor_producers}

Best wood Schneider's CLT BOX, Lignatur, Lignotrend, Novatop and Kielsteg with the sections of their approvals.

|  |  |  |  |
| --- | --- | --- | --- |
| ![](templates/templates_timber_floor_6_producers_clt_box_closed.webp) Schneider CLT BOX, closed, 240 | ![](templates/templates_timber_floor_6_producers_clt_box_open.webp) Schneider CLT BOX, open, 220 | ![](templates/templates_timber_floor_6_producers_clt_box_roof.webp) Schneider CLT BOX roof, 220 | ![](templates/templates_timber_floor_6_producers_lignatur_box.webp) Lignatur box LKE, 200 |
| ![](templates/templates_timber_floor_6_producers_lignatur_surface.webp) Lignatur surface LFE, 200 | ![](templates/templates_timber_floor_6_producers_lignatur_silence.webp) Lignatur LFE silence, 240 | ![](templates/templates_timber_floor_6_producers_lignatur_shell.webp) Lignatur shell LSE, 200 | ![](templates/templates_timber_floor_6_producers_lignotrend_rib.webp) Lignotrend Rippe Q3, 190 |
| ![](templates/templates_timber_floor_6_producers_lignotrend_block.webp) Lignotrend Block Q3, 190 | ![](templates/templates_timber_floor_6_producers_novatop_element.webp) Novatop Element A2, 240 | ![](templates/templates_timber_floor_6_producers_novatop_open.webp) Novatop Open, 247 | ![](templates/templates_timber_floor_6_producers_kielsteg.webp) Kielsteg 280-40-102 |
| ![](templates/templates_timber_floor_6_producers_kielsteg_deep.webp) Kielsteg 730-80-134 |  |  |  |

Their sections:

|  |  |
| --- | --- |
| ![](templates/templates_timber_floor_6_producers_clt_box_closed_section.webp) CLT BOX | ![](templates/templates_timber_floor_6_producers_lignatur_box_section.webp) Lignatur LKE |
| ![](templates/templates_timber_floor_6_producers_lignatur_surface_section.webp) Lignatur LFE | ![](templates/templates_timber_floor_6_producers_lignotrend_rib_section.webp) Lignotrend Rippe Q3 |
| ![](templates/templates_timber_floor_6_producers_novatop_element_section.webp) Novatop Element | ![](templates/templates_timber_floor_6_producers_kielsteg_section.webp) Kielsteg |

| Element | Depths mm | Width mm | Length m | Parts mm | kg/m² | Source |
| --- | --- | --- | --- | --- | --- | --- |
| CLT BOX closed | 220 to 480 by 20 | 1000 to 1200 | 2.3 to 16 | cross-laminated 60 (below 60 or 90), three glulam ribs 80, 100, 120 x 100 to 360 | 60 at 220, 64 at 260, 71 at 340 (computed) | [ETA-21/0336](https://www.schneider-holz.com/site/assets/files/6878/best-wood-schneider_eta_clt-box_de.pdf), [catalogue 2026](https://www.schneider-holz.com/de/service/downloads/download/produktuebersicht-holz-schweiz/) |
| CLT BOX open, roof | 160 to 460 by 20 | 900 to 1200 | to 16 | one flange 60 (90), ribs to 400, the roof's two | | as above |
| Lignatur LKE | 120 to 200 by 20, 240 to 320 by 20 | 200 | 12 | shells 31, webs 32, tongue 13 x 10 | 38 at 120, 50 at 200, 56 at 240, 74 at 320 | [ETA-11/0137](https://www.lignatur.ch/fileadmin/ablage/downloads/das-element/eta-11-0137-lignatur-de.pdf), [weights](https://www.lignatur.ch/produkt/statik/kennwerte), [spans](https://www.lignatur.ch/produkt/statik/vorstatik) |
| Lignatur LFE | 120 to 240 by 20, 280, 320, 360 | 1000 | 16 | boards 31 or 40, five webs 31 | 33 at 120, 39 at 200, 42 at 240, 45 at 280 | as above |
| Lignotrend Rippe Q3 | 150 to 450 by 20 | 625 | 15, from 190 18 | three stacks of boards 20 and 26.5, 70 to 75 wide; closed layer 32 | 49 at 150, 55 at 190, 66 at 250, 73 at 290, 120 at 450 | [Z-9.1-555](https://www.dibt.de/pdf_storage/2025/Z-9.1-555%281.9.1-38!22%29.pdf), [Rippe](https://lignotrend.com/_Resources/Persistent/5/d/e/d/5ded1af7816ec8419aab71ca8183ca7a4db2b885/2024-08-14_TD%20LIGNO%20Rippe-x_D-k.pdf), [Block](https://lignotrend.com/_Resources/Persistent/e/1/5/0/e150836a8beb14c43a7b97cae60426baaf4b709d/2026-02-18_TD%20LIGNO%20Block-x_DE_k.pdf) |
| Lignotrend Block Q3 | 150 to 450 by 20 | 625 | 15, from 190 18 | the same, closed above 45 | 50 at 150, 57 at 190, 75 at 290, 142 at 450 | as above |
| Novatop Element A2 | 160 to 400 by 20 | 1030, 2090, 2450 | 6, to 12 | three-layer panels 27, ribs 27 every 340, across them every 600 to 1000 | 32 at 160, 35 at 240, 39 at 320, 42 at 400 | [Element](https://novatop-system.com/wp-content/uploads/TD_NOVATOP_ELEMENT_EN.pdf), [Open](https://novatop-system.com/wp-content/uploads/TD_NOVATOP_OPEN_EN.pdf), [ETA-11/0310](https://novatop-system.com/wp-content/uploads/NOVATOP_Element_ETA_EN.pdf) |
| Novatop Open | 227, 247, 267 | 1030, 2090, 2450 | 6, to 12 | panel 27, ribs 60 x 200, 220, 240 every 625 | | as above |
| Kielsteg | 280 to 800, fifteen | 1200 | 2 to 35 | chords 35 to 80 x 85 to 134, plywood webs 4, from 485 oriented strand 8 to 12 | 43 to 152 | [ETA-18/1014](https://www.dibt.de/pdf_storage/2025/ETA-18!1014%288.03.04-76!24%29e.pdf), [guide](https://www.kielsteg.com/elements-guide/) |

| Span m | CLT BOX 60 / 60, depth / rib, at g 2.5, q 3.0 | Lignatur LFE in a house, q 2.8, g 1.0 | Novatop Element A2, g 2.0, q 3.0 | Kielsteg, q 2, 6 Hz |
| --- | --- | --- | --- | --- |
| 4 | | 160 (4.3) | 160 | |
| 5 | | 200 (5.2) | 200 | |
| 6 | 220 / 80 | 240 (6.1) | 260 | |
| 7 | 260 / 80 | 320 (7.7) | 300 | |
| 8 | 300 / 80 | 360 (8.8) | 360 | 280-35 (8.3) |
| 9 | 340 / 80 | | | 330-40 (9.3) |
| 10 | 380 / 80 | | | 370-60 (10.3) |
| 11 | 420 / 100 | | | 485-50 (11.1) |
| 12 | | | | 560-50 (12.1) |

Single spans as each producer gives them, under its own loads (kN/m²) and limits, so the columns are not comparable without them; the number in brackets is the span of that depth.

\include{lineno} templates_timber_floor_6_producers.cpp

### Ribs, voids, dowels {#timber_floor_ribs}

|  |  |  |  |
| --- | --- | --- | --- |
| ![](templates/templates_timber_floor_9_ribs_clt_rib.webp) Hasslacher, plate 120 on ribs 140 x 280 | ![](templates/templates_timber_floor_9_ribs_clt_rib_box.webp) Derix box, plates 100, ribs 120 x 300 | ![](templates/templates_timber_floor_9_ribs_void_panel.webp) Derix void panel 80 / 120 / 80 | ![](templates/templates_timber_floor_9_ribs_lvl_rib.webp) Stora Enso LVL rib, open |
| ![](templates/templates_timber_floor_9_ribs_lvl_rib_strips.webp) Stora Enso LVL rib, semi-open | ![](templates/templates_timber_floor_9_ribs_dowelled_layers.webp) Dowelled layers (Holz100, Nur-Holz) | ![](templates/templates_timber_floor_9_ribs_concrete_ribs_open.webp) Cree, ZECH: ribs 240 x 280, the notches | ![](templates/templates_timber_floor_9_ribs_concrete_ribs.webp) Cree, ZECH with its concrete 100 |

| Element | Sizes mm | Width m | Length m | Span m | Source |
| --- | --- | --- | --- | --- | --- |
| Hasslacher rib panel | plate 80 to 240, ribs 80 to 280 x 80 to 1280; plate 120 at g 1.0: 120 x 160 at 6 m, 140 x 280 at 8, 160 x 440 at 10 | 1.25, to 3.20 | 24 | 6 to 11 | [Hasslacher](https://www.hasslacher.com/data/_dateimanager/broschuere/Rippenplattendecke-DE.pdf) |
| KLH rib | plate 110, ribs 140 at 600: 160 at 6 m, 240 at 8, 360 at 10 (g 1.0) | 2.95 | 16.5 | from 6 | [KLH](https://www.klh.at/wp-content/uploads/2019/09/klh-rib-elements-en.pdf) |
| Derix rib, box | to 540, to 600 deep; plates to 120, ribs 60 to 180 x 100 to 480, clear to 600 | 3.00 | 10 | from 6 | [Derix](https://derix.de/media/freie-dokumente/XXL-Deckensysteme-aus-Holz.pdf) |
| Derix void panel | to 360: plates to 80, strips 60 to 120 x 120 to 186, voids to 372 | 2.15, 2.70, 3.25 | 18.5 | 6 to 10 | as above |
| Woodtec Fankhauser rib, box | to 400; plates 27 to 100, ribs 60 to 100 x 300 at 500 to 800 | 2.70 | 10 | | [Z-9.1-931](https://www.dibt.de/pdf_storage/2025/Z-9.1-931%281.9.1-9!21%29.pdf) |
| Stora Enso LVL rib | ribs 39 to 75 x 100 to 1150, panels 22 to 73 | 2.40, 2.50 | 6 to 18 | | [ETA-18/1132](https://storaenso.com/-/media/documents/download-center/certificates/wood-products-approvals-and-certificates/eta/eta-sylva-lvl-rib-by-stora-enso---may-2023.pdf) |
| Kerto-Ripa | veneer ribs glued to veneer panels, open, semi-open, box; the floor's rib pitch is not published | 1.80, 2.40, 2.50 | 6 to 20 | 5 to 10 | [Metsä](https://www.metsagroup.com/metsawood/explore-wood/kerto-lvl-applications/floor-elements/kerto-ripa-floor-elements/) |
| Holz100 | to 400; boards from 24, squared timber from 40, beech dowels 20; the layers here are an example | 3.00 | 10 | | [ETA-13/0785](https://www.dibt.de/pdf_storage/2018/ETA-13!0785%288.03.04-92!17%29.pdf) |
| ZECH, as Cree | 380: ribs 240 x 280, concrete 100, notches 250 x 200 x 40, two pins 10 x 300 each | 2.60 to 2.80 | 6.55 to 7.85 | 6.25 to 7.55 | [Z-9.1-911](https://www.dibt.de/pdf_storage/2024/Z-9.1-911%281.9.1-15!24%29.pdf) |
| TS3 point-supported | cross-laminated 180 on 4 x 4 m, 230 on 5 x 5, 280 on 6 x 6, 300 on 7 x 7, joints grouted 4 wide | 3.50 | 16 | 4 to 9 both ways | [TS3](https://www.ts3.biz/media/docs/downloads/en/2024.06_Factsheet_Architektur_en.pdf) |

\include{lineno} templates_timber_floor_9_ribs.cpp

---

## Joints and Concrete {#timber_floor_shear}

Joints between cross-laminated panels:

|  |  |  |  |
| --- | --- | --- | --- |
| ![](templates/templates_timber_floor_7_shear_joint_cover.webp) Cover board and strip | ![](templates/templates_timber_floor_7_shear_joint_grout.webp) Grouted joint | ![](templates/templates_timber_floor_7_shear_joint_keys.webp) Wedge keys | ![](templates/templates_timber_floor_7_shear_joint_blocks.webp) Steel shear blocks |

Ten ways to hold concrete to timber, without the concrete. The book shows them without sizes: the notches, lugs, locks and bars here are assumed, but the crossed pins at 45° and the plates 45 deep on both sides, which follow the approvals [ETA-13/0699](https://www.dibt.de/pdf_storage/2022/ETA-13!0699%288.03.03-36!22%29.pdf) and [Z-9.1-557](https://www.dibt.de/pdf_storage/2020/Z-9.1-557%281.9.1-41!19%29.pdf).

|  |  |  |  |
| --- | --- | --- | --- |
| ![](templates/templates_timber_floor_7_shear_concrete_stepped_open.webp) Boards of two depths | ![](templates/templates_timber_floor_7_shear_concrete_micro_open.webp) Small notches all over | ![](templates/templates_timber_floor_7_shear_concrete_notched_open.webp) Deep notches and pins | ![](templates/templates_timber_floor_7_shear_concrete_lugs_open.webp) Lugs of a precast slab in pockets |
| ![](templates/templates_timber_floor_7_shear_concrete_wave_open.webp) Wave-shaped recess in veneer lumber | ![](templates/templates_timber_floor_7_shear_concrete_crossed_open.webp) Crossed pins | ![](templates/templates_timber_floor_7_shear_concrete_locks_open.webp) Flat steel locks | ![](templates/templates_timber_floor_7_shear_concrete_bars_open.webp) V-shaped bars, glued in |
| ![](templates/templates_timber_floor_7_shear_concrete_plates_open.webp) Perforated plates, glued in | ![](templates/templates_timber_floor_7_shear_concrete_glued_open.webp) Glue alone |  |  |

| whole | | |
| --- | --- | --- |
| ![](templates/templates_timber_floor_7_shear_concrete_stepped.webp) | ![](templates/templates_timber_floor_7_shear_concrete_micro.webp) | ![](templates/templates_timber_floor_7_shear_concrete_notched.webp) |

\include{lineno} templates_timber_floor_7_shear.cpp

---

## Build-ups {#timber_floor_build_ups}

The 62 floors of the book's tables 11 to 24, every layer, with what it weighs and does (`FloorBuildUp`, `data/timber_floors/build_ups.json`). In a sample every layer ends 90 mm before the one under it; membranes are not drawn.

![](templates/templates_timber_floor_8_build_ups_all.webp)

Source: Kolb, Kolb, Müller, *Holzbau mit System*, 2024, p. 414 to 426. R'w is airborne, L'n,w impact sound, for comparing in a first design; energy in kWh and greenhouse gas in kg CO2-eq, both per m² and year; *on request* from the producer. Depths are as printed; in fifteen rows the printed layers add up to a few millimetres more or less, or the book counts resilient clips.

### Structural Floors (Raw Floors) {#timber_floor_table_11}

![](templates/templates_timber_floor_8_build_ups_table_11.webp)

| Layers from the top, mm | Depth mm | kg/m² | R'w dB | L'n,w dB | Energy | Greenhouse gas |
| --- | --- | --- | --- | --- | --- | --- |
| Timber boarding 21; Joist layer 140/200 | 221 | 29 | 26 | 90 | 0.40 | 0.05 |
| Three-layer panel glued to rib 27; Rib 60/180; Mineral-fibre board, 40–70 kg/m3 80 | 234 | 36 | 36 | 86 | 1.20 | 0.27 |
| Box element, system provider 200 | 200 | 47 | 33 | 89 | 0.29 | 0.07 |
| Glulam / planks / solid timber beams with tongue-and-groove 120 | 120 | 54 | 37 | 88 | 1.35 | 0.30 |
| Brettstapel 160 | 160 | 72 | 36 | 82 | 0.65 | 0.17 |
| Cross-laminated timber (CLT) 160 | 160 | 72 | 40 | 88 | 1.81 | 0.39 |
| Composite concrete topping 70; Timber boarding 21; Joist layer 140/200 | 273 | 204 | 42 | 88 | 1.87 | 0.66 |
| Composite concrete topping 70; Brettstapel 160 | 230 | 247 | 44 | 85 | 2.13 | 0.73 |

### Timber Joist Floors, Load-Bearing Structure Exposed {#timber_floor_table_12}

![](templates/templates_timber_floor_8_build_ups_table_12.webp)

| Layers from the top, mm | Depth mm | kg/m² | R'w dB | L'n,w dB | Energy | Greenhouse gas |
| --- | --- | --- | --- | --- | --- | --- |
| Particleboard, floating installation 25; Impact sound insulation fleece 3; Battens, fastened 60/60; Mineral-fibre board, 40–70 kg/m3 60; Timber boarding 21; Joist layer 140/200 | 309 | 52 | 40 | 73 | 4.43 | 0.97 |
| Cement screed, 120 kg/m2 50, 60; Separating layer; Mineral-fibre board, 80–100 kg/m3 15; Timber boarding 21; Joist layer 140/200 | 296 | 150 | 46 | 70 | 1.47 | 0.54 |

### Timber Joist Floors, Load-Bearing Structure Exposed and Ballasted {#timber_floor_table_13}

![](templates/templates_timber_floor_8_build_ups_table_13.webp)

| Layers from the top, mm | Depth mm | kg/m² | R'w dB | L'n,w dB | Energy | Greenhouse gas |
| --- | --- | --- | --- | --- | --- | --- |
| Particleboard, floating installation 25; Mineral-fibre board, 80–110 kg/m3 10; Concrete slabs 500/500 mm, dry, 120 kg/m2 50; Impact sound insulation fleece 3; Timber boarding 21; Joist layer 140/200 | 309 | 168 | 50 | 66 | 4.91 | 1.31 |
| Particleboard, floating installation 25; Impact sound insulation fleece 3; Battens, fastened 50/50; Dried sand, 1500 kg/m3 50; Trickle protection membrane; Timber boarding 21; Joist layer 140/200 | 301 | 127 | 50 | 69 | 4.54 | 0.98 |
| Fermacell screed element 30; Fermacell honeycomb fill 30; Trickle protection membrane; Timber boarding 21; Joist layer 140/200 | 281 | 99 | 49 | 63 | 3.99 | 1.00 |
| Cement screed, 120 kg/m2 50, 60; Separating layer; Mineral-fibre board, 80–110 kg/m3 30; Dried sand, 1500 kg/m3 30; Trickle protection membrane; Decking boards 22; Joist layer 140/220 | 342 | 203 | 56 | 58 | 2.00 | 0.65 |

### Timber Joist Floors, Load-Bearing Structure Partially Exposed {#timber_floor_table_14}

![](templates/templates_timber_floor_8_build_ups_table_14.webp)

| Layers from the top, mm | Depth mm | kg/m² | R'w dB | L'n,w dB | Energy | Greenhouse gas |
| --- | --- | --- | --- | --- | --- | --- |
| Particleboard, floating installation 25; Impact sound insulation fleece 2 × 6; Timber boarding 21; Joist layer 140/200; Mineral-fibre board, 30–70 kg/m3 80; Particleboard 16 | 252 | 62 | 46 | 67 | 7.61 | 1.62 |
| Cement screed, 120 kg/m2 50, 60; Separating layer; Mineral-fibre board, 80–110 kg/m3 15; Timber boarding 21; Joist layer 140/200; Mineral-fibre board, 30–70 kg/m3 80; Particleboard 16 | 296 | 155 | 50 | 60 | 2.57 | 0.79 |

### Timber Joist Floors, Load-Bearing Structure Partially Exposed and Ballasted {#timber_floor_table_15}

![](templates/templates_timber_floor_8_build_ups_table_15.webp)

| Layers from the top, mm | Depth mm | kg/m² | R'w dB | L'n,w dB | Energy | Greenhouse gas |
| --- | --- | --- | --- | --- | --- | --- |
| Particleboard, floating installation 25; Mineral-fibre board, 80–110 kg/m3 10; Concrete slabs 500/500 mm, dry, 120 kg/m2 50; Impact sound insulation fleece 3; Timber boarding 21; Joist layer 140/200; Mineral-fibre board, 30–70 kg/m3 80; Particleboard 16 | 309 | 197 | 55 | 58 | 6.01 | 1.57 |
| Particleboard, floating installation 25; Mineral-fibre board, 80–110 kg/m3 10; Battens, fastened 50/50; Dried sand, 1500 kg/m3 50; Trickle protection membrane; Timber boarding 21; Joist layer 140/200; Mineral-fibre board, 30–70 kg/m3 80; Particleboard 16 | 306 | 144 | 55 | 55 | 3.34 | 0.73 |

### Timber Joist Floors, Clad {#timber_floor_table_16}

![](templates/templates_timber_floor_8_build_ups_table_16.webp)

| Layers from the top, mm | Depth mm | kg/m² | R'w dB | L'n,w dB | Energy | Greenhouse gas |
| --- | --- | --- | --- | --- | --- | --- |
| Sanded medium-density hardboard 5.4; Timber boarding 21; Joist layer 140/200; Mineral-fibre board, 30–70 kg/m3 80; Battens mounted with resilient clips 30; Particleboard 16 | 273 | 59 | 51 | 68 | 2.14 | 0.47 |
| Particleboard, floating installation 25; Impact sound insulation fleece 2 × 6; Timber boarding 21; Joist layer 140/200; Mineral-fibre board, 30–70 kg/m3 80; Battens mounted with resilient clips 30; Particleboard 16 | 298 | 72 | 56 | 57 | 2.74 | 1.68 |
| Cement screed, 120 kg/m2 50, 60; Separating layer; Mineral-fibre board, 80–110 kg/m3 15; Timber boarding 21; Joist layer 140/200; Mineral-fibre board, 30–70 kg/m3 80; Battens mounted with resilient clips 30; Particleboard 16 | 346 | 174 | 60 | 55 | 2.69 | 0.81 |
| Cement screed, 120 kg/m2 50, 60; Separating layer; Mineral-fibre board, 80–110 kg/m3 15; Timber boarding 21; Joist layer 140/200; Mineral-fibre board, 30–70 kg/m3 80; Battens mounted with resilient clips 30; Gypsum fibreboards 2 × 12.5 | 355 | 183 | 63 | 53 | 4.20 | 1.16 |

### Timber Joist and Hollow Box Floors, Clad and Ballasted {#timber_floor_table_17}

![](templates/templates_timber_floor_8_build_ups_table_17.webp)

| Layers from the top, mm | Depth mm | kg/m² | R'w dB | L'n,w dB | Energy | Greenhouse gas |
| --- | --- | --- | --- | --- | --- | --- |
| Particleboard, floating installation 25; Mineral-fibre board, 80–110 kg/m3 10; Concrete slabs 500/500 mm, dry, 120 kg/m2 50; Impact sound insulation fleece 3; Timber boarding 21; Joist layer 140/200; Mineral-fibre board, 30–70 kg/m3 80; Battens mounted with resilient clips 30; Gypsum fibreboards 2 × 12.5 | 370 | 197 | 63 | 44 | 7.62 | 1.93 |
| Particleboard, floating installation 25; Mineral-fibre board, 80–110 kg/m3 10; Dried sand, 1500 kg/m3, battens fastened 50; Trickle protection membrane; Timber boarding 21; Joist layer 140/200; Mineral-fibre board, 30–70 kg/m3 80; Battens mounted with resilient clips 30; Particleboard 16 | 356 | 144 | 62 | 45 | 3.46 | 0.75 |
| Fermacell screed element 30; Fermacell honeycomb fill 30; Trickle protection membrane; Timber boarding 21; Joist layer 140/200; Mineral-fibre board, 30–70 kg/m3 80; Battens mounted with resilient clips 30; Fermacell HD board 15 | 329 | 62 | 62 | 47 | 5.30 | 1.30 |
| Cement screed, 120 kg/m2, separating layer 50, 60; Mineral-fibre board, 80–110 kg/m3 30; Dried sand, 1500 kg/m3 60; Trickle protection membrane; Decking boards 22; Joist layer 140/220; Mineral-fibre board, 30–70 kg/m3 100; Battens mounted with resilient clips 30; Gypsum fibreboards 12.5 | 424 | 267 | 63 | 44 | 3.73 | 1.06 |
| Cement screed, 170 kg/m2 80; Separating layer; Mineral-fibre board, 80–110 kg/m3 40; Concrete slabs, dry, 120 kg/m2, 500 × 500 mm, glued onto hollow box element 50; Hollow box element 234; Mineral-fibre board, 30–70 kg/m3 60 | 404 | 327 | 61 | 48 | 2.90 | 1.25 |
| Cement screed, 170 kg/m2 80; Separating layer; Mineral-fibre board, 80–110 kg/m3 40; Concrete slabs, dry, 120 kg/m2, 500 × 500 mm, glued onto hollow box element 50; Hollow box element 234; Concrete slabs, dry, 120 kg/m2, 500 × 500 mm, glued / or dry sand, 1500 kg/m3, with trickle protection membrane 50 / 80 | 404 | 435 | 62 | 44 | 3.00 | 1.27 |
| Cement screed, 170 kg/m2 80; Separating layer; Mineral-fibre board, 80–110 kg/m3 40; Gypsum fibreboard, stapled onto hollow box element, fastener spacing 250–300 mm 15; Hollow box element 204; Dry sand, 1500 kg/m3, with trickle protection membrane or grit fill, 1500 kg/m3 150 | 339 | 432 | 63 | 44 | 4.65 | 1.61 |
| Cement screed, 170 kg/m2 80; Separating layer; Mineral-fibre board, 80–110 kg/m3 40; Gypsum fibreboard, stapled onto hollow box element, fastener spacing 250–300 mm 15; Hollow box element 204; Dry sand, 1500 kg/m3, with trickle protection membrane or grit fill, 1500 kg/m3 80 | 339 | 432 | 63 | 45 | 4.10 | 1.38 |

### Solid Timber Floors, Various Insulation Interlayers {#timber_floor_table_18}

![](templates/templates_timber_floor_8_build_ups_table_18.webp)

| Layers from the top, mm | Depth mm | kg/m² | R'w dB | L'n,w dB | Energy | Greenhouse gas |
| --- | --- | --- | --- | --- | --- | --- |
| Particleboard 25; Extra-porous wood-fibre impact sound insulation board 10; Solid timber floor / glulam / dowel-joined planks 120 | 155 | 74 | 49 | 71 | 3.27 | 0.7 |
| Laminated hardboard 25; Mineral-fibre impact sound insulation board, 80–110 kg/m3 10; Solid timber floor / glulam / dowel-joined planks 120 | 155 | 72 | 49 | 67 | 4.28 | 1.03 |
| Particleboard 25; Homogeneous cork board 10; Solid timber floor / glulam / dowel-joined planks 120 | 155 | 73 | 44 | 73 | 3.75 | 0.75 |
| Particleboard 25; Polyester impact sound insulation board, elasticised 20; Solid timber floor / glulam / dowel-joined planks 120 | 165 | 73 | 47 | 68 | 3.17 | 0.67 |
| Particleboard 25; Dry insulating fill 30; Solid timber floor / glulam / dowel-joined planks 120 | 175 | 76 | 49 | 75 | 3.40 | 0.82 |

### Solid Timber Floors, Various Subfloor Boards or Cement Screeds {#timber_floor_table_19}

![](templates/templates_timber_floor_8_build_ups_table_19.webp)

| Layers from the top, mm | Depth mm | kg/m² | R'w dB | L'n,w dB | Energy | Greenhouse gas |
| --- | --- | --- | --- | --- | --- | --- |
| Particleboard 25; Mineral-fibre impact sound insulation board, 80–110 kg/m3 10; Solid timber floor / glulam / dowel-joined planks 120 | 155 | 72 | 49 | 67 | 2.79 | 0.59 |
| Gypsum fibreboards 20 mm, bonded 30; Mineral-fibre impact sound insulation board, 80–110 kg/m3 10; Solid timber floor / glulam / dowel-joined planks 120 | 150 | 79 | 47 | 68 | 3.33 | 0.73 |
| Cement screed, 120 kg/m2 50, 60; Separating layer; Mineral-fibre impact sound insulation board, 80–110 kg/m3 10; Solid timber floor / glulam / dowel-joined planks 120 | 180 | 164 | 53 | 64 | 2.29 | 0.72 |
| Cement screed, 120 kg/m2 50, 60; Separating layer; Mineral-fibre board, 80–110 kg/m3 30; Brettstapel, nailed or dowelled 120 | 200 | 176 | 53 | 57 | 1.77 | 0.62 |

### Solid Timber Floors with Ceiling Linings {#timber_floor_table_20}

![](templates/templates_timber_floor_8_build_ups_table_20.webp)

| Layers from the top, mm | Depth mm | kg/m² | R'w dB | L'n,w dB | Energy | Greenhouse gas |
| --- | --- | --- | --- | --- | --- | --- |
| Cement screed, 120 kg/m2 50, 60; Separating layer; Mineral-fibre impact sound insulation board, 80–110 kg/m3 15; Hollow box floor / LIGNATUR box element 140; Battens mounted with resilient clips 30; Gypsum fibreboard 18 | 257 | 172 | 59 | 52 | 3.50 | 0.99 |
| Cement screed, 120 kg/m2 50, 60; Separating layer; Mineral-fibre impact sound insulation board, 80–110 kg/m3 30; Dried sand, 1500 kg/m3 40; Trickle protection membrane; Brettstapel, nailed or dowelled 120; Battens mounted with resilient clips 30; Gypsum fibreboard 12.5 | 287 | 253 | 62 | 47 | 3.64 | 1.04 |

### Solid Timber Floors; Wearing Surface with Floorboards {#timber_floor_table_21}

![](templates/templates_timber_floor_8_build_ups_table_21.webp)

| Layers from the top, mm | Depth mm | kg/m² | R'w dB | L'n,w dB | Energy | Greenhouse gas |
| --- | --- | --- | --- | --- | --- | --- |
| Floorboards 21; Wood-fibre board, T&G, pre-assembled with battens, system provider 40; Solid timber floor / glulam / dowel-joined planks 120 | 181 | 73 | 49 | 67 | 5.22 | 1.08 |
| Floorboards 21; Wood-fibre board, T&G, pre-assembled with battens, system provider 40; Extra-porous wood-fibre board laid between battens 8; Dried sand, 1500 kg/m3, battens fastened 30; Trickle protection membrane; Solid timber floor / glulam / dowel-joined planks 120 | 219 | 125 | 53 | 60 | 5.80 | 1.21 |

### Solid Timber Floors, Ballasted {#timber_floor_table_22}

![](templates/templates_timber_floor_8_build_ups_table_22.webp)

| Layers from the top, mm | Depth mm | kg/m² | R'w dB | L'n,w dB | Energy | Greenhouse gas |
| --- | --- | --- | --- | --- | --- | --- |
| Wood-fibre board, hard 23; Wood-fibre board 22; Dried sand, 1500 kg/m3, battens fastened 50; Trickle protection membrane; Solid timber floor / glulam / dowel-joined planks 140 | 235 | 165 | 55 | 58 | 4.82 | 1.16 |
| Cement screed, 120 kg/m2 50; Separating layer; Mineral-fibre impact sound insulation board, 80–110 kg/m3 30; Concrete slabs 500/500 mm, dry, 100 kg/m2 40; Brettstapel, nailed 140 | 260 | 285 | 61 | 52 | 2.41 | 0.96 |
| Cement screed, 120 kg/m2 50, 60; Separating layer; Mineral-fibre impact sound insulation board, 80–110 kg/m3 30; Dried sand, 1500 kg/m3 40; Trickle protection membrane; Brettstapel, nailed 120 | 240 | 237 | 61 | 52 | 2.10 | 0.69 |

### Solid Timber Floors, Timber–Concrete Composite {#timber_floor_table_23}

![](templates/templates_timber_floor_8_build_ups_table_23.webp)

| Layers from the top, mm | Depth mm | kg/m² | R'w dB | L'n,w dB | Energy | Greenhouse gas |
| --- | --- | --- | --- | --- | --- | --- |
| Cement screed, 120 kg/m2 80; Separating layer; Mineral-fibre impact sound insulation board, 80–110 kg/m3 40; In-situ composite concrete 120; Brettstapel 140 | 380 | 537 | 62 | 44 | 4.63 | 1.79 |
| Cement screed, 170 kg/m2 80; Separating layer; Mineral-fibre impact sound insulation board, 80–110 kg/m3 40; In-situ composite concrete 120; Brettstapel 140; Service cavity, damped 25; Gypsum fibreboard on batten grid 18 | 423 | 555 | 62 | 45 | 6.37 | 2.18 |
| Cement screed, 170 kg/m2 80; Separating layer; Mineral-fibre impact sound insulation board, 80–110 kg/m3 40; In-situ composite concrete 120; Brettstapel 140; Service cavity 140; Gypsum fibreboard on batten grid 18 | 538 | 559 | 63 | 37 | 6.5 | 2.21 |

### System Providers {#timber_floor_table_24}

![](templates/templates_timber_floor_8_build_ups_table_24.webp)

| Layers from the top, mm | Depth mm | kg/m² | R'w dB | L'n,w dB | Energy | Greenhouse gas |
| --- | --- | --- | --- | --- | --- | --- |
| Subfloor board 28; Isover Akustic EP2, s′ up to  15 MN/m3 30; LIGNATUR box element 200 | 258 | 68 | 49 | 67 | 2.03 | 0.43 |
| Subfloor particleboard 28; Isover Akustic EP2, s′ up to  15 MN/m3 30; Fermacell honeycomb fill 30; LIGNATUR box element, silence 12 with fill 75 kg/m2 200 | 288 | 213 | 69 | 47 | on request | on request |
| Knauf gypsum fibreboard GIFAfloor 18; Knauf gypsum fibreboard GIFAfloor 32; Knauf TPT03, s′ = 9 MN/m3 30; Grit fill 30; LIGNATUR surface element with fill 50 kg/m2 200 | 310 | 218 | 73 | 47 | on request | on request |
| Cement screed, 120 kg/m2 50; Isover Akustic EP1, s′ up to  7 MN/m3 40; Grit fill 70; LIGNATUR surface element silence 12 240 | 400 | 296 | 72 | 45 | on request | on request |
| Hollow floor 32 mm GIFAfloor FHB 132; Cement screed, 120 kg/m2 50; Isover Akustic EP1, s′ up to  7 MN/m3 40; LIGNATUR surface element with fill 50 kg/m2 200 | 422 | 265 | 74 | 43 | on request | on request |
| Cement screed 50; Separating layer; Mineral-fibre impact sound insulation, Isover Akustic EP1 (dyn. stiffness s′ = 5 MN/m3) 40; Pressure distribution board, wood-fibre softboard Gutex Standard-n 15; Cross-laminated timber floor element LIGNO® Rippe-v with factory-finished timber soffit 250 | 355 | 136 | 66 | 55 | on request | on request |
| Dry screed element Fermacell 30; Pressure distribution board, wood-fibre softboard Gutex Standard-n 15; Cross-laminated timber floor element LIGNO® Rippe-v with factory-finished timber soffit, with grit fill 88 kg/m2 250 | 295 | 140 | 67 | 47 | on request | on request |
| Cement screed, 120 kg/m2 50; Separating layer; Mineral-fibre impact sound insulation, Isover Akustic EP1, s′ = 5 MN/m3 40/35; Pressure distribution board, wood-fibre softboard Gutex Standard-n 15; Cross-laminated timber floor element LIGNO® Rippe-v with factory-finished timber soffit, with grit fill approx. 196 kg/m2 250 | 355 | 251 | 78 | 37 | on request | on request |
| Timber decking grating 26; Support battens 40; Sylomer bearing; s′ 3 12; Concrete slabs, 40 mm; crushed stone grit (grading 5/8) between, 39 kg/m2 50; EPDM roofing membrane, alwitra 2.5; Flat roof insulation board EPS 035 DAA (dyn. stiffness s′ = 30 MN/m3) 200; Pressure distribution board OSB 4 TOP (Egger) 22; Cross-laminated timber floor element LIGNO® Rippe-v with factory-finished timber soffit, with grit fill approx. 98 kg/m2 250 | 593 | 155 | 64 | 35 | on request | on request |
| Gypsum fibreboard 20; Wood-fibre board 8; Screed honeycomb with fill; 90 kg/m2 60; NOVATOP element 240 | 328 | 151 | 59 | 60 | on request | on request |
| Gypsum fibreboard 20; Wood-fibre board 40; Screed honeycomb with fill; 45 kg/m2 30; NOVATOP element with limestone grit fill, 40 kg/m2 240 | 330 | 161 | 62 | 54 | on request | on request |
| Cement boards, 115 kg/m2 50; Mineral-fibre boards 40; NOVATOP element with limestone grit fill, 40 kg/m2 240 | 330 | 153 | 58 | 67 | on request | on request |
| Oak parquet flooring 12; Steico Underfloor 5; Cement screed 50; Isover TDPT 20; Isover TDPT 30; Stralon 6; NOVATOP element with timber grid 180 mm and limestone grit fill 80 kg/m2 240 | 363 | 244 | 63 | 44 | on request | on request |

\include{lineno} templates_timber_floor_8_build_ups.cpp

<!-- assembly:begin -->
## Assembly Sequence {#timber_floor_assembly_sequence}

The beams, then the ribs and the boards or the panels in their order, what joins them last, the earth rammed between its beams; a dowel-laminated panel's boards one by one. Every move is a `ModuleSequence` move: the module taken apart piece by piece, each piece swept as a solid against every piece still in, the moving ones drawn brown, a GIF per kind of module and one of the whole.

| assembly | | |
|---|---|---|
| **Systems**, 132 elements, 124 moves | | |
| ![box, 10 pieces](assembly/assembly_templates_timber_floor_1_systems_box.gif) box, 10 pieces | ![composite, 4 pieces](assembly/assembly_templates_timber_floor_1_systems_composite.gif) composite, 4 pieces | ![cross laminated, 5 pieces](assembly/assembly_templates_timber_floor_1_systems_cross_laminated.gif) cross laminated, 5 pieces |
| ![dowelled, 62 pieces](assembly/assembly_templates_timber_floor_1_systems_dowelled.gif) dowelled, 62 pieces | ![hollow, 20 pieces](assembly/assembly_templates_timber_floor_1_systems_hollow.gif) hollow, 20 pieces | ![joists, 8 pieces](assembly/assembly_templates_timber_floor_1_systems_joists.gif) joists, 8 pieces |
| ![planks, 14 pieces](assembly/assembly_templates_timber_floor_1_systems_planks.gif) planks, 14 pieces | ![ribbed, 9 pieces](assembly/assembly_templates_timber_floor_1_systems_ribbed.gif) ribbed, 9 pieces |  |
| **Bearings**, 11 elements, 8 moves | | |
| ![battens, 5 pieces](assembly/assembly_templates_timber_floor_2_bearings_battens.gif) battens, 5 pieces | ![rebated, 3 pieces](assembly/assembly_templates_timber_floor_2_bearings_rebated.gif) rebated, 3 pieces | ![resting, 3 pieces](assembly/assembly_templates_timber_floor_2_bearings_resting.gif) resting, 3 pieces |
| **Joints**, 18 elements, 14 moves | | |
| ![board, 5 pieces](assembly/assembly_templates_timber_floor_3_joints_board.gif) board, 5 pieces | ![keys, 4 pieces](assembly/assembly_templates_timber_floor_3_joints_keys.gif) keys, 4 pieces | ![strip, 5 pieces](assembly/assembly_templates_timber_floor_3_joints_strip.gif) strip, 5 pieces |
| ![tongue, 4 pieces](assembly/assembly_templates_timber_floor_3_joints_tongue.gif) tongue, 4 pieces |  |  |
| **Timber and earth**, 409 elements, 69 moves | | |
| ![arch, 18 pieces](assembly/assembly_templates_timber_floor_4_earth_arch.gif) arch, 18 pieces | ![flush, 18 pieces](assembly/assembly_templates_timber_floor_4_earth_flush.gif) flush, 18 pieces |  |
| **Floors of the book**, 149 elements, 133 moves | | |
| ![beams comb, 6 pieces](assembly/assembly_templates_timber_floor_5_systems_beams_comb.gif) beams comb, 6 pieces | ![boards dowelled, 18 pieces](assembly/assembly_templates_timber_floor_5_systems_boards_dowelled.gif) boards dowelled, 18 pieces | ![glulam elements, 31 pieces](assembly/assembly_templates_timber_floor_5_systems_glulam_elements.gif) glulam elements, 31 pieces |
| ![joists clad, 11 pieces](assembly/assembly_templates_timber_floor_5_systems_joists_clad.gif) joists clad, 11 pieces |  |  |
| **Producers**, 510 elements, 497 moves | | |
| ![clt box closed, 11 pieces](assembly/assembly_templates_timber_floor_6_producers_clt_box_closed.gif) clt box closed, 11 pieces | ![kielsteg, 39 pieces](assembly/assembly_templates_timber_floor_6_producers_kielsteg.gif) kielsteg, 39 pieces | ![lignatur box, 24 pieces](assembly/assembly_templates_timber_floor_6_producers_lignatur_box.gif) lignatur box, 24 pieces |
| ![lignatur silence, 75 pieces](assembly/assembly_templates_timber_floor_6_producers_lignatur_silence.gif) lignatur silence, 75 pieces | ![lignotrend rib, 92 pieces](assembly/assembly_templates_timber_floor_6_producers_lignotrend_rib.gif) lignotrend rib, 92 pieces | ![novatop element, 33 pieces](assembly/assembly_templates_timber_floor_6_producers_novatop_element.gif) novatop element, 33 pieces |
| **Joints and concrete**, 141 elements, 127 moves | | |
| ![concrete locks, 23 pieces](assembly/assembly_templates_timber_floor_7_shear_concrete_locks.gif) concrete locks, 23 pieces | ![concrete plates, 9 pieces](assembly/assembly_templates_timber_floor_7_shear_concrete_plates.gif) concrete plates, 9 pieces | ![concrete wave, 6 pieces](assembly/assembly_templates_timber_floor_7_shear_concrete_wave.gif) concrete wave, 6 pieces |
| ![joint cover, 12 pieces](assembly/assembly_templates_timber_floor_7_shear_joint_cover.gif) joint cover, 12 pieces | ![joint keys, 12 pieces](assembly/assembly_templates_timber_floor_7_shear_joint_keys.gif) joint keys, 12 pieces |  |
| **Ribs, voids, dowels**, 95 elements, 88 moves | | |
| ![clt rib box, 6 pieces](assembly/assembly_templates_timber_floor_9_ribs_clt_rib_box.gif) clt rib box, 6 pieces | ![concrete ribs, 21 pieces](assembly/assembly_templates_timber_floor_9_ribs_concrete_ribs.gif) concrete ribs, 21 pieces |  |

<!-- assembly:end -->
