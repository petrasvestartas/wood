# Truss {#templates_truss}

[TOC]

`wood_truss::Truss` is a timber truss, a `WoodSession` built from a `TrussGuide`: chords drawn as polylines through their nodes, webs and end posts as lines. The template makes a beam per chord run, web and post and reads every butt from how the lines meet at the nodes.

```cpp
wood_truss::Truss truss(wood_truss::TrussGuide::pratt());
```

| Where the lines meet | Butt |
| --- | --- |
| a chord turns, a kink | the two runs `side_side` on the miter |
| chords end at one node, a heel | `JointBeam::butt` reads the case |
| a chord's end on another chord's run | `top_side` on it |
| a web's or post's end on a run, or where several runs end | `top_side` on each run there |
| a web's or post's end at a run's lone end | `top_side` at the end, the chord running on over it |
| a web's end on another web's middle, a K | `top_side` on it |
| two webs or posts crossing | a half lap, `JointBeam::butt`'s `cross` |
| webs and posts ending at one node | sorted by angle, `side_side` with each neighbour; on a chord never across the widest gap |

## Constructor

The constructor is the recipe:

```cpp
Truss::Truss(const TrussGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    // input: the guide's lines as drawn
    add_input();

    // members: a beam per chord run and per web
    add_chords();
    add_webs();

    // chords: the runs mitered at kinks, butted at heels
    add_chord_butts();

    // webs and posts: on the chords, lapped where they cross, mitered with their neighbours at every node
    add_web_butts();
}
```

## Guide

```cpp
TrussGuide(const std::vector<Polyline>& chords, const std::vector<Line>& webs, const std::vector<Line>& posts = {}, double chord_radius = 80.0, double web_radius = 60.0);
```

## Pratt

![templates_truss_1_pratt](templates/templates_truss_1_pratt.png)

\include{lineno} templates_truss_1_pratt.cpp

## Library

Every guide of the library built by the template, `templates_truss_2_library`, each its own scene `truss_<name>.pb`. Each takes its span, heights and number of panels, and the half sections `chord_radius` and `web_radius`.

### Pratt

```cpp
TrussGuide::pratt(double span = 12000.0, double rise = 3000.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_pratt](templates/truss_pratt.png)

### King post

```cpp
TrussGuide::king_post(double span = 8000.0, double rise = 2667.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_king_post](templates/truss_king_post.png)

### Queen post

```cpp
TrussGuide::queen_post(double span = 9000.0, double height = 2000.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_queen_post](templates/truss_queen_post.png)

### Howe

```cpp
TrussGuide::howe(double span = 12000.0, double rise = 3000.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_howe](templates/truss_howe.png)

### Fink

```cpp
TrussGuide::fink(double span = 10000.0, double rise = 2500.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_fink](templates/truss_fink.png)

### Belgian

```cpp
TrussGuide::belgian(double span = 10000.0, double rise = 2500.0, int divisions = 3, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_belgian](templates/truss_belgian.png)

### Scissors

```cpp
TrussGuide::scissors(double span = 8000.0, double rise = 3000.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_scissors](templates/truss_scissors.png)

### Warren

```cpp
TrussGuide::warren(double span = 12000.0, double height = 1500.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_warren](templates/truss_warren.png)

### Warren with verticals

```cpp
TrussGuide::warren_verticals(double span = 12000.0, double height = 1500.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_warren_verticals](templates/truss_warren_verticals.png)

### Pratt bridge

```cpp
TrussGuide::pratt_bridge(double span = 12000.0, double height = 2000.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_pratt_bridge](templates/truss_pratt_bridge.png)

### Parker

```cpp
TrussGuide::parker(double span = 12000.0, double height = 2400.0, double hip = 1600.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_parker](templates/truss_parker.png)

### K-truss

```cpp
TrussGuide::k_truss(double span = 12000.0, double height = 2400.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_k_truss](templates/truss_k_truss.png)

### Cambered verticals

```cpp
TrussGuide::cambered_verticals(double span = 12000.0, double height = 1980.0, double camber = 800.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_cambered_verticals](templates/truss_cambered_verticals.png)

### Funicular arch

```cpp
TrussGuide::funicular_arch(double span = 12000.0, double height = 2400.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_funicular_arch](templates/truss_funicular_arch.png)

### Lens

```cpp
TrussGuide::lens(double span = 12000.0, double height = 1140.0, double sag = 1260.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_lens](templates/truss_lens.png)

### Triangulated arch

```cpp
TrussGuide::triangulated_arch(double span = 12000.0, double height = 5980.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_triangulated_arch](templates/truss_triangulated_arch.png)

### Sloped

```cpp
TrussGuide::sloped(double span = 12040.0, double height = 6020.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_sloped](templates/truss_sloped.png)

### Flat Pratt

```cpp
TrussGuide::pratt_flat(double span = 12000.0, double height = 1990.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_pratt_flat](templates/truss_pratt_flat.png)

