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

TrussGuide TrussGuide::king_post(double span, double rise) {

    // the chords: the bottom chord straight, the top chord up to the ridge and down
    const Point foot(0.5 * span, 0.0, 0.0);
    const Point ridge(0.5 * span, 0.0, rise);
    const std::vector<Polyline> chords = {
        Polyline({Point(0.0, 0.0, 0.0), Point(span, 0.0, 0.0)}),
        Polyline({Point(0.0, 0.0, 0.0), ridge, Point(span, 0.0, 0.0)}),
    };

    // the king post, and a strut from its foot to the middle of each top chord
    const std::vector<Line> webs = {
        Line::from_points(foot, ridge),
        Line::from_points(foot, Point(0.25 * span, 0.0, 0.5 * rise)),
        Line::from_points(foot, Point(0.75 * span, 0.0, 0.5 * rise)),
    };

    return TrussGuide(chords, webs);
}

TrussGuide TrussGuide::queen_post(double span, double height) {

    // the chords: the bottom chord straight, the top chord inclined to the third points and level between
    const Point left(span / 3.0, 0.0, height);
    const Point right(2.0 * span / 3.0, 0.0, height);
    const std::vector<Polyline> chords = {
        Polyline({Point(0.0, 0.0, 0.0), Point(span, 0.0, 0.0)}),
        Polyline({Point(0.0, 0.0, 0.0), left, right, Point(span, 0.0, 0.0)}),
    };

    // a queen post under each kink
    const std::vector<Line> webs = {
        Line::from_points(Point(left[0], 0.0, 0.0), left),
        Line::from_points(Point(right[0], 0.0, 0.0), right),
    };

    return TrussGuide(chords, webs);
}

TrussGuide TrussGuide::howe(double span, double rise, int panels) {

    const double panel = span / panels;
    const auto top = [&](int i) { return Point(i * panel, 0.0, rise * (1.0 - std::abs(i * panel - 0.5 * span) / (0.5 * span))); };
    const auto bottom = [&](int i) { return Point(i * panel, 0.0, 0.0); };

    // the chords: the bottom chord straight, the top chord up to the ridge and down
    const std::vector<Polyline> chords = {Polyline({bottom(0), bottom(panels)}), Polyline({top(0), top(panels / 2), top(panels)})};

    // a vertical at every inner panel point, the diagonals from the foot of each rising towards the middle
    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int i = 1; i < panels / 2; i++) {
        webs.push_back(Line::from_points(bottom(i), top(i + 1)));
        webs.push_back(Line::from_points(bottom(panels - i), top(panels - i - 1)));
    }

    return TrussGuide(chords, webs);
}

TrussGuide TrussGuide::fink(double span, double rise) {

    const auto top = [&](double x) { return Point(x, 0.0, rise * (1.0 - std::abs(x - 0.5 * span) / (0.5 * span))); };

    // the chords: the bottom chord straight, the top chord up to the ridge and down
    const std::vector<Polyline> chords = {
        Polyline({Point(0.0, 0.0, 0.0), Point(span, 0.0, 0.0)}),
        Polyline({top(0.0), top(0.5 * span), top(span)}),
    };

    // per side, from the third point to the quarter point and to the ridge
    std::vector<Line> webs;
    for (const double side : {0.0, 1.0}) {
        const double third = side == 0.0 ? span / 3.0 : 2.0 * span / 3.0;
        const double quarter = side == 0.0 ? 0.25 * span : 0.75 * span;
        webs.push_back(Line::from_points(Point(third, 0.0, 0.0), top(quarter)));
        webs.push_back(Line::from_points(Point(third, 0.0, 0.0), top(0.5 * span)));
    }

    return TrussGuide(chords, webs);
}

