# Truss {#templates_truss}

[TOC]

Parametric and custom timber trusses (`src/templates/truss/*.h`, examples `templates_truss_1_pratt` and `templates_truss_2_library`). `wood_truss::Truss` subclasses `WoodSession` and converts a wireframe `TrussGuide` (`chords` polylines + `webs` lines + `posts` lines) into mitered, butted, and half-lapped `Beam` elements by ranking members via iterative outer-boundary graph peeling.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#truss_ex_pratt">
    <img src="templates_truss_1_pratt.png" alt="Pratt Roof Truss Example">
    <span class="wood-card-title">Pratt Roof Truss</span>
    <span class="wood-card-sub">Full C++ example (`templates_truss_1_pratt.cpp`)</span>
  </a>
  <a class="wood-card" href="#truss_king_post">
    <img src="truss_king_post.png" alt="King Post">
    <span class="wood-card-title">King Post &amp; Queen Post</span>
    <span class="wood-card-sub">Classic pitched roof trusses (`king_post`, `queen_post`)</span>
  </a>
  <a class="wood-card" href="#truss_howe">
    <img src="truss_howe.png" alt="Howe &amp; Fink">
    <span class="wood-card-title">Howe, Fink &amp; Belgian</span>
    <span class="wood-card-sub">Pitched roof &amp; bridge trusses (`howe`, `fink`, `belgian`)</span>
  </a>
  <a class="wood-card" href="#truss_scissors">
    <img src="truss_scissors.png" alt="Scissors">
    <span class="wood-card-title">Scissors &amp; Raised Tie</span>
    <span class="wood-card-sub">Vaulted ceiling trusses (`scissors`, `raised_tie_king_post`)</span>
  </a>
  <a class="wood-card" href="#truss_warren">
    <img src="truss_warren.png" alt="Warren &amp; K-Truss">
    <span class="wood-card-title">Warren, Parker &amp; K-Truss</span>
    <span class="wood-card-sub">Long-span bridge trusses (`warren`, `parker`, `k_truss`)</span>
  </a>
  <a class="wood-card" href="#truss_bowstring">
    <img src="truss_bowstring.png" alt="Bowstring &amp; Lens">
    <span class="wood-card-title">Bowstring, Lens &amp; Arch</span>
    <span class="wood-card-sub">Curved chord &amp; arch trusses (`bowstring`, `lens`, `burr_arch`)</span>
  </a>
  <a class="wood-card" href="#truss_town_lattice">
    <img src="truss_town_lattice.png" alt="Town Lattice">
    <span class="wood-card-title">Town Lattice &amp; Double</span>
    <span class="wood-card-sub">Multi-crossing lattice trusses (`town_lattice`, `double_intersection_*`)</span>
  </a>
  <a class="wood-card" href="#truss_topology_optimised">
    <img src="truss_topology_optimised.png" alt="Topology Optimised">
    <span class="wood-card-title">Cantilever, Sawtooth &amp; Optimised</span>
    <span class="wood-card-sub">Special geometries (`cantilever`, `sawtooth`, `topology_optimised`)</span>
  </a>
</div>

![Timber Truss Primary Inputs, Boundary-Peeling Rank and Node Butts](diagrams/template_truss_pipeline.webp)

## Primary Inputs (`TrussGuide`)

| Input / Parameter | Type | Default | Geometric Role |
| --- | --- | --- | --- |
| `chords` | `std::vector<Polyline>` | Preset-specific | Continuous top/bottom chord polylines passing through panel nodes |
| `webs` | `std::vector<Line>` | Preset-specific | Interior vertical and diagonal web segments |
| `posts` | `std::vector<Line>` | `{}` | Supporting column/post segments at truss ends |
| `chord_radius` | `double` | `80.0` | Half-section (`mm`) of chord and post beams (`80.0` → `160 × 160 mm`) |
| `web_radius` | `double` | `60.0` | Half-section (`mm`) of web beams (`60.0` → `120 × 120 mm`) |

## Geometry Generation & Boundary-Peeling Joint Rules

Members are ranked by iteratively pruning the outer boundary of the line graph: outer boundary chords and posts receive `rank = 0` and have their corner miters/heels cut first; inner web layers (`rank >= 1`) are then let into the already-trimmed outer members so no corner is cut twice.

| Node / Crossing Configuration | Automatic Joint Rule (`Truss::add_chord_butts` / `add_web_butts`) |
| --- | --- |
| **Chord kink (interior polyline vertex)** | Adjacent chord runs mitered `side_side` on the angle bisector |
| **Chord heel (two chords ending at one node)** | Resolved by `JointBeam::butt` |
| **Chord end on another chord's run** | Stopped `top_side` against the continuous chord |
| **Web / post end on a chord run** | Let into the chord (`top_side`) |
| **Web / post end at a chord's lone end** | Outer member runs over the inner member (`top_side`); inner member let in |
| **Web end on another web's middle (K-joint)** | Let into the continuous web |
| **Chord end on a post's middle (tie on column)** | Let into the post |
| **Crossing chords, webs, or posts (X-joint)** | `JointBeam::butt_cross` half/multi-laps dividing member depth equally across $N$ crossing bars |
| **Multiple webs ending at one node** | Inner webs let into outer chords; same-rank webs mitered `side_side` with angular neighbours |

