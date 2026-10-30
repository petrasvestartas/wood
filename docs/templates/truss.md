# Truss {#templates_truss}

[TOC]

Parametric and custom timber trusses in `src/templates/truss/*.h`. `wood_truss::Truss` converts chord polylines and web/post lines into `Beam` elements joined by a member hierarchy: T joints with run-on ends, butts on the members' real surfaces, miters at gentle bends and ridges, half laps where members cross. The 340 roof trusses and framed bridges of Bow's 1873 catalogue are at the end of this page.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#truss_ex_pratt">
    <img src="templates_truss_1_pratt.png" alt="Pratt Truss">
    <span class="wood-card-title">Pratt Truss</span>
  </a>
  <a class="wood-card" href="#truss_king_post">
    <img src="truss_king_post.png" alt="King and Queen Post">
    <span class="wood-card-title">King and Queen Post</span>
  </a>
  <a class="wood-card" href="#truss_howe">
    <img src="truss_howe.png" alt="Howe and Fink">
    <span class="wood-card-title">Howe and Fink</span>
  </a>
  <a class="wood-card" href="#truss_scissors">
    <img src="truss_scissors.png" alt="Scissors Truss">
    <span class="wood-card-title">Scissors Truss</span>
  </a>
  <a class="wood-card" href="#truss_warren">
    <img src="truss_warren.png" alt="Warren and K-Truss">
    <span class="wood-card-title">Warren and K-Truss</span>
  </a>
  <a class="wood-card" href="#truss_bowstring">
    <img src="truss_bowstring.png" alt="Bowstring and Arch">
    <span class="wood-card-title">Bowstring and Arch</span>
  </a>
  <a class="wood-card" href="#truss_town_lattice">
    <img src="truss_town_lattice.png" alt="Town Lattice">
    <span class="wood-card-title">Town Lattice</span>
  </a>
  <a class="wood-card" href="#truss_topology_optimised">
    <img src="truss_topology_optimised.png" alt="Topology Optimised">
    <span class="wood-card-title">Topology Optimised</span>
  </a>
  <a class="wood-card" href="#truss_graph">
    <img src="truss_bow_64.webp" alt="Bow 1873">
    <span class="wood-card-title">Bow 1873: 340 Trusses</span>
  </a>
</div>

## Primary Inputs

| Input / Parameter | Type | Default | Geometric Role |
| --- | --- | --- | --- |
| `chords` | `std::vector<Polyline>` | Preset-specific | Continuous top/bottom chord polylines passing through panel nodes |
| `webs` | `std::vector<Line>` | Preset-specific | Interior vertical and diagonal web segments |
| `posts` | `std::vector<Line>` | `{}` | Supporting column/post segments at truss ends |
| `chord_radius` | `double` | `80.0` | Half-section (`mm`) of chord and post beams (`80.0` → `160 × 160 mm`) |
| `web_radius` | `double` | `60.0` | Half-section (`mm`) of web beams (`60.0` → `120 × 120 mm`) |

## Geometry Generation & Joint Rules

Every node is solved by a hierarchy: one member runs through or runs on, the others meet it in a T. The hierarchy is given by
the guide's `chord_levels`, `web_levels` and `post_levels` (1 primary, 2 secondary, 3 tertiary), or, when they are empty,
read from the drawing by iteratively pruning the outer boundary of the line graph: outer boundary chords and posts receive
`rank = 0`, inner web layers `rank >= 1`. A member stops on every member of a higher level at its node: it loses the shadow
of that member's cut solid swept along its own axis, so it butts on the actual surface (a kink, a corner) and keeps nothing
behind it. Where chord ends meet at one node the most important one runs on (`TrussGuide::runs_on_at`: named in `runs_on`,
else the higher level, else the more level; a lone end also runs on over webs and posts all lower than it) and the others are
seated on its side. It reaches past the node to its end cut (`TrussGuide::end_cut`): through the outer corners of the members
it carries on both its sides, so the outer contour runs on clean; flush with the outer face of the members on one side met at
40 degrees or more; else square, past the node by at least its half section. The node and the axes always lie in wood. Only
two ends of one slope, as at a ridge, and gentle bends of under 35 degrees are mitered; two chords bending at one node are
half-lapped; a chord ending at another's bend butts on its surface.

| Node / Crossing Configuration | Automatic Joint Rule (`Truss::add_chord_butts` / `add_web_butts`) |
| --- | --- |
| Chord kink (interior polyline vertex) | Adjacent chord runs mitered `side_side` on the angle bisector |
| Chord heel (two chords ending at one node) | A T: the one running on (`runs_on_at`) reaches past the node to its `end_cut`, the other seated on its side (`top_side`); one slope (ridge) or a gentle bend: mitered, `JointBeam::butt_side_side` |
| Chord end on another chord's run | Stopped `top_side` against the continuous chord |
| Web / post end on a chord run | Let into the chord: loses the chord's shadow along its axis |
| Web / post end at a chord's lone end | Outer member runs over the inner member (`top_side`); inner member let in |
| Web end on another web's middle (K-joint) | Let into the continuous web |
| Chord end on a post's middle (tie on column) | Let into the post |
| Crossing chords, webs, or posts (X-joint) | `JointBeam::butt_cross` half/multi-laps dividing member depth equally across $N$ crossing bars |
| Multiple webs ending at one node | Inner webs let into outer chords; same-rank webs mitered `side_side` with angular neighbours |

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

### 1. Custom Wireframe Truss {#truss_ex_custom}

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

### 2. Pratt Roof Truss {#truss_ex_pratt}

![templates_truss_1_pratt](templates/templates_truss_1_pratt.png)

```cpp
#include "wood_session.h"
#include "src/templates/truss/truss.h"

using namespace session_cpp;
using namespace wood_session;

/// A Pratt roof truss from its guide: chords and webs drawn as lines, the members and their butts made by the template.
int main() {

    wood_truss::Truss truss(wood_truss::TrussGuide::pratt());
    std::cout << truss << std::endl;
    truss.pb_dump(pb_path("live"));
    return 0;
}
```

## Truss Library {#truss_library}

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

## Graphs: Bow 1873 {#truss_graph}

A truss is one topology: nodes, chords through them, webs and posts between them. `TrussGraph` stores exactly that, in
spans, with its span, a section per member, each member's level in the hierarchy (1 primary, 2 secondary, 3 tertiary) and
the chord ends that run on where chords end together, and `TrussGraph::guide` draws it for the one `Truss` template, so a
catalogue of designs is data, not a factory per design. The roof trusses and framed bridges of Bow's *Economics of Construction in relation to Framed
Structures* (1873) are kept this way in `data/templates/truss_bow_1873.json` and built by `templates_truss_3_bow`.

```cpp
TrussGuide TrussGraph::guide() const;
static std::vector<TrussGraph> TrussGraph::library(const std::filesystem::path& json_path);
```

<!-- bow-gallery -->