### Flat peaked

```cpp
TrussGuide::flat_peaked(double span = 12000.0, double height = 1990.0, double peak = 3180.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_flat_peaked](templates/truss_flat_peaked.png)

### Bowstring

```cpp
TrussGuide::bowstring(double span = 12000.0, double height = 1990.0, double crown = 3320.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_bowstring](templates/truss_bowstring.png)

### Pitched on end posts

```cpp
TrussGuide::pitched_posts(double span = 12000.0, double height = 2000.0, double ridge = 3320.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_pitched_posts](templates/truss_pitched_posts.png)

### Roof cambered tie

```cpp
TrussGuide::roof_cambered_tie(double span = 12000.0, double rise = 2000.0, double camber = 500.0, double dip = 60.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_roof_cambered_tie](templates/truss_roof_cambered_tie.png)

### Quarter fan

```cpp
TrussGuide::quarter_fan(double span = 12000.0, double height = 5560.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_quarter_fan](templates/truss_quarter_fan.png)

### Cantilever

```cpp
TrussGuide::cantilever(double span = 12000.0, double height = 1830.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_cantilever](templates/truss_cantilever.png)

### Cantilever design 1

```cpp
TrussGuide::cantilever_design_1(double span = 11887.2, double height = 2000.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_cantilever_design_1](templates/truss_cantilever_design_1.png)

### Cantilever design 2

```cpp
TrussGuide::cantilever_design_2(double span = 12000.0, double height = 2000.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_cantilever_design_2](templates/truss_cantilever_design_2.png)

### Cantilever design 3

```cpp
TrussGuide::cantilever_design_3(double span = 12000.0, double height = 2000.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_cantilever_design_3](templates/truss_cantilever_design_3.png)

### Fish belly

```cpp
TrussGuide::fish_belly(double span = 12000.0, double rise = 1720.0, double sag = 1150.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_fish_belly](templates/truss_fish_belly.png)

### Sawtooth

```cpp
TrussGuide::sawtooth(double span = 12000.0, double height = 1800.0, double column = 2130.0, int teeth = 3, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_sawtooth](templates/truss_sawtooth.png)

### Howe flat

```cpp
TrussGuide::howe_flat(double span = 12000.0, double height = 1530.0, double ridge = 1890.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_howe_flat](templates/truss_howe_flat.png)

### Three hinged arch

```cpp
TrussGuide::three_hinged_arch(double span = 12000.0, double height = 3380.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_three_hinged_arch](templates/truss_three_hinged_arch.png)

### Burr arch

```cpp
TrussGuide::burr_arch(double span = 12000.0, double height = 2480.0, double crown = 1890.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_burr_arch](templates/truss_burr_arch.png)

### Town lattice

```cpp
TrussGuide::town_lattice(double span = 12000.0, double height = 3000.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_town_lattice](templates/truss_town_lattice.png)

### Bowstring arch

```cpp
TrussGuide::bowstring_arch(double span = 11900.0, double height = 2490.0, int panels = 7, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_bowstring_arch](templates/truss_bowstring_arch.png)

### Waddell a

```cpp
TrussGuide::waddell_a(double span = 12000.0, double rise = 4560.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_waddell_a](templates/truss_waddell_a.png)

### Wichert

```cpp
TrussGuide::wichert(double span = 12000.0, double height = 2320.0, double dip = 650.0, int panels = 10, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_wichert](templates/truss_wichert.png)

### Truss leg bedstead

```cpp
TrussGuide::truss_leg_bedstead(double span = 12000.0, double height = 2500.0, double leg = 900.0, int panels = 4, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_truss_leg_bedstead](templates/truss_truss_leg_bedstead.png)

### Kellogg

```cpp
TrussGuide::kellogg(double span = 12000.0, double height = 2650.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_kellogg](templates/truss_kellogg.png)

### Baltimore a

```cpp
TrussGuide::baltimore_a(double span = 14000.0, double height = 2140.0, int panels = 7, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_baltimore_a](templates/truss_baltimore_a.png)

### Pennsylvania a

```cpp
TrussGuide::pennsylvania_a(double span = 14000.0, double height = 2160.0, double hip = 1400.0, int panels = 7, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_pennsylvania_a](templates/truss_pennsylvania_a.png)

### Greiner

```cpp
TrussGuide::greiner(double span = 12000.0, double height = 2500.0, double hang = 400.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_greiner](templates/truss_greiner.png)

### Double intersection pratt

```cpp
TrussGuide::double_intersection_pratt(double span = 12000.0, double height = 2500.0, int panels = 12, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_double_intersection_pratt](templates/truss_double_intersection_pratt.png)

### Schwedler

```cpp
TrussGuide::schwedler(double span = 14000.0, double height = 2960.0, int panels = 14, int sloped = 4, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_schwedler](templates/truss_schwedler.png)

### Double intersection warren

```cpp
TrussGuide::double_intersection_warren(double span = 12000.0, double height = 2500.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_double_intersection_warren](templates/truss_double_intersection_warren.png)

