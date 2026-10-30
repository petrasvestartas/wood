#pragma once
#include "wood_session.h"

#include <filesystem>

using namespace session_cpp;
using namespace wood_session;

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// TrussGuide
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

/// The truss guide: the chords and the webs, each a member with its axis, section and level.
class TrussGuide {
public:
    std::vector<TrussMember> chords; // A member per straight run, mitered at kinks, the ends meeting others at heels.
    std::vector<TrussMember> webs; // Webs and posts: their ends stop on the chords and on each other.

    /// A guide from its members.
    TrussGuide(const std::vector<TrussMember>& chords, const std::vector<TrussMember>& webs);

    /// A guide from lines, one half section for the chords and one for the webs, the hierarchy read from the drawing; the
    /// posts are webs of the chords' section.
    TrussGuide(
        const std::vector<Polyline>& chords,
        const std::vector<Line>& webs,
        const std::vector<Line>& posts = {},
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Every truss of a library file such as data/templates/truss_bow_1873.json, by name, its nodes (x along the span from
    /// the left support and z up, in spans) drawn at its span; an empty list when the file is missing.
    static std::vector<std::pair<std::string, TrussGuide>> library(const std::filesystem::path& json_path);

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

    // ═══════════════════════════════════════════════════════════════════════════
    // Library
    // ═══════════════════════════════════════════════════════════════════════════

    /// Pratt roof truss.
    static TrussGuide pratt(
        double span = 12000.0,
        double rise = 3000.0,
        int panels = 6,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// King post roof truss.
    static TrussGuide king_post(
        double span = 8000.0,
        double rise = 2667.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Queen post truss.
    static TrussGuide queen_post(
        double span = 9000.0,
        double height = 2000.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Howe roof truss.
    static TrussGuide howe(
        double span = 12000.0,
        double rise = 3000.0,
        int panels = 6,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Fink roof truss.
    static TrussGuide fink(
        double span = 10000.0,
        double rise = 2500.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Belgian roof truss.
    static TrussGuide belgian(
        double span = 10000.0,
        double rise = 2500.0,
        int divisions = 3,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Scissors roof truss.
    static TrussGuide scissors(
        double span = 8000.0,
        double rise = 3000.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Warren bridge truss: parallel chords `height` apart over `span`, `panels` panels of diagonals alternating up and down.
    static TrussGuide warren(
        double span = 12000.0,
        double height = 1500.0,
        int panels = 8,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Warren bridge truss with a vertical at every top node and every inner bottom node; `span`, `height` and `panels` as the Warren.
    static TrussGuide warren_verticals(
        double span = 12000.0,
        double height = 1500.0,
        int panels = 8,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Pratt bridge truss.
    static TrussGuide pratt_bridge(
        double span = 12000.0,
        double height = 2000.0,
        int panels = 6,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Parker bridge truss.
    static TrussGuide parker(
        double span = 12000.0,
        double height = 2400.0,
        double hip = 1600.0,
        int panels = 6,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// K bridge truss.
    static TrussGuide k_truss(
        double span = 12000.0,
        double height = 2400.0,
        int panels = 8,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Cambered verticals.
    static TrussGuide cambered_verticals(
        double span = 12000.0,
        double height = 1980.0,
        double camber = 800.0,
        int panels = 6,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Funicular arch.
    static TrussGuide funicular_arch(
        double span = 12000.0,
        double height = 2400.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Lens.
    static TrussGuide lens(
        double span = 12000.0,
        double height = 1140.0,
        double sag = 1260.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Triangulated arch.
    static TrussGuide triangulated_arch(
        double span = 12000.0,
        double height = 5980.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Sloped.
    static TrussGuide sloped(
        double span = 12040.0,
        double height = 6020.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Flat Pratt.
    static TrussGuide pratt_flat(
        double span = 12000.0,
        double height = 1990.0,
        int panels = 6,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Flat peaked.
    static TrussGuide flat_peaked(
        double span = 12000.0,
        double height = 1990.0,
        double peak = 3180.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Bowstring.
    static TrussGuide bowstring(
        double span = 12000.0,
        double height = 1990.0,
        double crown = 3320.0,
        int panels = 6,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Pitched on end posts.
    static TrussGuide pitched_posts(
        double span = 12000.0,
        double height = 2000.0,
        double ridge = 3320.0,
        int panels = 6,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Cambered tie roof truss.
    static TrussGuide roof_cambered_tie(
        double span = 12000.0,
        double rise = 2000.0,
        double camber = 500.0,
        double dip = 60.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Quarter fan.
    static TrussGuide quarter_fan(
        double span = 12000.0,
        double height = 5560.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Cantilever.
    static TrussGuide cantilever(
        double span = 12000.0,
        double height = 1830.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Cantilever design 1.
    static TrussGuide cantilever_design_1(
        double span = 11887.2,
        double height = 2000.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Cantilever design 2.
    static TrussGuide cantilever_design_2(
        double span = 12000.0,
        double height = 2000.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Cantilever design 3.
    static TrussGuide cantilever_design_3(
        double span = 12000.0,
        double height = 2000.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Fish-belly truss.
    static TrussGuide fish_belly(
        double span = 12000.0,
        double rise = 1720.0,
        double sag = 1150.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Sawtooth roof.
    static TrussGuide sawtooth(
        double span = 12000.0,
        double height = 1800.0,
        double column = 2130.0,
        int teeth = 3,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Parallel-chord Howe roof truss.
    static TrussGuide howe_flat(
        double span = 12000.0,
        double height = 1530.0,
        double ridge = 1890.0,
        int panels = 8,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Three-hinged arch.
    static TrussGuide three_hinged_arch(
        double span = 12000.0,
        double height = 3380.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Burr arch truss.
    static TrussGuide burr_arch(
        double span = 12000.0,
        double height = 2480.0,
        double crown = 1890.0,
        int panels = 6,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Town lattice.
    static TrussGuide town_lattice(
        double span = 12000.0,
        double height = 3000.0,
        int panels = 8,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Bowstring arch truss.
    static TrussGuide bowstring_arch(
        double span = 11900.0,
        double height = 2490.0,
        int panels = 7,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Waddell "A" truss.
    static TrussGuide waddell_a(
        double span = 12000.0,
        double rise = 4560.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Wichert truss.
    static TrussGuide wichert(
        double span = 12000.0,
        double height = 2320.0,
        double dip = 650.0,
        int panels = 10,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Truss leg bedstead.
    static TrussGuide truss_leg_bedstead(
        double span = 12000.0,
        double height = 2500.0,
        double leg = 900.0,
        int panels = 4,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Kellogg.
    static TrussGuide kellogg(
        double span = 12000.0,
        double height = 2650.0,
        int panels = 6,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Baltimore with sub-struts.
    static TrussGuide baltimore_a(
        double span = 14000.0,
        double height = 2140.0,
        int panels = 7,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Pennsylvania with sub-struts.
    static TrussGuide pennsylvania_a(
        double span = 14000.0,
        double height = 2160.0,
        double hip = 1400.0,
        int panels = 7,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Greiner.
    static TrussGuide greiner(
        double span = 12000.0,
        double height = 2500.0,
        double hang = 400.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Double intersection Pratt: a Pratt `height` high over `span` on inclined end posts, `panels` panels (even), its diagonals across two panels.
    static TrussGuide double_intersection_pratt(
        double span = 12000.0,
        double height = 2500.0,
        int panels = 12,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Schwedler.
    static TrussGuide schwedler(
        double span = 14000.0,
        double height = 2960.0,
        int panels = 14,
        int sloped = 4,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Double intersection Warren: two Warren web systems superimposed between hip verticals, `height` high over `span` on inclined end posts, `panels` panels.
    static TrussGuide double_intersection_warren(
        double span = 12000.0,
        double height = 2500.0,
        int panels = 8,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Pegram.
    static TrussGuide pegram(
        double span = 12060.0,
        double height = 2530.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Post.
    static TrussGuide post(
        double span = 12000.0,
        double height = 2500.0,
        int panels = 8,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Bollman.
    static TrussGuide bollman(
        double span = 12000.0,
        double height = 2500.0,
        int panels = 6,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Fink bridge.
    static TrussGuide fink_bridge(
        double span = 12000.0,
        double height = 2600.0,
        int panels = 8,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Stearns: a six-panel Fink `height` high over `span` on inclined end posts with the verticals omitted at alternate panel points.
    static TrussGuide stearns(
        double span = 12000.0,
        double height = 2580.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Crescent with crossed webs.
    static TrussGuide crescent_crossed(
        double span = 12000.0,
        double height = 3900.0,
        double inner = 1800.0,
        int panels = 12,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Braced arch rib.
    static TrussGuide braced_arch_rib(
        double span = 19426.0,
        double height = 10850.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Belgian with verticals (roof catalogue B).
    static TrussGuide belgian_verticals(
        double span = 12000.0,
        double rise = 3540.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Raised tie with king post (roof catalogue C).
    static TrussGuide raised_tie_king_post(
        double span = 12000.0,
        double rise = 3530.0,
        double tie = 1160.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Trapezoid with crossed diagonals (roof catalogue D).
    static TrussGuide trapezoid_crossed(
        double span = 12000.0,
        double height = 3020.0,
        double ridge = 4600.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Fink on a cambered bottom chord (roof catalogue E).
    static TrussGuide fink_cambered(
        double span = 12000.0,
        double rise = 2890.0,
        double camber = 390.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Crescent with verticals and a lantern (roof catalogue F).
    static TrussGuide crescent_verticals(
        double span = 12000.0,
        double height = 3650.0,
        double inner = 1260.0,
        double lantern = 270.0,
        double lantern_rise = 380.0,
        int panels = 10,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Raised tie on posts (roof catalogue G).
    static TrussGuide raised_tie_posts(
        double span = 12000.0,
        double rise = 4130.0,
        double column = 600.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Pitched Warren on columns (roof catalogue H).
    static TrussGuide pitched_warren_columns(
        double span = 12000.0,
        double height = 1170.0,
        double ridge = 2140.0,
        double column = 970.0,
        int panels = 8,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Compound Fink (roof catalogue I).
    static TrussGuide fink_compound(
        double span = 12000.0,
        double rise = 4090.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Fink with a bent tie (roof catalogue J).
    static TrussGuide fink_bent_tie(
        double span = 12000.0,
        double rise = 4180.0,
        double bend = 1080.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Steep French truss (roof catalogue K).
    static TrussGuide french_steep(
        double span = 12000.0,
        double rise = 5930.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Compound Fink with king post and lantern (roof catalogue L).
    static TrussGuide fink_lantern(
        double span = 12000.0,
        double rise = 3230.0,
        double lantern = 1370.0,
        double lantern_rise = 790.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Arch with a polygonal tie (roof catalogue M).
    static TrussGuide arch_polygonal_tie(
        double span = 12000.0,
        double height = 3480.0,
        double tie = 980.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Pitched with crossed diagonals on end posts (roof catalogue N).
    static TrussGuide pitched_crossed_posts(
        double span = 12000.0,
        double height = 1220.0,
        double ridge = 2810.0,
        double camber = 650.0,
        int panels = 8,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Cambered with crossed diagonals and a lantern (roof catalogue O).
    static TrussGuide cambered_crossed_lantern(
        double span = 12000.0,
        double height = 2150.0,
        double camber = 410.0,
        double lantern = 800.0,
        double lantern_rise = 780.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Arched lower chord with crossed diagonals on columns (roof catalogue P).
    static TrussGuide arched_crossed_columns(
        double span = 12000.0,
        double height = 1690.0,
        double ridge = 5250.0,
        double crown = 2820.0,
        double column = 1290.0,
        int panels = 8,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Trapezoid with king post and crossed diagonals (roof catalogue Q).
    static TrussGuide trapezoid_king_crossed(
        double span = 12000.0,
        double height = 1880.0,
        double ridge = 2610.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );

    /// Topology-optimised beam.
    static TrussGuide topology_optimised(
        double span = 12000.0,
        double height = 1800.0,
        double chord_radius = 80.0,
        double web_radius = 60.0
    );
};

}