The 340 roof trusses and framed bridges of R. H. Bow, *Economics of Construction in relation to Framed Structures* (London: Spon, 1873), Part I, redrawn from the drawings of its classification tables. Each truss is stored as a graph in `data/templates/truss_bow_1873.json` (nodes in spans from the left support, x along the span and z up; chords as node sequences, webs and posts as node pairs) and built by the one `Truss` template: `TrussGraph::guide` draws the graph at its span with a section per member, and `Truss` makes the members and their butts and laps.

```cpp
const std::vector<wood_truss::TrussGraph> graphs = wood_truss::TrussGraph::library(
    config::session_data_dir() / "templates" / "truss_bow_1873.json");
for (const wood_truss::TrussGraph& graph : graphs) {
    wood_truss::Truss truss(graph.guide(), "truss_" + graph.name);
    truss.pb_dump(pb_path("truss_" + graph.name));
}
```

Each design was redrawn by hand from Bow's printed drawing as an exact construction: the few dimensions that define it read off the print (rise, collar height, camber, panel count), everything else constructed — equal panel points, members meeting at exact intersections, curved ribs on true circles or parabolas, a bar Bow draws through a joint kept as one member, symmetric designs mirrored exactly — and each checked over its scanned drawing. The numbers are Bow's; the page is that of his table. The book is transcribed in the BRG library (`framed-structures-bow`), where every table row links to its scan.

### Joints {#truss_bow_joints}

Every node is solved as carpentry by a hierarchy stored with the graph: each member is primary, secondary or tertiary (`chord_levels`, `web_levels`, `post_levels`), and at a node a member stops on every member of a higher level, butting on its surface. Joints are T joints wherever possible: a member that is continuous runs through, and at the ends of a truss the tie runs on past the rafter foot, the chords past the end posts, the rafters past a straining beam (`runs_on`), each cut flush with the far side of the member it carries, or square at shallow angles, and where a member carries others on both its sides its end is cut through their outer corners, so the outline of the truss runs on clean; the node where the axes meet always lies in wood. Miters are kept only for the gentle bends of one continuous member and for symmetric ridges.

### Scale and sections {#truss_bow_sizing}

Bow gives the span of only a few designs (Nos. 130, 137, 192, 195, 293, 332); his diagrams of forces are drawn to no absolute scale. Every other design is drawn at its number of sub-spans times 8 ft (2.44 m), the sub-span of his stated examples (No. 130: 86 1/4 ft over 12 sub-spans; No. 137: 97 1/2 ft over 12; No. 332: 204 ft over 24); the arched roofs whose sub-spans he calls irregular (Nos. 193, 194, 196, 197) are taken at 100 ft. Each member's section is then sized, per truss, for timber trusses 3 m apart: roofs 1.5 kN/m2 on the roof outline (the upper chords), bridges 5 kN/m2 on the deck, both x 1.4. The forces are those of truss action, as Bow's diagrams of forces give them: the pin-jointed frame, a curved or bent chord drawn through nodes where nothing else meets it being one bar between the joints where others do; such a bar also carries the bending of the loads between its joints and of its axial force on the sagitta of its curve. A member that the carpentry lands on another's face just beside a joint, to keep it from running alongside, meets that joint in the force model, the member landed on bending under the landing force on its eccentricity. Only designs that are mechanisms when pin-jointed (Bow's deficient Class II, most of Class IV) are analysed as frames with stiff joints, carrying by bending. Sections are square GL24h, N/A + M/W within the design strength, struts with a buckling factor, at least 100 mm for chords and 60 mm for webs, one section per member. The span, the load and the largest forces are stored with each graph; the cards show the span.

### Roof trusses, Class I {#truss_bow_roof_i}

