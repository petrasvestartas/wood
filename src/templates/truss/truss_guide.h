#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// TrussGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The truss guide, the lines a truss is drawn with: each chord a polyline through its nodes, each web a line between two nodes. A Truss
/// turns it into members and butts them by how the lines meet at the nodes.
class TrussGuide {
public:
    std::vector<Polyline> chords; // Each chord through its nodes, end to end; a node where it turns is a kink.
    std::vector<Line> webs; // Each web between two nodes, on a chord, at a kink or at a chord's end.
    std::vector<Line> posts; // Each post, as thick as a chord: between two chord ends, the chords running on over it, or a column through the chords, ending them or crossed by them.
    double chord_radius = 80.0; // mm, half the chords' and the posts' square section.
    double web_radius = 60.0; // mm, half the webs' square section.

    /// A guide from its chords, webs and end posts and their half sections.
    TrussGuide(const std::vector<Polyline>& chords, const std::vector<Line>& webs, const std::vector<Line>& posts = {}, double chord_radius = 80.0, double web_radius = 60.0);

    /// Each straight run of chord `chord`, from kink to kink: a member each.
    std::vector<std::array<Point, 2>> runs(size_t chord) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Library
    // ═══════════════════════════════════════════════════════════════════════════

    // Every design in the xz plane, z up, the first support at the origin. `span` and the heights scale it, `panels` sets the divisions where
    // the pattern repeats, and `chord_radius` and `web_radius` are the half sections. The defaults give the drawn design.

    /// Pratt roof truss: the bottom chord over `span`, the top chord up to `rise` at the middle, `panels` panels (even), a vertical at every inner
    /// panel point and the diagonals falling towards the middle.
    static TrussGuide pratt(double span = 12000.0, double rise = 3000.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);

