#pragma once
#include "wood_session.h"
#include "development.h"

using namespace session_cpp;
using namespace wood_session;

/// A closed curved-folded beam.
class CurvedFoldedBeam : public WoodSession {
public:
    const double span; // mm, along x between the supports.
    const double width; // mm, the section's width at the supports, its height half that.
    const double rise; // mm, how far the top and bottom plates bulge outward at mid-span.
    const double sag; // mm, how far the side plates sag inward at mid-span.
    const double thickness; // mm, every plate and web.
    const size_t webs; // Flat webs across every beam at even stations along the span.
    const size_t count; // Beams side by side along y, width apart.
    const size_t samples; // Rulings each plate is interpolated through.

private:
    double _distortion = 0.0; // The largest relative length error of any plate's development.

public:
    /// count curved-folded beams as the session named name.
    CurvedFoldedBeam(
        double span = 12000.0,
        double width = 800.0,
        double rise = 250.0,
        double sag = 80.0,
        double thickness = 27.0,
        size_t webs = 5,
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
    /// The corner at the top right sampled at equal x: the crease and the inner crease a thickness inside both plates.
    std::array<std::vector<Point>, 2> compute_corner() const;

    /// The circular arc through base at the supports and base + change at mid-span: its value and slope at x.
    std::pair<double, double> profile(double base, double change, double x) const;

    /// The point a thickness inside a profile at the arc's parameter u: its x and its distance from the axis.
    std::pair<double, double> inside_point(double base, double change, double u) const;

    /// The inner profile's distance from the axis at x, by bisection on the arc's parameter.
    double inner_distance(double base, double change, double x) const;

    /// Every beam, beam_<b> its group: its four plates and its webs.
    void add_beams(const std::array<std::vector<Point>, 2>& corner);

    /// Beam b's four plates, beam_<b>_<k>.
    void add_plates(
        const std::array<std::vector<Point>, 2>& corner,
        size_t b,
        const std::shared_ptr<TreeNode>& group
    );

    /// Beam b's webs, beam_<b>_<4 + w>: flat diaphragms at even stations, their corners on the inner crease.
    void add_webs(const NurbsCurve& inner_crease, size_t b, const std::shared_ptr<TreeNode>& group);
};

inline CurvedFoldedBeam::CurvedFoldedBeam(
    double span,
    double width,
    double rise,
    double sag,
    double thickness,
    size_t webs,
    size_t count,
    const std::string& name,
    size_t samples
)
    : WoodSession(name), span(span), width(width), rise(rise), sag(sag), thickness(thickness),
      webs(webs), count(std::max<size_t>(count, 1)), samples(std::max<size_t>(samples, 4)) {

    if (span <= 0.0 || width <= 0.0 || rise <= 0.0 || sag <= 0.0 || thickness <= 0.0 ||
        sag >= 0.5 * width - 2.0 * thickness)
        throw std::invalid_argument("A curved-folded beam needs a span, a width, a rise, a sag "
                                    "well inside the width and a thickness");

    // corner: the top right crease and the inner crease under it, the mitre between them
    const std::array<std::vector<Point>, 2> corner = compute_corner();

    // beams: every beam's four plates between its four corners, each a cylinder with its rulings straight across, and its webs
    add_beams(corner);

    // contacts: every two plates that share a face, found by the session's contact detection
    compute_face_contacts(0);
}

inline std::array<std::vector<Point>, 2> CurvedFoldedBeam::compute_corner() const {

    const double half = 0.5 * span;
    std::array<std::vector<Point>, 2> corner;

    for (size_t k = 0; k <= samples; k++) {
        const double x = -half + span * static_cast<double>(k) / static_cast<double>(samples);
        const double side = profile(0.5 * width, -sag, x).first;
        const double top = profile(0.25 * width, rise, x).first;
        const double inner_side = inner_distance(0.5 * width, -sag, x);
        const double inner_top = inner_distance(0.25 * width, rise, x);
        corner[0].push_back(Point(x, side, top));
        corner[1].push_back(Point(x, inner_side, inner_top));
    }

    return corner;
}

inline std::pair<double, double>
CurvedFoldedBeam::profile(double base, double change, double x) const {

    const double half = 0.5 * span;
    const double radius = (half * half + change * change) / (2.0 * std::abs(change));
    const double root = std::sqrt(radius * radius - x * x);
    const double sign = change > 0.0 ? 1.0 : -1.0;

    return std::make_pair(base + change - sign * radius + sign * root, -sign * x / root);
}

inline std::pair<double, double>
CurvedFoldedBeam::inside_point(double base, double change, double u) const {

    const std::pair<double, double> value = profile(base, change, u);
    const double length = std::sqrt(1.0 + value.second * value.second);

    return std::make_pair(u + thickness * value.second / length, value.first - thickness / length);
}

inline double CurvedFoldedBeam::inner_distance(double base, double change, double x) const {

    double low = x - 2.0 * thickness;
    double high = x + 2.0 * thickness;

    for (int i = 0; i < 80; i++) {
        const double middle = 0.5 * (low + high);

        if (inside_point(base, change, middle).first < x)
            low = middle;
        else
            high = middle;
    }

    return inside_point(base, change, 0.5 * (low + high)).second;
}

inline void CurvedFoldedBeam::add_beams(const std::array<std::vector<Point>, 2>& corner) {

    const NurbsCurve inner_crease =
        NurbsCurve::create_interpolated(corner[1], CurveNurbsKnotStyle::Uniform);

    for (size_t b = 0; b < count; b++) {
        const std::shared_ptr<TreeNode> group = add_group(fmt::format("beam_{}", b));
        add_plates(corner, b, group);
        add_webs(inner_crease, b, group);
    }
}

inline void CurvedFoldedBeam::add_plates(
    const std::array<std::vector<Point>, 2>& corner,
    size_t b,
    const std::shared_ptr<TreeNode>& group
) {

    // the four corners as signs of y and z, and the plates between two of them: top, right, bottom, left
    const std::array<std::array<double, 2>, 4> corners = {
        {{1.0, 1.0}, {1.0, -1.0}, {-1.0, -1.0}, {-1.0, 1.0}}
    };
    const std::array<std::array<size_t, 2>, 4> plates = {{{3, 0}, {0, 1}, {1, 2}, {2, 3}}};
    const double shift = width * static_cast<double>(b);

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

        const std::shared_ptr<PlateCurved> plate = std::make_shared<PlateCurved>(
            curves[0], curves[1], curves[2], curves[3], fmt::format("beam_{}_{}", b, k)
        );
        add(plate, group);

        const Development development(curves[0], curves[1], samples);
        _distortion = std::max(_distortion, development.distortion);
    }
}

