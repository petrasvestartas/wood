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

<!-- assembly:end -->
