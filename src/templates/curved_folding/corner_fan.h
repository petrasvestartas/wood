#pragma once
#include "wood_session.h"
#include "development.h"

using namespace session_cpp;
using namespace wood_session;

/// A fan of developable strips folded from one bottom corner of a square on parabolic creases to its two far sides.
class CornerFan : public WoodSession {
public:
    const double size; // mm, the square's side, the corner at the origin.
    const double height; // mm, where the high folds end above the corner.
    const size_t strips; // Strips dividing the two far sides, half on each, even.
    const double lift; // mm, every parabola's middle control point above its chord, below when negative.
    const double drop; // mm, how much lower every second fold ends.
    const double foot; // The share of the way up where the strips start, cut short of the corner.
    const double thickness; // mm, every strip.
    const Point corner; // The square's bottom corner the folds start from.
    const size_t samples; // Points every inner crease is interpolated through.

private:
    double _distortion = 0.0; // The largest relative length error of any strip's development.

public:
    /// The fan as the session named name.
    CornerFan(
        double size = 6000.0,
        double height = 4000.0,
        size_t strips = 6,
        double lift = 1200.0,
        double drop = 800.0,
        double foot = 0.15,
        double thickness = 40.0,
        const Point& corner = Point(0.0, 0.0, 0.0),
        const std::string& name = "corner_fan",
        size_t samples = 64
    );

    /// Not copied, as a session is not.
    CornerFan(const CornerFan&) = delete;

    /// Not assigned, as it is not copied.
    CornerFan& operator=(const CornerFan&) = delete;

    /// The largest relative length error of any strip's development, 0 for an exact cylinder.
    double distortion() const {
        return _distortion;
    }

private:
    /// Every fold's top on the square's two far sides, equally spaced, high and low in turn.
    std::vector<Point> compute_tops() const;

    /// Every fold's parabola from three points, the corner, its lifted middle and its top, cut at foot.
    std::vector<NurbsCurve> compute_creases(const std::vector<Point>& tops) const;

    /// Every fold's inner crease, from its foot to its top end, where the two strips' inner faces meet on their bisector.
    std::vector<NurbsCurve> compute_inners(const std::vector<Point>& tops) const;

    /// The inner point of fold k at s: a thickness under both strips on their bisector, square under the one at a free fold.
    Point inner_point(const std::vector<Point>& tops, size_t k, double s) const;

    /// The s near start where fold k's inner point lies on the plane, by bisection.
    double inner_on(const std::vector<Point>& tops, size_t k, const Plane& plane, double start) const;

    /// Every strip, strip_<k> in the group strips, between two folds' creases and inner creases.
    void add_strips(const std::vector<NurbsCurve>& creases, const std::vector<NurbsCurve>& inners);

    /// The fold's parabola at s, from the corner at 0 to its top at 1.
    Point crease_point(const Point& top, double s) const;

    /// The fold's parabola tangent at s.
    Vector crease_tangent(const Point& top, double s) const;

    /// The unit normal under the strip from top to next at the fold through top, at s.
    static Vector strip_normal(const Vector& tangent, const Point& top, const Point& next);
};

inline CornerFan::CornerFan(
    double size,
    double height,
    size_t strips,
    double lift,
    double drop,
    double foot,
    double thickness,
    const Point& corner,
    const std::string& name,
    size_t samples
)
    : WoodSession(name),
      size(size),
      height(height),
      strips(strips),
      lift(lift),
      drop(drop),
      foot(foot),
      thickness(thickness),
      corner(corner),
      samples(std::max<size_t>(samples, 8)) {

    if (size <= 0.0 || height <= 0.0 || strips < 2 || strips % 2 != 0 || foot <= 0.0 || foot >= 1.0 || thickness <= 0.0)
        throw std::invalid_argument("A corner fan needs a square, a height, an even number of strips, a foot between 0 and 1 and a thickness");

    // tops: every fold's end on the square's two far sides, high and low in turn
    const std::vector<Point> tops = compute_tops();

    // creases: every fold a parabola from the corner, all lifted alike, so two neighbours differ by s times their tops' difference
    // and every strip is a cylinder of rulings parallel to its straight top edge
    const std::vector<NurbsCurve> creases = compute_creases(tops);

    // inners: at every fold the curve both strips' inner faces end on, the bisector of the two
    const std::vector<NurbsCurve> inners = compute_inners(tops);

    // strips: every cylinder between two folds, its inner face between their inner creases
    add_strips(creases, inners);

    // contacts: every two strips that share a fold, found by the session's contact detection
    compute_face_contacts(0);
}

inline std::vector<Point> CornerFan::compute_tops() const {

    // from the side y = 0 up the far side x = size to the far corner, then along the far side y = size to the side x = 0
    std::vector<Point> tops;
    const size_t half = strips / 2;

    for (size_t k = 0; k <= strips; k++) {
        const double z = k % 2 == 0 ? height : height - drop;

        if (k <= half)
            tops.push_back(Point(size, size * static_cast<double>(k) / static_cast<double>(half), z));
        else
            tops.push_back(Point(size * static_cast<double>(strips - k) / static_cast<double>(half), size, z));
    }

    return tops;
}

inline Point CornerFan::crease_point(const Point& top, double s) const {

    // the quadratic from the corner, the chord's middle lifted by lift and the top
    const Vector to_top(top[0], top[1], top[2]);
    const Vector control = to_top * 0.5 + Vector(0.0, 0.0, lift);
    const Vector point = control * (2.0 * s * (1.0 - s)) + to_top * (s * s);

    return Point(point[0], point[1], point[2]);
}

