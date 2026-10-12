#include "pch.h"
#include "src/templates/truss/truss_guide.h"

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// TrussGuide
// ═══════════════════════════════════════════════════════════════════════════

TrussGuide::TrussGuide(const std::vector<Polyline>& chords, const std::vector<Line>& webs, const std::vector<Line>& posts, double chord_radius, double web_radius)
    : chords(chords),
      webs(webs),
      posts(posts),
      chord_radius(chord_radius),
      web_radius(web_radius) {
}

std::vector<std::array<Point, 2>> TrussGuide::runs(size_t chord) const {

    // a run ends where the chord turns; nodes on a straight stretch only mark where webs meet it
    const std::vector<Point> nodes = chords.at(chord).get_points();
    std::vector<std::array<Point, 2>> runs;
    Point start = nodes.front();
    for (size_t i = 1; i < nodes.size(); i++) {
        const bool last = i + 1 == nodes.size();
        const bool kink = !last && (nodes[i] - start).normalized().cross((nodes[i + 1] - nodes[i]).normalized()).magnitude() > 1e-6;
        if (last || kink) {
            runs.push_back({start, nodes[i]});
            start = nodes[i];
        }
    }

    return runs;
}

// ═══════════════════════════════════════════════════════════════════════════
// Library
// ═══════════════════════════════════════════════════════════════════════════

TrussGuide TrussGuide::pratt(double span, double rise, int panels) {

    const double panel = span / panels;
    const auto top = [&](int i) { return Point(i * panel, 0.0, rise * (1.0 - std::abs(i * panel - 0.5 * span) / (0.5 * span))); };
    const auto bottom = [&](int i) { return Point(i * panel, 0.0, 0.0); };

    // the chords: the bottom chord straight, the top chord up to the ridge and down
    const std::vector<Polyline> chords = {Polyline({bottom(0), bottom(panels)}), Polyline({top(0), top(panels / 2), top(panels)})};

    // a vertical at every inner panel point, the diagonals from the top of each falling towards the middle
    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int i = 1; i < panels / 2; i++) {
        webs.push_back(Line::from_points(top(i), bottom(i + 1)));
        webs.push_back(Line::from_points(top(panels - i), bottom(panels - i - 1)));
    }

    return TrussGuide(chords, webs);
}

} // namespace wood_truss