### Pegram

```cpp
TrussGuide::pegram(double span = 12060.0, double height = 2530.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_pegram](templates/truss_pegram.png)

### Post

```cpp
TrussGuide::post(double span = 12000.0, double height = 2500.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_post](templates/truss_post.png)

### Bollman

```cpp
TrussGuide::bollman(double span = 12000.0, double height = 2500.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_bollman](templates/truss_bollman.png)

### Fink bridge

```cpp
TrussGuide::fink_bridge(double span = 12000.0, double height = 2600.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_fink_bridge](templates/truss_fink_bridge.png)

### Stearns

```cpp
TrussGuide::stearns(double span = 12000.0, double height = 2580.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_stearns](templates/truss_stearns.png)

### Crescent crossed

```cpp
TrussGuide::crescent_crossed(double span = 12000.0, double height = 3900.0, double inner = 1800.0, int panels = 12, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_crescent_crossed](templates/truss_crescent_crossed.png)

### Braced arch rib

```cpp
TrussGuide::braced_arch_rib(double span = 19426.0, double height = 10850.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_braced_arch_rib](templates/truss_braced_arch_rib.png)

### Belgian verticals

```cpp
TrussGuide::belgian_verticals(double span = 12000.0, double rise = 3540.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_belgian_verticals](templates/truss_belgian_verticals.png)

### Raised tie king post

```cpp
TrussGuide::raised_tie_king_post(double span = 12000.0, double rise = 3530.0, double tie = 1160.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_raised_tie_king_post](templates/truss_raised_tie_king_post.png)

### Trapezoid crossed

```cpp
TrussGuide::trapezoid_crossed(double span = 12000.0, double height = 3020.0, double ridge = 4600.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_trapezoid_crossed](templates/truss_trapezoid_crossed.png)

### Fink cambered

```cpp
TrussGuide::fink_cambered(double span = 12000.0, double rise = 2890.0, double camber = 390.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_fink_cambered](templates/truss_fink_cambered.png)

### Crescent verticals

```cpp
TrussGuide::crescent_verticals(double span = 12000.0, double height = 3650.0, double inner = 1260.0, double lantern = 270.0, double lantern_rise = 380.0, int panels = 10, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_crescent_verticals](templates/truss_crescent_verticals.png)

### Raised tie posts

```cpp
TrussGuide::raised_tie_posts(double span = 12000.0, double rise = 4130.0, double column = 600.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_raised_tie_posts](templates/truss_raised_tie_posts.png)

### Pitched warren columns

```cpp
TrussGuide::pitched_warren_columns(double span = 12000.0, double height = 1170.0, double ridge = 2140.0, double column = 970.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_pitched_warren_columns](templates/truss_pitched_warren_columns.png)

### Fink compound

```cpp
TrussGuide::fink_compound(double span = 12000.0, double rise = 4090.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_fink_compound](templates/truss_fink_compound.png)

### Fink bent tie

```cpp
TrussGuide::fink_bent_tie(double span = 12000.0, double rise = 4180.0, double bend = 1080.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_fink_bent_tie](templates/truss_fink_bent_tie.png)

### French steep

```cpp
TrussGuide::french_steep(double span = 12000.0, double rise = 5930.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_french_steep](templates/truss_french_steep.png)

### Fink lantern

```cpp
TrussGuide::fink_lantern(double span = 12000.0, double rise = 3230.0, double lantern = 1370.0, double lantern_rise = 790.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_fink_lantern](templates/truss_fink_lantern.png)

### Arch polygonal tie

```cpp
TrussGuide::arch_polygonal_tie(double span = 12000.0, double height = 3480.0, double tie = 980.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_arch_polygonal_tie](templates/truss_arch_polygonal_tie.png)

### Pitched crossed posts

```cpp
TrussGuide::pitched_crossed_posts(double span = 12000.0, double height = 1220.0, double ridge = 2810.0, double camber = 650.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_pitched_crossed_posts](templates/truss_pitched_crossed_posts.png)

### Cambered crossed lantern

```cpp
TrussGuide::cambered_crossed_lantern(double span = 12000.0, double height = 2150.0, double camber = 410.0, double lantern = 800.0, double lantern_rise = 780.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_cambered_crossed_lantern](templates/truss_cambered_crossed_lantern.png)

### Arched crossed columns

```cpp
TrussGuide::arched_crossed_columns(double span = 12000.0, double height = 1690.0, double ridge = 5250.0, double crown = 2820.0, double column = 1290.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_arched_crossed_columns](templates/truss_arched_crossed_columns.png)

### Trapezoid king crossed

```cpp
TrussGuide::trapezoid_king_crossed(double span = 12000.0, double height = 1880.0, double ridge = 2610.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_trapezoid_king_crossed](templates/truss_trapezoid_king_crossed.png)

\include{lineno} templates_truss_2_library.cpp