TrussGuide TrussGuide::belgian(double span, double rise) {

    const auto top = [&](double x) { return Point(x, 0.0, rise * (1.0 - std::abs(x - 0.5 * span) / (0.5 * span))); };
    const double slope = rise / (0.5 * span);

    // the chords: the bottom chord straight, the top chord up to the ridge and down
    const std::vector<Polyline> chords = {
        Polyline({Point(0.0, 0.0, 0.0), Point(span, 0.0, 0.0)}),
        Polyline({top(0.0), top(0.5 * span), top(span)}),
    };

    // per side, strut k from top node k square down to the bottom chord, tie k from its foot up to top node k + 1
    std::vector<Line> webs;
    for (const double side : {0.0, 1.0}) {
        for (int k = 1; k <= 2; k++) {
            const double node = k * span / 6.0;
            const double foot = node + top(node)[2] * slope;
            const double next = (k + 1) * span / 6.0;
            const double mirror = side == 0.0 ? 1.0 : -1.0;
            const double origin = side == 0.0 ? 0.0 : span;
            webs.push_back(Line::from_points(top(origin + mirror * node), Point(origin + mirror * foot, 0.0, 0.0)));
            webs.push_back(Line::from_points(Point(origin + mirror * foot, 0.0, 0.0), top(origin + mirror * next)));
        }
    }

    return TrussGuide(chords, webs);
}

TrussGuide TrussGuide::scissors(double span, double rise) {

    // the chords: the bottom chord up to a third of the rise at the middle, the top chord up to the ridge
    const Point kink(0.5 * span, 0.0, rise / 3.0);
    const Point ridge(0.5 * span, 0.0, rise);
    const std::vector<Polyline> chords = {
        Polyline({Point(0.0, 0.0, 0.0), kink, Point(span, 0.0, 0.0)}),
        Polyline({Point(0.0, 0.0, 0.0), ridge, Point(span, 0.0, 0.0)}),
    };

    // the king post, and per side a vertical at the quarter point and a strut from the kink up to its top
    std::vector<Line> webs = {Line::from_points(kink, ridge)};
    for (const double x : {0.25 * span, 0.75 * span}) {
        const Point upper(x, 0.0, 0.5 * rise);
        webs.push_back(Line::from_points(Point(x, 0.0, 0.5 * kink[2]), upper));
        webs.push_back(Line::from_points(kink, upper));
    }

    return TrussGuide(chords, webs);
}

TrussGuide TrussGuide::warren(double span, double height, int panels) {

    const double panel = span / panels;

    // the chords: the bottom chord over the span, the top chord between the first and the last top node
    const std::vector<Polyline> chords = {
        Polyline({Point(0.0, 0.0, 0.0), Point(span, 0.0, 0.0)}),
        Polyline({Point(0.5 * panel, 0.0, height), Point(span - 0.5 * panel, 0.0, height)}),
    };

    // the diagonals up from bottom node i to top node i and down to bottom node i + 1
    std::vector<Line> webs;
    for (int i = 0; i < panels; i++) {
        const Point upper((i + 0.5) * panel, 0.0, height);
        webs.push_back(Line::from_points(Point(i * panel, 0.0, 0.0), upper));
        webs.push_back(Line::from_points(upper, Point((i + 1) * panel, 0.0, 0.0)));
    }

    return TrussGuide(chords, webs);
}

TrussGuide TrussGuide::warren_verticals(double span, double height, int panels) {

    TrussGuide guide = warren(span, height, panels);
    const double panel = span / panels;

    // a vertical at every top node and every inner bottom node
    for (int i = 0; i < panels; i++)
        guide.webs.push_back(Line::from_points(Point((i + 0.5) * panel, 0.0, 0.0), Point((i + 0.5) * panel, 0.0, height)));
    for (int i = 1; i < panels; i++)
        guide.webs.push_back(Line::from_points(Point(i * panel, 0.0, 0.0), Point(i * panel, 0.0, height)));

    return guide;
}