    /// King post roof truss: the post from the bottom chord up to the ridge `rise` over `span`, a strut from its foot to each top chord's middle.
    static TrussGuide king_post(double span = 8000.0, double rise = 2667.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Queen post truss: a trapezoid over `span`, the top chord inclined up to `height` at the third points and level between, a post under each kink.
    static TrussGuide queen_post(double span = 9000.0, double height = 2000.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Howe roof truss: `panels` panels (even) over `span`, the ridge at `rise`, a vertical at every inner panel point and the diagonals rising
    /// towards the middle.
    static TrussGuide howe(double span = 12000.0, double rise = 3000.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);

    /// Fink roof truss: a W of four webs over `span`, the ridge at `rise`, each pair rising from a third point of the bottom chord to a quarter
    /// point and to the ridge.
    static TrussGuide fink(double span = 10000.0, double rise = 2500.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Belgian roof truss: over `span`, the ridge at `rise`, each top chord in `divisions` parts, a strut square to it from every inner division
    /// point down to the bottom chord and a tie from its foot up to the next division point.
    static TrussGuide belgian(double span = 10000.0, double rise = 2500.0, int divisions = 3, double chord_radius = 80.0, double web_radius = 60.0);

    /// Scissors roof truss: over `span`, the ridge at `rise`, the bottom chords rising to a third of it at the middle, a king post, and per side a
    /// vertical and a strut.
    static TrussGuide scissors(double span = 8000.0, double rise = 3000.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Warren bridge truss: parallel chords `height` apart over `span`, `panels` panels of diagonals alternating up and down.
    static TrussGuide warren(double span = 12000.0, double height = 1500.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);

    /// Warren bridge truss with a vertical at every top node and every inner bottom node; `span`, `height` and `panels` as the Warren.
    static TrussGuide warren_verticals(double span = 12000.0, double height = 1500.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);

    /// Pratt bridge truss: parallel chords `height` apart over `span`, inclined end posts in the top chord, `panels` panels (even), a vertical
    /// at every inner panel point and the diagonals falling towards the middle.
    static TrussGuide pratt_bridge(double span = 12000.0, double height = 2000.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);

    /// Parker bridge truss: a Pratt bridge over `span`, `panels` panels (even), whose polygonal top chord rises on a parabola from `hip` at the
    /// first panel point to `height` at the middle.
    static TrussGuide parker(double span = 12000.0, double height = 2400.0, double hip = 1600.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);

    /// K bridge truss: parallel chords `height` apart over `span` on inclined end posts, `panels` panels (even), a vertical at every inner panel
    /// point, and in every panel a K, two diagonals from the middle of a vertical to the top and the foot of the next one towards the middle.
    static TrussGuide k_truss(double span = 12000.0, double height = 2400.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);

    /// Cambered verticals: the top chord pitched up to `height` over `span`, `panels` panels (even), the bottom chord flat
    /// over the end panels and cambered up to `camber` at the middle, a vertical at every inner panel point and no diagonal.
    static TrussGuide cambered_verticals(double span = 12000.0, double height = 1980.0, double camber = 800.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);

    /// Funicular arch: over `span`, a polygonal arch with a level crown at `height` over a straight tie, an inclined web from
    /// each inner tie node to its arch node, the nodes at the measured fractions.
    static TrussGuide funicular_arch(double span = 12000.0, double height = 2400.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Lens: over `span`, a polygonal arch up to `height` over a polygon sagging `sag`, meeting at the heels, an inclined
    /// hanger between each two inner nodes, at the measured fractions.
    static TrussGuide lens(double span = 12000.0, double height = 1140.0, double sag = 1260.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Triangulated arch: over `span`, an outer polygon up to `height` and an inner one from the same two supports, no tie,
    /// webs zigzagging between their nodes and a vertical at the crown, at the measured fractions.
    static TrussGuide triangulated_arch(double span = 12000.0, double height = 5980.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Sloped: a truss climbing to the right over `span` up to `height`, seven panels on the drawn grid, both chords meeting
    /// at each end, a level middle panel, a vertical at every inner node and a diagonal in every panel but the end and middle ones.
    static TrussGuide sloped(double span = 12040.0, double height = 6020.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Flat Pratt: parallel chords `height` apart over `span`, `panels` panels (even), an end post at each end, a vertical
    /// at every inner panel point and the diagonals falling towards the middle.
    static TrussGuide pratt_flat(double span = 12000.0, double height = 1990.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);

    /// Flat peaked: a flat Pratt of six panels on end posts `height` high over `span`, its top chord rising to `peak` at the
    /// fourth panel point, the diagonals falling to the right in the left half and rising to the right under the peak.
    static TrussGuide flat_peaked(double span = 12000.0, double height = 1990.0, double peak = 3180.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Bowstring: end posts `height` high over `span`, `panels` panels (even), a polygonal top chord on a parabola up to
    /// `crown`, a flat bottom chord, verticals and the diagonals falling towards the middle.
    static TrussGuide bowstring(double span = 12000.0, double height = 1990.0, double crown = 3320.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);

    /// Pitched on end posts: posts `height` high over `span`, the top chord straight up to `ridge` at the middle, `panels`
    /// panels (even), verticals and the diagonals falling towards the middle.
    static TrussGuide pitched_posts(double span = 12000.0, double height = 2000.0, double ridge = 3320.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);


    /// Cambered tie roof truss: the rafters up to `rise` over `span`, a polygonal tie dipping `dip` at the first load point and
    /// rising to `camber` at the middle, verticals at the load points and diagonals down to the next vertical's foot, at the measured fractions.
    static TrussGuide roof_cambered_tie(double span = 12000.0, double rise = 2000.0, double camber = 500.0, double dip = 60.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Quarter fan: a quarter arch `span` long and `height` high from two wall supports, its outer polygon over a straight
    /// bottom chord, the interior a fan of members from both supports meeting at shared nodes, at the measured fractions.
    static TrussGuide quarter_fan(double span = 12000.0, double height = 5560.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Cantilever: a horizontal top chord `span` long from the upper wall support `height` over the lower one, a polygonal
    /// bottom chord rising to the tip, webs zigzagging between them, at the measured fractions.
    static TrussGuide cantilever(double span = 12000.0, double height = 1830.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Cantilever design 1: nodes A to F from the drawn side lengths, scaled to `span` from the wall to C and `height` from B
    /// to A, chords A-D-C and B-F-C, the wall member A-B a post.
    static TrussGuide cantilever_design_1(double span = 11887.2, double height = 2000.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Cantilever design 2: nodes A to H from the drawn dimensions, scaled to `span` from the wall to C and `height` from B
    /// to A, chords A-D-E-C and B-G-H-C.
    static TrussGuide cantilever_design_2(double span = 12000.0, double height = 2000.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Cantilever design 3: nodes A to G from the drawn side lengths, scaled to `span` from the wall to G and `height` from A
    /// to B, chords B-E-G and A-D-F-G, the wall member A-B a post.
    static TrussGuide cantilever_design_3(double span = 12000.0, double height = 2000.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Fish-belly truss: a top chord pitched up to `rise` over `span`, a polygonal bottom chord sagging up to `sag` at the
    /// measured fractions, eight panels, a vertical at each load point.
    static TrussGuide fish_belly(double span = 12000.0, double rise = 1720.0, double sag = 1150.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Sawtooth roof: `teeth` teeth over `span` on one tie, each a long rafter up to `height` at three quarters of the tooth with a W of
    /// four webs and a short steep rafter, a column `column` high under every valley.
    static TrussGuide sawtooth(double span = 12000.0, double height = 1800.0, double column = 2130.0, int teeth = 3, double chord_radius = 80.0, double web_radius = 60.0);

    /// Parallel-chord Howe roof truss: end posts `height` high over `span`, the top chord pitched up to `ridge`, `panels` panels (even),
    /// verticals and the diagonals rising towards the middle.
    static TrussGuide howe_flat(double span = 12000.0, double height = 1530.0, double ridge = 1890.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);

    /// Three-hinged arch: two curved trusses from hinges `span` apart meeting at the crown hinge `height` high, each an outer and an inner
    /// polygon with rungs and diagonals between them, at the measured fractions.
    static TrussGuide three_hinged_arch(double span = 12000.0, double height = 3380.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Burr arch truss: a multiple king post truss `height` high over `span` on end posts, `panels` panels (even), its diagonals rising
    /// towards the middle, and an arch on a parabola from heel to heel up to `crown` through the verticals.
    static TrussGuide burr_arch(double span = 12000.0, double height = 2480.0, double crown = 1890.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);

    /// Town lattice: chords `height` apart over `span` on end posts, `panels` lattice pitches, planks both ways from each chord node to the
    /// other chord two pitches on, those past the ends stopping on the end posts' middles.
    static TrussGuide town_lattice(double span = 12000.0, double height = 3000.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);

    /// Bowstring arch truss: a polygonal arch up to `height` tied by the bottom chord over `span`, `panels` panels, verticals and crossed
    /// diagonals in every inner panel.
    static TrussGuide bowstring_arch(double span = 11900.0, double height = 2490.0, int panels = 7, double chord_radius = 80.0, double web_radius = 60.0);

    /// Waddell "A" truss: a king post truss over `span` up to `rise`, a vertical under each rafter's middle and a strut from there down to
    /// the king post's foot.
    static TrussGuide waddell_a(double span = 12000.0, double rise = 4560.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Wichert truss: a Warren `height` high over `span` on end posts, `panels` half panels (half of it odd), continuous over a middle pier,
    /// its bottom chord dipping `dip` to the pier.
    static TrussGuide wichert(double span = 12000.0, double height = 2320.0, double dip = 650.0, int panels = 10, double chord_radius = 80.0, double web_radius = 60.0);

    /// Truss leg bedstead: a Pratt `height` high over `span`, `panels` panels (even), whose vertical end posts run on `leg` below the bottom
    /// chord into the foundations.
    static TrussGuide truss_leg_bedstead(double span = 12000.0, double height = 2500.0, double leg = 900.0, int panels = 4, double chord_radius = 80.0, double web_radius = 60.0);

    /// Kellogg: a Pratt `height` high over `span` on inclined end posts, `panels` panels (even), with additional diagonals from the upper
    /// panel points to the middle of the lower panels.
    static TrussGuide kellogg(double span = 12000.0, double height = 2650.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);

    /// Baltimore with sub-struts: a Pratt `height` high over `span` on inclined end posts, `panels` main panels (odd), each split by a
    /// sub-vertical up to the middle of its main diagonal, a sub-strut from the panel's foot to it, crossed diagonals in the middle panel.
    static TrussGuide baltimore_a(double span = 14000.0, double height = 2140.0, int panels = 7, double chord_radius = 80.0, double web_radius = 60.0);

    /// Pennsylvania with sub-struts: a Baltimore with sub-struts over `span`, `panels` main panels (odd), under a polygonal top chord
    /// rising on a parabola from `hip` at the first panel point to `height` at the middle.
    static TrussGuide pennsylvania_a(double span = 14000.0, double height = 2160.0, double hip = 1400.0, int panels = 7, double chord_radius = 80.0, double web_radius = 60.0);

    /// Greiner: a six-panel Pratt `height` high over `span` on inclined end posts whose middle diagonals are replaced by an inverted
    /// bowstring hung `hang` above the bottom chord on the second verticals.
    static TrussGuide greiner(double span = 12000.0, double height = 2500.0, double hang = 400.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Double intersection Pratt: a Pratt `height` high over `span` on inclined end posts, `panels` panels (even), its diagonals
    /// across two panels.
    static TrussGuide double_intersection_pratt(double span = 12000.0, double height = 2500.0, int panels = 12, double chord_radius = 80.0, double web_radius = 60.0);

    /// Schwedler: over `span`, `panels` panels (even), a Parker whose top chord rises over `sloped` panels at each end to `height`, a Pratt
    /// in the sloped panels and a double intersection Pratt between them.
    static TrussGuide schwedler(double span = 14000.0, double height = 2960.0, int panels = 14, int sloped = 4, double chord_radius = 80.0, double web_radius = 60.0);

    /// Double intersection Warren: two Warren web systems superimposed between hip verticals, `height` high over `span` on inclined end
    /// posts, `panels` panels.
    static TrussGuide double_intersection_warren(double span = 12000.0, double height = 2500.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);

    /// Pegram: over `span`, a polygonal top chord up to `height` over nine panels, its nodes between the bottom nodes, webs zigzagging
    /// between them and crossed in the middle, at the measured fractions.
    static TrussGuide pegram(double span = 12060.0, double height = 2530.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Post: a rectangle `height` high over `span` on end posts, `panels` panels (even), inclined posts leaning to the middle from the half
    /// panel points and diagonals across two of them.
    static TrussGuide post(double span = 12000.0, double height = 2500.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);

    /// Bollman: a rectangle `height` high over `span` on end posts, `panels` panels, verticals and diagonals from each end post's top to
    /// every bottom panel point.
    static TrussGuide bollman(double span = 12000.0, double height = 2500.0, int panels = 6, double chord_radius = 80.0, double web_radius = 60.0);

    /// Fink bridge: a rectangle `height` high over `span` on end posts, `panels` panels (a power of two), verticals and the Fink's
    /// diagonals, from the end posts to the middle, then from every node so reached to the middles of the halves on each side.
    static TrussGuide fink_bridge(double span = 12000.0, double height = 2600.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);

    /// Stearns: a six-panel Fink `height` high over `span` on inclined end posts with the verticals omitted at alternate panel points.
    static TrussGuide stearns(double span = 12000.0, double height = 2580.0, double chord_radius = 80.0, double web_radius = 60.0);
    /// Crescent with crossed webs: an outer arc up to `height` and an inner arc up to `inner` from the same two supports `span`
    /// apart, the outer arc in `panels` (even) equal angles, two zigzags of webs between the arcs crossing in every panel.
    static TrussGuide crescent_crossed(double span = 12000.0, double height = 3900.0, double inner = 1800.0, int panels = 12, double chord_radius = 80.0, double web_radius = 60.0);

    /// Braced arch rib: from the support up a vertical outer face of 5, 6 and 8.05
    /// ft panels, over the knee and along the 30 degree slope to the crown hinge `height` high, at the measured fractions.
    static TrussGuide braced_arch_rib(double span = 19426.0, double height = 10850.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Belgian with verticals (roof catalogue B): rafters up to `rise` over `span`, per side a strut from the rafter at a sixth to the third point,
    /// the vertical over it and the diagonal to the ridge.
    static TrussGuide belgian_verticals(double span = 12000.0, double rise = 3540.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Raised tie with king post (roof catalogue C): rafters up to `rise` over `span`, inclined ties from the heels up to a knot `tie` high under
    /// the ridge, the king post and a strut from each rafter's middle to the knot.
    static TrussGuide raised_tie_king_post(double span = 12000.0, double rise = 3530.0, double tie = 1160.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Trapezoid with crossed diagonals (roof catalogue D): over `span`, the top chord up to shoulders `height` high and a ridge `ridge` high,
    /// diagonals from each heel to the far shoulder.
    static TrussGuide trapezoid_crossed(double span = 12000.0, double height = 3020.0, double ridge = 4600.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Fink on a cambered bottom chord (roof catalogue E): rafters up to `rise` over `span`, the bottom chord cambered up to `camber`, a Fink W
    /// per side.
    static TrussGuide fink_cambered(double span = 12000.0, double rise = 2890.0, double camber = 390.0, double chord_radius = 80.0, double web_radius = 60.0);
    /// Crescent with verticals and a lantern (roof catalogue F): an outer arc up to `height` and an inner one up to `inner` over `span`, `panels`
    /// panels (even), verticals and diagonals falling towards the middle, a lantern on posts `lantern` high with a roof rising `lantern_rise`.
    static TrussGuide crescent_verticals(double span = 12000.0, double height = 3650.0, double inner = 1260.0, double lantern = 270.0, double lantern_rise = 380.0, int panels = 10, double chord_radius = 80.0, double web_radius = 60.0);

    /// Raised tie on posts (roof catalogue G): rafters up to `rise` over `span`, the tie raised between two posts on columns `column` long, a Fink
    /// W in each upper corner.
    static TrussGuide raised_tie_posts(double span = 12000.0, double rise = 4130.0, double column = 600.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Pitched Warren on columns (roof catalogue H): a top chord pitched from `height` up to `ridge` over `span`, `panels` panels (even),
    /// verticals and alternating diagonals, the columns running on `column` below the bottom chord.
    static TrussGuide pitched_warren_columns(double span = 12000.0, double height = 1170.0, double ridge = 2140.0, double column = 970.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);

    /// Compound Fink (roof catalogue I): rafters up to `rise` over `span`, per side a lower Fink W and an upper one hung from a knot on the main
    /// strut.
    static TrussGuide fink_compound(double span = 12000.0, double rise = 4090.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Fink with a bent tie (roof catalogue J): rafters up to `rise` over `span`, per side a tie from the heel bent `bend` high up to the ridge,
    /// a strut from the rafter to the bend.
    static TrussGuide fink_bent_tie(double span = 12000.0, double rise = 4180.0, double bend = 1080.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Steep French truss (roof catalogue K): rafters up to `rise` over `span`, a central triangle down to the third points, per side three
    /// struts from the rafter.
    static TrussGuide french_steep(double span = 12000.0, double rise = 5930.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Compound Fink with king post and lantern (roof catalogue L): rafters up to `rise` over `span`, per side a Fink, the king post, a lantern on
    /// posts `lantern` high with a roof rising `lantern_rise` and struts from the ridge.
    static TrussGuide fink_lantern(double span = 12000.0, double rise = 3230.0, double lantern = 1370.0, double lantern_rise = 790.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Arch with a polygonal tie (roof catalogue M): an arc up to `height` over `span`, a polygonal tie up to `tie`, the king post and triangulated
    /// webs.
    static TrussGuide arch_polygonal_tie(double span = 12000.0, double height = 3480.0, double tie = 980.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Pitched with crossed diagonals on end posts (roof catalogue N): over `span`, the top chord from `height` up to `ridge`, the bottom chord up
    /// to `camber`, `panels` panels (even), verticals and both diagonals in every panel.
    static TrussGuide pitched_crossed_posts(double span = 12000.0, double height = 1220.0, double ridge = 2810.0, double camber = 650.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);

    /// Cambered with crossed diagonals and a lantern (roof catalogue O): over `span`, the top chord up to `height` and level in the middle, the
    /// bottom chord cambered up to `camber`, crossed diagonals, a lantern on posts `lantern` high with a roof rising `lantern_rise`.
    static TrussGuide cambered_crossed_lantern(double span = 12000.0, double height = 2150.0, double camber = 410.0, double lantern = 800.0, double lantern_rise = 780.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Arched lower chord with crossed diagonals on columns (roof catalogue P): over `span`, the top chord from `height` up to `ridge`, an arched
    /// lower chord up to `crown`, `panels` panels (even), verticals and both diagonals in every panel, the columns running on `column` below.
    static TrussGuide arched_crossed_columns(double span = 12000.0, double height = 1690.0, double ridge = 5250.0, double crown = 2820.0, double column = 1290.0, int panels = 8, double chord_radius = 80.0, double web_radius = 60.0);

    /// Trapezoid with king post and crossed diagonals (roof catalogue Q): over `span`, the top chord up to shoulders `height` high at the quarter
    /// points and a ridge `ridge` high, verticals, the king post and both diagonals in the two middle panels.
    static TrussGuide trapezoid_king_crossed(double span = 12000.0, double height = 1880.0, double ridge = 2610.0, double chord_radius = 80.0, double web_radius = 60.0);

    /// Topology-optimised beam: over `span`, a level top chord `height` high between sloped ends, a triangle at the middle and per side a
    /// zigzag of webs with a crossed pair, as a density optimisation of a simply supported beam leaves them.
    static TrussGuide topology_optimised(double span = 12000.0, double height = 1800.0, double chord_radius = 80.0, double web_radius = 60.0);
};

} // namespace wood_truss