Class I: theoretically perfect frames: with every joint hinged they still cannot distort, and no part can be lengthened or shortened so as to induce stress.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_1.webp" alt="Bow No. 1" loading="lazy">
    <span class="wood-card-title">No. 1 · 2.4 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_2.webp" alt="Bow No. 2" loading="lazy">
    <span class="wood-card-title">No. 2 · 2.4 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_3.webp" alt="Bow No. 3" loading="lazy">
    <span class="wood-card-title">No. 3 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_4.webp" alt="Bow No. 4" loading="lazy">
    <span class="wood-card-title">No. 4 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_5.webp" alt="Bow No. 5" loading="lazy">
    <span class="wood-card-title">No. 5 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_6.webp" alt="Bow No. 6" loading="lazy">
    <span class="wood-card-title">No. 6 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_7.webp" alt="Bow No. 7" loading="lazy">
    <span class="wood-card-title">No. 7 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_8.webp" alt="Bow No. 8" loading="lazy">
    <span class="wood-card-title">No. 8 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_9.webp" alt="Bow No. 9" loading="lazy">
    <span class="wood-card-title">No. 9 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_10.webp" alt="Bow No. 10" loading="lazy">
    <span class="wood-card-title">No. 10 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_11.webp" alt="Bow No. 11" loading="lazy">
    <span class="wood-card-title">No. 11 · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_12.webp" alt="Bow No. 12" loading="lazy">
    <span class="wood-card-title">No. 12 · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_13.webp" alt="Bow No. 13" loading="lazy">
    <span class="wood-card-title">No. 13 · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_14.webp" alt="Bow No. 14" loading="lazy">
    <span class="wood-card-title">No. 14 · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_15.webp" alt="Bow No. 15" loading="lazy">
    <span class="wood-card-title">No. 15 · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_16.webp" alt="Bow No. 16" loading="lazy">
    <span class="wood-card-title">No. 16 · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_17.webp" alt="Bow No. 17" loading="lazy">
    <span class="wood-card-title">No. 17 · 7.3 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_18.webp" alt="Bow No. 18" loading="lazy">
    <span class="wood-card-title">No. 18 · 7.3 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_19.webp" alt="Bow No. 19" loading="lazy">
    <span class="wood-card-title">No. 19 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_20.webp" alt="Bow No. 20" loading="lazy">
    <span class="wood-card-title">No. 20 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_21.webp" alt="Bow No. 21" loading="lazy">
    <span class="wood-card-title">No. 21 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_22.webp" alt="Bow No. 22" loading="lazy">
    <span class="wood-card-title">No. 22 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_23.webp" alt="Bow No. 23" loading="lazy">
    <span class="wood-card-title">No. 23 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_24.webp" alt="Bow No. 24" loading="lazy">
    <span class="wood-card-title">No. 24 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_25.webp" alt="Bow No. 25" loading="lazy">
    <span class="wood-card-title">No. 25 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_26.webp" alt="Bow No. 26" loading="lazy">
    <span class="wood-card-title">No. 26 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_27.webp" alt="Bow No. 27" loading="lazy">
    <span class="wood-card-title">No. 27 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_28.webp" alt="Bow No. 28" loading="lazy">
    <span class="wood-card-title">No. 28 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_29.webp" alt="Bow No. 29" loading="lazy">
    <span class="wood-card-title">No. 29 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_30.webp" alt="Bow No. 30" loading="lazy">
    <span class="wood-card-title">No. 30 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_31.webp" alt="Bow No. 31" loading="lazy">
    <span class="wood-card-title">No. 31 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_32.webp" alt="Bow No. 32" loading="lazy">
    <span class="wood-card-title">No. 32 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_33.webp" alt="Bow No. 33" loading="lazy">
    <span class="wood-card-title">No. 33 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_34.webp" alt="Bow No. 34" loading="lazy">
    <span class="wood-card-title">No. 34 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_35.webp" alt="Bow No. 35" loading="lazy">
    <span class="wood-card-title">No. 35 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_36.webp" alt="Bow No. 36" loading="lazy">
    <span class="wood-card-title">No. 36 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_37.webp" alt="Bow No. 37" loading="lazy">
    <span class="wood-card-title">No. 37 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_38.webp" alt="Bow No. 38" loading="lazy">
    <span class="wood-card-title">No. 38 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_39.webp" alt="Bow No. 39" loading="lazy">
    <span class="wood-card-title">No. 39 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_40.webp" alt="Bow No. 40" loading="lazy">
    <span class="wood-card-title">No. 40 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_41.webp" alt="Bow No. 41" loading="lazy">
    <span class="wood-card-title">No. 41 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_42.webp" alt="Bow No. 42" loading="lazy">
    <span class="wood-card-title">No. 42 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_43.webp" alt="Bow No. 43" loading="lazy">
    <span class="wood-card-title">No. 43 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_44.webp" alt="Bow No. 44" loading="lazy">
    <span class="wood-card-title">No. 44 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_45.webp" alt="Bow No. 45" loading="lazy">
    <span class="wood-card-title">No. 45 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_46.webp" alt="Bow No. 46" loading="lazy">
    <span class="wood-card-title">No. 46 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_47.webp" alt="Bow No. 47" loading="lazy">
    <span class="wood-card-title">No. 47 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_48.webp" alt="Bow No. 48" loading="lazy">
    <span class="wood-card-title">No. 48 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_49.webp" alt="Bow No. 49" loading="lazy">
    <span class="wood-card-title">No. 49 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_50.webp" alt="Bow No. 50" loading="lazy">
    <span class="wood-card-title">No. 50 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_51.webp" alt="Bow No. 51" loading="lazy">
    <span class="wood-card-title">No. 51 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_52.webp" alt="Bow No. 52" loading="lazy">
    <span class="wood-card-title">No. 52 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_53.webp" alt="Bow No. 53" loading="lazy">
    <span class="wood-card-title">No. 53 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_54.webp" alt="Bow No. 54" loading="lazy">
    <span class="wood-card-title">No. 54 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_55.webp" alt="Bow No. 55" loading="lazy">
    <span class="wood-card-title">No. 55 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_56.webp" alt="Bow No. 56" loading="lazy">
    <span class="wood-card-title">No. 56 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_57.webp" alt="Bow No. 57" loading="lazy">
    <span class="wood-card-title">No. 57 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_58.webp" alt="Bow No. 58" loading="lazy">
    <span class="wood-card-title">No. 58 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_59.webp" alt="Bow No. 59" loading="lazy">
    <span class="wood-card-title">No. 59 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_60.webp" alt="Bow No. 60" loading="lazy">
    <span class="wood-card-title">No. 60 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_61.webp" alt="Bow No. 61" loading="lazy">
    <span class="wood-card-title">No. 61 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_62.webp" alt="Bow No. 62" loading="lazy">
    <span class="wood-card-title">No. 62 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_63.webp" alt="Bow No. 63" loading="lazy">
    <span class="wood-card-title">No. 63 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_64.webp" alt="Bow No. 64" loading="lazy">
    <span class="wood-card-title">No. 64 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_65.webp" alt="Bow No. 65" loading="lazy">
    <span class="wood-card-title">No. 65 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_66.webp" alt="Bow No. 66" loading="lazy">
    <span class="wood-card-title">No. 66 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_67.webp" alt="Bow No. 67" loading="lazy">
    <span class="wood-card-title">No. 67 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_68.webp" alt="Bow No. 68" loading="lazy">
    <span class="wood-card-title">No. 68 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_69.webp" alt="Bow No. 69" loading="lazy">
    <span class="wood-card-title">No. 69 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_70.webp" alt="Bow No. 70" loading="lazy">
    <span class="wood-card-title">No. 70 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_71.webp" alt="Bow No. 71" loading="lazy">
    <span class="wood-card-title">No. 71 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_72.webp" alt="Bow No. 72" loading="lazy">
    <span class="wood-card-title">No. 72 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_73.webp" alt="Bow No. 73" loading="lazy">
    <span class="wood-card-title">No. 73 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_74.webp" alt="Bow No. 74" loading="lazy">
    <span class="wood-card-title">No. 74 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_75.webp" alt="Bow No. 75" loading="lazy">
    <span class="wood-card-title">No. 75 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_76.webp" alt="Bow No. 76" loading="lazy">
    <span class="wood-card-title">No. 76 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_77.webp" alt="Bow No. 77" loading="lazy">
    <span class="wood-card-title">No. 77 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_78.webp" alt="Bow No. 78" loading="lazy">
    <span class="wood-card-title">No. 78 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_79.webp" alt="Bow No. 79" loading="lazy">
    <span class="wood-card-title">No. 79 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_80.webp" alt="Bow No. 80" loading="lazy">
    <span class="wood-card-title">No. 80 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_81.webp" alt="Bow No. 81" loading="lazy">
    <span class="wood-card-title">No. 81 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_82.webp" alt="Bow No. 82" loading="lazy">
    <span class="wood-card-title">No. 82 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_83.webp" alt="Bow No. 83" loading="lazy">
    <span class="wood-card-title">No. 83 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_84.webp" alt="Bow No. 84" loading="lazy">
    <span class="wood-card-title">No. 84 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_85.webp" alt="Bow No. 85" loading="lazy">
    <span class="wood-card-title">No. 85 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_86.webp" alt="Bow No. 86" loading="lazy">
    <span class="wood-card-title">No. 86 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_87.webp" alt="Bow No. 87" loading="lazy">
    <span class="wood-card-title">No. 87 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_88.webp" alt="Bow No. 88" loading="lazy">
    <span class="wood-card-title">No. 88 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_89.webp" alt="Bow No. 89" loading="lazy">
    <span class="wood-card-title">No. 89 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_90.webp" alt="Bow No. 90" loading="lazy">
    <span class="wood-card-title">No. 90 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_91.webp" alt="Bow No. 91" loading="lazy">
    <span class="wood-card-title">No. 91 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_92.webp" alt="Bow No. 92" loading="lazy">
    <span class="wood-card-title">No. 92 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_93.webp" alt="Bow No. 93" loading="lazy">
    <span class="wood-card-title">No. 93 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_94.webp" alt="Bow No. 94" loading="lazy">
    <span class="wood-card-title">No. 94 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_95.webp" alt="Bow No. 95" loading="lazy">
    <span class="wood-card-title">No. 95 · 14.6 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_96.webp" alt="Bow No. 96" loading="lazy">
    <span class="wood-card-title">No. 96 · 14.6 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_97.webp" alt="Bow No. 97" loading="lazy">
    <span class="wood-card-title">No. 97 · 14.6 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_98.webp" alt="Bow No. 98" loading="lazy">
    <span class="wood-card-title">No. 98 · 14.6 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_99.webp" alt="Bow No. 99" loading="lazy">
    <span class="wood-card-title">No. 99 · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_100.webp" alt="Bow No. 100" loading="lazy">
    <span class="wood-card-title">No. 100 · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_101.webp" alt="Bow No. 101" loading="lazy">
    <span class="wood-card-title">No. 101 · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_102.webp" alt="Bow No. 102" loading="lazy">
    <span class="wood-card-title">No. 102 · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_103.webp" alt="Bow No. 103" loading="lazy">
    <span class="wood-card-title">No. 103 · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_104.webp" alt="Bow No. 104" loading="lazy">
    <span class="wood-card-title">No. 104 · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_105.webp" alt="Bow No. 105" loading="lazy">
    <span class="wood-card-title">No. 105 · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_106.webp" alt="Bow No. 106" loading="lazy">
    <span class="wood-card-title">No. 106 · 19.5 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_107.webp" alt="Bow No. 107" loading="lazy">
    <span class="wood-card-title">No. 107 · 19.5 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_108.webp" alt="Bow No. 108" loading="lazy">
    <span class="wood-card-title">No. 108 · 19.5 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_109.webp" alt="Bow No. 109" loading="lazy">
    <span class="wood-card-title">No. 109 · 19.5 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_110.webp" alt="Bow No. 110" loading="lazy">
    <span class="wood-card-title">No. 110 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_111.webp" alt="Bow No. 111" loading="lazy">
    <span class="wood-card-title">No. 111 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_112.webp" alt="Bow No. 112" loading="lazy">
    <span class="wood-card-title">No. 112 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_113.webp" alt="Bow No. 113" loading="lazy">
    <span class="wood-card-title">No. 113 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_114.webp" alt="Bow No. 114" loading="lazy">
    <span class="wood-card-title">No. 114 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_115.webp" alt="Bow No. 115" loading="lazy">
    <span class="wood-card-title">No. 115 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_116.webp" alt="Bow No. 116" loading="lazy">
    <span class="wood-card-title">No. 116 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_117.webp" alt="Bow No. 117" loading="lazy">
    <span class="wood-card-title">No. 117 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_118.webp" alt="Bow No. 118" loading="lazy">
    <span class="wood-card-title">No. 118 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_119.webp" alt="Bow No. 119" loading="lazy">
    <span class="wood-card-title">No. 119 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_120.webp" alt="Bow No. 120" loading="lazy">
    <span class="wood-card-title">No. 120 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_121.webp" alt="Bow No. 121" loading="lazy">
    <span class="wood-card-title">No. 121 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_122.webp" alt="Bow No. 122" loading="lazy">
    <span class="wood-card-title">No. 122 · 21.9 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_123.webp" alt="Bow No. 123" loading="lazy">
    <span class="wood-card-title">No. 123 · 24.4 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_124.webp" alt="Bow No. 124" loading="lazy">
    <span class="wood-card-title">No. 124 · 24.4 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_125.webp" alt="Bow No. 125" loading="lazy">
    <span class="wood-card-title">No. 125 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_126.webp" alt="Bow No. 126" loading="lazy">
    <span class="wood-card-title">No. 126 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_127.webp" alt="Bow No. 127" loading="lazy">
    <span class="wood-card-title">No. 127 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_128.webp" alt="Bow No. 128" loading="lazy">
    <span class="wood-card-title">No. 128 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_129.webp" alt="Bow No. 129" loading="lazy">
    <span class="wood-card-title">No. 129 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_130.webp" alt="Bow No. 130" loading="lazy">
    <span class="wood-card-title">No. 130 · 26.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_131.webp" alt="Bow No. 131" loading="lazy">
    <span class="wood-card-title">No. 131 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_132.webp" alt="Bow No. 132" loading="lazy">
    <span class="wood-card-title">No. 132 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_133.webp" alt="Bow No. 133" loading="lazy">
    <span class="wood-card-title">No. 133 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_134.webp" alt="Bow No. 134" loading="lazy">
    <span class="wood-card-title">No. 134 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_135.webp" alt="Bow No. 135" loading="lazy">
    <span class="wood-card-title">No. 135 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_136.webp" alt="Bow No. 136" loading="lazy">
    <span class="wood-card-title">No. 136 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_137.webp" alt="Bow No. 137" loading="lazy">
    <span class="wood-card-title">No. 137 · 29.7 m · p. 25</span>
  </div>
