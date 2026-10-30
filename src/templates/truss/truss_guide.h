#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// TrussMember
// ═══════════════════════════════════════════════════════════════════════════

/// One member of a truss: its axis through its nodes, its half section and its place in the hierarchy.
struct TrussMember {
    Polyline axis; // Through its nodes, end to end: a chord turns at its kinks, a web or post is straight between two.
    double radius = 80.0; // mm, half its square section.
    int level = 0; // 1 primary, 2 secondary, 3 tertiary: it stops on every higher member at its node; 0 read from the drawing.
    std::array<bool, 2> runs_on = {false, false}; // A chord's start and end running on where chord ends meet.

    /// The first node of the axis.
    Point start() const;

    /// The last node of the axis.
    Point end() const;
};

// ═══════════════════════════════════════════════════════════════════════════
// Designs
// ═══════════════════════════════════════════════════════════════════════════

// A design of the library is its parameters, a TrussGuide overload each: lengths in mm, chord_radius the half
// section of the chords and posts, web_radius that of the webs.

/// Pratt roof truss.
struct Pratt {
    double span = 12000.0;
    double rise = 3000.0;
    int panels = 6;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// King post roof truss.
struct KingPost {
    double span = 8000.0;
    double rise = 2667.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Queen post truss.
struct QueenPost {
    double span = 9000.0;
    double height = 2000.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Howe roof truss.
struct Howe {
    double span = 12000.0;
    double rise = 3000.0;
    int panels = 6;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Fink roof truss.
struct Fink {
    double span = 10000.0;
    double rise = 2500.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Belgian roof truss.
struct Belgian {
    double span = 10000.0;
    double rise = 2500.0;
    int divisions = 3;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Scissors roof truss.
struct Scissors {
    double span = 8000.0;
    double rise = 3000.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Warren bridge truss: parallel chords `height` apart over `span`, `panels` panels of diagonals alternating up and down.
struct Warren {
    double span = 12000.0;
    double height = 1500.0;
    int panels = 8;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Warren bridge truss with a vertical at every top node and every inner bottom node; `span`, `height` and `panels` as the Warren.
struct WarrenVerticals {
    double span = 12000.0;
    double height = 1500.0;
    int panels = 8;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Pratt bridge truss.
struct PrattBridge {
    double span = 12000.0;
    double height = 2000.0;
    int panels = 6;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Parker bridge truss.
struct Parker {
    double span = 12000.0;
    double height = 2400.0;
    double hip = 1600.0;
    int panels = 6;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// K bridge truss.
struct KTruss {
    double span = 12000.0;
    double height = 2400.0;
    int panels = 8;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Cambered verticals.
struct CamberedVerticals {
    double span = 12000.0;
    double height = 1980.0;
    double camber = 800.0;
    int panels = 6;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Funicular arch.
struct FunicularArch {
    double span = 12000.0;
    double height = 2400.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Lens.
struct Lens {
    double span = 12000.0;
    double height = 1140.0;
    double sag = 1260.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Triangulated arch.
struct TriangulatedArch {
    double span = 12000.0;
    double height = 5980.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Sloped.
struct Sloped {
    double span = 12040.0;
    double height = 6020.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Flat Pratt.
struct PrattFlat {
    double span = 12000.0;
    double height = 1990.0;
    int panels = 6;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Flat peaked.
struct FlatPeaked {
    double span = 12000.0;
    double height = 1990.0;
    double peak = 3180.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Bowstring.
struct Bowstring {
    double span = 12000.0;
    double height = 1990.0;
    double crown = 3320.0;
    int panels = 6;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Pitched on end posts.
struct PitchedPosts {
    double span = 12000.0;
    double height = 2000.0;
    double ridge = 3320.0;
    int panels = 6;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Cambered tie roof truss.
struct RoofCamberedTie {
    double span = 12000.0;
    double rise = 2000.0;
    double camber = 500.0;
    double dip = 60.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Quarter fan.
struct QuarterFan {
    double span = 12000.0;
    double height = 5560.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Cantilever.
struct Cantilever {
    double span = 12000.0;
    double height = 1830.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Cantilever design 1.
struct CantileverDesign1 {
    double span = 11887.2;
    double height = 2000.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Cantilever design 2.
struct CantileverDesign2 {
    double span = 12000.0;
    double height = 2000.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Cantilever design 3.
struct CantileverDesign3 {
    double span = 12000.0;
    double height = 2000.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Fish-belly truss.
struct FishBelly {
    double span = 12000.0;
    double rise = 1720.0;
    double sag = 1150.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Sawtooth roof.
struct Sawtooth {
    double span = 12000.0;
    double height = 1800.0;
    double column = 2130.0;
    int teeth = 3;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Parallel-chord Howe roof truss.
struct HoweFlat {
    double span = 12000.0;
    double height = 1530.0;
    double ridge = 1890.0;
    int panels = 8;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Three-hinged arch.
struct ThreeHingedArch {
    double span = 12000.0;
    double height = 3380.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Burr arch truss.
struct BurrArch {
    double span = 12000.0;
    double height = 2480.0;
    double crown = 1890.0;
    int panels = 6;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Town lattice.
struct TownLattice {
    double span = 12000.0;
    double height = 3000.0;
    int panels = 8;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Bowstring arch truss.
struct BowstringArch {
    double span = 11900.0;
    double height = 2490.0;
    int panels = 7;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Waddell "A" truss.
struct WaddellA {
    double span = 12000.0;
    double rise = 4560.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Wichert truss.
struct Wichert {
    double span = 12000.0;
    double height = 2320.0;
    double dip = 650.0;
    int panels = 10;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Truss leg bedstead.
struct TrussLegBedstead {
    double span = 12000.0;
    double height = 2500.0;
    double leg = 900.0;
    int panels = 4;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Kellogg.
struct Kellogg {
    double span = 12000.0;
    double height = 2650.0;
    int panels = 6;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Baltimore with sub-struts.
struct BaltimoreA {
    double span = 14000.0;
    double height = 2140.0;
    int panels = 7;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Pennsylvania with sub-struts.
struct PennsylvaniaA {
    double span = 14000.0;
    double height = 2160.0;
    double hip = 1400.0;
    int panels = 7;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Greiner.
struct Greiner {
    double span = 12000.0;
    double height = 2500.0;
    double hang = 400.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Double intersection Pratt: a Pratt `height` high over `span` on inclined end posts, `panels` panels (even), its diagonals across two panels.
struct DoubleIntersectionPratt {
    double span = 12000.0;
    double height = 2500.0;
    int panels = 12;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Schwedler.
struct Schwedler {
    double span = 14000.0;
    double height = 2960.0;
    int panels = 14;
    int sloped = 4;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Double intersection Warren: two Warren web systems superimposed between hip verticals, `height` high over `span` on inclined end posts, `panels` panels.
struct DoubleIntersectionWarren {
    double span = 12000.0;
    double height = 2500.0;
    int panels = 8;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Pegram.
struct Pegram {
    double span = 12060.0;
    double height = 2530.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Post.
struct Post {
    double span = 12000.0;
    double height = 2500.0;
    int panels = 8;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Bollman.
struct Bollman {
    double span = 12000.0;
    double height = 2500.0;
    int panels = 6;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Fink bridge.
struct FinkBridge {
    double span = 12000.0;
    double height = 2600.0;
    int panels = 8;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Stearns: a six-panel Fink `height` high over `span` on inclined end posts with the verticals omitted at alternate panel points.
struct Stearns {
    double span = 12000.0;
    double height = 2580.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Crescent with crossed webs.
struct CrescentCrossed {
    double span = 12000.0;
    double height = 3900.0;
    double inner = 1800.0;
    int panels = 12;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Braced arch rib.
struct BracedArchRib {
    double span = 19426.0;
    double height = 10850.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Belgian with verticals (roof catalogue B).
struct BelgianVerticals {
    double span = 12000.0;
    double rise = 3540.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Raised tie with king post (roof catalogue C).
struct RaisedTieKingPost {
    double span = 12000.0;
    double rise = 3530.0;
    double tie = 1160.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Trapezoid with crossed diagonals (roof catalogue D).
struct TrapezoidCrossed {
    double span = 12000.0;
    double height = 3020.0;
    double ridge = 4600.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Fink on a cambered bottom chord (roof catalogue E).
struct FinkCambered {
    double span = 12000.0;
    double rise = 2890.0;
    double camber = 390.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Crescent with verticals and a lantern (roof catalogue F).
struct CrescentVerticals {
    double span = 12000.0;
    double height = 3650.0;
    double inner = 1260.0;
    double lantern = 270.0;
    double lantern_rise = 380.0;
    int panels = 10;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Raised tie on posts (roof catalogue G).
struct RaisedTiePosts {
    double span = 12000.0;
    double rise = 4130.0;
    double column = 600.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Pitched Warren on columns (roof catalogue H).
struct PitchedWarrenColumns {
    double span = 12000.0;
    double height = 1170.0;
    double ridge = 2140.0;
    double column = 970.0;
    int panels = 8;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Compound Fink (roof catalogue I).
struct FinkCompound {
    double span = 12000.0;
    double rise = 4090.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Fink with a bent tie (roof catalogue J).
struct FinkBentTie {
    double span = 12000.0;
    double rise = 4180.0;
    double bend = 1080.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Steep French truss (roof catalogue K).
struct FrenchSteep {
    double span = 12000.0;
    double rise = 5930.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Compound Fink with king post and lantern (roof catalogue L).
struct FinkLantern {
    double span = 12000.0;
    double rise = 3230.0;
    double lantern = 1370.0;
    double lantern_rise = 790.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Arch with a polygonal tie (roof catalogue M).
struct ArchPolygonalTie {
    double span = 12000.0;
    double height = 3480.0;
    double tie = 980.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Pitched with crossed diagonals on end posts (roof catalogue N).
struct PitchedCrossedPosts {
    double span = 12000.0;
    double height = 1220.0;
    double ridge = 2810.0;
    double camber = 650.0;
    int panels = 8;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Cambered with crossed diagonals and a lantern (roof catalogue O).
struct CamberedCrossedLantern {
    double span = 12000.0;
    double height = 2150.0;
    double camber = 410.0;
    double lantern = 800.0;
    double lantern_rise = 780.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Arched lower chord with crossed diagonals on columns (roof catalogue P).
struct ArchedCrossedColumns {
    double span = 12000.0;
    double height = 1690.0;
    double ridge = 5250.0;
    double crown = 2820.0;
    double column = 1290.0;
    int panels = 8;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Trapezoid with king post and crossed diagonals (roof catalogue Q).
struct TrapezoidKingCrossed {
    double span = 12000.0;
    double height = 1880.0;
    double ridge = 2610.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// Topology-optimised beam.
struct TopologyOptimised {
    double span = 12000.0;
    double height = 1800.0;
    double chord_radius = 80.0;
    double web_radius = 60.0;
};

/// A truss of Bow's Economics of Construction in relation to Framed Structures (1873) by its number, from
/// data/templates/truss_bow_1873.json: its graph drawn at its span, with a section and a level per member.
struct Bow {
    std::string number; // Bow's number: "6", "202A", "200½".

