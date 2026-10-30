# Truss {#templates_truss}

[TOC]

Timber trusses in `src/templates/truss/*.h`. Every truss is a `TrussGuide`, made by one constructor and its overloads: from its members, or from a design by its parameters, `TrussGuide(Pratt{})` or `TrussGuide(Bow{"64"})`. `wood_truss::Truss` makes each member a `Beam` and joins them by a member hierarchy: T joints with run-on ends, butts on the members' real surfaces, miters at gentle bends and ridges, half laps where members cross. The 68 designs of the library and the 326 roof trusses and framed bridges of Bow's 1873 catalogue are at the end of this page.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#truss_ex_pratt">
    <img src="templates_truss_1_pratt.png" alt="Pratt Truss">
    <span class="wood-card-title">Pratt Truss</span>
  </a>
  <a class="wood-card" href="#truss_library">
    <img src="truss_king_post.png" alt="King and Queen Post">
    <span class="wood-card-title">King and Queen Post</span>
  </a>
  <a class="wood-card" href="#truss_library">
    <img src="truss_howe.png" alt="Howe and Fink">
    <span class="wood-card-title">Howe and Fink</span>
  </a>
  <a class="wood-card" href="#truss_library">
    <img src="truss_scissors.png" alt="Scissors Truss">
    <span class="wood-card-title">Scissors Truss</span>
  </a>
  <a class="wood-card" href="#truss_library">
    <img src="truss_warren.png" alt="Warren and K-Truss">
    <span class="wood-card-title">Warren and K-Truss</span>
  </a>
  <a class="wood-card" href="#truss_library">
    <img src="truss_bowstring.png" alt="Bowstring and Arch">
    <span class="wood-card-title">Bowstring and Arch</span>
  </a>
  <a class="wood-card" href="#truss_library">
    <img src="truss_town_lattice.png" alt="Town Lattice">
    <span class="wood-card-title">Town Lattice</span>
  </a>
  <a class="wood-card" href="#truss_library">
    <img src="truss_topology_optimised.png" alt="Topology Optimised">
    <span class="wood-card-title">Topology Optimised</span>
  </a>
  <a class="wood-card" href="#truss_graph">
    <img src="truss_bow_64.webp" alt="Bow 1873">
    <span class="wood-card-title">Bow 1873: 326 Trusses</span>
  </a>
</div>

## Primary Inputs

A truss is two lists of members, the chords and the webs (a post is a web), each a `TrussMember`:

| `TrussMember` | Type | Default | Role |
| --- | --- | --- | --- |
| `axis` | `Polyline` | | Through its nodes, end to end: a chord turns at its kinks, a web is straight between two nodes |
| `radius` | `double` | `80.0` | Half its square section, mm (`80.0` → `160 × 160 mm`) |
| `level` | `int` | `0` | 1 primary, 2 secondary, 3 tertiary; `0`: read from the drawing |
| `runs_on` | `std::array<bool, 2>` | `{false, false}` | A chord's start and end running on where chord ends meet, in place of the automatic choice |

Every truss is a `TrussGuide`, made from its members or from a design by its parameters, an overload per design:

```cpp
TrussGuide(const std::vector<TrussMember>& chords, const std::vector<TrussMember>& webs);
TrussGuide(const Pratt& design);  // and so on: an overload per design of the library
TrussGuide(const Bow& design);    // a truss of Bow's catalogue by its number
```

