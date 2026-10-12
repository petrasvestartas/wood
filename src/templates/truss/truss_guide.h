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
};

} // namespace wood_truss
