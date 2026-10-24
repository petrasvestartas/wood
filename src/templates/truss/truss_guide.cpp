#include "pch.h"
#include "src/templates/truss/truss_guide.h"

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// TrussGuide
// ═══════════════════════════════════════════════════════════════════════════
TrussGuide::TrussGuide(
    const std::vector<Polyline>& chords,
    const std::vector<Line>& webs,
    const std::vector<Line>& posts,
    double chord_radius,
    double web_radius
)
    : chords(chords), webs(webs), posts(posts), chord_radius(chord_radius), web_radius(web_radius) {
}

std::vector<std::array<Point, 2>> TrussGuide::runs(size_t chord) const {

    const std::vector<Point> nodes = chords.at(chord).get_points();
    std::vector<std::array<Point, 2>> runs;
    Point start = nodes.front();
    for (size_t i = 1; i < nodes.size(); i++) {
        const bool last = i + 1 == nodes.size();
        const bool kink = !last && (nodes[i] - start)
                                           .normalized()
                                           .cross((nodes[i + 1] - nodes[i]).normalized())
                                           .magnitude() > 1e-6;
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

/// The point mirrored across the middle of the span.
static Point mirror(const Point& point, double span) {

    return Point(span - point[0], point[1], point[2]);
}

/// A polyline's nodes from its left half, the middle node last when there is one, mirrored to the right in reverse.
static std::vector<Point> symmetric(const std::vector<Point>& half, double span) {

    std::vector<Point> nodes = half;
    for (size_t i = half.size(); i-- > 0;)
        if (std::abs(half[i][0] - 0.5 * span) > 1e-9)
            nodes.push_back(mirror(half[i], span));

    return nodes;
}

/// Nodes from their measured fractions of the span along x and of the height along z.
static std::vector<Point>
scaled(const std::vector<std::array<double, 2>>& fractions, double span, double height) {

    std::vector<Point> nodes;
    for (const std::array<double, 2>& fraction : fractions)
        nodes.emplace_back(fraction[0] * span, 0.0, fraction[1] * height);

    return nodes;
}

TrussGuide
TrussGuide::pratt(double span, double rise, int panels, double chord_radius, double web_radius) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(
            i * panel, 0.0, rise * (1.0 - std::abs(i * panel - 0.5 * span) / (0.5 * span))
        );
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };

    // the chords: the bottom chord straight, the top chord up to the ridge and down
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(panels)}), Polyline({top(0), top(panels / 2), top(panels)})
    };

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int i = 1; i < panels / 2; i++) {
        webs.push_back(Line::from_points(top(i), bottom(i + 1)));
        webs.push_back(Line::from_points(top(panels - i), bottom(panels - i - 1)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::king_post(double span, double rise, double chord_radius, double web_radius) {

    // the chords: the bottom chord straight, the top chord up to the ridge and down
    const Point foot(0.5 * span, 0.0, 0.0);
    const Point ridge(0.5 * span, 0.0, rise);
    const std::vector<Polyline> chords = {
        Polyline({Point(0.0, 0.0, 0.0), Point(span, 0.0, 0.0)}),
        Polyline({Point(0.0, 0.0, 0.0), ridge, Point(span, 0.0, 0.0)}),
    };

    const std::vector<Line> webs = {
        Line::from_points(foot, ridge),
        Line::from_points(foot, Point(0.25 * span, 0.0, 0.5 * rise)),
        Line::from_points(foot, Point(0.75 * span, 0.0, 0.5 * rise)),
    };

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide
TrussGuide::queen_post(double span, double height, double chord_radius, double web_radius) {

    // the chords: the bottom chord straight, the top chord inclined to the third points and level between
    const Point left(span / 3.0, 0.0, height);
    const Point right(2.0 * span / 3.0, 0.0, height);
    const std::vector<Polyline> chords = {
        Polyline({Point(0.0, 0.0, 0.0), Point(span, 0.0, 0.0)}),
        Polyline({Point(0.0, 0.0, 0.0), left, right, Point(span, 0.0, 0.0)}),
    };

    const std::vector<Line> webs = {
        Line::from_points(Point(left[0], 0.0, 0.0), left),
        Line::from_points(Point(right[0], 0.0, 0.0), right),
    };

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide
TrussGuide::howe(double span, double rise, int panels, double chord_radius, double web_radius) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(
            i * panel, 0.0, rise * (1.0 - std::abs(i * panel - 0.5 * span) / (0.5 * span))
        );
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };

    // the chords: the bottom chord straight, the top chord up to the ridge and down
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(panels)}), Polyline({top(0), top(panels / 2), top(panels)})
    };

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int i = 1; i < panels / 2; i++) {
        webs.push_back(Line::from_points(bottom(i), top(i + 1)));
        webs.push_back(Line::from_points(bottom(panels - i), top(panels - i - 1)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::fink(double span, double rise, double chord_radius, double web_radius) {

    const auto top = [&](double x) {
        return Point(x, 0.0, rise * (1.0 - std::abs(x - 0.5 * span) / (0.5 * span)));
    };

    // the chords: the bottom chord straight, the top chord up to the ridge and down
    const std::vector<Polyline> chords = {
        Polyline({Point(0.0, 0.0, 0.0), Point(span, 0.0, 0.0)}),
        Polyline({top(0.0), top(0.5 * span), top(span)}),
    };

    std::vector<Line> webs;
    for (const double side : {0.0, 1.0}) {
        const double third = side == 0.0 ? span / 3.0 : 2.0 * span / 3.0;
        const double quarter = side == 0.0 ? 0.25 * span : 0.75 * span;
        webs.push_back(Line::from_points(Point(third, 0.0, 0.0), top(quarter)));
        webs.push_back(Line::from_points(Point(third, 0.0, 0.0), top(0.5 * span)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::belgian(
    double span, double rise, int divisions, double chord_radius, double web_radius
) {

    const auto top = [&](double x) {
        return Point(x, 0.0, rise * (1.0 - std::abs(x - 0.5 * span) / (0.5 * span)));
    };
    const double slope = rise / (0.5 * span);

    // the chords: the bottom chord straight, the top chord up to the ridge and down
    const std::vector<Polyline> chords = {
        Polyline({Point(0.0, 0.0, 0.0), Point(span, 0.0, 0.0)}),
        Polyline({top(0.0), top(0.5 * span), top(span)}),
    };

    std::vector<Line> webs;
    for (int k = 1; k < divisions; k++) {
        const double node = 0.5 * span * k / divisions;
        const double foot = node + top(node)[2] * slope;
        const double next = 0.5 * span * (k + 1) / divisions;
        const Line strut = Line::from_points(top(node), Point(foot, 0.0, 0.0));
        const Line tie = Line::from_points(Point(foot, 0.0, 0.0), top(next));
        for (const Line& web : {strut, tie}) {
            webs.push_back(web);
            webs.push_back(Line::from_points(mirror(web.start(), span), mirror(web.end(), span)));
        }
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::scissors(double span, double rise, double chord_radius, double web_radius) {

    // the chords: the bottom chord up to a third of the rise at the middle, the top chord up to the ridge
    const Point kink(0.5 * span, 0.0, rise / 3.0);
    const Point ridge(0.5 * span, 0.0, rise);
    const std::vector<Polyline> chords = {
        Polyline({Point(0.0, 0.0, 0.0), kink, Point(span, 0.0, 0.0)}),
        Polyline({Point(0.0, 0.0, 0.0), ridge, Point(span, 0.0, 0.0)}),
    };

    std::vector<Line> webs = {Line::from_points(kink, ridge)};
    for (const double x : {0.25 * span, 0.75 * span}) {
        const Point upper(x, 0.0, 0.5 * rise);
        webs.push_back(Line::from_points(Point(x, 0.0, 0.5 * kink[2]), upper));
        webs.push_back(Line::from_points(kink, upper));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide
TrussGuide::warren(double span, double height, int panels, double chord_radius, double web_radius) {

    const double panel = span / panels;

    // the chords: the bottom chord over the span, the top chord between the first and the last top node
    const std::vector<Polyline> chords = {
        Polyline({Point(0.0, 0.0, 0.0), Point(span, 0.0, 0.0)}),
        Polyline({Point(0.5 * panel, 0.0, height), Point(span - 0.5 * panel, 0.0, height)}),
    };

    std::vector<Line> webs;
    for (int i = 0; i < panels; i++) {
        const Point upper((i + 0.5) * panel, 0.0, height);
        webs.push_back(Line::from_points(Point(i * panel, 0.0, 0.0), upper));
        webs.push_back(Line::from_points(upper, Point((i + 1) * panel, 0.0, 0.0)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::warren_verticals(
    double span, double height, int panels, double chord_radius, double web_radius
) {

    TrussGuide guide = warren(span, height, panels, chord_radius, web_radius);
    const double panel = span / panels;

    for (int i = 0; i < panels; i++)
        guide.webs.push_back(Line::from_points(
            Point((i + 0.5) * panel, 0.0, 0.0), Point((i + 0.5) * panel, 0.0, height)
        ));
    for (int i = 1; i < panels; i++)
        guide.webs.push_back(
            Line::from_points(Point(i * panel, 0.0, 0.0), Point(i * panel, 0.0, height))
        );

    return guide;
}

/// A Pratt bridge's webs between a straight bottom chord and the top nodes `top`.
static std::vector<Line> pratt_webs(const std::vector<Point>& top) {

    const int panels = static_cast<int>(top.size()) - 1;
    const auto bottom = [&](int i) {
        return Point(top[i][0], 0.0, 0.0);
    };

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top[i]));

    for (int i = 1; 2 * i < panels; i++) {
        webs.push_back(Line::from_points(top[i], bottom(i + 1)));
        webs.push_back(Line::from_points(top[panels - i], bottom(panels - i - 1)));
    }

    return webs;
}

TrussGuide TrussGuide::pratt_bridge(
    double span, double height, int panels, double chord_radius, double web_radius
) {

    // the top nodes: the heels and the panel points at the height
    std::vector<Point> top;
    for (int i = 0; i <= panels; i++)
        top.emplace_back(i * span / panels, 0.0, i == 0 || i == panels ? 0.0 : height);

    // the chords: the bottom chord straight, the top chord with an inclined end post down to each heel
    const std::vector<Polyline> chords = {Polyline({top.front(), top.back()}), Polyline(top)};

    return TrussGuide(chords, pratt_webs(top), {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::parker(
    double span, double height, double hip, int panels, double chord_radius, double web_radius
) {

    // the top nodes: the heels, then a parabola from the hip at the first panel point to the height at the middle
    std::vector<Point> top;
    for (int i = 0; i <= panels; i++) {
        const int from_end = std::min(i, panels - i);
        const double t = panels > 4 ? (from_end - 1.0) / (0.5 * panels - 1.0) : 1.0;
        const double z = from_end == 0 ? 0.0 : hip + (height - hip) * (1.0 - (1.0 - t) * (1.0 - t));
        top.emplace_back(i * span / panels, 0.0, z);
    }

    // the chords: the bottom chord straight, the top chord through every panel point
    const std::vector<Polyline> chords = {Polyline({top.front(), top.back()}), Polyline(top)};

    return TrussGuide(chords, pratt_webs(top), {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::k_truss(
    double span, double height, int panels, double chord_radius, double web_radius
) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(i * panel, 0.0, height);
    };
    const auto middle = [&](int i) {
        return Point(i * panel, 0.0, 0.5 * height);
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };

    // the chords: the bottom chord straight, the top chord with an inclined end post down to each heel
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(panels)}),
        Polyline({bottom(0), top(1), top(panels - 1), bottom(panels)})
    };

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

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::cambered_verticals(
    double span, double height, double camber, int panels, double chord_radius, double web_radius
) {

    // the bottom nodes: flat over the end panels, then up to the camber at the middle on the measured curve, 0.35 t + 0.65 t^2
    const double panel = span / panels;
    std::vector<Point> bottom;
    for (int i = 0; i <= panels; i++) {
        const int from_end = std::min(i, panels - i);
        const double t = from_end <= 1 ? 0.0 : (from_end - 1.0) / (0.5 * panels - 1.0);
        bottom.emplace_back(i * panel, 0.0, camber * (0.35 * t + 0.65 * t * t));
    }

    // the chords: the cambered bottom chord through every panel point, the top chord up to the ridge and down
    const std::vector<Polyline> chords = {
        Polyline(bottom),
        Polyline({Point(0.0, 0.0, 0.0), Point(0.5 * span, 0.0, height), Point(span, 0.0, 0.0)})
    };

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(
            bottom[i],
            Point(
                bottom[i][0],
                0.0,
                height * (1.0 - std::abs(bottom[i][0] - 0.5 * span) / (0.5 * span))
            )
        ));

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide
TrussGuide::funicular_arch(double span, double height, double chord_radius, double web_radius) {

    // the chords: the tie through its five panel points, the arch with a level crown at the measured fractions
    std::vector<Point> tie;
    for (int i = 0; i <= 5; i++)
        tie.emplace_back(i * span / 5.0, 0.0, 0.0);
    const std::vector<Point> arch =
        symmetric(scaled({{0.0, 0.0}, {0.130833, 0.5875}, {0.365833, 1.0}}, span, height), span);
    const std::vector<Polyline> chords = {Polyline(tie), Polyline(arch)};

    std::vector<Line> webs;
    for (int i = 1; i <= 4; i++)
        webs.push_back(Line::from_points(tie[i], arch[i]));

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide
TrussGuide::lens(double span, double height, double sag, double chord_radius, double web_radius) {

    // the chords: the sagging polygon and the arch, meeting at the heels, at the measured fractions
    const std::vector<Point> bottom =
        symmetric(scaled({{0.0, 0.0}, {0.2, -0.674603}, {0.4, -1.0}}, span, sag), span);
    const std::vector<Point> top =
        symmetric(scaled({{0.0, 0.0}, {0.166667, 0.614035}, {0.383333, 1.0}}, span, height), span);
    const std::vector<Polyline> chords = {Polyline(bottom), Polyline(top)};

    std::vector<Line> webs;
    for (size_t i = 1; i + 1 < top.size(); i++)
        webs.push_back(Line::from_points(bottom[i], top[i]));

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide
TrussGuide::triangulated_arch(double span, double height, double chord_radius, double web_radius) {

    // the chords: the outer and the inner polygon from the same two supports, at the measured fractions
    const std::vector<Point> outer = symmetric(
        scaled({{0.0, 0.0}, {0.066667, 0.5}, {0.25, 0.867893}, {0.5, 1.0}}, span, height), span
    );
    const std::vector<Point> inner = symmetric(
        scaled(
            {{0.0, 0.0}, {0.139167, 0.41806}, {0.291667, 0.72408}, {0.5, 0.834448}}, span, height
        ),
        span
    );
    const std::vector<Polyline> chords = {Polyline(outer), Polyline(inner)};

    const std::vector<Line> webs = {
        Line::from_points(outer[1], inner[1]),
        Line::from_points(inner[1], outer[2]),
        Line::from_points(outer[2], inner[2]),
        Line::from_points(inner[2], outer[3]),
        Line::from_points(outer[3], inner[3]),
        Line::from_points(outer[3], inner[4]),
        Line::from_points(inner[4], outer[4]),
        Line::from_points(outer[4], inner[5]),
        Line::from_points(inner[5], outer[5]),
    };

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::sloped(double span, double height, double chord_radius, double web_radius) {

    const std::array<int, 8> top_steps = {0, 3, 4, 5, 5, 6, 7, 6};
    const std::array<int, 8> bottom_steps = {0, 1, 2, 3, 3, 4, 5, 6};
    std::vector<Point> top;
    std::vector<Point> bottom;
    for (int i = 0; i <= 7; i++) {
        top.emplace_back(i * span / 7.0, 0.0, top_steps[i] * height / 7.0);
        bottom.emplace_back(i * span / 7.0, 0.0, bottom_steps[i] * height / 7.0);
    }
    const std::vector<Polyline> chords = {Polyline(bottom), Polyline(top)};

    std::vector<Line> webs;
    for (int i = 1; i <= 6; i++)
        webs.push_back(Line::from_points(bottom[i], top[i]));
    webs.push_back(Line::from_points(bottom[1], top[2]));
    webs.push_back(Line::from_points(bottom[2], top[3]));
    webs.push_back(Line::from_points(top[4], bottom[5]));
    webs.push_back(Line::from_points(top[5], bottom[6]));

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

/// A Pratt on end posts.
static TrussGuide posted_pratt(
    const std::vector<double>& heights, double panel, double chord_radius, double web_radius
) {

    const int panels = static_cast<int>(heights.size()) - 1;
    const auto top = [&](int i) {
        return Point(i * panel, 0.0, heights[i]);
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };

    // the chords: the bottom chord straight, the top chord through every panel point
    std::vector<Point> polygon;
    for (int i = 0; i <= panels; i++)
        polygon.push_back(top(i));
    const std::vector<Polyline> chords = {Polyline({bottom(0), bottom(panels)}), Polyline(polygon)};

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int i = 0; i < panels / 2; i++) {
        webs.push_back(Line::from_points(top(i), bottom(i + 1)));
        webs.push_back(Line::from_points(top(panels - i), bottom(panels - i - 1)));
    }

    const std::vector<Line> posts = {
        Line::from_points(bottom(0), top(0)), Line::from_points(bottom(panels), top(panels))
    };

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::pratt_flat(
    double span, double height, int panels, double chord_radius, double web_radius
) {

    return posted_pratt(
        std::vector<double>(panels + 1, height), span / panels, chord_radius, web_radius
    );
}

TrussGuide TrussGuide::flat_peaked(
    double span, double height, double peak, double chord_radius, double web_radius
) {

    const std::array<double, 7> heights = {height, height, height, height, peak, height, height};
    const auto top = [&](int i) {
        return Point(span * i / 6.0, 0.0, heights[i]);
    };
    const auto bottom = [&](int i) {
        return Point(span * i / 6.0, 0.0, 0.0);
    };

    // the chords: the bottom chord straight, the top chord through every panel point, up to the peak
    std::vector<Point> polygon;
    for (int i = 0; i <= 6; i++)
        polygon.push_back(top(i));
    const std::vector<Polyline> chords = {Polyline({bottom(0), bottom(6)}), Polyline(polygon)};

    std::vector<Line> webs;
    for (int i = 1; i <= 5; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int i = 0; i < 3; i++)
        webs.push_back(Line::from_points(top(i), bottom(i + 1)));
    webs.push_back(Line::from_points(bottom(3), top(4)));
    webs.push_back(Line::from_points(bottom(4), top(5)));
    webs.push_back(Line::from_points(top(6), bottom(5)));

    const std::vector<Line> posts = {
        Line::from_points(bottom(0), top(0)), Line::from_points(bottom(6), top(6))
    };

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::bowstring(
    double span, double height, double crown, int panels, double chord_radius, double web_radius
) {

    std::vector<double> heights;
    for (int i = 0; i <= panels; i++) {
        const double u = (i - 0.5 * panels) / (0.5 * panels);
        heights.push_back(height + (crown - height) * (1.0 - u * u));
    }

    return posted_pratt(heights, span / panels, chord_radius, web_radius);
}

TrussGuide TrussGuide::pitched_posts(
    double span, double height, double ridge, int panels, double chord_radius, double web_radius
) {

    std::vector<double> heights;
    for (int i = 0; i <= panels; i++)
        heights.push_back(
            height + (ridge - height) * (1.0 - std::abs(i - 0.5 * panels) / (0.5 * panels))
        );

    return posted_pratt(heights, span / panels, chord_radius, web_radius);
}

TrussGuide TrussGuide::roof_cambered_tie(
    double span, double rise, double camber, double dip, double chord_radius, double web_radius
) {

    const std::array<double, 2> loads = {0.195, 0.3575};
    const auto top = [&](double x) {
        return Point(x, 0.0, rise * (1.0 - std::abs(x - 0.5 * span) / (0.5 * span)));
    };

    // the chords: the tie dipping at the first load point and rising to the camber at the middle, the rafters up to the ridge
    const std::vector<Point> tie = symmetric(
        {Point(0.0, 0.0, 0.0),
         Point(loads[0] * span, 0.0, -dip),
         Point(loads[1] * span, 0.0, 0.22 * camber),
         Point(0.5 * span, 0.0, camber)},
        span
    );
    const std::vector<Polyline> chords = {
        Polyline(tie), Polyline({top(0.0), top(0.5 * span), top(span)})
    };

    std::vector<Line> webs;
    for (size_t i = 1; i + 1 < tie.size(); i++)
        webs.push_back(Line::from_points(tie[i], top(tie[i][0])));
    for (size_t i = 1; i <= 2; i++) {
        webs.push_back(Line::from_points(top(tie[i][0]), tie[i + 1]));
        webs.push_back(Line::from_points(top(tie[tie.size() - 1 - i][0]), tie[tie.size() - 2 - i]));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide
TrussGuide::quarter_fan(double span, double height, double chord_radius, double web_radius) {

    // the nodes at the measured fractions: the supports, the outer polygon, the bottom chord's and the fan's crossings
    const std::vector<Point> nodes = scaled(
        {{0.0, 1.0},
         {0.445833, 1.0},
         {0.668333, 0.820144},
         {0.8475, 0.528777},
         {0.945, 0.278777},
         {1.0, 0.0},
         {0.0, 0.0},
         {0.405, 0.0},
         {0.575833, 0.0},
         {0.376667, 0.627698},
         {0.3125, 0.494604},
         {0.4975, 0.370504},
         {0.384167, 0.233813},
         {0.568333, 0.095324}},
        span,
        height
    );
    const Point& upper = nodes[0];
    const Point& lower = nodes[6];
    const std::array<Point, 5> fan = {nodes[9], nodes[10], nodes[11], nodes[12], nodes[13]};

    // the chords: the outer polygon from the upper support to the tip, the bottom chord from the lower support
    const std::vector<Polyline> chords = {
        Polyline({nodes[0], nodes[1], nodes[2], nodes[3], nodes[4], nodes[5]}),
        Polyline({lower, nodes[7], nodes[8], nodes[5]})
    };

    std::vector<Line> webs = {
        Line::from_points(upper, fan[0]),
        Line::from_points(fan[0], fan[2]),
        Line::from_points(fan[2], fan[4]),
        Line::from_points(fan[4], nodes[8]),
        Line::from_points(upper, fan[1]),
        Line::from_points(fan[1], fan[3]),
        Line::from_points(fan[3], nodes[7]),
    };

    const std::vector<Line> rising = {
        Line::from_points(lower, fan[1]),
        Line::from_points(fan[1], fan[0]),
        Line::from_points(fan[0], nodes[1]),
        Line::from_points(lower, fan[3]),
        Line::from_points(fan[3], fan[2]),
        Line::from_points(fan[2], nodes[2]),
        Line::from_points(nodes[7], fan[4]),
        Line::from_points(fan[4], nodes[3]),
        Line::from_points(nodes[8], nodes[4]),
    };
    webs.insert(webs.end(), rising.begin(), rising.end());

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide
TrussGuide::cantilever(double span, double height, double chord_radius, double web_radius) {

    const std::vector<double> top = {0.0, 0.166667, 0.335833, 0.501667, 0.6725, 0.8375, 1.0};
    const std::vector<std::array<double, 2>> bottom = {
        {0.0, 0.0},
        {0.131667, 0.147541},
        {0.2975, 0.114754},
        {0.433333, 0.098361},
        {0.625833, 0.202186},
        {0.806667, 0.497268}
    };

    // the chords: the top chord straight at the height, the bottom chord from the lower support up to the tip
    std::vector<Point> upper;
    for (const double fraction : top)
        upper.emplace_back(fraction * span, 0.0, height);
    std::vector<Point> lower = scaled(bottom, span, height);
    lower.push_back(upper.back());
    const std::vector<Polyline> chords = {Polyline(upper), Polyline(lower)};

    // the webs zigzagging from the upper support: down to each bottom node and up to the next top node
    std::vector<Line> webs;
    for (size_t i = 1; i < bottom.size(); i++) {
        webs.push_back(Line::from_points(upper[i - 1], lower[i]));
        webs.push_back(Line::from_points(lower[i], upper[i]));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::cantilever_design_1(
    double span, double height, double chord_radius, double web_radius
) {

    // the nodes in cm from the drawn side lengths and angles: A over B 5 apart, E equilateral on them, D 9.8481 from A at 20 degrees up, F its
    const double pi = Tolerance::PI;
    const double x = span / 29.718;
    const double z = height / 5.0;
    const Point a(0.0, 0.0, 5.0 * z);
    const Point b(0.0, 0.0, 0.0);
    const Point e(5.0 * std::cos(pi / 6.0) * x, 0.0, 2.5 * z);
    const Point d(9.8481 * std::cos(pi / 9.0) * x, 0.0, (5.0 + 9.8481 * std::sin(pi / 9.0)) * z);
    const Point f(d[0], 0.0, 5.0 * z - d[2]);
    const Point c(29.718 * x, 0.0, 2.5 * z);

    const std::vector<Polyline> chords = {Polyline({a, d, c}), Polyline({b, f, c})};
    const std::vector<Line> webs = {
        Line::from_points(a, e),
        Line::from_points(b, e),
        Line::from_points(e, d),
        Line::from_points(e, f)
    };
    const std::vector<Line> posts = {Line::from_points(b, a)};

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::cantilever_design_2(
    double span, double height, double chord_radius, double web_radius
) {

    // the nodes in cm from the drawn dimensions: B at the wall, A 5 over it, F and G 1.7 over and under it at 9.6, D and E at A's height at 15
    const double x = span / 30.0;
    const double z = height / 5.0;
    const Point a(0.0, 0.0, 5.0 * z);
    const Point b(0.0, 0.0, 0.0);
    const Point f(9.6 * x, 0.0, 1.7 * z);
    const Point g(9.6 * x, 0.0, -1.7 * z);
    const Point d(15.0 * x, 0.0, 5.0 * z);
    const Point e(25.0 * x, 0.0, 5.0 * z);
    const Point h(20.0 * x, 0.0, -1.7 * z);
    const Point c(30.0 * x, 0.0, 0.0);

    const std::vector<Polyline> chords = {Polyline({a, d, e, c}), Polyline({b, g, h, c})};
    const std::vector<Line> webs = {
        Line::from_points(a, f),
        Line::from_points(b, f),
        Line::from_points(f, g),
        Line::from_points(f, d),
        Line::from_points(f, h),
        Line::from_points(d, h),
        Line::from_points(e, h),
    };

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::cantilever_design_3(
    double span, double height, double chord_radius, double web_radius
) {

    // the nodes in cm solved from the drawn side lengths: A at the wall foot, B 5 over it, C, D, E, F and the tip G
    const double x = span / 30.0;
    const double z = height / 5.0;
    const Point a(0.0, 0.0, 0.0);
    const Point b(0.0, 0.0, 5.0 * z);
    const Point c(8.0 * x, 0.0, 4.0 * z);
    const Point d(11.0 * x, 0.0, 0.0);
    const Point e(19.0 * x, 0.0, 11.0 * z);
    const Point f(26.0 * x, 0.0, 6.0 * z);
    const Point g(30.0 * x, 0.0, 15.0 * z);

    const std::vector<Polyline> chords = {Polyline({b, e, g}), Polyline({a, d, f, g})};
    const std::vector<Line> webs = {
        Line::from_points(b, c),
        Line::from_points(a, c),
        Line::from_points(c, d),
        Line::from_points(c, e),
        Line::from_points(d, e),
        Line::from_points(e, f),
    };
    const std::vector<Line> posts = {Line::from_points(a, b)};

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::fish_belly(
    double span, double rise, double sag, double chord_radius, double web_radius
) {

    const std::array<double, 5> sags = {0.0, 0.6, 0.965217, 1.0, 0.765217};
    std::vector<Point> half;
    for (int i = 0; i <= 4; i++)
        half.emplace_back(i * span / 8.0, 0.0, -sags[i] * sag);
    const std::vector<Point> bottom = symmetric(half, span);

    // the chords: the sagging bottom chord, the top chord up to the ridge and down
    const std::vector<Polyline> chords = {
        Polyline(bottom),
        Polyline({Point(0.0, 0.0, 0.0), Point(0.5 * span, 0.0, rise), Point(span, 0.0, 0.0)})
    };

    std::vector<Line> webs;
    for (size_t i = 1; i + 1 < bottom.size(); i++)
        webs.push_back(Line::from_points(
            bottom[i],
            Point(
                bottom[i][0], 0.0, rise * (1.0 - std::abs(bottom[i][0] - 0.5 * span) / (0.5 * span))
            )
        ));

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::sawtooth(
    double span, double height, double column, int teeth, double chord_radius, double web_radius
) {

    const double tooth = span / teeth;
    const double slope = height / (0.75 * tooth);

    // the chords: the tie over the span, per tooth a long rafter through its web nodes up to the apex at three quarters and a short one down
    std::vector<Polyline> chords = {Polyline({Point(0.0, 0.0, 0.0), Point(span, 0.0, 0.0)})};
    std::vector<Line> webs;
    for (int k = 0; k < teeth; k++) {
        const double start = k * tooth;
        const auto rafter = [&](double fraction) {
            return Point(start + fraction * tooth, 0.0, fraction * tooth * slope);
        };
        const Point apex = rafter(0.75);
        chords.push_back(Polyline(
            {Point(start, 0.0, 0.0),
             rafter(0.1875),
             rafter(0.45),
             apex,
             Point(start + tooth, 0.0, 0.0)}
        ));

        const Point first(start + 0.2675 * tooth, 0.0, 0.0);
        const Point second(start + 0.6325 * tooth, 0.0, 0.0);
        webs.push_back(Line::from_points(rafter(0.1875), first));
        webs.push_back(Line::from_points(first, rafter(0.45)));
        webs.push_back(Line::from_points(rafter(0.45), second));
        webs.push_back(Line::from_points(second, apex));
    }

    std::vector<Line> posts;
    for (int k = 1; k <= teeth; k++)
        posts.push_back(
            Line::from_points(Point(k * tooth, 0.0, -column), Point(k * tooth, 0.0, 0.0))
        );

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::howe_flat(
    double span, double height, double ridge, int panels, double chord_radius, double web_radius
) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(
            i * panel,
            0.0,
            height + (ridge - height) * (1.0 - std::abs(i - 0.5 * panels) / (0.5 * panels))
        );
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };

    // the chords: the bottom chord straight, the top chord pitched up to the ridge and down
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(panels)}), Polyline({top(0), top(panels / 2), top(panels)})
    };

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int i = 0; i < panels / 2; i++) {
        webs.push_back(Line::from_points(bottom(i), top(i + 1)));
        webs.push_back(Line::from_points(bottom(panels - i), top(panels - i - 1)));
    }

    const std::vector<Line> posts = {
        Line::from_points(bottom(0), top(0)), Line::from_points(bottom(panels), top(panels))
    };

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide
TrussGuide::three_hinged_arch(double span, double height, double chord_radius, double web_radius) {

    // the left half at the measured fractions: the outer polygon from its foot beside the hinge up to the crown, the inner one from the hinge
    const std::vector<Point> outer = scaled(
        {{-0.024167, 0.0},
         {-0.024167, 0.428994},
         {0.101667, 0.727811},
         {0.238333, 0.902367},
         {0.37, 0.988166},
         {0.5, 1.0}},
        span,
        height
    );
    const std::vector<Point> inner = scaled(
        {{0.0, 0.0}, {0.085833, 0.41716}, {0.2225, 0.727811}, {0.343333, 0.87574}, {0.5, 1.0}},
        span,
        height
    );

    const std::vector<Line> half = {
        Line::from_points(outer[0], inner[0]),
        Line::from_points(outer[1], inner[0]),
        Line::from_points(outer[1], inner[1]),
        Line::from_points(inner[1], outer[2]),
        Line::from_points(outer[2], inner[2]),
        Line::from_points(inner[2], outer[3]),
        Line::from_points(outer[3], inner[3]),
        Line::from_points(inner[3], outer[4]),
    };

    // both halves: the chords to the crown hinge, the right ones mirrored
    std::vector<Polyline> chords = {Polyline(outer), Polyline(inner)};
    std::vector<Line> webs = half;
    for (const std::vector<Point>* polygon : {&outer, &inner}) {
        std::vector<Point> mirrored;
        for (const Point& point : *polygon)
            mirrored.push_back(mirror(point, span));
        chords.push_back(Polyline(mirrored));
    }
    for (const Line& web : half)
        webs.push_back(Line::from_points(mirror(web.start(), span), mirror(web.end(), span)));

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::burr_arch(
    double span, double height, double crown, int panels, double chord_radius, double web_radius
) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(i * panel, 0.0, height);
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };
    const auto arch = [&](int i) {
        const double u = (i - 0.5 * panels) / (0.5 * panels);
        return Point(i * panel, 0.0, crown * (1.0 - u * u));
    };

    // the chords: both straight over the span
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(panels)}), Polyline({top(0), top(panels)})
    };

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int i = 0; i < panels / 2; i++) {
        webs.push_back(Line::from_points(bottom(i), top(i + 1)));
        webs.push_back(Line::from_points(bottom(panels - i), top(panels - i - 1)));
    }

    for (int i = 0; i < panels; i++)
        webs.push_back(Line::from_points(arch(i), arch(i + 1)));

    const std::vector<Line> posts = {
        Line::from_points(bottom(0), top(0)), Line::from_points(bottom(panels), top(panels))
    };

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::town_lattice(
    double span, double height, int panels, double chord_radius, double web_radius
) {

    const double pitch = span / panels;
    const auto top = [&](int i) {
        return Point(i * pitch, 0.0, height);
    };
    const auto bottom = [&](int i) {
        return Point(i * pitch, 0.0, 0.0);
    };
    const Point left(0.0, 0.0, 0.5 * height);
    const Point right(span, 0.0, 0.5 * height);

    // the chords: both straight over the span
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(panels)}), Polyline({top(0), top(panels)})
    };

    std::vector<Line> webs;
    for (int i = 0; i + 2 <= panels; i++) {
        webs.push_back(Line::from_points(top(i), bottom(i + 2)));
        webs.push_back(Line::from_points(bottom(i), top(i + 2)));
    }

    webs.push_back(Line::from_points(left, bottom(1)));
    webs.push_back(Line::from_points(left, top(1)));
    webs.push_back(Line::from_points(top(panels - 1), right));
    webs.push_back(Line::from_points(bottom(panels - 1), right));

    const std::vector<Line> posts = {
        Line::from_points(bottom(0), top(0)), Line::from_points(bottom(panels), top(panels))
    };

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::bowstring_arch(
    double span, double height, int panels, double chord_radius, double web_radius
) {

    const double panel = span / panels;
    std::vector<Point> arch;
    for (int i = 0; i <= panels; i++) {
        const double u = std::abs(i - 0.5 * panels) / (0.5 * panels);
        arch.emplace_back(i * panel, 0.0, height * (1.0 - std::pow(u, 2.5)));
    }
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };

    // the chords: the tie straight, the arch through every panel point
    const std::vector<Polyline> chords = {Polyline({bottom(0), bottom(panels)}), Polyline(arch)};

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), arch[i]));
    for (int i = 1; i + 1 < panels; i++) {
        webs.push_back(Line::from_points(arch[i], bottom(i + 1)));
        webs.push_back(Line::from_points(bottom(i), arch[i + 1]));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::waddell_a(double span, double rise, double chord_radius, double web_radius) {

    // the chords: the bottom chord straight, the rafters up to the ridge
    const Point foot(0.5 * span, 0.0, 0.0);
    const Point ridge(0.5 * span, 0.0, rise);
    const std::vector<Polyline> chords = {
        Polyline({Point(0.0, 0.0, 0.0), Point(span, 0.0, 0.0)}),
        Polyline({Point(0.0, 0.0, 0.0), ridge, Point(span, 0.0, 0.0)}),
    };

    std::vector<Line> webs = {Line::from_points(foot, ridge)};
    for (const double x : {0.25 * span, 0.75 * span}) {
        const Point middle(x, 0.0, 0.5 * rise);
        webs.push_back(Line::from_points(Point(x, 0.0, 0.0), middle));
        webs.push_back(Line::from_points(middle, foot));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::wichert(
    double span, double height, double dip, int panels, double chord_radius, double web_radius
) {

    const double step = span / panels;

    // the chords: the top chord straight, the bottom chord through every second step, dipping to the pier at the middle
    std::vector<Point> bottom;
    for (int i = 0; i <= panels; i += 2) {
        if (i > 0 && i - 1 == panels / 2)
            bottom.emplace_back(0.5 * span, 0.0, -dip);
        bottom.emplace_back(i * step, 0.0, 0.0);
    }
    const std::vector<Polyline> chords = {
        Polyline(bottom), Polyline({Point(0.0, 0.0, height), Point(span, 0.0, height)})
    };

    std::vector<Line> webs;
    for (int i = 0; i < panels; i++) {
        const bool up = i % 2 == 0;
        webs.push_back(Line::from_points(
            Point(i * step, 0.0, up ? 0.0 : height), Point((i + 1) * step, 0.0, up ? height : 0.0)
        ));
    }

    const std::vector<Line> posts = {
        Line::from_points(Point(0.0, 0.0, 0.0), Point(0.0, 0.0, height)),
        Line::from_points(Point(span, 0.0, 0.0), Point(span, 0.0, height))
    };

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::truss_leg_bedstead(
    double span, double height, double leg, int panels, double chord_radius, double web_radius
) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(i * panel, 0.0, height);
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };

    // the chords: both straight
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(panels)}), Polyline({top(0), top(panels)})
    };

    // the legs: columns from below the bottom chord, which ends on them, up to the top chord's ends
    const std::vector<Line> posts = {
        Line::from_points(Point(0.0, 0.0, -leg), top(0)),
        Line::from_points(Point(span, 0.0, -leg), top(panels))
    };

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int i = 0; i < panels / 2; i++) {
        webs.push_back(Line::from_points(top(i), bottom(i + 1)));
        webs.push_back(Line::from_points(top(panels - i), bottom(panels - i - 1)));
    }

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

/// The webs of a subdivided Pratt panel.
static void add_subdivided_panel(
    std::vector<Line>& webs,
    const Point& upper,
    const Point& lower,
    const std::optional<Point>& strut,
    const std::optional<Point>& tie
) {

    const Point middle = upper + (lower - upper) * 0.5;
    webs.push_back(Line::from_points(upper, lower));
    webs.push_back(Line::from_points(Point(middle[0], 0.0, 0.0), middle));
    if (strut)
        webs.push_back(Line::from_points(*strut, middle));
    if (tie)
        webs.push_back(Line::from_points(middle, *tie));
}

TrussGuide TrussGuide::kellogg(
    double span, double height, int panels, double chord_radius, double web_radius
) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(i * panel, 0.0, height);
    };
    const auto bottom = [&](double i) {
        return Point(i * panel, 0.0, 0.0);
    };

    // the chords: the bottom chord through every half panel point, the top chord with an inclined end post down to each heel
    std::vector<Point> lower;
    for (int i = 0; i <= 2 * panels; i++)
        lower.push_back(bottom(0.5 * i));
    const std::vector<Polyline> chords = {
        Polyline(lower), Polyline({bottom(0), top(1), top(panels - 1), bottom(panels)})
    };

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    webs.push_back(Line::from_points(top(1), bottom(0.5)));
    webs.push_back(Line::from_points(top(panels - 1), bottom(panels - 0.5)));
    for (int i = 1; i < panels / 2; i++) {
        webs.push_back(Line::from_points(top(i), bottom(i + 0.5)));
        webs.push_back(Line::from_points(top(i), bottom(i + 1)));
        webs.push_back(Line::from_points(top(panels - i), bottom(panels - i - 0.5)));
        webs.push_back(Line::from_points(top(panels - i), bottom(panels - i - 1)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

/// A Petit truss on the top nodes `top`.
static TrussGuide
petit(const std::vector<Point>& top, bool sub_struts, double chord_radius, double web_radius) {

    const int panels = static_cast<int>(top.size()) - 1;
    const auto bottom = [&](int i) {
        return Point(top[i][0], 0.0, 0.0);
    };

    // the chords: the bottom chord straight, the top chord through every panel point
    const std::vector<Polyline> chords = {Polyline({top.front(), top.back()}), Polyline(top)};

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top[i]));

    // the end panels: each end post's middle, a sub-vertical under it and a diagonal from it down to the first vertical's foot
    for (const int end : {0, panels}) {
        const int next = end == 0 ? 1 : panels - 1;
        const Point middle = top[end] + (top[next] - top[end]) * 0.5;
        webs.push_back(Line::from_points(Point(middle[0], 0.0, 0.0), middle));
        webs.push_back(Line::from_points(middle, bottom(next)));
    }

    for (int i = 1; 2 * i + 1 < panels; i++) {
        const int mirrored = panels - i;
        add_subdivided_panel(
            webs,
            top[i],
            bottom(i + 1),
            sub_struts ? std::optional<Point>(bottom(i)) : std::nullopt,
            sub_struts ? std::nullopt : std::optional<Point>(top[i + 1])
        );
        add_subdivided_panel(
            webs,
            top[mirrored],
            bottom(mirrored - 1),
            sub_struts ? std::optional<Point>(bottom(mirrored)) : std::nullopt,
            sub_struts ? std::nullopt : std::optional<Point>(top[mirrored - 1])
        );
    }

    const int middle = panels / 2;
    add_subdivided_panel(webs, top[middle], bottom(middle + 1), std::nullopt, std::nullopt);
    webs.push_back(Line::from_points(bottom(middle), top[middle + 1]));

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

/// The top nodes of a Pennsylvania.
static std::vector<Point> pennsylvania_top(double span, double height, double hip, int panels) {

    std::vector<Point> top;
    for (int i = 0; i <= panels; i++) {
        const int from_end = std::min(i, panels - i);
        const double t = (from_end - 1.0) / (0.5 * (panels - 1) - 1.0);
        const double z = from_end == 0 ? 0.0 : hip + (height - hip) * (1.0 - (1.0 - t) * (1.0 - t));
        top.emplace_back(i * span / panels, 0.0, z);
    }

    return top;
}

TrussGuide TrussGuide::baltimore_a(
    double span, double height, int panels, double chord_radius, double web_radius
) {

    return petit(pennsylvania_top(span, height, height, panels), true, chord_radius, web_radius);
}

TrussGuide TrussGuide::pennsylvania_a(
    double span, double height, double hip, int panels, double chord_radius, double web_radius
) {

    return petit(pennsylvania_top(span, height, hip, panels), true, chord_radius, web_radius);
}

TrussGuide TrussGuide::greiner(
    double span, double height, double hang, double chord_radius, double web_radius
) {

    const double panel = span / 6.0;
    const auto top = [&](int i) {
        return Point(i * panel, 0.0, height);
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };
    const Point left(2.0 * panel, 0.0, hang);
    const Point right(4.0 * panel, 0.0, hang);

    // the chords: the bottom chord straight, the top chord with an inclined end post down to each heel
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(6)}), Polyline({bottom(0), top(1), top(5), bottom(6)})
    };

    std::vector<Line> webs;
    for (int i = 1; i < 6; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));

    webs.push_back(Line::from_points(left, bottom(3)));
    webs.push_back(Line::from_points(bottom(3), right));
    webs.push_back(Line::from_points(top(1), left));
    webs.push_back(Line::from_points(left, top(3)));
    webs.push_back(Line::from_points(top(3), right));
    webs.push_back(Line::from_points(right, top(5)));

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::double_intersection_pratt(
    double span, double height, int panels, double chord_radius, double web_radius
) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(i * panel, 0.0, height);
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };

    // the chords: the bottom chord straight, the top chord with an inclined end post down to each heel
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(panels)}),
        Polyline({bottom(0), top(1), top(panels - 1), bottom(panels)})
    };

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    webs.push_back(Line::from_points(top(1), bottom(2)));
    webs.push_back(Line::from_points(top(panels - 1), bottom(panels - 2)));
    for (int i = 1; i < panels / 2; i++) {
        webs.push_back(Line::from_points(top(i), bottom(i + 2)));
        webs.push_back(Line::from_points(top(panels - i), bottom(panels - i - 2)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::schwedler(
    double span, double height, int panels, int sloped, double chord_radius, double web_radius
) {

    // the top nodes: the heels, rising as the square root over the sloped panels to the height, level between
    std::vector<Point> top;
    for (int i = 0; i <= panels; i++) {
        const int from_end = std::min(i, panels - i);
        top.emplace_back(
            i * span / panels,
            0.0,
            height * std::sqrt(std::min(1.0, static_cast<double>(from_end) / sloped))
        );
    }
    const auto bottom = [&](int i) {
        return Point(top[i][0], 0.0, 0.0);
    };
    const std::vector<Polyline> chords = {Polyline({top.front(), top.back()}), Polyline(top)};

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top[i]));
    for (int i = 1; i < sloped; i++) {
        webs.push_back(Line::from_points(top[i], bottom(i + 1)));
        webs.push_back(Line::from_points(top[panels - i], bottom(panels - i - 1)));
    }

    // the double intersection between: a single-panel diagonal from the first level node, then the diagonals across two panels
    webs.push_back(Line::from_points(top[sloped], bottom(sloped + 1)));
    webs.push_back(Line::from_points(top[panels - sloped], bottom(panels - sloped - 1)));
    for (int i = sloped; i < panels / 2; i++) {
        webs.push_back(Line::from_points(top[i], bottom(i + 2)));
        webs.push_back(Line::from_points(top[panels - i], bottom(panels - i - 2)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::double_intersection_warren(
    double span, double height, int panels, double chord_radius, double web_radius
) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(i * panel, 0.0, height);
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };

    // the chords: the bottom chord straight, the top chord with an inclined end post down to each heel
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(panels)}),
        Polyline({bottom(0), top(1), top(panels - 1), bottom(panels)})
    };

    // the hip verticals, then both Warren systems: from every top node down to the bottom nodes one panel on each side
    std::vector<Line> webs = {
        Line::from_points(bottom(1), top(1)), Line::from_points(bottom(panels - 1), top(panels - 1))
    };
    for (int i = 1; i + 1 < panels; i++) {
        webs.push_back(Line::from_points(top(i), bottom(i + 1)));
        webs.push_back(Line::from_points(top(i + 1), bottom(i)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::pegram(double span, double height, double chord_radius, double web_radius) {

    // the nodes: nine bottom panels, the top polygon's nodes between them at the measured fractions
    std::vector<Point> bottom;
    for (int i = 0; i <= 9; i++)
        bottom.emplace_back(i * span / 9.0, 0.0, 0.0);
    const std::vector<Point> top = symmetric(
        scaled(
            {{0.086235, 0.616601},
             {0.170813, 0.72332},
             {0.263682, 0.826087},
             {0.356551, 0.909091},
             {0.453566, 1.0}},
            span,
            height
        ),
        span
    );

    // the chords: the bottom chord straight, the top chord from heel to heel through its nodes
    std::vector<Point> polygon = {bottom.front()};
    polygon.insert(polygon.end(), top.begin(), top.end());
    polygon.push_back(bottom.back());
    const std::vector<Polyline> chords = {
        Polyline({bottom.front(), bottom.back()}), Polyline(polygon)
    };

    std::vector<Line> webs;
    for (int i = 0; i < 4; i++) {
        webs.push_back(Line::from_points(top[i], bottom[i + 1]));
        webs.push_back(Line::from_points(bottom[i + 1], top[i + 1]));
        webs.push_back(Line::from_points(top[9 - i], bottom[8 - i]));
        webs.push_back(Line::from_points(bottom[8 - i], top[8 - i]));
    }
    webs.push_back(Line::from_points(top[4], bottom[5]));
    webs.push_back(Line::from_points(top[5], bottom[4]));

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide
TrussGuide::post(double span, double height, int panels, double chord_radius, double web_radius) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(i * panel, 0.0, height);
    };
    const auto bottom = [&](double i) {
        return Point(std::clamp(i, 0.0, static_cast<double>(panels)) * panel, 0.0, 0.0);
    };

    // the chords: the bottom chord through the half panel points, the top chord straight
    std::vector<Point> lower = {bottom(0)};
    for (int i = 0; i < panels; i++)
        lower.push_back(bottom(i + 0.5));
    lower.push_back(bottom(panels));
    const std::vector<Polyline> chords = {Polyline(lower), Polyline({top(0), top(panels)})};

    std::vector<Line> webs;
    for (int i = 1; i <= panels / 2; i++) {
        webs.push_back(Line::from_points(bottom(i - 0.5), top(i)));
        webs.push_back(Line::from_points(bottom(panels - i + 0.5), top(panels - i)));
    }
    webs.push_back(Line::from_points(top(0), bottom(0.5)));
    webs.push_back(Line::from_points(top(panels), bottom(panels - 0.5)));
    for (int i = 0; i < panels / 2; i++) {
        webs.push_back(Line::from_points(top(i), bottom(i + 1.5)));
        webs.push_back(Line::from_points(top(panels - i), bottom(panels - i - 1.5)));
    }

    const std::vector<Line> posts = {
        Line::from_points(bottom(0), top(0)), Line::from_points(bottom(panels), top(panels))
    };

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::bollman(
    double span, double height, int panels, double chord_radius, double web_radius
) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(i * panel, 0.0, height);
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };

    // the chords: both straight over the span
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(panels)}), Polyline({top(0), top(panels)})
    };

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int i = 1; i < panels; i++) {
        webs.push_back(Line::from_points(top(0), bottom(i)));
        webs.push_back(Line::from_points(top(panels), bottom(i)));
    }

    const std::vector<Line> posts = {
        Line::from_points(bottom(0), top(0)), Line::from_points(bottom(panels), top(panels))
    };

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::fink_bridge(
    double span, double height, int panels, double chord_radius, double web_radius
) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(i * panel, 0.0, height);
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };

    // the chords: both straight over the span
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(panels)}), Polyline({top(0), top(panels)})
    };

    // a vertical at every inner panel point, then the diagonals halving: from both ends of every stretch, the whole span first, to its middle
    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int width = panels; width >= 2; width /= 2)
        for (int start = 0; start + width <= panels; start += width) {
            webs.push_back(Line::from_points(top(start), bottom(start + width / 2)));
            webs.push_back(Line::from_points(top(start + width), bottom(start + width / 2)));
        }

    const std::vector<Line> posts = {
        Line::from_points(bottom(0), top(0)), Line::from_points(bottom(panels), top(panels))
    };

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::stearns(double span, double height, double chord_radius, double web_radius) {

    const double panel = span / 6.0;
    const auto top = [&](int i) {
        return Point(i * panel, 0.0, height);
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };

    // the chords: the bottom chord straight, the top chord with an inclined end post down to each heel
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(6)}), Polyline({bottom(0), top(1), top(5), bottom(6)})
    };

    const std::vector<Line> webs = {
        Line::from_points(bottom(1), top(1)),
        Line::from_points(bottom(3), top(3)),
        Line::from_points(bottom(5), top(5)),
        Line::from_points(top(1), bottom(2)),
        Line::from_points(top(1), bottom(3)),
        Line::from_points(top(3), bottom(2)),
        Line::from_points(top(3), bottom(4)),
        Line::from_points(top(5), bottom(4)),
        Line::from_points(top(5), bottom(3)),
    };

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

/// The point at `x` on the circular arc through both supports, `span` apart, rising `rise` at the middle.
static Point arc_at(double span, double rise, double x) {

    const double radius = (0.25 * span * span + rise * rise) / (2.0 * rise);
    return Point(
        x, 0.0, std::sqrt(radius * radius - (x - 0.5 * span) * (x - 0.5 * span)) - (radius - rise)
    );
}

/// The point at the fraction `t` of the angle of the circular arc through both supports, `span` apart, rising `rise` at the middle.
static Point arc_angle_at(double span, double rise, double t) {

    const double radius = (0.25 * span * span + rise * rise) / (2.0 * rise);
    const double half = std::asin(0.5 * span / radius);
    const double angle = -half + 2.0 * half * t;
    return Point(
        0.5 * span + radius * std::sin(angle), 0.0, radius * std::cos(angle) - (radius - rise)
    );
}

TrussGuide TrussGuide::crescent_crossed(
    double span, double height, double inner, int panels, double chord_radius, double web_radius
) {

    std::vector<Point> outer;
    for (int j = 0; j <= panels; j++)
        outer.push_back(arc_angle_at(span, height, static_cast<double>(j) / panels));
    std::vector<Point> lower = {Point(0.0, 0.0, 0.0)};
    for (int k = 1; k <= panels - 3; k++)
        lower.push_back(arc_angle_at(span, inner, (k + 1.0) / panels));
    lower.push_back(Point(span, 0.0, 0.0));
    const std::vector<Polyline> chords = {Polyline(outer), Polyline(lower)};

    std::vector<Line> webs;
    for (int j = 1; j <= panels - 3; j++) {
        webs.push_back(Line::from_points(outer[j], lower[j]));
        webs.push_back(Line::from_points(lower[j], outer[j + 2]));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide
TrussGuide::braced_arch_rib(double span, double height, double chord_radius, double web_radius) {

    const std::vector<Point> outer = scaled(
        {{0.0, 0.0},
         {-0.019613, 0.140461},
         {-0.019613, 0.309032},
         {-0.019613, 0.535115},
         {0.090394, 0.642857},
         {0.159528, 0.714101},
         {0.228663, 0.780922},
         {0.29512, 0.847465},
         {0.363225, 0.91318},
         {0.430711, 0.980369},
         {0.5, 1.0}},
        span,
        height
    );
    const std::vector<Point> inner = scaled(
        {{0.0, 0.0},
         {0.030372, 0.140461},
         {0.05853, 0.306636},
         {0.086688, 0.411336},
         {0.130341, 0.518986},
         {0.188768, 0.626728},
         {0.250026, 0.705069},
         {0.316535, 0.771429},
         {0.385669, 0.843318},
         {0.454803, 0.907465},
         {0.5, 1.0}},
        span,
        height
    );
    const std::vector<Polyline> chords = {Polyline(outer), Polyline(inner)};

    // the foot: rungs and diagonals up the vertical face to the knee
    std::vector<Line> webs = {
        Line::from_points(outer[1], inner[1]),
        Line::from_points(inner[1], outer[2]),
        Line::from_points(outer[2], inner[2]),
        Line::from_points(inner[2], outer[3]),
        Line::from_points(inner[3], outer[3]),
        Line::from_points(outer[3], inner[4]),
    };

    // the slope: from each outer node down to its inner node and across to the next one
    for (int j = 4; j <= 9; j++) {
        webs.push_back(Line::from_points(outer[j], inner[j]));
        if (j < 9)
            webs.push_back(Line::from_points(outer[j], inner[j + 1]));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide
TrussGuide::belgian_verticals(double span, double rise, double chord_radius, double web_radius) {

    const auto top = [&](double x) {
        return Point(x, 0.0, rise * (1.0 - std::abs(x - 0.5 * span) / (0.5 * span)));
    };
    const Point ridge = top(0.5 * span);

    // the chords: the bottom chord straight, the rafters up to the ridge
    const std::vector<Polyline> chords = {
        Polyline({top(0.0), top(span)}), Polyline({top(0.0), ridge, top(span)})
    };

    // per side: a strut from the rafter at a sixth down to the third point, the vertical over it and the diagonal from it to the ridge
    std::vector<Line> webs;
    const Point third(span / 3.0, 0.0, 0.0);
    for (const Line& web :
         {Line::from_points(top(span / 6.0), third),
          Line::from_points(third, top(span / 3.0)),
          Line::from_points(third, ridge)}) {
        webs.push_back(web);
        webs.push_back(Line::from_points(mirror(web.start(), span), mirror(web.end(), span)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::raised_tie_king_post(
    double span, double rise, double tie, double chord_radius, double web_radius
) {

    const auto top = [&](double x) {
        return Point(x, 0.0, rise * (1.0 - std::abs(x - 0.5 * span) / (0.5 * span)));
    };
    const Point ridge = top(0.5 * span);
    const Point knot(0.5 * span, 0.0, tie);

    // the chords: the inclined ties from the heels up to the knot under the ridge, the rafters up to the ridge
    const std::vector<Polyline> chords = {
        Polyline({top(0.0), knot, top(span)}), Polyline({top(0.0), ridge, top(span)})
    };

    const std::vector<Line> webs = {
        Line::from_points(knot, ridge),
        Line::from_points(top(0.25 * span), knot),
        Line::from_points(top(0.75 * span), knot)
    };

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::trapezoid_crossed(
    double span, double height, double ridge, double chord_radius, double web_radius
) {

    // the chords: the bottom chord straight, the top chord up to the shoulders at 0.19 of the span and to the ridge
    const Point left(0.19 * span, 0.0, height);
    const Point right(0.81 * span, 0.0, height);
    const Point start(0.0, 0.0, 0.0);
    const Point end(span, 0.0, 0.0);
    const std::vector<Polyline> chords = {
        Polyline({start, end}), Polyline({start, left, Point(0.5 * span, 0.0, ridge), right, end})
    };

    const std::vector<Line> webs = {Line::from_points(start, right), Line::from_points(end, left)};

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::fink_cambered(
    double span, double rise, double camber, double chord_radius, double web_radius
) {

    const auto top = [&](double x) {
        return Point(x, 0.0, rise * (1.0 - std::abs(x - 0.5 * span) / (0.5 * span)));
    };
    const Point ridge = top(0.5 * span);

    // the chords: the bottom chord cambered through its nodes at the measured fractions, the rafters up to the ridge
    const std::vector<Point> bottom = symmetric(
        {Point(0.0, 0.0, 0.0),
         Point(0.19 * span, 0.0, 0.62 * camber),
         Point(0.39 * span, 0.0, camber)},
        span
    );
    const std::vector<Polyline> chords = {Polyline(bottom), Polyline({top(0.0), ridge, top(span)})};

    std::vector<Line> webs;
    for (const Line& web :
         {Line::from_points(top(span / 6.0), bottom[1]),
          Line::from_points(bottom[1], top(span / 3.0)),
          Line::from_points(top(span / 3.0), bottom[2]),
          Line::from_points(bottom[2], ridge)}) {
        webs.push_back(web);
        webs.push_back(Line::from_points(mirror(web.start(), span), mirror(web.end(), span)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::crescent_verticals(
    double span,
    double height,
    double inner,
    double lantern,
    double lantern_rise,
    int panels,
    double chord_radius,
    double web_radius
) {

    // the chords: the outer arc through a node half a panel in from each support and over every panel point, the inner arc under the panel points
    std::vector<Point> outer = {Point(0.0, 0.0, 0.0), arc_at(span, height, 0.5 * span / panels)};
    std::vector<Point> lower = {Point(0.0, 0.0, 0.0)};
    for (int i = 1; i < panels; i++) {
        outer.push_back(arc_at(span, height, i * span / panels));
        lower.push_back(arc_at(span, inner, i * span / panels));
    }
    outer.push_back(arc_at(span, height, span - 0.5 * span / panels));
    outer.push_back(Point(span, 0.0, 0.0));
    lower.push_back(Point(span, 0.0, 0.0));

    // the lantern: posts on the outer arc's panel points each side of the middle, its roof over them
    const int left = panels / 2 - 1;
    const int right = panels / 2 + 1;
    const Point eave_left = outer[left + 1] + Vector(0.0, 0.0, lantern);
    const Point eave_right = outer[right + 1] + Vector(0.0, 0.0, lantern);
    const Point lantern_ridge(0.5 * span, 0.0, 0.5 * (eave_left[2] + eave_right[2]) + lantern_rise);
    const std::vector<Polyline> chords = {
        Polyline(outer), Polyline(lower), Polyline({eave_left, lantern_ridge, eave_right})
    };
    const std::vector<Line> posts = {
        Line::from_points(outer[left + 1], eave_left),
        Line::from_points(outer[right + 1], eave_right)
    };

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(lower[i], outer[i + 1]));
    webs.push_back(Line::from_points(outer[1], lower[1]));
    webs.push_back(Line::from_points(outer[panels + 1], lower[panels - 1]));
    for (int i = 1; i < panels / 2; i++) {
        webs.push_back(Line::from_points(outer[i + 1], lower[i + 1]));
        webs.push_back(Line::from_points(outer[panels - i + 1], lower[panels - i - 1]));
    }

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::raised_tie_posts(
    double span, double rise, double column, double chord_radius, double web_radius
) {

    const auto top = [&](double x) {
        return Point(x, 0.0, rise * (1.0 - std::abs(x - 0.5 * span) / (0.5 * span)));
    };
    const Point ridge = top(0.5 * span);

    // the measured fractions: the posts at 0.183 of the span, the raised tie's corners at 0.391, its level the rafters' height over the posts
    const double post = 0.183 * span;
    const double corner = 0.391 * span;
    const double tie = top(post)[2];

    // the chords: the tie from the heels to the posts, up to the corners and level between, the rafters up to the ridge
    const std::vector<Point> lower =
        symmetric({Point(0.0, 0.0, 0.0), Point(post, 0.0, 0.0), Point(corner, 0.0, tie)}, span);
    const std::vector<Polyline> chords = {Polyline(lower), Polyline({top(0.0), ridge, top(span)})};

    // per side: the level member from the rafter over the column to the corner, a strut from the rafter between down to the corner, the
    std::vector<Line> webs;
    const Point corner_node(corner, 0.0, tie);
    for (const Line& web :
         {Line::from_points(top(post), corner_node),
          Line::from_points(top(0.5 * (post + corner)), corner_node),
          Line::from_points(corner_node, top(corner)),
          Line::from_points(corner_node, ridge)}) {
        webs.push_back(web);
        webs.push_back(Line::from_points(mirror(web.start(), span), mirror(web.end(), span)));
    }

    const std::vector<Line> posts = {
        Line::from_points(Point(post, 0.0, -column), top(post)),
        Line::from_points(Point(span - post, 0.0, -column), top(span - post))
    };

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::pitched_warren_columns(
    double span,
    double height,
    double ridge,
    double column,
    int panels,
    double chord_radius,
    double web_radius
) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(
            i * panel,
            0.0,
            height + (ridge - height) * (1.0 - std::abs(i - 0.5 * panels) / (0.5 * panels))
        );
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, 0.0);
    };

    // the chords: the bottom chord straight, the top chord pitched
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(panels)}), Polyline({top(0), top(panels / 2), top(panels)})
    };

    const std::vector<Line> posts = {
        Line::from_points(Point(0.0, 0.0, -column), top(0)),
        Line::from_points(Point(span, 0.0, -column), top(panels))
    };

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int i = 0; i < panels / 2; i++) {
        const bool down = i % 2 == 0;
        webs.push_back(
            down ? Line::from_points(top(i), bottom(i + 1))
                 : Line::from_points(bottom(i), top(i + 1))
        );
        webs.push_back(
            down ? Line::from_points(top(panels - i), bottom(panels - i - 1))
                 : Line::from_points(bottom(panels - i), top(panels - i - 1))
        );
    }

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide
TrussGuide::fink_compound(double span, double rise, double chord_radius, double web_radius) {

    const auto top = [&](double x) {
        return Point(x, 0.0, rise * (1.0 - std::abs(x - 0.5 * span) / (0.5 * span)));
    };
    const Point ridge = top(0.5 * span);

    // the measured fractions: the bottom nodes at 0.181 and 0.371, the rafter nodes at twelfths of the span; the knot on the main strut from
    const Point first(0.181 * span, 0.0, 0.0);
    const Point second(0.371 * span, 0.0, 0.0);
    const Point middle = top(0.25 * span);
    const Point knot = second + (ridge - second) * (middle[2] / rise);

    // the chords: the bottom chord straight, the rafters up to the ridge
    const std::vector<Polyline> chords = {
        Polyline({top(0.0), top(span)}), Polyline({top(0.0), ridge, top(span)})
    };

    // per side: the lower W from the rafter's first two twelfths, the strut down from its middle, the level member to the knot, the main strut
    std::vector<Line> webs;
    for (const Line& web :
         {Line::from_points(top(span / 12.0), first),
          Line::from_points(top(span / 6.0), first),
          Line::from_points(first, middle),
          Line::from_points(middle, second),
          Line::from_points(middle, knot),
          Line::from_points(second, ridge),
          Line::from_points(top(span / 3.0), knot),
          Line::from_points(top(5.0 * span / 12.0), knot)}) {
        webs.push_back(web);
        webs.push_back(Line::from_points(mirror(web.start(), span), mirror(web.end(), span)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::fink_bent_tie(
    double span, double rise, double bend, double chord_radius, double web_radius
) {

    const auto top = [&](double x) {
        return Point(x, 0.0, rise * (1.0 - std::abs(x - 0.5 * span) / (0.5 * span)));
    };
    const Point ridge = top(0.5 * span);

    // the chords: the bottom chord straight, the rafters up to the ridge
    const std::vector<Polyline> chords = {
        Polyline({top(0.0), top(span)}), Polyline({top(0.0), ridge, top(span)})
    };

    // per side: the bent tie from the heel up to its bend at 0.326 of the span and on to the ridge, a strut from the rafter at 0.262 to the bend
    std::vector<Line> webs;
    const Point knee(0.326 * span, 0.0, bend);
    for (const Line& web :
         {Line::from_points(top(0.0), knee),
          Line::from_points(knee, ridge),
          Line::from_points(top(0.262 * span), knee)}) {
        webs.push_back(web);
        webs.push_back(Line::from_points(mirror(web.start(), span), mirror(web.end(), span)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide
TrussGuide::french_steep(double span, double rise, double chord_radius, double web_radius) {

    const auto top = [&](double x) {
        return Point(x, 0.0, rise * (1.0 - std::abs(x - 0.5 * span) / (0.5 * span)));
    };
    const Point ridge = top(0.5 * span);

    // the chords: the bottom chord straight, the rafters up to the ridge
    const std::vector<Polyline> chords = {
        Polyline({top(0.0), top(span)}), Polyline({top(0.0), ridge, top(span)})
    };

    // per side: the central triangle's side from the ridge to the third point, a strut from the rafter at 0.165 down to the third point and
    std::vector<Line> webs;
    const Point third(span / 3.0, 0.0, 0.0);
    const Point middle = third + (ridge - third) * 0.5;
    for (const Line& web :
         {Line::from_points(ridge, third),
          Line::from_points(top(0.165 * span), third),
          Line::from_points(top(0.165 * span), middle),
          Line::from_points(top(span / 3.0), middle)}) {
        webs.push_back(web);
        webs.push_back(Line::from_points(mirror(web.start(), span), mirror(web.end(), span)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::fink_lantern(
    double span,
    double rise,
    double lantern,
    double lantern_rise,
    double chord_radius,
    double web_radius
) {

    const auto top = [&](double x) {
        return Point(x, 0.0, rise * (1.0 - std::abs(x - 0.5 * span) / (0.5 * span)));
    };
    const Point ridge = top(0.5 * span);

    // the measured fractions: the bottom nodes at 0.163 and 0.327, the rafter nodes at 0.129, 0.252 and 0.374, the knot on the main strut level
    const Point first(0.163 * span, 0.0, 0.0);
    const Point second(0.327 * span, 0.0, 0.0);
    const Point knot = second + (ridge - second) * (top(0.252 * span)[2] / rise);

    // the lantern on the third rafter nodes: posts up to its eaves and its roof over them
    const Point eave_left = top(0.374 * span) + Vector(0.0, 0.0, lantern);
    const Point eave_right = mirror(eave_left, span);
    const Point lantern_ridge(0.5 * span, 0.0, eave_left[2] + lantern_rise);

    // the chords: the bottom chord straight, the rafters up to the ridge, the lantern's roof
    const std::vector<Polyline> chords = {
        Polyline({top(0.0), top(span)}),
        Polyline({top(0.0), ridge, top(span)}),
        Polyline({eave_left, lantern_ridge, eave_right})
    };
    const std::vector<Line> posts = {
        Line::from_points(top(0.374 * span), eave_left),
        Line::from_points(top(0.626 * span), eave_right)
    };

    // per side: the first strut, the second rafter node's diagonal and level member, the main strut, the third rafter node's strut; the king
    std::vector<Line> webs = {
        Line::from_points(Point(0.5 * span, 0.0, 0.0), ridge),
        Line::from_points(ridge, lantern_ridge),
        Line::from_points(ridge, eave_left),
        Line::from_points(ridge, eave_right)
    };
    for (const Line& web :
         {Line::from_points(top(0.129 * span), first),
          Line::from_points(top(0.252 * span), second),
          Line::from_points(top(0.252 * span), knot),
          Line::from_points(second, ridge),
          Line::from_points(top(0.374 * span), knot)}) {
        webs.push_back(web);
        webs.push_back(Line::from_points(mirror(web.start(), span), mirror(web.end(), span)));
    }

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::arch_polygonal_tie(
    double span, double height, double tie, double chord_radius, double web_radius
) {

    const std::vector<Point> arch = symmetric(
        {Point(0.0, 0.0, 0.0),
         arc_at(span, height, 0.086 * span),
         arc_at(span, height, 0.205 * span),
         arc_at(span, height, 0.346 * span),
         Point(0.5 * span, 0.0, height)},
        span
    );
    const std::vector<Point> lower = symmetric(
        {Point(0.0, 0.0, 0.0),
         Point(0.156 * span, 0.0, 0.57 * tie),
         Point(0.283 * span, 0.0, tie),
         Point(0.5 * span, 0.0, tie)},
        span
    );
    const Point crown = arch[4];
    const Point knot = lower[2] + (crown - lower[2]) * 0.42;
    const std::vector<Polyline> chords = {Polyline(arch), Polyline(lower)};

    std::vector<Line> webs = {Line::from_points(lower[3], crown)};
    for (const Line& web :
         {Line::from_points(arch[1], lower[1]),
          Line::from_points(lower[1], arch[2]),
          Line::from_points(arch[2], lower[2]),
          Line::from_points(lower[2], crown),
          Line::from_points(arch[2], knot),
          Line::from_points(arch[3], knot)}) {
        webs.push_back(web);
        webs.push_back(Line::from_points(mirror(web.start(), span), mirror(web.end(), span)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide TrussGuide::pitched_crossed_posts(
    double span,
    double height,
    double ridge,
    double camber,
    int panels,
    double chord_radius,
    double web_radius
) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(
            i * panel,
            0.0,
            height + (ridge - height) * (1.0 - std::abs(i - 0.5 * panels) / (0.5 * panels))
        );
    };
    const auto bottom = [&](int i) {
        return Point(i * panel, 0.0, camber * (1.0 - std::abs(i - 0.5 * panels) / (0.5 * panels)));
    };

    // the chords: both pitched, the bottom up to the camber
    const std::vector<Polyline> chords = {
        Polyline({bottom(0), bottom(panels / 2), bottom(panels)}),
        Polyline({top(0), top(panels / 2), top(panels)})
    };

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(bottom(i), top(i)));
    for (int i = 0; i < panels; i++) {
        webs.push_back(Line::from_points(top(i), bottom(i + 1)));
        webs.push_back(Line::from_points(bottom(i), top(i + 1)));
    }

    const std::vector<Line> posts = {
        Line::from_points(bottom(0), top(0)), Line::from_points(bottom(panels), top(panels))
    };

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::cambered_crossed_lantern(
    double span,
    double height,
    double camber,
    double lantern,
    double lantern_rise,
    double chord_radius,
    double web_radius
) {

    // the chords at the measured eighths: the top chord straight up to the height at three eighths and level to five, the bottom chord
    const std::array<double, 4> rises = {0.0, 0.3, 0.7, 1.0};
    std::vector<Point> top;
    std::vector<Point> bottom;
    for (int i = 0; i <= 3; i++) {
        top.emplace_back(i * span / 8.0, 0.0, height * i / 3.0);
        bottom.emplace_back(i * span / 8.0, 0.0, camber * rises[i]);
    }
    top = symmetric(top, span);
    bottom = symmetric(bottom, span);

    const Point eave_left = top[3] + Vector(0.0, 0.0, lantern);
    const Point eave_right = top[4] + Vector(0.0, 0.0, lantern);
    const Point lantern_middle(0.5 * span, 0.0, eave_left[2]);
    const Point lantern_ridge(0.5 * span, 0.0, eave_left[2] + lantern_rise);
    const std::vector<Polyline> chords = {
        Polyline(bottom),
        Polyline(top),
        Polyline({eave_left, lantern_middle, eave_right}),
        Polyline({eave_left, lantern_ridge, eave_right})
    };
    const std::vector<Line> posts = {
        Line::from_points(top[3], eave_left), Line::from_points(top[4], eave_right)
    };

    std::vector<Line> webs = {
        Line::from_points(lantern_middle, lantern_ridge),
        Line::from_points(top[3], lantern_middle),
        Line::from_points(top[4], lantern_middle)
    };
    for (size_t i = 1; i + 1 < top.size(); i++)
        webs.push_back(Line::from_points(bottom[i], top[i]));
    for (size_t i = 1; i + 2 < top.size(); i++) {
        webs.push_back(Line::from_points(top[i], bottom[i + 1]));
        webs.push_back(Line::from_points(bottom[i], top[i + 1]));
    }

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::arched_crossed_columns(
    double span,
    double height,
    double ridge,
    double crown,
    double column,
    int panels,
    double chord_radius,
    double web_radius
) {

    const double panel = span / panels;
    const auto top = [&](int i) {
        return Point(
            i * panel,
            0.0,
            height + (ridge - height) * (1.0 - std::abs(i - 0.5 * panels) / (0.5 * panels))
        );
    };
    const auto arch = [&](int i) {
        return arc_at(span, crown, i * panel);
    };

    // the chords: the arch from column to column through every panel point, the top chord pitched
    std::vector<Point> lower;
    for (int i = 0; i <= panels; i++)
        lower.push_back(arch(i));
    const std::vector<Polyline> chords = {
        Polyline(lower), Polyline({top(0), top(panels / 2), top(panels)})
    };

    const std::vector<Line> posts = {
        Line::from_points(Point(0.0, 0.0, -column), top(0)),
        Line::from_points(Point(span, 0.0, -column), top(panels))
    };

    std::vector<Line> webs;
    for (int i = 1; i < panels; i++)
        webs.push_back(Line::from_points(arch(i), top(i)));
    for (int i = 0; i < panels; i++) {
        webs.push_back(Line::from_points(top(i), arch(i + 1)));
        webs.push_back(Line::from_points(arch(i), top(i + 1)));
    }

    return TrussGuide(chords, webs, posts, chord_radius, web_radius);
}

TrussGuide TrussGuide::trapezoid_king_crossed(
    double span, double height, double ridge, double chord_radius, double web_radius
) {

    // the chords: the bottom chord straight, the top chord up to the shoulders at the quarter points and to the ridge
    const Point start(0.0, 0.0, 0.0);
    const Point end(span, 0.0, 0.0);
    const Point left(0.25 * span, 0.0, height);
    const Point right(0.75 * span, 0.0, height);
    const Point peak(0.5 * span, 0.0, ridge);
    const std::vector<Polyline> chords = {
        Polyline({start, end}), Polyline({start, left, peak, right, end})
    };

    const Point left_foot(0.25 * span, 0.0, 0.0);
    const Point middle_foot(0.5 * span, 0.0, 0.0);
    const Point right_foot(0.75 * span, 0.0, 0.0);
    const std::vector<Line> webs = {
        Line::from_points(left_foot, left),
        Line::from_points(middle_foot, peak),
        Line::from_points(right_foot, right),
        Line::from_points(left, middle_foot),
        Line::from_points(left_foot, peak),
        Line::from_points(peak, right_foot),
        Line::from_points(middle_foot, right),
    };

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

TrussGuide
TrussGuide::topology_optimised(double span, double height, double chord_radius, double web_radius) {

    const auto top = [&](double fraction) {
        return Point(fraction * span, 0.0, height);
    };
    const auto bottom = [&](double fraction) {
        return Point(fraction * span, 0.0, 0.0);
    };

    // the chords: the bottom chord straight, the top chord level from 0.13 to 0.87 of the span and sloping down to the supports
    const std::vector<Polyline> chords = {
        Polyline({bottom(0.0), bottom(1.0)}),
        Polyline({bottom(0.0), top(0.13), top(0.33), top(0.5), top(0.67), top(0.87), bottom(1.0)})
    };

    std::vector<Line> webs = {
        Line::from_points(top(0.5), bottom(0.41)), Line::from_points(top(0.5), bottom(0.59))
    };
    for (const Line& web :
         {Line::from_points(top(0.13), bottom(0.2)),
          Line::from_points(bottom(0.2), top(0.33)),
          Line::from_points(top(0.33), bottom(0.41)),
          Line::from_points(top(0.13), bottom(0.41))}) {
        webs.push_back(web);
        webs.push_back(Line::from_points(mirror(web.start(), span), mirror(web.end(), span)));
    }

    return TrussGuide(chords, webs, {}, chord_radius, web_radius);
}

}