inline void CurvedFoldedBeam::add_webs(
    const NurbsCurve& inner_crease, size_t b, const std::shared_ptr<TreeNode>& group
) {

    const double shift = width * static_cast<double>(b);

    for (size_t w = 0; w < webs; w++) {
        const double station =
            -0.5 * span + span * static_cast<double>(w + 1) / static_cast<double>(webs + 1);
        std::array<NurbsCurve, 4> curves;

        for (size_t e = 0; e < 4; e++) {
            const double x = station + (e < 2 ? -0.5 : 0.5) * thickness;
            const double side = e % 2 == 0 ? 1.0 : -1.0;
            double low = inner_crease.domain_start();
            double high = inner_crease.domain_end();

            for (int i = 0; i < 80; i++) {
                const double middle = 0.5 * (low + high);

                if (inner_crease.point_at(middle)[0] < x)
                    low = middle;
                else
                    high = middle;
            }

            const Point point = inner_crease.point_at(0.5 * (low + high));
            const Point left(x, shift - point[1], side * point[2]);
            const Point right(x, shift + point[1], side * point[2]);
            curves[e] = NurbsCurve::create(false, 1, {left, right});
        }

        const std::shared_ptr<PlateCurved> web = std::make_shared<PlateCurved>(
            curves[0], curves[1], curves[2], curves[3], fmt::format("beam_{}_{}", b, 4 + w)
        );
        add(web, group);
    }
}