TrussGuide TrussGuide::pratt_bridge(double span, double height, int panels) {

    const double panel = span / panels;
    const auto top = [&](int i) { return Point(i * panel, 0.0, height); };
    const auto bottom = [&](int i) { return Point(i * panel, 0.0, 0.0); };

    // the chords: the bottom chord straight, the top chord with an inclined end post down to each heel
    const std::vector<Polyline> chords = {Polyline({bottom(0), bottom(panels)}), Polyline({bottom(0), top(1), top(panels - 1), bottom(panels)})};

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

TrussGuide TrussGuide::parker() {

    const int panels = 6;
    const std::array<double, 7> heights = {0.0, 1600.0, 2200.0, 2400.0, 2200.0, 1600.0, 0.0};
    const auto top = [&](int i) { return Point(2000.0 * i, 0.0, heights[i]); };
    const auto bottom = [&](int i) { return Point(2000.0 * i, 0.0, 0.0); };

    // the chords: the bottom chord straight, the top chord through every panel point
    std::vector<Point> polygon;
    for (int i = 0; i <= panels; i++)
        polygon.push_back(top(i));
    const std::vector<Polyline> chords = {Polyline({bottom(0), bottom(panels)}), Polyline(polygon)};

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

TrussGuide TrussGuide::k_truss(double span, double height, int panels) {

    const double panel = span / panels;
    const auto top = [&](int i) { return Point(i * panel, 0.0, height); };
    const auto middle = [&](int i) { return Point(i * panel, 0.0, 0.5 * height); };
    const auto bottom = [&](int i) { return Point(i * panel, 0.0, 0.0); };

    // the chords: the bottom chord straight, the top chord with an inclined end post down to each heel
    const std::vector<Polyline> chords = {Polyline({bottom(0), bottom(panels)}), Polyline({bottom(0), top(1), top(panels - 1), bottom(panels)})};

    // a vertical at every inner panel point, whole: the Ks stop on its middle
    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));

    // the Ks: from the middle of vertical i to the top and the foot of the next one towards the middle
    for (int i = 1; i < panels / 2; i++) {
        for (const int from : {i, panels - i}) {
            const int to = from < panels / 2 ? from + 1 : from - 1;
            webs.push_back(Line::from_points(middle(from), top(to)));
            webs.push_back(Line::from_points(middle(from), bottom(to)));
        }
    }

    return TrussGuide(chords, webs);
}

TrussGuide TrussGuide::cambered_verticals() {

    const std::array<double, 9> cambers = {0.0, 0.0, 300.0, 600.0, 900.0, 600.0, 300.0, 0.0, 0.0};

    // the chords: the bottom chord through every panel point, the top chord up to the ridge and down
    std::vector<Point> bottom;
    for (int i = 0; i <= 8; i++)
        bottom.emplace_back(1500.0 * i, 0.0, cambers[i]);
    const std::vector<Polyline> chords = {Polyline(bottom), Polyline({Point(0.0, 0.0, 0.0), Point(6000.0, 0.0, 2700.0), Point(12000.0, 0.0, 0.0)})};

    // a vertical at every inner panel point, up to the top chord
    std::vector<Line> webs;
    for (int i = 1; i < 8; i++)
        webs.push_back(Line::from_points(bottom[i], Point(bottom[i][0], 0.0, 2700.0 * (1.0 - std::abs(bottom[i][0] - 6000.0) / 6000.0))));

    return TrussGuide(chords, webs);
}

TrussGuide TrussGuide::funicular_arch() {

    // the chords: the tie through its nodes, the arch with a level crown
    const std::vector<Point> tie = {{0.0, 0.0, 0.0}, {2400.0, 0.0, 0.0}, {4800.0, 0.0, 0.0}, {7200.0, 0.0, 0.0}, {9600.0, 0.0, 0.0}, {12000.0, 0.0, 0.0}};
    const std::vector<Point> arch = {{0.0, 0.0, 0.0}, {1500.0, 0.0, 1300.0}, {4000.0, 0.0, 2200.0}, {8000.0, 0.0, 2200.0}, {10500.0, 0.0, 1300.0}, {12000.0, 0.0, 0.0}};
    const std::vector<Polyline> chords = {Polyline(tie), Polyline(arch)};

    // inclined webs from the tie's inner nodes to the nearest arch nodes
    const std::vector<Line> webs = {
        Line::from_points(tie[1], arch[1]),
        Line::from_points(tie[1], arch[2]),
        Line::from_points(tie[2], arch[2]),
        Line::from_points(tie[3], arch[3]),
        Line::from_points(tie[4], arch[3]),
        Line::from_points(tie[4], arch[4]),
    };

    return TrussGuide(chords, webs);
}

