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
    std::vector<Line> posts; // Each end post between two chord ends, the chords running on over it; as thick as a chord.
    double chord_radius = 80.0; // mm, half the chords' and the posts' square section.
    double web_radius = 60.0; // mm, half the webs' square section.

    /// A guide from its chords, webs and end posts and their half sections.
    TrussGuide(const std::vector<Polyline>& chords, const std::vector<Line>& webs, const std::vector<Line>& posts = {}, double chord_radius = 80.0, double web_radius = 60.0);

    /// Each straight run of chord `chord`, from kink to kink: a member each.
    std::vector<std::array<Point, 2>> runs(size_t chord) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Library
    // ═══════════════════════════════════════════════════════════════════════════

    /// Pratt roof truss in the xz plane: the bottom chord over `span`, the top chord up to `rise` at the middle, `panels` panels (even), a
    /// vertical at every inner panel point and the diagonals falling towards the middle.
    static TrussGuide pratt(double span = 12000.0, double rise = 3000.0, int panels = 6);

    /// King post roof truss: the post from the bottom chord up to the ridge `rise` over `span`, a strut from its foot to each top chord's middle.
    static TrussGuide king_post(double span = 8000.0, double rise = 2667.0);

    /// Queen post truss: a trapezoid over `span`, the top chord inclined up to `height` at the third points and level between, a post under each kink.
    static TrussGuide queen_post(double span = 9000.0, double height = 2000.0);

    /// Howe roof truss: `panels` panels (even) over `span`, a vertical at every inner panel point and the diagonals rising towards the middle.
    static TrussGuide howe(double span = 12000.0, double rise = 3000.0, int panels = 6);

    /// Fink roof truss: a W of four webs, each pair rising from a third point of the bottom chord to a quarter point and to the ridge.
    static TrussGuide fink(double span = 10000.0, double rise = 2500.0);

    /// Belgian roof truss: struts square to the top chord from its third points down to the bottom chord, ties from their feet up to the next node.
    static TrussGuide belgian(double span = 10000.0, double rise = 2500.0);

    /// Scissors roof truss: the bottom chords rising to a third of `rise` at the middle, a king post, and per side a vertical and a strut.
    static TrussGuide scissors(double span = 8000.0, double rise = 3000.0);

    /// Warren bridge truss: parallel chords `height` apart, `panels` panels of diagonals alternating up and down.
    static TrussGuide warren(double span = 12000.0, double height = 1500.0, int panels = 8);

    /// Warren bridge truss with a vertical at every top node and every inner bottom node.
    static TrussGuide warren_verticals(double span = 12000.0, double height = 1500.0, int panels = 8);

    /// Pratt bridge truss: parallel chords `height` apart, inclined end posts in the top chord, a vertical at every inner panel point and the
    /// diagonals falling towards the middle.
    static TrussGuide pratt_bridge(double span = 12000.0, double height = 2000.0, int panels = 6);

    /// Parker bridge truss: a Pratt bridge over 12000 whose top chord is a polygon through 1600, 2200 and 2400 at the panel points, highest at the middle.
    static TrussGuide parker();

    /// K bridge truss: parallel chords and inclined end posts, a vertical at every inner panel point, and in every panel a K, two diagonals from
    /// the middle of a vertical, split there, to the top and the foot of the next one towards the middle.
    static TrussGuide k_truss(double span = 12000.0, double height = 2400.0, int panels = 8);

    /// Cambered verticals: a pitched top chord up to 2700 over 12000, the bottom chord flat at the heels and cambered up to 900 at the middle,
    /// eight panels, a vertical at every inner panel point and no diagonal.
    static TrussGuide cambered_verticals();

    /// Funicular arch: a polygonal arch with a level crown over a straight tie, inclined webs from the tie's nodes to the nearest arch nodes.
    static TrussGuide funicular_arch();

    /// Lens: a polygonal arch over a sagging polygon, the two meeting at the heels, a hanger between every two inner nodes.
    static TrussGuide lens();

    /// Triangulated arch: an outer and an inner polygonal chord springing from the same two supports, no tie, webs zigzagging between their
    /// nodes and a vertical at the crown.
    static TrussGuide triangulated_arch();

    /// Sloped: a truss climbing to the right, a heel at the left, a level middle panel, the bottom chord 1200 under the top chord, a vertical at
    /// every inner node, one diagonal per panel falling to the right and an end post at the right.
    static TrussGuide sloped();

    /// Flat Pratt truss: parallel chords `height` apart over `span`, an end post at each end, a vertical at every inner panel point and the
    /// diagonals falling towards the middle.
    static TrussGuide pratt_flat(double span = 12000.0, double height = 2000.0, int panels = 6);

    /// Flat peaked: a flat Pratt with end posts over 12000, its top chord rising to a peak at 8000 over the right half, the diagonals falling to
    /// the right but for the two from the peak down to its neighbouring bottom nodes.
    static TrussGuide flat_peaked();

    /// Bowstring truss: end posts 1500 high, a polygonal top chord up to 2650 at the middle over 12000, a flat bottom chord, verticals and the
    /// diagonals falling towards the middle.
    static TrussGuide bowstring();

    /// Pitched truss on end posts: the posts `post` high, the top chord straight up to `ridge` at the middle, `panels` panels (even), verticals
    /// and the diagonals falling towards the middle.
    static TrussGuide pitched_posts(double span = 12000.0, double post = 1500.0, double ridge = 2600.0, int panels = 6);
};

} // namespace wood_truss