</div>

### Roof trusses, Class II {#truss_bow_roof_ii}

Class II: a deficiency of parts: with hinged joints they hold their shape only under equilibrating loads, and owe their resistance to distortion to stiff joints and bending of the parts.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_138.webp" alt="Bow No. 138" loading="lazy">
    <span class="wood-card-title">No. 138 · 7.3 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_139.webp" alt="Bow No. 139" loading="lazy">
    <span class="wood-card-title">No. 139 · 7.3 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_140.webp" alt="Bow No. 140" loading="lazy">
    <span class="wood-card-title">No. 140 · 7.3 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_141.webp" alt="Bow No. 141" loading="lazy">
    <span class="wood-card-title">No. 141 · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_142.webp" alt="Bow No. 142" loading="lazy">
    <span class="wood-card-title">No. 142 · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_143.webp" alt="Bow No. 143" loading="lazy">
    <span class="wood-card-title">No. 143 · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_144.webp" alt="Bow No. 144" loading="lazy">
    <span class="wood-card-title">No. 144 · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_145.webp" alt="Bow No. 145" loading="lazy">
    <span class="wood-card-title">No. 145 · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_146.webp" alt="Bow No. 146" loading="lazy">
    <span class="wood-card-title">No. 146 · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_147.webp" alt="Bow No. 147" loading="lazy">
    <span class="wood-card-title">No. 147 · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_148.webp" alt="Bow No. 148" loading="lazy">
    <span class="wood-card-title">No. 148 · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_149.webp" alt="Bow No. 149" loading="lazy">
    <span class="wood-card-title">No. 149 · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_150.webp" alt="Bow No. 150" loading="lazy">
    <span class="wood-card-title">No. 150 · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_151.webp" alt="Bow No. 151" loading="lazy">
    <span class="wood-card-title">No. 151 · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_152.webp" alt="Bow No. 152" loading="lazy">
    <span class="wood-card-title">No. 152 · 14.6 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_153.webp" alt="Bow No. 153" loading="lazy">
    <span class="wood-card-title">No. 153 · 14.6 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_154.webp" alt="Bow No. 154" loading="lazy">
    <span class="wood-card-title">No. 154 · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_155.webp" alt="Bow No. 155" loading="lazy">
    <span class="wood-card-title">No. 155 · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_156.webp" alt="Bow No. 156" loading="lazy">
    <span class="wood-card-title">No. 156 · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_157.webp" alt="Bow No. 157" loading="lazy">
    <span class="wood-card-title">No. 157 · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_158.webp" alt="Bow No. 158" loading="lazy">
    <span class="wood-card-title">No. 158 · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_159.webp" alt="Bow No. 159" loading="lazy">
    <span class="wood-card-title">No. 159 · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_160.webp" alt="Bow No. 160" loading="lazy">
    <span class="wood-card-title">No. 160 · 17.1 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_161.webp" alt="Bow No. 161" loading="lazy">
    <span class="wood-card-title">No. 161 · 19.5 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_162.webp" alt="Bow No. 162" loading="lazy">
    <span class="wood-card-title">No. 162 · 19.5 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_163.webp" alt="Bow No. 163" loading="lazy">
    <span class="wood-card-title">No. 163 · 24.4 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_164.webp" alt="Bow No. 164" loading="lazy">
    <span class="wood-card-title">No. 164 · 29.3 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_165.webp" alt="Bow No. 165" loading="lazy">
    <span class="wood-card-title">No. 165 · 29.3 m · p. 27</span>
  </div>