TrussGuide TrussGuide::lens() {

    // the chords: the sagging polygon and the arch, meeting at the heels
    const std::vector<Point> bottom = {{0.0, 0.0, 0.0}, {2000.0, 0.0, -700.0}, {4000.0, 0.0, -1100.0}, {8000.0, 0.0, -1100.0}, {10000.0, 0.0, -700.0}, {12000.0, 0.0, 0.0}};
    const std::vector<Point> top = {{0.0, 0.0, 0.0}, {2000.0, 0.0, 1100.0}, {4000.0, 0.0, 1600.0}, {8000.0, 0.0, 1600.0}, {10000.0, 0.0, 1100.0}, {12000.0, 0.0, 0.0}};
    const std::vector<Polyline> chords = {Polyline(bottom), Polyline(top)};

    // a hanger between every two inner nodes
    std::vector<Line> webs;
    for (size_t i = 1; i + 1 < top.size(); i++)
        webs.push_back(Line::from_points(bottom[i], top[i]));

    return TrussGuide(chords, webs);
}

TrussGuide TrussGuide::triangulated_arch() {

    // the chords: the outer and the inner polygon from the same two supports
    const std::vector<Point> outer = {{0.0, 0.0, 0.0}, {800.0, 0.0, 3000.0}, {2800.0, 0.0, 5000.0}, {6000.0, 0.0, 5800.0}, {9200.0, 0.0, 5000.0}, {11200.0, 0.0, 3000.0}, {12000.0, 0.0, 0.0}};
    const std::vector<Point> inner = {{0.0, 0.0, 0.0}, {1100.0, 0.0, 2600.0}, {3000.0, 0.0, 4300.0}, {6000.0, 0.0, 5000.0}, {9000.0, 0.0, 4300.0}, {10900.0, 0.0, 2600.0}, {12000.0, 0.0, 0.0}};
    const std::vector<Polyline> chords = {Polyline(outer), Polyline(inner)};

    // webs zigzagging from each inner node to its outer neighbours, the vertical at the crown
    const std::vector<Line> webs = {
        Line::from_points(inner[1], outer[1]),
        Line::from_points(inner[1], outer[2]),
        Line::from_points(inner[2], outer[2]),
        Line::from_points(inner[2], outer[3]),
        Line::from_points(inner[3], outer[3]),
        Line::from_points(inner[4], outer[3]),
        Line::from_points(inner[4], outer[4]),
        Line::from_points(inner[5], outer[4]),
        Line::from_points(inner[5], outer[5]),
    };

    return TrussGuide(chords, webs);
}

TrussGuide TrussGuide::sloped() {

    const std::array<double, 7> top_heights = {0.0, 1600.0, 2200.0, 2200.0, 2800.0, 3400.0, 4000.0};
    const std::array<double, 7> bottom_heights = {0.0, 400.0, 1000.0, 1000.0, 1600.0, 2200.0, 2800.0};

    // the chords: both through every panel point, meeting at the heel on the left
    std::vector<Point> top;
    std::vector<Point> bottom;
    for (int i = 0; i <= 6; i++) {
        top.emplace_back(2000.0 * i, 0.0, top_heights[i]);
        bottom.emplace_back(2000.0 * i, 0.0, bottom_heights[i]);
    }
    const std::vector<Polyline> chords = {Polyline(bottom), Polyline(top)};

    // a vertical at every inner node, one diagonal per panel falling to the right
    std::vector<Line> webs;
    for (int i = 1; i <= 5; i++)
        webs.push_back(Line::from_points(bottom[i], top[i]));
    for (int i = 1; i <= 5; i++)
        webs.push_back(Line::from_points(top[i], bottom[i + 1]));

    // the end post on the right
    const std::vector<Line> posts = {Line::from_points(bottom[6], top[6])};

    return TrussGuide(chords, webs, posts);
}