A design is a struct of its parameters, lengths in mm, each with its default: `TrussGuide(Pratt{})` is the Pratt of the
[library](#truss_library), `TrussGuide(Pratt{.span = 15000.0, .panels = 8})` a longer one. Every design also takes
`chord_radius` and `web_radius`, the half sections of the chords and posts and of the webs; it draws its chords and webs
and reads the hierarchy from the drawing. `TrussGuide(Bow{"64"})` is Bow's No. 64, its graph read from
`data/templates/truss_bow_1873.json` and drawn at its span with a section and a level per member
([Bow 1873](#truss_graph)).

## Geometry Generation & Joint Rules

Every node is solved by six rules:

1. Hierarchy: Each member has a level: 1 primary (chords, posts), 2 secondary (webs), 3 tertiary (sub-struts); where
   `level` is 0 it is read from the drawing, the outer boundary first, by pruning the line graph layer by layer. At its node
   a member stops on every higher member there, in a T: it loses that member's cut solid swept along its own axis (its
   shadow), so it butts on the real surface, a kink or a corner, and keeps nothing behind it.
2. One end runs on: Where chord ends meet, one runs on past the node and the others are seated on its side
   (`TrussGuide::runs_on_at`): the one marked `runs_on`, else the higher level, else the more level, so the tie runs past
   the rafter foot and a chord past an end post. A lone chord end runs on over webs that are all lower than it.
3. Clean ends, node in wood: An end that runs on reaches to its end cut (`TrussGuide::end_cut`): through the outer
   corners of the members it carries on both its sides, so the contour runs on clean; flush with the outer face of a member
   met at 40 degrees or more; else square, past the node by at least its half section. The node and the axes always lie in
   wood.
4. Miters only within one piece: A chord's kinks, two ends of one slope (a ridge) and bends of under 35 degrees are
   mitered; every other meeting is a T, easier to cut than a sharp miter.
5. Laps where members cross: Members of one level crossing share the depth in half laps; two chords bending at one node
   are half-lapped; a chord ending at another's bend butts on its surface.
6. The lower never cuts the higher: A lower member passing a higher member's end leaves it whole.

| Node / Crossing Configuration | Automatic Joint Rule (`Truss::add_chord_butts` / `add_web_butts`) |
| --- | --- |
| Chord kink (interior polyline vertex) | Adjacent chord runs mitered `side_side` on the angle bisector |
| Chord heel (two chords ending at one node) | A T: the one running on (`runs_on_at`) reaches past the node to its `end_cut`, the other seated on its side (`top_side`); one slope (ridge) or a gentle bend: mitered, `JointBeam::butt_side_side` |
| Chord end on another chord's run | Stopped `top_side` against the continuous chord |
| Web / post end on a chord run | Let into the chord: loses the chord's shadow along its axis |
| Web / post end at a chord's lone end | Outer member runs over the inner member (`top_side`); inner member let in |
| Web end on another web's middle (K-joint) | Let into the continuous web |
| Chord end on a post's middle (tie on column) | Let into the post |
| Crossing chords, webs, or posts (X-joint) | `JointBeam::butt_cross` half/multi-laps dividing member depth equally across `N` crossing bars |
| Multiple webs ending at one node | Inner webs let into outer chords; same-rank webs mitered `side_side` with angular neighbours |

## Constructor Pipeline

```cpp
Truss::Truss(const TrussGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    // 1. input: the guide's chords and webs as drawn
    add_input();

    // 2. members: a Beam per chord run and per web
    add_chords();
    add_webs();

    // 3. chords: runs mitered at kinks, butted at heels, lapped at crossings
    add_chord_butts();

    // 4. webs: let into chords, lapped at crossings, mitered with same-rank neighbours
    add_web_butts();
}
```

---

## Full C++ Examples

### Custom Wireframe Truss {#truss_ex_custom}

```cpp
#include "wood_session.h"
#include "src/templates/truss/truss.h"

using namespace session_cpp;
using namespace wood_session;

int main() {
    // 1. members: the chords through their nodes and the webs between theirs, each with its half section
    const std::vector<wood_truss::TrussMember> chords = {
        {Polyline({{0.0, 0.0, 0.0}, {4000.0, 0.0, 0.0}, {8000.0, 0.0, 0.0}}), 80.0},
        {Polyline({{0.0, 0.0, 0.0}, {4000.0, 0.0, 2200.0}, {8000.0, 0.0, 0.0}}), 80.0},
    };
    const std::vector<wood_truss::TrussMember> webs = {
        {Polyline({{2000.0, 0.0, 1100.0}, {2000.0, 0.0, 0.0}}), 60.0},
        {Polyline({{4000.0, 0.0, 2200.0}, {4000.0, 0.0, 0.0}}), 60.0},
        {Polyline({{6000.0, 0.0, 1100.0}, {6000.0, 0.0, 0.0}}), 60.0},
        {Polyline({{2000.0, 0.0, 1100.0}, {4000.0, 0.0, 0.0}}), 60.0},
        {Polyline({{6000.0, 0.0, 1100.0}, {4000.0, 0.0, 0.0}}), 60.0},
    };

    // 2. the guide (160 x 160 mm chords, 120 x 120 mm webs) and the truss
    const wood_truss::TrussGuide guide(chords, webs);
    wood_truss::Truss truss(guide, "custom_pratt_truss");
    truss.pb_dump(pb_path("live"));
    return 0;
}
```

### Pratt Roof Truss {#truss_ex_pratt}

![templates_truss_1_pratt](templates/templates_truss_1_pratt.png)

```cpp
#include "wood_session.h"
#include "src/templates/truss/truss.h"

using namespace session_cpp;
using namespace wood_session;

/// A Pratt roof truss from its guide, the design's overload by its parameters: the members and their butts made by the
/// template.
int main() {

    const wood_truss::TrussGuide guide(wood_truss::Pratt{.span = 12000.0, .rise = 3000.0, .panels = 6});
    wood_truss::Truss truss(guide);
    std::cout << truss << std::endl;
    truss.pb_dump(pb_path("live"));
    return 0;
}
```

## Truss Library {#truss_library}

<!-- library-gallery -->

The 68 designs of the library, each a struct of its parameters and its own `TrussGuide` overload, pictured with their defaults as `templates_truss_2_library` builds them, each its own scene `truss_<name>.pb`. Every design also takes `.chord_radius = 80` and `.web_radius = 60`, the half sections of the chords and posts and of the webs.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_pratt.png" alt="Pratt" loading="lazy">
    <span class="wood-card-title">Pratt{.span = 12000, .rise = 3000, .panels = 6}</span>
  </div>
  <div class="wood-card">
    <img src="truss_king_post.png" alt="KingPost" loading="lazy">
    <span class="wood-card-title">KingPost{.span = 8000, .rise = 2667}</span>
  </div>
  <div class="wood-card">
    <img src="truss_queen_post.png" alt="QueenPost" loading="lazy">
    <span class="wood-card-title">QueenPost{.span = 9000, .height = 2000}</span>
  </div>
  <div class="wood-card">
    <img src="truss_howe.png" alt="Howe" loading="lazy">
    <span class="wood-card-title">Howe{.span = 12000, .rise = 3000, .panels = 6}</span>
  </div>
  <div class="wood-card">
    <img src="truss_fink.png" alt="Fink" loading="lazy">
    <span class="wood-card-title">Fink{.span = 10000, .rise = 2500}</span>
  </div>
  <div class="wood-card">
    <img src="truss_belgian.png" alt="Belgian" loading="lazy">
    <span class="wood-card-title">Belgian{.span = 10000, .rise = 2500, .divisions = 3}</span>
  </div>
  <div class="wood-card">
    <img src="truss_scissors.png" alt="Scissors" loading="lazy">
    <span class="wood-card-title">Scissors{.span = 8000, .rise = 3000}</span>
  </div>
  <div class="wood-card">
    <img src="truss_warren.png" alt="Warren" loading="lazy">
    <span class="wood-card-title">Warren{.span = 12000, .height = 1500, .panels = 8}</span>
  </div>
  <div class="wood-card">
    <img src="truss_warren_verticals.png" alt="WarrenVerticals" loading="lazy">
    <span class="wood-card-title">WarrenVerticals{.span = 12000, .height = 1500, .panels = 8}</span>
  </div>
  <div class="wood-card">
    <img src="truss_pratt_bridge.png" alt="PrattBridge" loading="lazy">
    <span class="wood-card-title">PrattBridge{.span = 12000, .height = 2000, .panels = 6}</span>
  </div>
  <div class="wood-card">
    <img src="truss_parker.png" alt="Parker" loading="lazy">
    <span class="wood-card-title">Parker{.span = 12000, .height = 2400, .hip = 1600, .panels = 6}</span>
  </div>
  <div class="wood-card">
    <img src="truss_k_truss.png" alt="KTruss" loading="lazy">
    <span class="wood-card-title">KTruss{.span = 12000, .height = 2400, .panels = 8}</span>
  </div>
  <div class="wood-card">
    <img src="truss_cambered_verticals.png" alt="CamberedVerticals" loading="lazy">
    <span class="wood-card-title">CamberedVerticals{.span = 12000, .height = 1980, .camber = 800, .panels = 6}</span>
  </div>
  <div class="wood-card">
    <img src="truss_funicular_arch.png" alt="FunicularArch" loading="lazy">
    <span class="wood-card-title">FunicularArch{.span = 12000, .height = 2400}</span>
  </div>
  <div class="wood-card">
    <img src="truss_lens.png" alt="Lens" loading="lazy">
    <span class="wood-card-title">Lens{.span = 12000, .height = 1140, .sag = 1260}</span>
  </div>
  <div class="wood-card">
    <img src="truss_triangulated_arch.png" alt="TriangulatedArch" loading="lazy">
    <span class="wood-card-title">TriangulatedArch{.span = 12000, .height = 5980}</span>
  </div>
  <div class="wood-card">
    <img src="truss_sloped.png" alt="Sloped" loading="lazy">
    <span class="wood-card-title">Sloped{.span = 12040, .height = 6020}</span>
  </div>
  <div class="wood-card">
    <img src="truss_pratt_flat.png" alt="PrattFlat" loading="lazy">
    <span class="wood-card-title">PrattFlat{.span = 12000, .height = 1990, .panels = 6}</span>
  </div>
  <div class="wood-card">
    <img src="truss_flat_peaked.png" alt="FlatPeaked" loading="lazy">
    <span class="wood-card-title">FlatPeaked{.span = 12000, .height = 1990, .peak = 3180}</span>
  </div>
  <div class="wood-card">
    <img src="truss_bowstring.png" alt="Bowstring" loading="lazy">
    <span class="wood-card-title">Bowstring{.span = 12000, .height = 1990, .crown = 3320, .panels = 6}</span>
  </div>
  <div class="wood-card">
    <img src="truss_pitched_posts.png" alt="PitchedPosts" loading="lazy">
    <span class="wood-card-title">PitchedPosts{.span = 12000, .height = 2000, .ridge = 3320, .panels = 6}</span>
  </div>
  <div class="wood-card">
    <img src="truss_roof_cambered_tie.png" alt="RoofCamberedTie" loading="lazy">
    <span class="wood-card-title">RoofCamberedTie{.span = 12000, .rise = 2000, .camber = 500, .dip = 60}</span>
  </div>
  <div class="wood-card">
    <img src="truss_quarter_fan.png" alt="QuarterFan" loading="lazy">
    <span class="wood-card-title">QuarterFan{.span = 12000, .height = 5560}</span>
  </div>
  <div class="wood-card">
    <img src="truss_cantilever.png" alt="Cantilever" loading="lazy">
    <span class="wood-card-title">Cantilever{.span = 12000, .height = 1830}</span>
  </div>
  <div class="wood-card">
    <img src="truss_cantilever_design_1.png" alt="CantileverDesign1" loading="lazy">
    <span class="wood-card-title">CantileverDesign1{.span = 11887.2, .height = 2000}</span>
  </div>
  <div class="wood-card">
    <img src="truss_cantilever_design_2.png" alt="CantileverDesign2" loading="lazy">
    <span class="wood-card-title">CantileverDesign2{.span = 12000, .height = 2000}</span>
  </div>
  <div class="wood-card">
    <img src="truss_cantilever_design_3.png" alt="CantileverDesign3" loading="lazy">
    <span class="wood-card-title">CantileverDesign3{.span = 12000, .height = 2000}</span>
  </div>
  <div class="wood-card">
    <img src="truss_fish_belly.png" alt="FishBelly" loading="lazy">
    <span class="wood-card-title">FishBelly{.span = 12000, .rise = 1720, .sag = 1150}</span>
  </div>
  <div class="wood-card">
    <img src="truss_sawtooth.png" alt="Sawtooth" loading="lazy">
    <span class="wood-card-title">Sawtooth{.span = 12000, .height = 1800, .column = 2130, .teeth = 3}</span>
  </div>
  <div class="wood-card">
    <img src="truss_howe_flat.png" alt="HoweFlat" loading="lazy">
    <span class="wood-card-title">HoweFlat{.span = 12000, .height = 1530, .ridge = 1890, .panels = 8}</span>
  </div>
  <div class="wood-card">
    <img src="truss_three_hinged_arch.png" alt="ThreeHingedArch" loading="lazy">
    <span class="wood-card-title">ThreeHingedArch{.span = 12000, .height = 3380}</span>
  </div>
  <div class="wood-card">
    <img src="truss_burr_arch.png" alt="BurrArch" loading="lazy">
    <span class="wood-card-title">BurrArch{.span = 12000, .height = 2480, .crown = 1890, .panels = 6}</span>
  </div>
  <div class="wood-card">
    <img src="truss_town_lattice.png" alt="TownLattice" loading="lazy">
    <span class="wood-card-title">TownLattice{.span = 12000, .height = 3000, .panels = 8}</span>
  </div>
  <div class="wood-card">
    <img src="truss_bowstring_arch.png" alt="BowstringArch" loading="lazy">
    <span class="wood-card-title">BowstringArch{.span = 11900, .height = 2490, .panels = 7}</span>
  </div>
  <div class="wood-card">
    <img src="truss_waddell_a.png" alt="WaddellA" loading="lazy">
    <span class="wood-card-title">WaddellA{.span = 12000, .rise = 4560}</span>
  </div>
  <div class="wood-card">
    <img src="truss_wichert.png" alt="Wichert" loading="lazy">
    <span class="wood-card-title">Wichert{.span = 12000, .height = 2320, .dip = 650, .panels = 10}</span>
  </div>
  <div class="wood-card">
    <img src="truss_truss_leg_bedstead.png" alt="TrussLegBedstead" loading="lazy">
    <span class="wood-card-title">TrussLegBedstead{.span = 12000, .height = 2500, .leg = 900, .panels = 4}</span>
  </div>
  <div class="wood-card">
    <img src="truss_kellogg.png" alt="Kellogg" loading="lazy">
    <span class="wood-card-title">Kellogg{.span = 12000, .height = 2650, .panels = 6}</span>
  </div>
  <div class="wood-card">
    <img src="truss_baltimore_a.png" alt="BaltimoreA" loading="lazy">
    <span class="wood-card-title">BaltimoreA{.span = 14000, .height = 2140, .panels = 7}</span>
  </div>
  <div class="wood-card">
    <img src="truss_pennsylvania_a.png" alt="PennsylvaniaA" loading="lazy">
    <span class="wood-card-title">PennsylvaniaA{.span = 14000, .height = 2160, .hip = 1400, .panels = 7}</span>
  </div>
  <div class="wood-card">
    <img src="truss_greiner.png" alt="Greiner" loading="lazy">
    <span class="wood-card-title">Greiner{.span = 12000, .height = 2500, .hang = 400}</span>
  </div>
  <div class="wood-card">
    <img src="truss_double_intersection_pratt.png" alt="DoubleIntersectionPratt" loading="lazy">
    <span class="wood-card-title">DoubleIntersectionPratt{.span = 12000, .height = 2500, .panels = 12}</span>
  </div>
  <div class="wood-card">
    <img src="truss_schwedler.png" alt="Schwedler" loading="lazy">
    <span class="wood-card-title">Schwedler{.span = 14000, .height = 2960, .panels = 14, .sloped = 4}</span>
  </div>
  <div class="wood-card">
    <img src="truss_double_intersection_warren.png" alt="DoubleIntersectionWarren" loading="lazy">
    <span class="wood-card-title">DoubleIntersectionWarren{.span = 12000, .height = 2500, .panels = 8}</span>
  </div>
  <div class="wood-card">
    <img src="truss_pegram.png" alt="Pegram" loading="lazy">
    <span class="wood-card-title">Pegram{.span = 12060, .height = 2530}</span>
  </div>
  <div class="wood-card">
    <img src="truss_post.png" alt="Post" loading="lazy">
    <span class="wood-card-title">Post{.span = 12000, .height = 2500, .panels = 8}</span>
  </div>
  <div class="wood-card">
    <img src="truss_bollman.png" alt="Bollman" loading="lazy">
    <span class="wood-card-title">Bollman{.span = 12000, .height = 2500, .panels = 6}</span>
  </div>
  <div class="wood-card">
    <img src="truss_fink_bridge.png" alt="FinkBridge" loading="lazy">
    <span class="wood-card-title">FinkBridge{.span = 12000, .height = 2600, .panels = 8}</span>
  </div>
  <div class="wood-card">
    <img src="truss_stearns.png" alt="Stearns" loading="lazy">
    <span class="wood-card-title">Stearns{.span = 12000, .height = 2580}</span>
  </div>
  <div class="wood-card">
    <img src="truss_crescent_crossed.png" alt="CrescentCrossed" loading="lazy">
    <span class="wood-card-title">CrescentCrossed{.span = 12000, .height = 3900, .inner = 1800, .panels = 12}</span>
  </div>
  <div class="wood-card">
    <img src="truss_braced_arch_rib.png" alt="BracedArchRib" loading="lazy">
    <span class="wood-card-title">BracedArchRib{.span = 19426, .height = 10850}</span>
  </div>
  <div class="wood-card">
    <img src="truss_belgian_verticals.png" alt="BelgianVerticals" loading="lazy">
    <span class="wood-card-title">BelgianVerticals{.span = 12000, .rise = 3540}</span>
  </div>
  <div class="wood-card">
    <img src="truss_raised_tie_king_post.png" alt="RaisedTieKingPost" loading="lazy">
    <span class="wood-card-title">RaisedTieKingPost{.span = 12000, .rise = 3530, .tie = 1160}</span>
  </div>
  <div class="wood-card">
    <img src="truss_trapezoid_crossed.png" alt="TrapezoidCrossed" loading="lazy">
    <span class="wood-card-title">TrapezoidCrossed{.span = 12000, .height = 3020, .ridge = 4600}</span>
  </div>
  <div class="wood-card">
    <img src="truss_fink_cambered.png" alt="FinkCambered" loading="lazy">
    <span class="wood-card-title">FinkCambered{.span = 12000, .rise = 2890, .camber = 390}</span>
  </div>
  <div class="wood-card">
    <img src="truss_crescent_verticals.png" alt="CrescentVerticals" loading="lazy">
    <span class="wood-card-title">CrescentVerticals{.span = 12000, .height = 3650, .inner = 1260, .lantern = 270, .lantern_rise = 380, .panels = 10}</span>
  </div>
  <div class="wood-card">
    <img src="truss_raised_tie_posts.png" alt="RaisedTiePosts" loading="lazy">
    <span class="wood-card-title">RaisedTiePosts{.span = 12000, .rise = 4130, .column = 600}</span>
  </div>
  <div class="wood-card">
    <img src="truss_pitched_warren_columns.png" alt="PitchedWarrenColumns" loading="lazy">
    <span class="wood-card-title">PitchedWarrenColumns{.span = 12000, .height = 1170, .ridge = 2140, .column = 970, .panels = 8}</span>
  </div>
  <div class="wood-card">
    <img src="truss_fink_compound.png" alt="FinkCompound" loading="lazy">
    <span class="wood-card-title">FinkCompound{.span = 12000, .rise = 4090}</span>
  </div>
  <div class="wood-card">
    <img src="truss_fink_bent_tie.png" alt="FinkBentTie" loading="lazy">
    <span class="wood-card-title">FinkBentTie{.span = 12000, .rise = 4180, .bend = 1080}</span>
  </div>
  <div class="wood-card">
    <img src="truss_french_steep.png" alt="FrenchSteep" loading="lazy">
    <span class="wood-card-title">FrenchSteep{.span = 12000, .rise = 5930}</span>
  </div>
  <div class="wood-card">
    <img src="truss_fink_lantern.png" alt="FinkLantern" loading="lazy">
    <span class="wood-card-title">FinkLantern{.span = 12000, .rise = 3230, .lantern = 1370, .lantern_rise = 790}</span>
  </div>
  <div class="wood-card">
    <img src="truss_arch_polygonal_tie.png" alt="ArchPolygonalTie" loading="lazy">
    <span class="wood-card-title">ArchPolygonalTie{.span = 12000, .height = 3480, .tie = 980}</span>
  </div>
  <div class="wood-card">
    <img src="truss_pitched_crossed_posts.png" alt="PitchedCrossedPosts" loading="lazy">
    <span class="wood-card-title">PitchedCrossedPosts{.span = 12000, .height = 1220, .ridge = 2810, .camber = 650, .panels = 8}</span>
  </div>
  <div class="wood-card">
    <img src="truss_cambered_crossed_lantern.png" alt="CamberedCrossedLantern" loading="lazy">
    <span class="wood-card-title">CamberedCrossedLantern{.span = 12000, .height = 2150, .camber = 410, .lantern = 800, .lantern_rise = 780}</span>
  </div>
  <div class="wood-card">
    <img src="truss_arched_crossed_columns.png" alt="ArchedCrossedColumns" loading="lazy">
    <span class="wood-card-title">ArchedCrossedColumns{.span = 12000, .height = 1690, .ridge = 5250, .crown = 2820, .column = 1290, .panels = 8}</span>
  </div>
  <div class="wood-card">
    <img src="truss_trapezoid_king_crossed.png" alt="TrapezoidKingCrossed" loading="lazy">
    <span class="wood-card-title">TrapezoidKingCrossed{.span = 12000, .height = 1880, .ridge = 2610}</span>
  </div>
  <div class="wood-card">
    <img src="truss_topology_optimised.png" alt="TopologyOptimised" loading="lazy">
    <span class="wood-card-title">TopologyOptimised{.span = 12000, .height = 1800}</span>
  </div>
</div>

<!-- /library-gallery -->

## Bow 1873 {#truss_graph}

A truss is one topology: nodes, chords through them, webs and posts between them. Bow's catalogue stores exactly that, in
spans, with each truss's span, a section per member, each member's level in the hierarchy (1 primary, 2 secondary, 3
tertiary) and the chord ends that run on where chords end together; `TrussGuide(Bow{number})` draws one at its span for
the one `Truss` template, so the catalogue is data, not a function per design. The roof trusses and framed bridges of
Bow's *Economics of Construction in relation to Framed Structures* (1873) are kept this way in
`data/templates/truss_bow_1873.json` and built by `templates_truss_3_bow`.

<!-- bow-gallery -->

The 326 roof trusses and framed bridges of R. H. Bow, *Economics of Construction in relation to Framed Structures* (London: Spon, 1873), Part I, redrawn from the drawings of its classification tables. Each truss is stored as a graph in `data/templates/truss_bow_1873.json` (nodes in spans from the left support, x along the span and z up; chords as node sequences, webs and posts as node pairs) and built by the one `Truss` template: `TrussGuide(Bow{number})` draws its graph at its span with a section per member, and `Truss` makes the members and their butts and laps.

```cpp
for (const wood_truss::Bow& design : wood_truss::Bow::all()) {
    wood_truss::Truss truss(wood_truss::TrussGuide(design), "truss_" + design.name());
    truss.pb_dump(pb_path("truss_" + design.name()));
}
```

Each design was redrawn by hand from Bow's printed drawing as an exact construction: the few dimensions that define it read off the print (rise, collar height, camber, panel count), everything else constructed — equal panel points, members meeting at exact intersections, curved ribs on true circles or parabolas, a bar Bow draws through a joint kept as one member, symmetric designs mirrored exactly — and each checked over its scanned drawing. The numbers are Bow's; the page is that of his table. The book is transcribed in the BRG library (`framed-structures-bow`), where every table row links to its scan.

### Joints {#truss_bow_joints}

Every node is solved as carpentry by a hierarchy stored with the graph: each member is primary, secondary or tertiary (`chord_levels`, `web_levels`, `post_levels`), and at a node a member stops on every member of a higher level, butting on its surface. Joints are T joints wherever possible: a member that is continuous runs through, and at the ends of a truss the tie runs on past the rafter foot, the chords past the end posts, the rafters past a straining beam (`runs_on`), each cut flush with the far side of the member it carries, or square at shallow angles, and where a member carries others on both its sides its end is cut through their outer corners, so the outline of the truss runs on clean; the node where the axes meet always lies in wood. Miters are kept only for the gentle bends of one continuous member and for symmetric ridges.

### Scale and sections {#truss_bow_sizing}

Bow gives the span of only a few designs (Nos. 130, 137, 192, 195, 293, 332); his diagrams of forces are drawn to no absolute scale. Every other design is drawn at its number of sub-spans times 8 ft (2.44 m), the sub-span of his stated examples (No. 130: 86 1/4 ft over 12 sub-spans; No. 137: 97 1/2 ft over 12; No. 332: 204 ft over 24); the arched roofs whose sub-spans he calls irregular (Nos. 193, 194, 197) are taken at 100 ft. Each member's section is then sized, per truss, for timber trusses 3 m apart: roofs 1.5 kN/m2 on the roof outline (the upper chords), bridges 5 kN/m2 on the deck, both x 1.4. The forces are those of truss action, as Bow's diagrams of forces give them: the pin-jointed frame, a curved or bent chord drawn through nodes where nothing else meets it being one bar between the joints where others do; such a bar also carries the bending of the loads between its joints and of its axial force on the sagitta of its curve. A member that the carpentry lands on another's face just beside a joint, to keep it from running alongside, meets that joint in the force model, the member landed on bending under the landing force on its eccentricity. Only designs that are mechanisms when pin-jointed (Bow's deficient Class II, most of Class IV) are analysed as frames with stiff joints, carrying by bending. Sections are square GL24h, N/A + M/W within the design strength, struts with a buckling factor, at least 100 mm for chords and 60 mm for webs, one section per member. The span, the load and the largest forces are stored with each graph; the cards show the span.

### Roof trusses, Class I {#truss_bow_roof_i}

Class I: theoretically perfect frames: with every joint hinged they still cannot distort, and no part can be lengthened or shortened so as to induce stress.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_6.webp" alt="Bow No. 6" loading="lazy">
    <span class="wood-card-title">Bow{"6"} · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_7.webp" alt="Bow No. 7" loading="lazy">
    <span class="wood-card-title">Bow{"7"} · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_8.webp" alt="Bow No. 8" loading="lazy">
    <span class="wood-card-title">Bow{"8"} · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_9.webp" alt="Bow No. 9" loading="lazy">
    <span class="wood-card-title">Bow{"9"} · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_10.webp" alt="Bow No. 10" loading="lazy">
    <span class="wood-card-title">Bow{"10"} · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_11.webp" alt="Bow No. 11" loading="lazy">
    <span class="wood-card-title">Bow{"11"} · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_12.webp" alt="Bow No. 12" loading="lazy">
    <span class="wood-card-title">Bow{"12"} · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_13.webp" alt="Bow No. 13" loading="lazy">
    <span class="wood-card-title">Bow{"13"} · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_14.webp" alt="Bow No. 14" loading="lazy">
    <span class="wood-card-title">Bow{"14"} · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_15.webp" alt="Bow No. 15" loading="lazy">
    <span class="wood-card-title">Bow{"15"} · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_16.webp" alt="Bow No. 16" loading="lazy">
    <span class="wood-card-title">Bow{"16"} · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_17.webp" alt="Bow No. 17" loading="lazy">
    <span class="wood-card-title">Bow{"17"} · 7.3 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_18.webp" alt="Bow No. 18" loading="lazy">
    <span class="wood-card-title">Bow{"18"} · 7.3 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_19.webp" alt="Bow No. 19" loading="lazy">
    <span class="wood-card-title">Bow{"19"} · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_20.webp" alt="Bow No. 20" loading="lazy">
    <span class="wood-card-title">Bow{"20"} · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_21.webp" alt="Bow No. 21" loading="lazy">
    <span class="wood-card-title">Bow{"21"} · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_22.webp" alt="Bow No. 22" loading="lazy">
    <span class="wood-card-title">Bow{"22"} · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_23.webp" alt="Bow No. 23" loading="lazy">
    <span class="wood-card-title">Bow{"23"} · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_24.webp" alt="Bow No. 24" loading="lazy">
    <span class="wood-card-title">Bow{"24"} · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_25.webp" alt="Bow No. 25" loading="lazy">
    <span class="wood-card-title">Bow{"25"} · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_26.webp" alt="Bow No. 26" loading="lazy">
    <span class="wood-card-title">Bow{"26"} · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_27.webp" alt="Bow No. 27" loading="lazy">
    <span class="wood-card-title">Bow{"27"} · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_28.webp" alt="Bow No. 28" loading="lazy">
    <span class="wood-card-title">Bow{"28"} · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_29.webp" alt="Bow No. 29" loading="lazy">
    <span class="wood-card-title">Bow{"29"} · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_30.webp" alt="Bow No. 30" loading="lazy">
    <span class="wood-card-title">Bow{"30"} · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_31.webp" alt="Bow No. 31" loading="lazy">
    <span class="wood-card-title">Bow{"31"} · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_32.webp" alt="Bow No. 32" loading="lazy">
    <span class="wood-card-title">Bow{"32"} · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_33.webp" alt="Bow No. 33" loading="lazy">
    <span class="wood-card-title">Bow{"33"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_34.webp" alt="Bow No. 34" loading="lazy">
    <span class="wood-card-title">Bow{"34"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_35.webp" alt="Bow No. 35" loading="lazy">
    <span class="wood-card-title">Bow{"35"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_36.webp" alt="Bow No. 36" loading="lazy">
    <span class="wood-card-title">Bow{"36"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_37.webp" alt="Bow No. 37" loading="lazy">
    <span class="wood-card-title">Bow{"37"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_38.webp" alt="Bow No. 38" loading="lazy">
    <span class="wood-card-title">Bow{"38"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_39.webp" alt="Bow No. 39" loading="lazy">
    <span class="wood-card-title">Bow{"39"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_40.webp" alt="Bow No. 40" loading="lazy">
    <span class="wood-card-title">Bow{"40"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_41.webp" alt="Bow No. 41" loading="lazy">
    <span class="wood-card-title">Bow{"41"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_42.webp" alt="Bow No. 42" loading="lazy">
    <span class="wood-card-title">Bow{"42"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_43.webp" alt="Bow No. 43" loading="lazy">
    <span class="wood-card-title">Bow{"43"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_44.webp" alt="Bow No. 44" loading="lazy">
    <span class="wood-card-title">Bow{"44"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_45.webp" alt="Bow No. 45" loading="lazy">
    <span class="wood-card-title">Bow{"45"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_46.webp" alt="Bow No. 46" loading="lazy">
    <span class="wood-card-title">Bow{"46"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_47.webp" alt="Bow No. 47" loading="lazy">
    <span class="wood-card-title">Bow{"47"} · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_48.webp" alt="Bow No. 48" loading="lazy">
    <span class="wood-card-title">Bow{"48"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_49.webp" alt="Bow No. 49" loading="lazy">
    <span class="wood-card-title">Bow{"49"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_50.webp" alt="Bow No. 50" loading="lazy">
    <span class="wood-card-title">Bow{"50"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_51.webp" alt="Bow No. 51" loading="lazy">
    <span class="wood-card-title">Bow{"51"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_52.webp" alt="Bow No. 52" loading="lazy">
    <span class="wood-card-title">Bow{"52"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_53.webp" alt="Bow No. 53" loading="lazy">
    <span class="wood-card-title">Bow{"53"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_54.webp" alt="Bow No. 54" loading="lazy">
    <span class="wood-card-title">Bow{"54"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_55.webp" alt="Bow No. 55" loading="lazy">
    <span class="wood-card-title">Bow{"55"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_56.webp" alt="Bow No. 56" loading="lazy">
    <span class="wood-card-title">Bow{"56"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_57.webp" alt="Bow No. 57" loading="lazy">
    <span class="wood-card-title">Bow{"57"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_58.webp" alt="Bow No. 58" loading="lazy">
    <span class="wood-card-title">Bow{"58"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_59.webp" alt="Bow No. 59" loading="lazy">
    <span class="wood-card-title">Bow{"59"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_60.webp" alt="Bow No. 60" loading="lazy">
    <span class="wood-card-title">Bow{"60"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_61.webp" alt="Bow No. 61" loading="lazy">
    <span class="wood-card-title">Bow{"61"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_62.webp" alt="Bow No. 62" loading="lazy">
    <span class="wood-card-title">Bow{"62"} · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_63.webp" alt="Bow No. 63" loading="lazy">
    <span class="wood-card-title">Bow{"63"} · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_64.webp" alt="Bow No. 64" loading="lazy">
    <span class="wood-card-title">Bow{"64"} · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_65.webp" alt="Bow No. 65" loading="lazy">
    <span class="wood-card-title">Bow{"65"} · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_66.webp" alt="Bow No. 66" loading="lazy">
    <span class="wood-card-title">Bow{"66"} · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_67.webp" alt="Bow No. 67" loading="lazy">
    <span class="wood-card-title">Bow{"67"} · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_68.webp" alt="Bow No. 68" loading="lazy">
    <span class="wood-card-title">Bow{"68"} · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_69.webp" alt="Bow No. 69" loading="lazy">
    <span class="wood-card-title">Bow{"69"} · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_70.webp" alt="Bow No. 70" loading="lazy">
    <span class="wood-card-title">Bow{"70"} · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_71.webp" alt="Bow No. 71" loading="lazy">
    <span class="wood-card-title">Bow{"71"} · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_72.webp" alt="Bow No. 72" loading="lazy">
    <span class="wood-card-title">Bow{"72"} · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_73.webp" alt="Bow No. 73" loading="lazy">
    <span class="wood-card-title">Bow{"73"} · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_74.webp" alt="Bow No. 74" loading="lazy">
    <span class="wood-card-title">Bow{"74"} · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_75.webp" alt="Bow No. 75" loading="lazy">
    <span class="wood-card-title">Bow{"75"} · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_76.webp" alt="Bow No. 76" loading="lazy">
    <span class="wood-card-title">Bow{"76"} · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_77.webp" alt="Bow No. 77" loading="lazy">
    <span class="wood-card-title">Bow{"77"} · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_78.webp" alt="Bow No. 78" loading="lazy">
    <span class="wood-card-title">Bow{"78"} · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_79.webp" alt="Bow No. 79" loading="lazy">
    <span class="wood-card-title">Bow{"79"} · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_80.webp" alt="Bow No. 80" loading="lazy">
    <span class="wood-card-title">Bow{"80"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_81.webp" alt="Bow No. 81" loading="lazy">
    <span class="wood-card-title">Bow{"81"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_82.webp" alt="Bow No. 82" loading="lazy">
    <span class="wood-card-title">Bow{"82"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_83.webp" alt="Bow No. 83" loading="lazy">
    <span class="wood-card-title">Bow{"83"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_84.webp" alt="Bow No. 84" loading="lazy">
    <span class="wood-card-title">Bow{"84"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_85.webp" alt="Bow No. 85" loading="lazy">
    <span class="wood-card-title">Bow{"85"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_86.webp" alt="Bow No. 86" loading="lazy">
    <span class="wood-card-title">Bow{"86"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_87.webp" alt="Bow No. 87" loading="lazy">
    <span class="wood-card-title">Bow{"87"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_88.webp" alt="Bow No. 88" loading="lazy">
    <span class="wood-card-title">Bow{"88"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_89.webp" alt="Bow No. 89" loading="lazy">
    <span class="wood-card-title">Bow{"89"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_90.webp" alt="Bow No. 90" loading="lazy">
    <span class="wood-card-title">Bow{"90"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_91.webp" alt="Bow No. 91" loading="lazy">
    <span class="wood-card-title">Bow{"91"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_92.webp" alt="Bow No. 92" loading="lazy">
    <span class="wood-card-title">Bow{"92"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_93.webp" alt="Bow No. 93" loading="lazy">
    <span class="wood-card-title">Bow{"93"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_94.webp" alt="Bow No. 94" loading="lazy">
    <span class="wood-card-title">Bow{"94"} · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_95.webp" alt="Bow No. 95" loading="lazy">
    <span class="wood-card-title">Bow{"95"} · 14.6 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_96.webp" alt="Bow No. 96" loading="lazy">
    <span class="wood-card-title">Bow{"96"} · 14.6 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_97.webp" alt="Bow No. 97" loading="lazy">
    <span class="wood-card-title">Bow{"97"} · 14.6 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_98.webp" alt="Bow No. 98" loading="lazy">
    <span class="wood-card-title">Bow{"98"} · 14.6 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_99.webp" alt="Bow No. 99" loading="lazy">
    <span class="wood-card-title">Bow{"99"} · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_100.webp" alt="Bow No. 100" loading="lazy">
    <span class="wood-card-title">Bow{"100"} · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_101.webp" alt="Bow No. 101" loading="lazy">
    <span class="wood-card-title">Bow{"101"} · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_102.webp" alt="Bow No. 102" loading="lazy">
    <span class="wood-card-title">Bow{"102"} · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_103.webp" alt="Bow No. 103" loading="lazy">
    <span class="wood-card-title">Bow{"103"} · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_104.webp" alt="Bow No. 104" loading="lazy">
    <span class="wood-card-title">Bow{"104"} · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_105.webp" alt="Bow No. 105" loading="lazy">
    <span class="wood-card-title">Bow{"105"} · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_106.webp" alt="Bow No. 106" loading="lazy">
    <span class="wood-card-title">Bow{"106"} · 19.5 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_107.webp" alt="Bow No. 107" loading="lazy">
    <span class="wood-card-title">Bow{"107"} · 19.5 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_108.webp" alt="Bow No. 108" loading="lazy">
    <span class="wood-card-title">Bow{"108"} · 19.5 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_109.webp" alt="Bow No. 109" loading="lazy">
    <span class="wood-card-title">Bow{"109"} · 19.5 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_110.webp" alt="Bow No. 110" loading="lazy">
    <span class="wood-card-title">Bow{"110"} · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_111.webp" alt="Bow No. 111" loading="lazy">
    <span class="wood-card-title">Bow{"111"} · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_112.webp" alt="Bow No. 112" loading="lazy">
    <span class="wood-card-title">Bow{"112"} · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_113.webp" alt="Bow No. 113" loading="lazy">
    <span class="wood-card-title">Bow{"113"} · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_114.webp" alt="Bow No. 114" loading="lazy">
    <span class="wood-card-title">Bow{"114"} · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_115.webp" alt="Bow No. 115" loading="lazy">
    <span class="wood-card-title">Bow{"115"} · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_116.webp" alt="Bow No. 116" loading="lazy">
    <span class="wood-card-title">Bow{"116"} · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_117.webp" alt="Bow No. 117" loading="lazy">
    <span class="wood-card-title">Bow{"117"} · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_118.webp" alt="Bow No. 118" loading="lazy">
    <span class="wood-card-title">Bow{"118"} · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_119.webp" alt="Bow No. 119" loading="lazy">
    <span class="wood-card-title">Bow{"119"} · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_120.webp" alt="Bow No. 120" loading="lazy">
    <span class="wood-card-title">Bow{"120"} · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_121.webp" alt="Bow No. 121" loading="lazy">
    <span class="wood-card-title">Bow{"121"} · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_122.webp" alt="Bow No. 122" loading="lazy">
    <span class="wood-card-title">Bow{"122"} · 21.9 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_123.webp" alt="Bow No. 123" loading="lazy">
    <span class="wood-card-title">Bow{"123"} · 24.4 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_124.webp" alt="Bow No. 124" loading="lazy">
    <span class="wood-card-title">Bow{"124"} · 24.4 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_125.webp" alt="Bow No. 125" loading="lazy">
    <span class="wood-card-title">Bow{"125"} · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_126.webp" alt="Bow No. 126" loading="lazy">
    <span class="wood-card-title">Bow{"126"} · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_127.webp" alt="Bow No. 127" loading="lazy">
    <span class="wood-card-title">Bow{"127"} · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_128.webp" alt="Bow No. 128" loading="lazy">
    <span class="wood-card-title">Bow{"128"} · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_129.webp" alt="Bow No. 129" loading="lazy">
    <span class="wood-card-title">Bow{"129"} · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_130.webp" alt="Bow No. 130" loading="lazy">
    <span class="wood-card-title">Bow{"130"} · 26.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_131.webp" alt="Bow No. 131" loading="lazy">
    <span class="wood-card-title">Bow{"131"} · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_132.webp" alt="Bow No. 132" loading="lazy">
    <span class="wood-card-title">Bow{"132"} · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_133.webp" alt="Bow No. 133" loading="lazy">
    <span class="wood-card-title">Bow{"133"} · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_134.webp" alt="Bow No. 134" loading="lazy">
    <span class="wood-card-title">Bow{"134"} · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_135.webp" alt="Bow No. 135" loading="lazy">
    <span class="wood-card-title">Bow{"135"} · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_136.webp" alt="Bow No. 136" loading="lazy">
    <span class="wood-card-title">Bow{"136"} · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_137.webp" alt="Bow No. 137" loading="lazy">
    <span class="wood-card-title">Bow{"137"} · 29.7 m · p. 25</span>
  </div>
</div>

### Roof trusses, Class II {#truss_bow_roof_ii}

Class II: a deficiency of parts: with hinged joints they hold their shape only under equilibrating loads, and owe their resistance to distortion to stiff joints and bending of the parts.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_139.webp" alt="Bow No. 139" loading="lazy">
    <span class="wood-card-title">Bow{"139"} · 7.3 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_140.webp" alt="Bow No. 140" loading="lazy">
    <span class="wood-card-title">Bow{"140"} · 7.3 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_141.webp" alt="Bow No. 141" loading="lazy">
    <span class="wood-card-title">Bow{"141"} · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_142.webp" alt="Bow No. 142" loading="lazy">
    <span class="wood-card-title">Bow{"142"} · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_143.webp" alt="Bow No. 143" loading="lazy">
    <span class="wood-card-title">Bow{"143"} · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_144.webp" alt="Bow No. 144" loading="lazy">
    <span class="wood-card-title">Bow{"144"} · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_145.webp" alt="Bow No. 145" loading="lazy">
    <span class="wood-card-title">Bow{"145"} · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_146.webp" alt="Bow No. 146" loading="lazy">
    <span class="wood-card-title">Bow{"146"} · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_147.webp" alt="Bow No. 147" loading="lazy">
    <span class="wood-card-title">Bow{"147"} · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_148.webp" alt="Bow No. 148" loading="lazy">
    <span class="wood-card-title">Bow{"148"} · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_149.webp" alt="Bow No. 149" loading="lazy">
    <span class="wood-card-title">Bow{"149"} · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_150.webp" alt="Bow No. 150" loading="lazy">
    <span class="wood-card-title">Bow{"150"} · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_151.webp" alt="Bow No. 151" loading="lazy">
    <span class="wood-card-title">Bow{"151"} · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_152.webp" alt="Bow No. 152" loading="lazy">
    <span class="wood-card-title">Bow{"152"} · 14.6 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_153.webp" alt="Bow No. 153" loading="lazy">
    <span class="wood-card-title">Bow{"153"} · 14.6 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_154.webp" alt="Bow No. 154" loading="lazy">
    <span class="wood-card-title">Bow{"154"} · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_155.webp" alt="Bow No. 155" loading="lazy">
    <span class="wood-card-title">Bow{"155"} · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_156.webp" alt="Bow No. 156" loading="lazy">
    <span class="wood-card-title">Bow{"156"} · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_157.webp" alt="Bow No. 157" loading="lazy">
    <span class="wood-card-title">Bow{"157"} · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_158.webp" alt="Bow No. 158" loading="lazy">
    <span class="wood-card-title">Bow{"158"} · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_159.webp" alt="Bow No. 159" loading="lazy">
    <span class="wood-card-title">Bow{"159"} · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_160.webp" alt="Bow No. 160" loading="lazy">
    <span class="wood-card-title">Bow{"160"} · 17.1 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_161.webp" alt="Bow No. 161" loading="lazy">
    <span class="wood-card-title">Bow{"161"} · 19.5 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_162.webp" alt="Bow No. 162" loading="lazy">
    <span class="wood-card-title">Bow{"162"} · 19.5 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_163.webp" alt="Bow No. 163" loading="lazy">
    <span class="wood-card-title">Bow{"163"} · 24.4 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_164.webp" alt="Bow No. 164" loading="lazy">
    <span class="wood-card-title">Bow{"164"} · 29.3 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_165.webp" alt="Bow No. 165" loading="lazy">
    <span class="wood-card-title">Bow{"165"} · 29.3 m · p. 27</span>
  </div>
</div>

### Roof trusses, Class III {#truss_bow_roof_iii}

Class III: a redundancy of parts: no part can be made longer or shorter without inducing stress in others, so the stresses depend on the fitting, elasticity and temperature.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_166.webp" alt="Bow No. 166" loading="lazy">
    <span class="wood-card-title">Bow{"166"} · 4.9 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_167.webp" alt="Bow No. 167" loading="lazy">
    <span class="wood-card-title">Bow{"167"} · 7.3 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_168.webp" alt="Bow No. 168" loading="lazy">
    <span class="wood-card-title">Bow{"168"} · 7.3 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_169.webp" alt="Bow No. 169" loading="lazy">
    <span class="wood-card-title">Bow{"169"} · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_170.webp" alt="Bow No. 170" loading="lazy">
    <span class="wood-card-title">Bow{"170"} · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_171.webp" alt="Bow No. 171" loading="lazy">
    <span class="wood-card-title">Bow{"171"} · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_172.webp" alt="Bow No. 172" loading="lazy">
    <span class="wood-card-title">Bow{"172"} · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_173.webp" alt="Bow No. 173" loading="lazy">
    <span class="wood-card-title">Bow{"173"} · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_174.webp" alt="Bow No. 174" loading="lazy">
    <span class="wood-card-title">Bow{"174"} · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_175.webp" alt="Bow No. 175" loading="lazy">
    <span class="wood-card-title">Bow{"175"} · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_176.webp" alt="Bow No. 176" loading="lazy">
    <span class="wood-card-title">Bow{"176"} · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_177.webp" alt="Bow No. 177" loading="lazy">
    <span class="wood-card-title">Bow{"177"} · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_178.webp" alt="Bow No. 178" loading="lazy">
    <span class="wood-card-title">Bow{"178"} · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_179.webp" alt="Bow No. 179" loading="lazy">
    <span class="wood-card-title">Bow{"179"} · 12.2 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_180.webp" alt="Bow No. 180" loading="lazy">
    <span class="wood-card-title">Bow{"180"} · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_181.webp" alt="Bow No. 181" loading="lazy">
    <span class="wood-card-title">Bow{"181"} · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_182.webp" alt="Bow No. 182" loading="lazy">
    <span class="wood-card-title">Bow{"182"} · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_183.webp" alt="Bow No. 183" loading="lazy">
    <span class="wood-card-title">Bow{"183"} · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_184.webp" alt="Bow No. 184" loading="lazy">
    <span class="wood-card-title">Bow{"184"} · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_185.webp" alt="Bow No. 185" loading="lazy">
    <span class="wood-card-title">Bow{"185"} · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_186.webp" alt="Bow No. 186" loading="lazy">
    <span class="wood-card-title">Bow{"186"} · 14.6 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_187.webp" alt="Bow No. 187" loading="lazy">
    <span class="wood-card-title">Bow{"187"} · 14.6 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_188.webp" alt="Bow No. 188" loading="lazy">
    <span class="wood-card-title">Bow{"188"} · 19.5 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_189.webp" alt="Bow No. 189" loading="lazy">
    <span class="wood-card-title">Bow{"189"} · 19.5 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_190.webp" alt="Bow No. 190" loading="lazy">
    <span class="wood-card-title">Bow{"190"} · 19.5 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_191.webp" alt="Bow No. 191" loading="lazy">
    <span class="wood-card-title">Bow{"191"} · 21.9 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_192.webp" alt="Bow No. 192" loading="lazy">
    <span class="wood-card-title">Bow{"192"} · 40.5 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_193.webp" alt="Bow No. 193" loading="lazy">
    <span class="wood-card-title">Bow{"193"} · 30.5 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_194.webp" alt="Bow No. 194" loading="lazy">
    <span class="wood-card-title">Bow{"194"} · 30.5 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_195.webp" alt="Bow No. 195" loading="lazy">
    <span class="wood-card-title">Bow{"195"} · 64.3 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_197.webp" alt="Bow No. 197" loading="lazy">
    <span class="wood-card-title">Bow{"197"} · 30.5 m · p. 30</span>
  </div>
</div>

### Roof trusses, Class IV {#truss_bow_roof_iv}

Class IV: the defects of both Classes II and III: redundant in one region, deficient in another.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_198.webp" alt="Bow No. 198" loading="lazy">
    <span class="wood-card-title">Bow{"198"} · 7.3 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_199.webp" alt="Bow No. 199" loading="lazy">
    <span class="wood-card-title">Bow{"199"} · 12.2 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_200.webp" alt="Bow No. 200" loading="lazy">
    <span class="wood-card-title">Bow{"200"} · 12.2 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_200_half.webp" alt="Bow No. 200½" loading="lazy">
    <span class="wood-card-title">Bow{"200½"} · 14.6 m · p. 30</span>
  </div>
</div>

### Framed bridges, Class I {#truss_bow_bridge_i}

Class I: theoretically perfect frames: with every joint hinged they still cannot distort, and no part can be lengthened or shortened so as to induce stress.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_201iii.webp" alt="Bow No. 201iii" loading="lazy">
    <span class="wood-card-title">Bow{"201iii"} · 4.9 m · p. 31</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_202a.webp" alt="Bow No. 202A" loading="lazy">
    <span class="wood-card-title">Bow{"202A"} · 4.9 m · p. 31</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_202c.webp" alt="Bow No. 202C" loading="lazy">
    <span class="wood-card-title">Bow{"202C"} · 4.9 m · p. 31</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_203.webp" alt="Bow No. 203" loading="lazy">
    <span class="wood-card-title">Bow{"203"} · 4.9 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_204.webp" alt="Bow No. 204" loading="lazy">
    <span class="wood-card-title">Bow{"204"} · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_205.webp" alt="Bow No. 205" loading="lazy">
    <span class="wood-card-title">Bow{"205"} · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_206.webp" alt="Bow No. 206" loading="lazy">
    <span class="wood-card-title">Bow{"206"} · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_207.webp" alt="Bow No. 207" loading="lazy">
    <span class="wood-card-title">Bow{"207"} · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_208.webp" alt="Bow No. 208" loading="lazy">
    <span class="wood-card-title">Bow{"208"} · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_209.webp" alt="Bow No. 209" loading="lazy">
    <span class="wood-card-title">Bow{"209"} · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_210.webp" alt="Bow No. 210" loading="lazy">
    <span class="wood-card-title">Bow{"210"} · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_211.webp" alt="Bow No. 211" loading="lazy">
    <span class="wood-card-title">Bow{"211"} · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_212.webp" alt="Bow No. 212" loading="lazy">
    <span class="wood-card-title">Bow{"212"} · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_213.webp" alt="Bow No. 213" loading="lazy">
    <span class="wood-card-title">Bow{"213"} · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_214.webp" alt="Bow No. 214" loading="lazy">
    <span class="wood-card-title">Bow{"214"} · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_215.webp" alt="Bow No. 215" loading="lazy">
    <span class="wood-card-title">Bow{"215"} · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_216.webp" alt="Bow No. 216" loading="lazy">
    <span class="wood-card-title">Bow{"216"} · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_217.webp" alt="Bow No. 217" loading="lazy">
    <span class="wood-card-title">Bow{"217"} · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_218.webp" alt="Bow No. 218" loading="lazy">
    <span class="wood-card-title">Bow{"218"} · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_219.webp" alt="Bow No. 219" loading="lazy">
    <span class="wood-card-title">Bow{"219"} · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_220.webp" alt="Bow No. 220" loading="lazy">
    <span class="wood-card-title">Bow{"220"} · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_221.webp" alt="Bow No. 221" loading="lazy">
    <span class="wood-card-title">Bow{"221"} · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_222.webp" alt="Bow No. 222" loading="lazy">
    <span class="wood-card-title">Bow{"222"} · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_223.webp" alt="Bow No. 223" loading="lazy">
    <span class="wood-card-title">Bow{"223"} · 12.2 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_224.webp" alt="Bow No. 224" loading="lazy">
    <span class="wood-card-title">Bow{"224"} · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_225.webp" alt="Bow No. 225" loading="lazy">
    <span class="wood-card-title">Bow{"225"} · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_226.webp" alt="Bow No. 226" loading="lazy">
    <span class="wood-card-title">Bow{"226"} · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_227.webp" alt="Bow No. 227" loading="lazy">
    <span class="wood-card-title">Bow{"227"} · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_228.webp" alt="Bow No. 228" loading="lazy">
    <span class="wood-card-title">Bow{"228"} · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_229.webp" alt="Bow No. 229" loading="lazy">
    <span class="wood-card-title">Bow{"229"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_230.webp" alt="Bow No. 230" loading="lazy">
    <span class="wood-card-title">Bow{"230"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_231.webp" alt="Bow No. 231" loading="lazy">
    <span class="wood-card-title">Bow{"231"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_232.webp" alt="Bow No. 232" loading="lazy">
    <span class="wood-card-title">Bow{"232"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_233.webp" alt="Bow No. 233" loading="lazy">
    <span class="wood-card-title">Bow{"233"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_234.webp" alt="Bow No. 234" loading="lazy">
    <span class="wood-card-title">Bow{"234"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_235.webp" alt="Bow No. 235" loading="lazy">
    <span class="wood-card-title">Bow{"235"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_236.webp" alt="Bow No. 236" loading="lazy">
    <span class="wood-card-title">Bow{"236"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_237.webp" alt="Bow No. 237" loading="lazy">
    <span class="wood-card-title">Bow{"237"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_238.webp" alt="Bow No. 238" loading="lazy">
    <span class="wood-card-title">Bow{"238"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_239.webp" alt="Bow No. 239" loading="lazy">
    <span class="wood-card-title">Bow{"239"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_240.webp" alt="Bow No. 240" loading="lazy">
    <span class="wood-card-title">Bow{"240"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_241.webp" alt="Bow No. 241" loading="lazy">
    <span class="wood-card-title">Bow{"241"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_242.webp" alt="Bow No. 242" loading="lazy">
    <span class="wood-card-title">Bow{"242"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_243.webp" alt="Bow No. 243" loading="lazy">
    <span class="wood-card-title">Bow{"243"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_244.webp" alt="Bow No. 244" loading="lazy">
    <span class="wood-card-title">Bow{"244"} · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_245.webp" alt="Bow No. 245" loading="lazy">
    <span class="wood-card-title">Bow{"245"} · 17.1 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_247.webp" alt="Bow No. 247" loading="lazy">
    <span class="wood-card-title">Bow{"247"} · 17.1 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_248.webp" alt="Bow No. 248" loading="lazy">
    <span class="wood-card-title">Bow{"248"} · 17.1 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_249.webp" alt="Bow No. 249" loading="lazy">
    <span class="wood-card-title">Bow{"249"} · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_250.webp" alt="Bow No. 250" loading="lazy">
    <span class="wood-card-title">Bow{"250"} · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_251.webp" alt="Bow No. 251" loading="lazy">
    <span class="wood-card-title">Bow{"251"} · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_252.webp" alt="Bow No. 252" loading="lazy">
    <span class="wood-card-title">Bow{"252"} · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_253.webp" alt="Bow No. 253" loading="lazy">
    <span class="wood-card-title">Bow{"253"} · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_255.webp" alt="Bow No. 255" loading="lazy">
    <span class="wood-card-title">Bow{"255"} · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_256.webp" alt="Bow No. 256" loading="lazy">
    <span class="wood-card-title">Bow{"256"} · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_257.webp" alt="Bow No. 257" loading="lazy">
    <span class="wood-card-title">Bow{"257"} · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_258.webp" alt="Bow No. 258" loading="lazy">
    <span class="wood-card-title">Bow{"258"} · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_259.webp" alt="Bow No. 259" loading="lazy">
    <span class="wood-card-title">Bow{"259"} · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_260.webp" alt="Bow No. 260" loading="lazy">
    <span class="wood-card-title">Bow{"260"} · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_261.webp" alt="Bow No. 261" loading="lazy">
    <span class="wood-card-title">Bow{"261"} · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_262.webp" alt="Bow No. 262" loading="lazy">
    <span class="wood-card-title">Bow{"262"} · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_263.webp" alt="Bow No. 263" loading="lazy">
    <span class="wood-card-title">Bow{"263"} · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_264.webp" alt="Bow No. 264" loading="lazy">
    <span class="wood-card-title">Bow{"264"} · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_265.webp" alt="Bow No. 265" loading="lazy">
    <span class="wood-card-title">Bow{"265"} · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_266.webp" alt="Bow No. 266" loading="lazy">
    <span class="wood-card-title">Bow{"266"} · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_267.webp" alt="Bow No. 267" loading="lazy">
    <span class="wood-card-title">Bow{"267"} · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_268.webp" alt="Bow No. 268" loading="lazy">
    <span class="wood-card-title">Bow{"268"} · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_269.webp" alt="Bow No. 269" loading="lazy">
    <span class="wood-card-title">Bow{"269"} · 24.4 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_270.webp" alt="Bow No. 270" loading="lazy">
    <span class="wood-card-title">Bow{"270"} · 24.4 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_271.webp" alt="Bow No. 271" loading="lazy">
    <span class="wood-card-title">Bow{"271"} · 24.4 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_273.webp" alt="Bow No. 273" loading="lazy">
    <span class="wood-card-title">Bow{"273"} · 24.4 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_274.webp" alt="Bow No. 274" loading="lazy">
    <span class="wood-card-title">Bow{"274"} · 29.3 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_275.webp" alt="Bow No. 275" loading="lazy">
    <span class="wood-card-title">Bow{"275"} · 29.3 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_276.webp" alt="Bow No. 276" loading="lazy">
    <span class="wood-card-title">Bow{"276"} · 29.3 m · p. 35</span>
  </div>
</div>

### Framed bridges, Class II {#truss_bow_bridge_ii}

Class II: a deficiency of parts: with hinged joints they hold their shape only under equilibrating loads, and owe their resistance to distortion to stiff joints and bending of the parts.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_277.webp" alt="Bow No. 277" loading="lazy">
    <span class="wood-card-title">Bow{"277"} · 4.9 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_279.webp" alt="Bow No. 279" loading="lazy">
    <span class="wood-card-title">Bow{"279"} · 7.3 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_280.webp" alt="Bow No. 280" loading="lazy">
    <span class="wood-card-title">Bow{"280"} · 9.8 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_281.webp" alt="Bow No. 281" loading="lazy">
    <span class="wood-card-title">Bow{"281"} · 12.2 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_282.webp" alt="Bow No. 282" loading="lazy">
    <span class="wood-card-title">Bow{"282"} · 12.2 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_283.webp" alt="Bow No. 283" loading="lazy">
    <span class="wood-card-title">Bow{"283"} · 14.6 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_284.webp" alt="Bow No. 284" loading="lazy">
    <span class="wood-card-title">Bow{"284"} · 17.1 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_285.webp" alt="Bow No. 285" loading="lazy">
    <span class="wood-card-title">Bow{"285"} · 24.4 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_286.webp" alt="Bow No. 286" loading="lazy">
    <span class="wood-card-title">Bow{"286"} · 24.4 m · p. 36</span>
  </div>
</div>

### Framed bridges, Class III {#truss_bow_bridge_iii}

Class III: a redundancy of parts: no part can be made longer or shorter without inducing stress in others, so the stresses depend on the fitting, elasticity and temperature.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_288.webp" alt="Bow No. 288" loading="lazy">
    <span class="wood-card-title">Bow{"288"} · 4.9 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_289.webp" alt="Bow No. 289" loading="lazy">
    <span class="wood-card-title">Bow{"289"} · 4.9 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_290.webp" alt="Bow No. 290" loading="lazy">
    <span class="wood-card-title">Bow{"290"} · 4.9 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_291.webp" alt="Bow No. 291" loading="lazy">
    <span class="wood-card-title">Bow{"291"} · 7.3 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_292.webp" alt="Bow No. 292" loading="lazy">
    <span class="wood-card-title">Bow{"292"} · 7.3 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_293.webp" alt="Bow No. 293" loading="lazy">
    <span class="wood-card-title">Bow{"293"} · 90.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_294.webp" alt="Bow No. 294" loading="lazy">
    <span class="wood-card-title">Bow{"294"} · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_295.webp" alt="Bow No. 295" loading="lazy">
    <span class="wood-card-title">Bow{"295"} · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_296.webp" alt="Bow No. 296" loading="lazy">
    <span class="wood-card-title">Bow{"296"} · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_297.webp" alt="Bow No. 297" loading="lazy">
    <span class="wood-card-title">Bow{"297"} · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_298.webp" alt="Bow No. 298" loading="lazy">
    <span class="wood-card-title">Bow{"298"} · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_299.webp" alt="Bow No. 299" loading="lazy">
    <span class="wood-card-title">Bow{"299"} · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_300.webp" alt="Bow No. 300" loading="lazy">
    <span class="wood-card-title">Bow{"300"} · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_301.webp" alt="Bow No. 301" loading="lazy">
    <span class="wood-card-title">Bow{"301"} · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_302.webp" alt="Bow No. 302" loading="lazy">
    <span class="wood-card-title">Bow{"302"} · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_303.webp" alt="Bow No. 303" loading="lazy">
    <span class="wood-card-title">Bow{"303"} · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_304.webp" alt="Bow No. 304" loading="lazy">
    <span class="wood-card-title">Bow{"304"} · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_305.webp" alt="Bow No. 305" loading="lazy">
    <span class="wood-card-title">Bow{"305"} · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_306.webp" alt="Bow No. 306" loading="lazy">
    <span class="wood-card-title">Bow{"306"} · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_307.webp" alt="Bow No. 307" loading="lazy">
    <span class="wood-card-title">Bow{"307"} · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_308.webp" alt="Bow No. 308" loading="lazy">
    <span class="wood-card-title">Bow{"308"} · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_309.webp" alt="Bow No. 309" loading="lazy">
    <span class="wood-card-title">Bow{"309"} · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_310.webp" alt="Bow No. 310" loading="lazy">
    <span class="wood-card-title">Bow{"310"} · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_311.webp" alt="Bow No. 311" loading="lazy">
    <span class="wood-card-title">Bow{"311"} · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_312.webp" alt="Bow No. 312" loading="lazy">
    <span class="wood-card-title">Bow{"312"} · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_313.webp" alt="Bow No. 313" loading="lazy">
    <span class="wood-card-title">Bow{"313"} · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_314.webp" alt="Bow No. 314" loading="lazy">
    <span class="wood-card-title">Bow{"314"} · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_315.webp" alt="Bow No. 315" loading="lazy">
    <span class="wood-card-title">Bow{"315"} · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_316.webp" alt="Bow No. 316" loading="lazy">
    <span class="wood-card-title">Bow{"316"} · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_317.webp" alt="Bow No. 317" loading="lazy">
    <span class="wood-card-title">Bow{"317"} · 17.1 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_318.webp" alt="Bow No. 318" loading="lazy">
    <span class="wood-card-title">Bow{"318"} · 17.1 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_319.webp" alt="Bow No. 319" loading="lazy">
    <span class="wood-card-title">Bow{"319"} · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_320.webp" alt="Bow No. 320" loading="lazy">
    <span class="wood-card-title">Bow{"320"} · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_321.webp" alt="Bow No. 321" loading="lazy">
    <span class="wood-card-title">Bow{"321"} · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_322.webp" alt="Bow No. 322" loading="lazy">
    <span class="wood-card-title">Bow{"322"} · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_323.webp" alt="Bow No. 323" loading="lazy">
    <span class="wood-card-title">Bow{"323"} · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_324.webp" alt="Bow No. 324" loading="lazy">
    <span class="wood-card-title">Bow{"324"} · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_325.webp" alt="Bow No. 325" loading="lazy">
    <span class="wood-card-title">Bow{"325"} · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_326.webp" alt="Bow No. 326" loading="lazy">
    <span class="wood-card-title">Bow{"326"} · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_327.webp" alt="Bow No. 327" loading="lazy">
    <span class="wood-card-title">Bow{"327"} · 21.9 m · p. 39</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_329.webp" alt="Bow No. 329" loading="lazy">
    <span class="wood-card-title">Bow{"329"} · 29.3 m · p. 39</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_330.webp" alt="Bow No. 330" loading="lazy">
    <span class="wood-card-title">Bow{"330"} · 29.3 m · p. 39</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_331.webp" alt="Bow No. 331" loading="lazy">
    <span class="wood-card-title">Bow{"331"} · 39.0 m · p. 39</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_332.webp" alt="Bow No. 332" loading="lazy">
    <span class="wood-card-title">Bow{"332"} · 62.2 m · p. 39</span>
  </div>
</div>

### Framed bridges, Class IV {#truss_bow_bridge_iv}

Class IV: the defects of both Classes II and III: redundant in one region, deficient in another.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_333.webp" alt="Bow No. 333" loading="lazy">
    <span class="wood-card-title">Bow{"333"} · 14.6 m · p. 40</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_334.webp" alt="Bow No. 334" loading="lazy">
    <span class="wood-card-title">Bow{"334"} · 17.1 m · p. 40</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_335.webp" alt="Bow No. 335" loading="lazy">
    <span class="wood-card-title">Bow{"335"} · 21.9 m · p. 40</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_336.webp" alt="Bow No. 336" loading="lazy">
    <span class="wood-card-title">Bow{"336"} · 21.9 m · p. 40</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_337.webp" alt="Bow No. 337" loading="lazy">
    <span class="wood-card-title">Bow{"337"} · 21.9 m · p. 40</span>
  </div>
</div>

<!-- /bow-gallery -->

## Library Runner

```cpp
#include "wood_session.h"
#include "src/templates/truss/truss.h"

using namespace session_cpp;
using namespace wood_session;

/// Every design of the TrussGuide library, its overload with the default parameters, built by the Truss template, each
/// its own scene truss_<name>.
int main() {

    const std::vector<std::pair<std::string, wood_truss::TrussGuide>> guides = {
        {"pratt", wood_truss::Pratt{}},
        {"king_post", wood_truss::KingPost{}},
        {"queen_post", wood_truss::QueenPost{}},
        {"howe", wood_truss::Howe{}},
        {"fink", wood_truss::Fink{}},
        {"belgian", wood_truss::Belgian{}},
        {"scissors", wood_truss::Scissors{}},
        {"warren", wood_truss::Warren{}},
        {"warren_verticals", wood_truss::WarrenVerticals{}},
        {"pratt_bridge", wood_truss::PrattBridge{}},
        {"parker", wood_truss::Parker{}},
        {"k_truss", wood_truss::KTruss{}},
        {"cambered_verticals", wood_truss::CamberedVerticals{}},
        {"funicular_arch", wood_truss::FunicularArch{}},
        {"lens", wood_truss::Lens{}},
        {"triangulated_arch", wood_truss::TriangulatedArch{}},
        {"sloped", wood_truss::Sloped{}},
        {"pratt_flat", wood_truss::PrattFlat{}},
        {"flat_peaked", wood_truss::FlatPeaked{}},
        {"bowstring", wood_truss::Bowstring{}},
        {"pitched_posts", wood_truss::PitchedPosts{}},
        {"roof_cambered_tie", wood_truss::RoofCamberedTie{}},
        {"quarter_fan", wood_truss::QuarterFan{}},
        {"cantilever", wood_truss::Cantilever{}},
        {"cantilever_design_1", wood_truss::CantileverDesign1{}},
        {"cantilever_design_2", wood_truss::CantileverDesign2{}},
        {"cantilever_design_3", wood_truss::CantileverDesign3{}},
        {"fish_belly", wood_truss::FishBelly{}},
        {"sawtooth", wood_truss::Sawtooth{}},
        {"howe_flat", wood_truss::HoweFlat{}},
        {"three_hinged_arch", wood_truss::ThreeHingedArch{}},
        {"burr_arch", wood_truss::BurrArch{}},
        {"town_lattice", wood_truss::TownLattice{}},
        {"bowstring_arch", wood_truss::BowstringArch{}},
        {"waddell_a", wood_truss::WaddellA{}},
        {"wichert", wood_truss::Wichert{}},
        {"truss_leg_bedstead", wood_truss::TrussLegBedstead{}},
        {"kellogg", wood_truss::Kellogg{}},
        {"baltimore_a", wood_truss::BaltimoreA{}},
        {"pennsylvania_a", wood_truss::PennsylvaniaA{}},
        {"greiner", wood_truss::Greiner{}},
        {"double_intersection_pratt", wood_truss::DoubleIntersectionPratt{}},
        {"schwedler", wood_truss::Schwedler{}},
        {"double_intersection_warren", wood_truss::DoubleIntersectionWarren{}},
        {"pegram", wood_truss::Pegram{}},
        {"post", wood_truss::Post{}},
        {"bollman", wood_truss::Bollman{}},
        {"fink_bridge", wood_truss::FinkBridge{}},
        {"stearns", wood_truss::Stearns{}},
        {"crescent_crossed", wood_truss::CrescentCrossed{}},
        {"braced_arch_rib", wood_truss::BracedArchRib{}},
        {"belgian_verticals", wood_truss::BelgianVerticals{}},
        {"raised_tie_king_post", wood_truss::RaisedTieKingPost{}},
        {"trapezoid_crossed", wood_truss::TrapezoidCrossed{}},
        {"fink_cambered", wood_truss::FinkCambered{}},
        {"crescent_verticals", wood_truss::CrescentVerticals{}},
        {"raised_tie_posts", wood_truss::RaisedTiePosts{}},
        {"pitched_warren_columns", wood_truss::PitchedWarrenColumns{}},
        {"fink_compound", wood_truss::FinkCompound{}},
        {"fink_bent_tie", wood_truss::FinkBentTie{}},
        {"french_steep", wood_truss::FrenchSteep{}},
        {"fink_lantern", wood_truss::FinkLantern{}},
        {"arch_polygonal_tie", wood_truss::ArchPolygonalTie{}},
        {"pitched_crossed_posts", wood_truss::PitchedCrossedPosts{}},
        {"cambered_crossed_lantern", wood_truss::CamberedCrossedLantern{}},
        {"arched_crossed_columns", wood_truss::ArchedCrossedColumns{}},
        {"trapezoid_king_crossed", wood_truss::TrapezoidKingCrossed{}},
        {"topology_optimised", wood_truss::TopologyOptimised{}},
    };
    for (const std::pair<std::string, wood_truss::TrussGuide>& guide : guides) {
        wood_truss::Truss truss(guide.second, "truss_" + guide.first);
        truss.pb_dump(pb_path("truss_" + guide.first));
    }
    return 0;
}
```