</div>

### Roof trusses, Class III {#truss_bow_roof_iii}

Class III: a redundancy of parts: no part can be made longer or shorter without inducing stress in others, so the stresses depend on the fitting, elasticity and temperature.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_166.webp" alt="Bow No. 166" loading="lazy">
    <span class="wood-card-title">No. 166 · 4.9 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_167.webp" alt="Bow No. 167" loading="lazy">
    <span class="wood-card-title">No. 167 · 7.3 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_168.webp" alt="Bow No. 168" loading="lazy">
    <span class="wood-card-title">No. 168 · 7.3 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_169.webp" alt="Bow No. 169" loading="lazy">
    <span class="wood-card-title">No. 169 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_170.webp" alt="Bow No. 170" loading="lazy">
    <span class="wood-card-title">No. 170 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_171.webp" alt="Bow No. 171" loading="lazy">
    <span class="wood-card-title">No. 171 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_172.webp" alt="Bow No. 172" loading="lazy">
    <span class="wood-card-title">No. 172 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_173.webp" alt="Bow No. 173" loading="lazy">
    <span class="wood-card-title">No. 173 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_174.webp" alt="Bow No. 174" loading="lazy">
    <span class="wood-card-title">No. 174 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_175.webp" alt="Bow No. 175" loading="lazy">
    <span class="wood-card-title">No. 175 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_176.webp" alt="Bow No. 176" loading="lazy">
    <span class="wood-card-title">No. 176 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_177.webp" alt="Bow No. 177" loading="lazy">
    <span class="wood-card-title">No. 177 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_178.webp" alt="Bow No. 178" loading="lazy">
    <span class="wood-card-title">No. 178 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_179.webp" alt="Bow No. 179" loading="lazy">
    <span class="wood-card-title">No. 179 · 12.2 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_180.webp" alt="Bow No. 180" loading="lazy">
    <span class="wood-card-title">No. 180 · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_181.webp" alt="Bow No. 181" loading="lazy">
    <span class="wood-card-title">No. 181 · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_182.webp" alt="Bow No. 182" loading="lazy">
    <span class="wood-card-title">No. 182 · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_183.webp" alt="Bow No. 183" loading="lazy">
    <span class="wood-card-title">No. 183 · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_184.webp" alt="Bow No. 184" loading="lazy">
    <span class="wood-card-title">No. 184 · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_185.webp" alt="Bow No. 185" loading="lazy">
    <span class="wood-card-title">No. 185 · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_186.webp" alt="Bow No. 186" loading="lazy">
    <span class="wood-card-title">No. 186 · 14.6 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_187.webp" alt="Bow No. 187" loading="lazy">
    <span class="wood-card-title">No. 187 · 14.6 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_188.webp" alt="Bow No. 188" loading="lazy">
    <span class="wood-card-title">No. 188 · 19.5 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_189.webp" alt="Bow No. 189" loading="lazy">
    <span class="wood-card-title">No. 189 · 19.5 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_190.webp" alt="Bow No. 190" loading="lazy">
    <span class="wood-card-title">No. 190 · 19.5 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_191.webp" alt="Bow No. 191" loading="lazy">
    <span class="wood-card-title">No. 191 · 21.9 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_192.webp" alt="Bow No. 192" loading="lazy">
    <span class="wood-card-title">No. 192 · 40.5 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_193.webp" alt="Bow No. 193" loading="lazy">
    <span class="wood-card-title">No. 193 · 30.5 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_194.webp" alt="Bow No. 194" loading="lazy">
    <span class="wood-card-title">No. 194 · 30.5 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_195.webp" alt="Bow No. 195" loading="lazy">
    <span class="wood-card-title">No. 195 · 64.3 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_196.webp" alt="Bow No. 196" loading="lazy">
    <span class="wood-card-title">No. 196 · 30.5 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_197.webp" alt="Bow No. 197" loading="lazy">
    <span class="wood-card-title">No. 197 · 30.5 m · p. 30</span>
  </div>
</div>

### Roof trusses, Class IV {#truss_bow_roof_iv}

Class IV: the defects of both Classes II and III: redundant in one region, deficient in another.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_198.webp" alt="Bow No. 198" loading="lazy">
    <span class="wood-card-title">No. 198 · 7.3 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_199.webp" alt="Bow No. 199" loading="lazy">
    <span class="wood-card-title">No. 199 · 12.2 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_200.webp" alt="Bow No. 200" loading="lazy">
    <span class="wood-card-title">No. 200 · 12.2 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_200_half.webp" alt="Bow No. 200½" loading="lazy">
    <span class="wood-card-title">No. 200½ · 14.6 m · p. 30</span>
  </div>
</div>

### Framed bridges, Class I {#truss_bow_bridge_i}

