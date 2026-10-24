#pragma once
#include "wood_session.h"
#include "development.h"

using namespace session_cpp;
using namespace wood_session;

/// A closed curved-folded beam of four plates: a box spanning along x, square at the supports, its four plates each a cylinder bent along the span with its rulings straight across. The
/// two pairs fold opposite ways: the top and bottom plates bulge outward by rise toward mid-span, the side plates sag inward by sag, so
/// the beam deepens and narrows to mid-span. The corners are the curved creases where a top or bottom plate meets a side plate, and
/// every plate is mitred there: its inner face ends where it meets its neighbour's, both a thickness inside, and the mitre is the
/// ruled face between the crease and that inner crease, one face shared by the two plates. The ends are cut square at the supports. A
/// row of beams side by side along y, touching at the supports, leaves lens-shaped gaps between them, widest at mid-span.
///
/// Fields: the span, the size, the rise, the sag, the thickness, the count and the sampling; the plates are in the session.
class CurvedFoldedBeam : public WoodSession {
public:
    const double span; // mm, along x between the supports.
    const double size; // mm, half the side of the square section at the supports.
    const double rise; // mm, how far the top and bottom plates bulge outward at mid-span.
    const double sag; // mm, how far the side plates sag inward at mid-span.
    const double thickness; // mm, every plate.
    const size_t count; // Beams side by side along y, 2 size apart.
    const size_t samples; // Rulings each plate is interpolated through.

    /// count curved-folded beams as the session named name.
    CurvedFoldedBeam(
        double span = 12000.0,
        double size = 400.0,
        double rise = 250.0,
        double sag = 220.0,
        double thickness = 27.0,
        size_t count = 6,
        const std::string& name = "curved_folded_beam",
        size_t samples = 48
    );

    /// Not copied, as a session is not.
    CurvedFoldedBeam(const CurvedFoldedBeam&) = delete;

    /// Not assigned, as it is not copied.
    CurvedFoldedBeam& operator=(const CurvedFoldedBeam&) = delete;

    /// The largest relative length error of any plate's development, the proof every plate is cut flat.
    double distortion() const {
        return _distortion;
    }

private:
    double _distortion = 0.0;

    /// The corner at the top right, sampled at equal x from support to support: the crease, where the top plate's outer face meets the
    /// right plate's, and the inner crease, where their inner faces meet, a thickness inside both.
    std::array<std::vector<Point>, 2> compute_corner() const;

    /// Every beam's four plates, beam_<b>_<k> in the group beam_<b>: top, right, bottom and left, each between two corners, the corner
    /// mirrored in the planes y 0 and z 0, and moved along y.
    void add_beams(const std::array<std::vector<Point>, 2>& corner);
};

inline CurvedFoldedBeam::CurvedFoldedBeam(double span, double size, double rise, double sag, double thickness, size_t count, const std::string& name, size_t samples)
    : WoodSession(name),
      span(span),
      size(size),
      rise(rise),
      sag(sag),
      thickness(thickness),
      count(std::max<size_t>(count, 1)),
      samples(std::max<size_t>(samples, 4)) {

    if (span <= 0.0 || size <= 0.0 || rise <= 0.0 || sag <= 0.0 || sag >= size - 2.0 * thickness || thickness <= 0.0)
        throw std::invalid_argument("A curved-folded beam needs a span, a size, a rise, a sag well inside the size and a thickness");

    // corner: the top right crease and the inner crease under it, the mitre between them
    const std::array<std::vector<Point>, 2> corner = compute_corner();

    // beams: every beam's four plates between its four corners, each a cylinder with its rulings straight across
    add_beams(corner);

    // contacts: every two plates that share a face, the four mitres of every beam, found by the session's contact detection
    compute_face_contacts(0);
}

inline std::array<std::vector<Point>, 2> CurvedFoldedBeam::compute_corner() const {

    // a profile along the span: a circular arc through size at the supports and size + change at mid-span, its value and slope at x
    const double half = 0.5 * span;
    const std::function<std::pair<double, double>(double, double)> profile = [&](double change, double x) {
        const double radius = (half * half + change * change) / (2.0 * std::abs(change));
        const double root = std::sqrt(radius * radius - x * x);
        const double sign = change > 0.0 ? 1.0 : -1.0;
        return std::make_pair(size + change - sign * radius + sign * root, -sign * x / root);
    };

    // the point a thickness inside a profile at the arc's parameter u: its x, and its distance from the axis
    const std::function<std::pair<double, double>(double, double)> inside = [&](double change, double u) {
        const std::pair<double, double> value = profile(change, u);
        const double length = std::sqrt(1.0 + value.second * value.second);
        return std::make_pair(u + thickness * value.second / length, value.first - thickness / length);
    };

    // the inner profile's distance from the axis at x: the arc's parameter whose inner point lies at x, by bisection
    const std::function<double(double, double)> inner = [&](double change, double x) {
        double low = x - 2.0 * thickness;
        double high = x + 2.0 * thickness;

        for (int i = 0; i < 80; i++) {
            const double middle = 0.5 * (low + high);
            (inside(change, middle).first < x ? low : high) = middle;
        }

        return inside(change, 0.5 * (low + high)).second;
    };

    // the crease at the top and right profiles' heights, the inner crease at their inner faces', at equal x
    std::array<std::vector<Point>, 2> corner;

    for (size_t k = 0; k <= samples; k++) {
        const double x = -half + span * static_cast<double>(k) / static_cast<double>(samples);
        corner[0].push_back(Point(x, profile(-sag, x).first, profile(rise, x).first));
        corner[1].push_back(Point(x, inner(-sag, x), inner(rise, x)));
    }

    return corner;
}

inline void CurvedFoldedBeam::add_beams(const std::array<std::vector<Point>, 2>& corner) {

    // the four corners as signs of y and z, and the plates between two of them: top, right, bottom, left
    const std::array<std::array<double, 2>, 4> corners = {{{1.0, 1.0}, {1.0, -1.0}, {-1.0, -1.0}, {-1.0, 1.0}}};
    const std::array<std::array<size_t, 2>, 4> plates = {{{3, 0}, {0, 1}, {1, 2}, {2, 3}}};

    for (size_t b = 0; b < count; b++) {
        const std::shared_ptr<TreeNode> group = add_group(fmt::format("beam_{}", b));
        const double shift = 2.0 * size * static_cast<double>(b);

        for (size_t k = 0; k < 4; k++) {
            // outer_a, outer_b, inner_a, inner_b: the plate's two corners, the crease then the inner crease of each
            std::array<NurbsCurve, 4> curves;

            for (size_t e = 0; e < 4; e++) {
                const std::array<double, 2>& sign = corners[plates[k][e % 2]];
                std::vector<Point> points;

                for (const Point& point : corner[e / 2])
                    points.push_back(Point(point[0], sign[0] * point[1] + shift, sign[1] * point[2]));

                curves[e] = NurbsCurve::create_interpolated(points, CurveNurbsKnotStyle::Uniform);
            }

            add(std::make_shared<PlateCurved>(curves[0], curves[1], curves[2], curves[3], fmt::format("beam_{}_{}", b, k)), group);

            // its development, the proof it is cut flat
            const Development development(curves[0], curves[1], samples);
            _distortion = std::max(_distortion, development.distortion);
        }
    }
}