/// A Pratt on end posts: the top chord through `heights` at the panel points, the bottom chord straight, a vertical at every inner panel point
/// and the diagonals falling towards the middle.
static TrussGuide posted_pratt(const std::vector<double>& heights, double panel) {

    const int panels = static_cast<int>(heights.size()) - 1;
    const auto top = [&](int i) { return Point(i * panel, 0.0, heights[i]); };
    const auto bottom = [&](int i) { return Point(i * panel, 0.0, 0.0); };

    // the chords: the bottom chord straight, the top chord through every panel point
    std::vector<Point> polygon;
    for (int i = 0; i <= panels; i++)
        polygon.push_back(top(i));
    const std::vector<Polyline> chords = {Polyline({bottom(0), bottom(panels)}), Polyline(polygon)};

    // a vertical at every inner panel point, the diagonals from the top of each, the posts' too, falling towards the middle
    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int i = 0; i < panels / 2; i++) {
        webs.push_back(Line::from_points(top(i), bottom(i + 1)));
        webs.push_back(Line::from_points(top(panels - i), bottom(panels - i - 1)));
    }

    // the end posts
    const std::vector<Line> posts = {Line::from_points(bottom(0), top(0)), Line::from_points(bottom(panels), top(panels))};

    return TrussGuide(chords, webs, posts);
}

TrussGuide TrussGuide::pratt_flat(double span, double height, int panels) {

    return posted_pratt(std::vector<double>(panels + 1, height), span / panels);
}

TrussGuide TrussGuide::flat_peaked() {

    const std::array<double, 7> heights = {2000.0, 2000.0, 2000.0, 2000.0, 3200.0, 2000.0, 2000.0};
    const auto top = [&](int i) { return Point(2000.0 * i, 0.0, heights[i]); };
    const auto bottom = [&](int i) { return Point(2000.0 * i, 0.0, 0.0); };

    // the chords: the bottom chord straight, the top chord through every panel point, up to the peak at 8000
    std::vector<Point> polygon;
    for (int i = 0; i <= 6; i++)
        polygon.push_back(top(i));
    const std::vector<Polyline> chords = {Polyline({bottom(0), bottom(6)}), Polyline(polygon)};

    // a vertical at every inner panel point; diagonals falling to the right, but for the two from the peak down to its neighbours
    std::vector<Line> webs;
    for (int i = 1; i <= 5; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int i = 0; i < 3; i++)
        webs.push_back(Line::from_points(top(i), bottom(i + 1)));
    webs.push_back(Line::from_points(top(4), bottom(3)));
    webs.push_back(Line::from_points(top(4), bottom(5)));
    webs.push_back(Line::from_points(top(5), bottom(6)));

    // the end posts
    const std::vector<Line> posts = {Line::from_points(bottom(0), top(0)), Line::from_points(bottom(6), top(6))};

    return TrussGuide(chords, webs, posts);
}

TrussGuide TrussGuide::bowstring() {

    return posted_pratt({1500.0, 2100.0, 2500.0, 2650.0, 2500.0, 2100.0, 1500.0}, 2000.0);
}

TrussGuide TrussGuide::pitched_posts(double span, double post, double ridge, int panels) {

    std::vector<double> heights;
    for (int i = 0; i <= panels; i++)
        heights.push_back(post + (ridge - post) * (1.0 - std::abs(i - 0.5 * panels) / (0.5 * panels)));

    return posted_pratt(heights, span / panels);
}

} // namespace wood_truss