Class I: theoretically perfect frames: with every joint hinged they still cannot distort, and no part can be lengthened or shortened so as to induce stress.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_201i.webp" alt="Bow No. 201i" loading="lazy">
    <span class="wood-card-title">No. 201i · 4.9 m · p. 31</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_201iii.webp" alt="Bow No. 201iii" loading="lazy">
    <span class="wood-card-title">No. 201iii · 4.9 m · p. 31</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_202a.webp" alt="Bow No. 202A" loading="lazy">
    <span class="wood-card-title">No. 202A · 4.9 m · p. 31</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_202b.webp" alt="Bow No. 202B" loading="lazy">
    <span class="wood-card-title">No. 202B · 4.9 m · p. 31</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_202c.webp" alt="Bow No. 202C" loading="lazy">
    <span class="wood-card-title">No. 202C · 4.9 m · p. 31</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_203.webp" alt="Bow No. 203" loading="lazy">
    <span class="wood-card-title">No. 203 · 4.9 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_204.webp" alt="Bow No. 204" loading="lazy">
    <span class="wood-card-title">No. 204 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_205.webp" alt="Bow No. 205" loading="lazy">
    <span class="wood-card-title">No. 205 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_206.webp" alt="Bow No. 206" loading="lazy">
    <span class="wood-card-title">No. 206 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_207.webp" alt="Bow No. 207" loading="lazy">
    <span class="wood-card-title">No. 207 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_208.webp" alt="Bow No. 208" loading="lazy">
    <span class="wood-card-title">No. 208 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_209.webp" alt="Bow No. 209" loading="lazy">
    <span class="wood-card-title">No. 209 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_210.webp" alt="Bow No. 210" loading="lazy">
    <span class="wood-card-title">No. 210 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_211.webp" alt="Bow No. 211" loading="lazy">
    <span class="wood-card-title">No. 211 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_212.webp" alt="Bow No. 212" loading="lazy">
    <span class="wood-card-title">No. 212 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_213.webp" alt="Bow No. 213" loading="lazy">
    <span class="wood-card-title">No. 213 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_214.webp" alt="Bow No. 214" loading="lazy">
    <span class="wood-card-title">No. 214 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_215.webp" alt="Bow No. 215" loading="lazy">
    <span class="wood-card-title">No. 215 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_216.webp" alt="Bow No. 216" loading="lazy">
    <span class="wood-card-title">No. 216 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_217.webp" alt="Bow No. 217" loading="lazy">
    <span class="wood-card-title">No. 217 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_218.webp" alt="Bow No. 218" loading="lazy">
    <span class="wood-card-title">No. 218 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_219.webp" alt="Bow No. 219" loading="lazy">
    <span class="wood-card-title">No. 219 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_220.webp" alt="Bow No. 220" loading="lazy">
    <span class="wood-card-title">No. 220 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_221.webp" alt="Bow No. 221" loading="lazy">
    <span class="wood-card-title">No. 221 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_222.webp" alt="Bow No. 222" loading="lazy">
    <span class="wood-card-title">No. 222 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_223.webp" alt="Bow No. 223" loading="lazy">
    <span class="wood-card-title">No. 223 · 12.2 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_224.webp" alt="Bow No. 224" loading="lazy">
    <span class="wood-card-title">No. 224 · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_225.webp" alt="Bow No. 225" loading="lazy">
    <span class="wood-card-title">No. 225 · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_226.webp" alt="Bow No. 226" loading="lazy">
    <span class="wood-card-title">No. 226 · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_227.webp" alt="Bow No. 227" loading="lazy">
    <span class="wood-card-title">No. 227 · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_228.webp" alt="Bow No. 228" loading="lazy">
    <span class="wood-card-title">No. 228 · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_229.webp" alt="Bow No. 229" loading="lazy">
    <span class="wood-card-title">No. 229 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_230.webp" alt="Bow No. 230" loading="lazy">
    <span class="wood-card-title">No. 230 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_231.webp" alt="Bow No. 231" loading="lazy">
    <span class="wood-card-title">No. 231 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_232.webp" alt="Bow No. 232" loading="lazy">
    <span class="wood-card-title">No. 232 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_233.webp" alt="Bow No. 233" loading="lazy">
    <span class="wood-card-title">No. 233 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_234.webp" alt="Bow No. 234" loading="lazy">
    <span class="wood-card-title">No. 234 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_235.webp" alt="Bow No. 235" loading="lazy">
    <span class="wood-card-title">No. 235 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_236.webp" alt="Bow No. 236" loading="lazy">
    <span class="wood-card-title">No. 236 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_237.webp" alt="Bow No. 237" loading="lazy">
    <span class="wood-card-title">No. 237 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_238.webp" alt="Bow No. 238" loading="lazy">
    <span class="wood-card-title">No. 238 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_239.webp" alt="Bow No. 239" loading="lazy">
    <span class="wood-card-title">No. 239 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_240.webp" alt="Bow No. 240" loading="lazy">
    <span class="wood-card-title">No. 240 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_241.webp" alt="Bow No. 241" loading="lazy">
    <span class="wood-card-title">No. 241 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_242.webp" alt="Bow No. 242" loading="lazy">
    <span class="wood-card-title">No. 242 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_243.webp" alt="Bow No. 243" loading="lazy">
    <span class="wood-card-title">No. 243 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_244.webp" alt="Bow No. 244" loading="lazy">
    <span class="wood-card-title">No. 244 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_245.webp" alt="Bow No. 245" loading="lazy">
    <span class="wood-card-title">No. 245 · 17.1 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_246.webp" alt="Bow No. 246" loading="lazy">
    <span class="wood-card-title">No. 246 · 17.1 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_247.webp" alt="Bow No. 247" loading="lazy">
    <span class="wood-card-title">No. 247 · 17.1 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_248.webp" alt="Bow No. 248" loading="lazy">
    <span class="wood-card-title">No. 248 · 17.1 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_249.webp" alt="Bow No. 249" loading="lazy">
    <span class="wood-card-title">No. 249 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_250.webp" alt="Bow No. 250" loading="lazy">
    <span class="wood-card-title">No. 250 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_251.webp" alt="Bow No. 251" loading="lazy">
    <span class="wood-card-title">No. 251 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_252.webp" alt="Bow No. 252" loading="lazy">
    <span class="wood-card-title">No. 252 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_253.webp" alt="Bow No. 253" loading="lazy">
    <span class="wood-card-title">No. 253 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_254.webp" alt="Bow No. 254" loading="lazy">
    <span class="wood-card-title">No. 254 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_255.webp" alt="Bow No. 255" loading="lazy">
    <span class="wood-card-title">No. 255 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_256.webp" alt="Bow No. 256" loading="lazy">
    <span class="wood-card-title">No. 256 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_257.webp" alt="Bow No. 257" loading="lazy">
    <span class="wood-card-title">No. 257 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_258.webp" alt="Bow No. 258" loading="lazy">
    <span class="wood-card-title">No. 258 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_259.webp" alt="Bow No. 259" loading="lazy">
    <span class="wood-card-title">No. 259 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_260.webp" alt="Bow No. 260" loading="lazy">
    <span class="wood-card-title">No. 260 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_261.webp" alt="Bow No. 261" loading="lazy">
    <span class="wood-card-title">No. 261 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_262.webp" alt="Bow No. 262" loading="lazy">
    <span class="wood-card-title">No. 262 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_263.webp" alt="Bow No. 263" loading="lazy">
    <span class="wood-card-title">No. 263 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_264.webp" alt="Bow No. 264" loading="lazy">
    <span class="wood-card-title">No. 264 · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_265.webp" alt="Bow No. 265" loading="lazy">
    <span class="wood-card-title">No. 265 · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_266.webp" alt="Bow No. 266" loading="lazy">
    <span class="wood-card-title">No. 266 · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_267.webp" alt="Bow No. 267" loading="lazy">
    <span class="wood-card-title">No. 267 · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_268.webp" alt="Bow No. 268" loading="lazy">
    <span class="wood-card-title">No. 268 · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_269.webp" alt="Bow No. 269" loading="lazy">
    <span class="wood-card-title">No. 269 · 24.4 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_270.webp" alt="Bow No. 270" loading="lazy">
    <span class="wood-card-title">No. 270 · 24.4 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_271.webp" alt="Bow No. 271" loading="lazy">
    <span class="wood-card-title">No. 271 · 24.4 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_272.webp" alt="Bow No. 272" loading="lazy">
    <span class="wood-card-title">No. 272 · 24.4 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_273.webp" alt="Bow No. 273" loading="lazy">
    <span class="wood-card-title">No. 273 · 24.4 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_274.webp" alt="Bow No. 274" loading="lazy">
    <span class="wood-card-title">No. 274 · 29.3 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_275.webp" alt="Bow No. 275" loading="lazy">
    <span class="wood-card-title">No. 275 · 29.3 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_276.webp" alt="Bow No. 276" loading="lazy">
    <span class="wood-card-title">No. 276 · 29.3 m · p. 35</span>
  </div>