## Constructor Pipeline

```cpp
Truss::Truss(const TrussGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    // 1. input: the guide's chord polylines and web/post lines as drawn
    add_input();

    // 2. members: a Beam per chord segment, web, and post
    add_chords();
    add_webs();

    // 3. chords: runs mitered at kinks, butted at heels, lapped at crossings
    add_chord_butts();

    // 4. webs and posts: let into chords, lapped at crossings, mitered with same-rank neighbours
    add_web_butts();
}
```

---

## Full C++ Examples

### 1. Custom Wireframe `TrussGuide` (Explicit Chords, Webs & Posts) {#truss_ex_custom}

```cpp
#include "wood_session.h"
#include "src/templates/truss/truss.h"

using namespace session_cpp;
using namespace wood_session;

int main() {
    // 1. Primary inputs: top & bottom chord Polylines + web Lines
    const std::vector<Polyline> chords = {
        Polyline({{0.0, 0.0, 0.0}, {4000.0, 0.0, 0.0}, {8000.0, 0.0, 0.0}}),
        Polyline({{0.0, 0.0, 0.0}, {4000.0, 0.0, 2200.0}, {8000.0, 0.0, 0.0}}),
    };
    const std::vector<Line> webs = {
        Line::from_points({2000.0, 0.0, 1100.0}, {2000.0, 0.0, 0.0}),
        Line::from_points({4000.0, 0.0, 2200.0}, {4000.0, 0.0, 0.0}),
        Line::from_points({6000.0, 0.0, 1100.0}, {6000.0, 0.0, 0.0}),
        Line::from_points({2000.0, 0.0, 1100.0}, {4000.0, 0.0, 0.0}),
        Line::from_points({6000.0, 0.0, 1100.0}, {4000.0, 0.0, 0.0}),
    };

    // 2. Build TrussGuide (160x160 chords, 120x120 webs) and generate Truss
    const wood_truss::TrussGuide guide(chords, webs, {}, 80.0, 60.0);
    wood_truss::Truss truss(guide, "custom_pratt_truss");
    truss.pb_dump(pb_path("live"));
    return 0;
}
```

### 2. Pratt Roof Truss (`examples/templates_truss_1_pratt.cpp`) {#truss_ex_pratt}

![templates_truss_1_pratt](templates/templates_truss_1_pratt.png)

\include{lineno} templates_truss_1_pratt.cpp

## Library (`80` Parametric `TrussGuide` Presets) {#truss_library}

Every guide of the library built by the template, `templates_truss_2_library`, each its own scene `truss_<name>.pb`. Each takes its span, heights and number of panels, and the half sections `chord_radius` and `web_radius`.

### Pratt {#truss_pratt}

```cpp
TrussGuide::pratt(double span = 12000.0, double rise = 3000.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_pratt](templates/truss_pratt.png)

### King post {#truss_king_post}

```cpp
TrussGuide::king_post(double span = 8000.0, double rise = 2667.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_king_post](templates/truss_king_post.png)

### Queen post {#truss_queen_post}

```cpp
TrussGuide::queen_post(double span = 9000.0, double height = 2000.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_queen_post](templates/truss_queen_post.png)

### Howe {#truss_howe}

```cpp
TrussGuide::howe(double span = 12000.0, double rise = 3000.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_howe](templates/truss_howe.png)

### Fink {#truss_fink}

```cpp
TrussGuide::fink(double span = 10000.0, double rise = 2500.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_fink](templates/truss_fink.png)

### Belgian {#truss_belgian}

```cpp
TrussGuide::belgian(double span = 10000.0, double rise = 2500.0, int divisions = 3, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_belgian](templates/truss_belgian.png)

### Scissors {#truss_scissors}

```cpp
TrussGuide::scissors(double span = 8000.0, double rise = 3000.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_scissors](templates/truss_scissors.png)

### Warren {#truss_warren}

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

### Bowstring {#truss_bowstring}

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

### Town lattice {#truss_town_lattice}

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

### Topology optimised {#truss_topology_optimised}

```cpp
TrussGuide::topology_optimised(double span = 12000.0, double height = 1800.0, double chord_radius = 80.0, double web_radius = 60.0);
```

![truss_topology_optimised](templates/truss_topology_optimised.png)

\include{lineno} templates_truss_2_library.cpp