    /// Every truss of the file, in its order.
    static std::vector<Bow> all();

    /// Its scene name, as stored: bow_<number> in lower case, ½ as _half.
    std::string name() const;
};

// ═══════════════════════════════════════════════════════════════════════════
// TrussGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The truss guide: the chords and the webs, each a member with its axis, section and level.
class TrussGuide {
public:
    std::vector<TrussMember> chords; // A member per straight run, mitered at kinks, the ends meeting others at heels.
    std::vector<TrussMember> webs; // Webs and posts: their ends stop on the chords and on each other.

    /// A guide from its members.
    TrussGuide(const std::vector<TrussMember>& chords, const std::vector<TrussMember>& webs);

    /// A design of the library by its parameters: the same guide, an overload per design.
    TrussGuide(const Pratt& design);
    TrussGuide(const KingPost& design);
    TrussGuide(const QueenPost& design);
    TrussGuide(const Howe& design);
    TrussGuide(const Fink& design);
    TrussGuide(const Belgian& design);
    TrussGuide(const Scissors& design);
    TrussGuide(const Warren& design);
    TrussGuide(const WarrenVerticals& design);
    TrussGuide(const PrattBridge& design);
    TrussGuide(const Parker& design);
    TrussGuide(const KTruss& design);
    TrussGuide(const CamberedVerticals& design);
    TrussGuide(const FunicularArch& design);
    TrussGuide(const Lens& design);
    TrussGuide(const TriangulatedArch& design);
    TrussGuide(const Sloped& design);
    TrussGuide(const PrattFlat& design);
    TrussGuide(const FlatPeaked& design);
    TrussGuide(const Bowstring& design);
    TrussGuide(const PitchedPosts& design);
    TrussGuide(const RoofCamberedTie& design);
    TrussGuide(const QuarterFan& design);
    TrussGuide(const Cantilever& design);
    TrussGuide(const CantileverDesign1& design);
    TrussGuide(const CantileverDesign2& design);
    TrussGuide(const CantileverDesign3& design);
    TrussGuide(const FishBelly& design);
    TrussGuide(const Sawtooth& design);
    TrussGuide(const HoweFlat& design);
    TrussGuide(const ThreeHingedArch& design);
    TrussGuide(const BurrArch& design);
    TrussGuide(const TownLattice& design);
    TrussGuide(const BowstringArch& design);
    TrussGuide(const WaddellA& design);
    TrussGuide(const Wichert& design);
    TrussGuide(const TrussLegBedstead& design);
    TrussGuide(const Kellogg& design);
    TrussGuide(const BaltimoreA& design);
    TrussGuide(const PennsylvaniaA& design);
    TrussGuide(const Greiner& design);
    TrussGuide(const DoubleIntersectionPratt& design);
    TrussGuide(const Schwedler& design);
    TrussGuide(const DoubleIntersectionWarren& design);
    TrussGuide(const Pegram& design);
    TrussGuide(const Post& design);
    TrussGuide(const Bollman& design);
    TrussGuide(const FinkBridge& design);
    TrussGuide(const Stearns& design);
    TrussGuide(const CrescentCrossed& design);
    TrussGuide(const BracedArchRib& design);
    TrussGuide(const BelgianVerticals& design);
    TrussGuide(const RaisedTieKingPost& design);
    TrussGuide(const TrapezoidCrossed& design);
    TrussGuide(const FinkCambered& design);
    TrussGuide(const CrescentVerticals& design);
    TrussGuide(const RaisedTiePosts& design);
    TrussGuide(const PitchedWarrenColumns& design);
    TrussGuide(const FinkCompound& design);
    TrussGuide(const FinkBentTie& design);
    TrussGuide(const FrenchSteep& design);
    TrussGuide(const FinkLantern& design);
    TrussGuide(const ArchPolygonalTie& design);
    TrussGuide(const PitchedCrossedPosts& design);
    TrussGuide(const CamberedCrossedLantern& design);
    TrussGuide(const ArchedCrossedColumns& design);
    TrussGuide(const TrapezoidKingCrossed& design);
    TrussGuide(const TopologyOptimised& design);

    /// A truss of Bow's catalogue: its graph drawn at its span.
    TrussGuide(const Bow& design);

    /// Each straight run of chord `chord`, from kink to kink: a member each; an end running on reaches as far as its end cut.
    std::vector<std::array<Point, 2>> runs(size_t chord) const;

    /// Whether chord `chord`'s end `side` (0 start, 1 end) runs on past its node: where chord ends meet and none passes, the
    /// one marked runs_on, else the highest in the hierarchy, else the most level, none of one level and slope (an apex) or
    /// at a gentle bend of under 35 degrees; a lone end, when every web there is lower in the hierarchy.
    bool runs_on_at(size_t chord, size_t side) const;

    /// The cut of an end running on, through the two points where it crosses the run's faces: through the outer corners of
    /// the members it carries on both its sides, so the contour runs on clean; flush with the outer face of the members on one
    /// side met at 40 degrees or more; else square past the node; the node always in wood. None for an end not running on.
    std::optional<std::array<Point, 2>> end_cut(size_t chord, size_t side) const;
};

}