</div>

### Framed bridges, Class II {#truss_bow_bridge_ii}

Class II: a deficiency of parts: with hinged joints they hold their shape only under equilibrating loads, and owe their resistance to distortion to stiff joints and bending of the parts.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_277.webp" alt="Bow No. 277" loading="lazy">
    <span class="wood-card-title">No. 277 · 4.9 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_278.webp" alt="Bow No. 278" loading="lazy">
    <span class="wood-card-title">No. 278 · 7.3 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_279.webp" alt="Bow No. 279" loading="lazy">
    <span class="wood-card-title">No. 279 · 7.3 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_280.webp" alt="Bow No. 280" loading="lazy">
    <span class="wood-card-title">No. 280 · 9.8 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_281.webp" alt="Bow No. 281" loading="lazy">
    <span class="wood-card-title">No. 281 · 12.2 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_282.webp" alt="Bow No. 282" loading="lazy">
    <span class="wood-card-title">No. 282 · 12.2 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_283.webp" alt="Bow No. 283" loading="lazy">
    <span class="wood-card-title">No. 283 · 14.6 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_284.webp" alt="Bow No. 284" loading="lazy">
    <span class="wood-card-title">No. 284 · 17.1 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_285.webp" alt="Bow No. 285" loading="lazy">
    <span class="wood-card-title">No. 285 · 24.4 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_286.webp" alt="Bow No. 286" loading="lazy">
    <span class="wood-card-title">No. 286 · 24.4 m · p. 36</span>
  </div>
</div>

### Framed bridges, Class III {#truss_bow_bridge_iii}

Class III: a redundancy of parts: no part can be made longer or shorter without inducing stress in others, so the stresses depend on the fitting, elasticity and temperature.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_288.webp" alt="Bow No. 288" loading="lazy">
    <span class="wood-card-title">No. 288 · 4.9 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_289.webp" alt="Bow No. 289" loading="lazy">
    <span class="wood-card-title">No. 289 · 4.9 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_290.webp" alt="Bow No. 290" loading="lazy">
    <span class="wood-card-title">No. 290 · 4.9 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_291.webp" alt="Bow No. 291" loading="lazy">
    <span class="wood-card-title">No. 291 · 7.3 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_292.webp" alt="Bow No. 292" loading="lazy">
    <span class="wood-card-title">No. 292 · 7.3 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_293.webp" alt="Bow No. 293" loading="lazy">
    <span class="wood-card-title">No. 293 · 90.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_294.webp" alt="Bow No. 294" loading="lazy">
    <span class="wood-card-title">No. 294 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_295.webp" alt="Bow No. 295" loading="lazy">
    <span class="wood-card-title">No. 295 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_296.webp" alt="Bow No. 296" loading="lazy">
    <span class="wood-card-title">No. 296 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_297.webp" alt="Bow No. 297" loading="lazy">
    <span class="wood-card-title">No. 297 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_298.webp" alt="Bow No. 298" loading="lazy">
    <span class="wood-card-title">No. 298 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_299.webp" alt="Bow No. 299" loading="lazy">
    <span class="wood-card-title">No. 299 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_300.webp" alt="Bow No. 300" loading="lazy">
    <span class="wood-card-title">No. 300 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_301.webp" alt="Bow No. 301" loading="lazy">
    <span class="wood-card-title">No. 301 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_302.webp" alt="Bow No. 302" loading="lazy">
    <span class="wood-card-title">No. 302 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_303.webp" alt="Bow No. 303" loading="lazy">
    <span class="wood-card-title">No. 303 · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_304.webp" alt="Bow No. 304" loading="lazy">
    <span class="wood-card-title">No. 304 · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_305.webp" alt="Bow No. 305" loading="lazy">
    <span class="wood-card-title">No. 305 · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_306.webp" alt="Bow No. 306" loading="lazy">
    <span class="wood-card-title">No. 306 · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_307.webp" alt="Bow No. 307" loading="lazy">
    <span class="wood-card-title">No. 307 · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_308.webp" alt="Bow No. 308" loading="lazy">
    <span class="wood-card-title">No. 308 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_309.webp" alt="Bow No. 309" loading="lazy">
    <span class="wood-card-title">No. 309 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_310.webp" alt="Bow No. 310" loading="lazy">
    <span class="wood-card-title">No. 310 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_311.webp" alt="Bow No. 311" loading="lazy">
    <span class="wood-card-title">No. 311 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_312.webp" alt="Bow No. 312" loading="lazy">
    <span class="wood-card-title">No. 312 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_313.webp" alt="Bow No. 313" loading="lazy">
    <span class="wood-card-title">No. 313 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_314.webp" alt="Bow No. 314" loading="lazy">
    <span class="wood-card-title">No. 314 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_315.webp" alt="Bow No. 315" loading="lazy">
    <span class="wood-card-title">No. 315 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_316.webp" alt="Bow No. 316" loading="lazy">
    <span class="wood-card-title">No. 316 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_317.webp" alt="Bow No. 317" loading="lazy">
    <span class="wood-card-title">No. 317 · 17.1 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_318.webp" alt="Bow No. 318" loading="lazy">
    <span class="wood-card-title">No. 318 · 17.1 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_319.webp" alt="Bow No. 319" loading="lazy">
    <span class="wood-card-title">No. 319 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_320.webp" alt="Bow No. 320" loading="lazy">
    <span class="wood-card-title">No. 320 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_321.webp" alt="Bow No. 321" loading="lazy">
    <span class="wood-card-title">No. 321 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_322.webp" alt="Bow No. 322" loading="lazy">
    <span class="wood-card-title">No. 322 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_323.webp" alt="Bow No. 323" loading="lazy">
    <span class="wood-card-title">No. 323 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_324.webp" alt="Bow No. 324" loading="lazy">
    <span class="wood-card-title">No. 324 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_325.webp" alt="Bow No. 325" loading="lazy">
    <span class="wood-card-title">No. 325 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_326.webp" alt="Bow No. 326" loading="lazy">
    <span class="wood-card-title">No. 326 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_327.webp" alt="Bow No. 327" loading="lazy">
    <span class="wood-card-title">No. 327 · 21.9 m · p. 39</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_328.webp" alt="Bow No. 328" loading="lazy">
    <span class="wood-card-title">No. 328 · 24.4 m · p. 39</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_329.webp" alt="Bow No. 329" loading="lazy">
    <span class="wood-card-title">No. 329 · 29.3 m · p. 39</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_330.webp" alt="Bow No. 330" loading="lazy">
    <span class="wood-card-title">No. 330 · 29.3 m · p. 39</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_331.webp" alt="Bow No. 331" loading="lazy">
    <span class="wood-card-title">No. 331 · 39.0 m · p. 39</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_332.webp" alt="Bow No. 332" loading="lazy">
    <span class="wood-card-title">No. 332 · 62.2 m · p. 39</span>
  </div>