inline Vector CornerFan::crease_tangent(const Point& top, double s) const {

    const Vector to_top(top[0], top[1], top[2]);
    const Vector control = to_top * 0.5 + Vector(0.0, 0.0, lift);

    return control * (2.0 * (1.0 - 2.0 * s)) + to_top * (2.0 * s);
}

inline Vector CornerFan::strip_normal(const Vector& tangent, const Point& top, const Point& next) {

    // square to the strip's rulings, parallel to its top edge, and to the fold's tangent, turned under the sheet
    Vector normal = (next - top).cross(tangent).normalized();

    if (normal[2] > 0.0)
        normal = -normal;

    return normal;
}

inline std::vector<NurbsCurve> CornerFan::compute_creases(const std::vector<Point>& tops) const {

    // the parabola cut at foot by de Casteljau, still a parabola of three control points
    std::vector<NurbsCurve> creases;

    for (const Point& top : tops) {
        const Vector to_top(top[0], top[1], top[2]);
        const Vector control = to_top * 0.5 + Vector(0.0, 0.0, lift);
        const Vector between = control * (1.0 - foot) + to_top * foot;
        const std::vector<Point> controls = {crease_point(top, foot), Point(between[0], between[1], between[2]), top};
        creases.push_back(NurbsCurve::create(false, 2, controls));
    }

    return creases;
}

inline Point CornerFan::inner_point(const std::vector<Point>& tops, size_t k, double s) const {

    const Point point = crease_point(tops[k], s);
    const Vector tangent = crease_tangent(tops[k], s);

    // a free fold: square under the one strip
    if (k == 0)
        return point + strip_normal(tangent, tops[0], tops[1]) * thickness;

    if (k == strips)
        return point + strip_normal(tangent, tops[k - 1], tops[k]) * thickness;

    // an inner fold: a thickness under both strips, on their bisector square to the fold
    const Vector before = strip_normal(tangent, tops[k - 1], tops[k]);
    const Vector after = strip_normal(tangent, tops[k], tops[k + 1]);

    return point + (before + after) * (thickness / (1.0 + before.dot(after)));
}

inline double CornerFan::inner_on(const std::vector<Point>& tops, size_t k, const Plane& plane, double start) const {

    // the plane's side of the inner point changes within a tenth of the fold either side of start
    const Vector normal = plane.z_axis();
    double low = start - 0.1;
    double high = start + 0.1;
    const double sign = normal.dot(inner_point(tops, k, low) - plane.origin()) < 0.0 ? -1.0 : 1.0;

    for (int i = 0; i < 80; i++) {
        const double middle = 0.5 * (low + high);

        if (sign * normal.dot(inner_point(tops, k, middle) - plane.origin()) > 0.0)
            low = middle;
        else
            high = middle;
    }

    return 0.5 * (low + high);
}

inline std::vector<NurbsCurve> CornerFan::compute_inners(const std::vector<Point>& tops) const {

    // top ends: every inner crease cut by the square's far side its fold ends on, so every strip ends on a vertical plane
    std::vector<double> ends;

    for (size_t k = 0; k <= strips; k++) {
        const Plane side = k <= strips / 2 ? Plane::from_point_normal(tops[k], Vector(1.0, 0.0, 0.0)) : Plane::from_point_normal(tops[k], Vector(0.0, 1.0, 0.0));
        ends.push_back(inner_on(tops, k, side, 1.0));
    }

    // feet: every strip's foot a plane through its outer foot ruling and the inner foot of the fold before, so the next fold's
    // inner foot is found on it, strip by strip from the first free fold
    std::vector<double> feet = {foot};

    for (size_t k = 0; k < strips; k++) {
        const Point first = crease_point(tops[k], foot);
        const Point second = crease_point(tops[k + 1], foot);
        const Point inner = inner_point(tops, k, feet[k]);
        const Vector normal = (second - first).cross(inner - first).normalized();
        feet.push_back(inner_on(tops, k + 1, Plane::from_point_normal(first, normal), foot));
    }

    // the inner creases between their feet and their ends
    std::vector<NurbsCurve> inners;

    for (size_t k = 0; k <= strips; k++) {
        std::vector<Point> points;

        for (size_t i = 0; i <= samples; i++) {
            const double s = feet[k] + (ends[k] - feet[k]) * static_cast<double>(i) / static_cast<double>(samples);
            points.push_back(inner_point(tops, k, s));
        }

        inners.push_back(NurbsCurve::create_interpolated(points, CurveNurbsKnotStyle::Uniform));
    }

    return inners;
}

inline void CornerFan::add_strips(const std::vector<NurbsCurve>& creases, const std::vector<NurbsCurve>& inners) {

    const std::shared_ptr<TreeNode> group = add_group("strips");
    const Xform move = Xform::translation(corner[0], corner[1], corner[2]);

    for (size_t k = 0; k < strips; k++) {
        // the fan built at the origin, moved to its corner
        const std::shared_ptr<PlateCurved> strip = std::make_shared<PlateCurved>(
            creases[k].transformed(move),
            creases[k + 1].transformed(move),
            inners[k].transformed(move),
            inners[k + 1].transformed(move),
            fmt::format("strip_{}", k)
        );
        add(strip, group);

        // its development, the proof it is cut flat
        const Development development(creases[k], creases[k + 1], samples);
        _distortion = std::max(_distortion, development.distortion);
    }
}