</div>

### Framed bridges, Class IV {#truss_bow_bridge_iv}

Class IV: the defects of both Classes II and III: redundant in one region, deficient in another.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_333.webp" alt="Bow No. 333" loading="lazy">
    <span class="wood-card-title">No. 333 · 14.6 m · p. 40</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_334.webp" alt="Bow No. 334" loading="lazy">
    <span class="wood-card-title">No. 334 · 17.1 m · p. 40</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_335.webp" alt="Bow No. 335" loading="lazy">
    <span class="wood-card-title">No. 335 · 21.9 m · p. 40</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_336.webp" alt="Bow No. 336" loading="lazy">
    <span class="wood-card-title">No. 336 · 21.9 m · p. 40</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_337.webp" alt="Bow No. 337" loading="lazy">
    <span class="wood-card-title">No. 337 · 21.9 m · p. 40</span>
  </div>
</div>

<!-- /bow-gallery -->

## Library Runner

```cpp
#include "wood_session.h"
#include "src/templates/truss/truss.h"

using namespace session_cpp;
using namespace wood_session;

/// Every truss of the TrussGuide library built by the Truss template, each its own scene truss_<name>.
int main() {

    const std::vector<std::pair<std::string, wood_truss::TrussGuide>> guides = {
        {"pratt", wood_truss::TrussGuide::pratt()},
        {"king_post", wood_truss::TrussGuide::king_post()},
        {"queen_post", wood_truss::TrussGuide::queen_post()},
        {"howe", wood_truss::TrussGuide::howe()},
        {"fink", wood_truss::TrussGuide::fink()},
        {"belgian", wood_truss::TrussGuide::belgian()},
        {"scissors", wood_truss::TrussGuide::scissors()},
        {"warren", wood_truss::TrussGuide::warren()},
        {"warren_verticals", wood_truss::TrussGuide::warren_verticals()},
        {"pratt_bridge", wood_truss::TrussGuide::pratt_bridge()},
        {"parker", wood_truss::TrussGuide::parker()},
        {"k_truss", wood_truss::TrussGuide::k_truss()},
        {"cambered_verticals", wood_truss::TrussGuide::cambered_verticals()},
        {"funicular_arch", wood_truss::TrussGuide::funicular_arch()},
        {"lens", wood_truss::TrussGuide::lens()},
        {"triangulated_arch", wood_truss::TrussGuide::triangulated_arch()},
        {"sloped", wood_truss::TrussGuide::sloped()},
        {"pratt_flat", wood_truss::TrussGuide::pratt_flat()},
        {"flat_peaked", wood_truss::TrussGuide::flat_peaked()},
        {"bowstring", wood_truss::TrussGuide::bowstring()},
        {"pitched_posts", wood_truss::TrussGuide::pitched_posts()},
        {"roof_cambered_tie", wood_truss::TrussGuide::roof_cambered_tie()},
        {"quarter_fan", wood_truss::TrussGuide::quarter_fan()},
        {"cantilever", wood_truss::TrussGuide::cantilever()},
        {"cantilever_design_1", wood_truss::TrussGuide::cantilever_design_1()},
        {"cantilever_design_2", wood_truss::TrussGuide::cantilever_design_2()},
        {"cantilever_design_3", wood_truss::TrussGuide::cantilever_design_3()},
        {"fish_belly", wood_truss::TrussGuide::fish_belly()},
        {"sawtooth", wood_truss::TrussGuide::sawtooth()},
        {"howe_flat", wood_truss::TrussGuide::howe_flat()},
        {"three_hinged_arch", wood_truss::TrussGuide::three_hinged_arch()},
        {"burr_arch", wood_truss::TrussGuide::burr_arch()},
        {"town_lattice", wood_truss::TrussGuide::town_lattice()},
        {"bowstring_arch", wood_truss::TrussGuide::bowstring_arch()},
        {"waddell_a", wood_truss::TrussGuide::waddell_a()},
        {"wichert", wood_truss::TrussGuide::wichert()},
        {"truss_leg_bedstead", wood_truss::TrussGuide::truss_leg_bedstead()},
        {"kellogg", wood_truss::TrussGuide::kellogg()},
        {"baltimore_a", wood_truss::TrussGuide::baltimore_a()},
        {"pennsylvania_a", wood_truss::TrussGuide::pennsylvania_a()},
        {"greiner", wood_truss::TrussGuide::greiner()},
        {"double_intersection_pratt", wood_truss::TrussGuide::double_intersection_pratt()},
        {"schwedler", wood_truss::TrussGuide::schwedler()},
        {"double_intersection_warren", wood_truss::TrussGuide::double_intersection_warren()},
        {"pegram", wood_truss::TrussGuide::pegram()},
        {"post", wood_truss::TrussGuide::post()},
        {"bollman", wood_truss::TrussGuide::bollman()},
        {"fink_bridge", wood_truss::TrussGuide::fink_bridge()},
        {"stearns", wood_truss::TrussGuide::stearns()},
        {"crescent_crossed", wood_truss::TrussGuide::crescent_crossed()},
        {"braced_arch_rib", wood_truss::TrussGuide::braced_arch_rib()},
        {"belgian_verticals", wood_truss::TrussGuide::belgian_verticals()},
        {"raised_tie_king_post", wood_truss::TrussGuide::raised_tie_king_post()},
        {"trapezoid_crossed", wood_truss::TrussGuide::trapezoid_crossed()},
        {"fink_cambered", wood_truss::TrussGuide::fink_cambered()},
        {"crescent_verticals", wood_truss::TrussGuide::crescent_verticals()},
        {"raised_tie_posts", wood_truss::TrussGuide::raised_tie_posts()},
        {"pitched_warren_columns", wood_truss::TrussGuide::pitched_warren_columns()},
        {"fink_compound", wood_truss::TrussGuide::fink_compound()},
        {"fink_bent_tie", wood_truss::TrussGuide::fink_bent_tie()},
        {"french_steep", wood_truss::TrussGuide::french_steep()},
        {"fink_lantern", wood_truss::TrussGuide::fink_lantern()},
        {"arch_polygonal_tie", wood_truss::TrussGuide::arch_polygonal_tie()},
        {"pitched_crossed_posts", wood_truss::TrussGuide::pitched_crossed_posts()},
        {"cambered_crossed_lantern", wood_truss::TrussGuide::cambered_crossed_lantern()},
        {"arched_crossed_columns", wood_truss::TrussGuide::arched_crossed_columns()},
        {"trapezoid_king_crossed", wood_truss::TrussGuide::trapezoid_king_crossed()},
        {"topology_optimised", wood_truss::TrussGuide::topology_optimised()},
    };
    for (const std::pair<std::string, wood_truss::TrussGuide>& guide : guides) {
        wood_truss::Truss truss(guide.second, "truss_" + guide.first);
        truss.pb_dump(pb_path("truss_" + guide.first));
    }
    return 0;
}
```
