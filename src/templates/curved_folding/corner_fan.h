#pragma once
#include "wood_session.h"
#include "development.h"

using namespace session_cpp;
using namespace wood_session;

/// A fan of developable strips folded from one bottom corner on parabolic creases, each the mirror image of the one before.
class CornerFan : public WoodSession {
public:
    const double radius; // mm, how far every strip's middle reaches from the corner in plan.
    const double height; // mm, the middle of every strip's top above the corner.
    const size_t strips; // Strips dividing the quarter between the two box faces at the corner.
    const double lift; // mm, the parabola's middle control point above its chord, below when negative.
    const double drop; // mm, how much lower every second fold ends than the folds beside it.
    const double foot; // The share of the way up where the strips start, cut short of the corner.
    const double thickness; // mm, every strip.
    const size_t samples; // Points every inner crease is interpolated through.

private:
    double _distortion = 0.0; // The largest relative length error of any strip's development.

public:
    /// The fan as the session named name.
    CornerFan(
        double radius = 6000.0,
        double height = 4000.0,
        size_t strips = 6,
        double lift = 1200.0,
        double drop = 800.0,
        double foot = 0.15,
        double thickness = 40.0,
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
    /// The profile's three control points: the corner, the lifted middle and the top, in the plane halving the first strip.
    std::array<Point, 3> compute_profile() const;

    /// The first strip's four creases: outer and inner on the box face y = 0, then on the first fold plane.
    std::array<NurbsCurve, 4> compute_first(const std::array<Point, 3>& profile) const;

    /// Every strip, strip_<k> in the group strips: the first, mirrored in every fold plane in turn.
    void add_strips(const std::array<NurbsCurve, 4>& first);

    /// The rulings' unit direction: square to the first strip's middle in plan, tilted by the drop.
    Vector ruling() const;

    /// The angle every strip spans in plan.
    double wedge() const;

    /// The point where the ruling through point meets the vertical plane through the corner at angle.
    Point on_plane(const Point& point, double angle) const;

    /// The point of the profile's parabola at s, from the corner at 0 to the top at 1.
    static Point bezier(const std::array<Point, 3>& profile, double s);

    /// The curve's points mirrored in the vertical plane through the corner at mirror_angle, when mirror, then turned by turn.
    static NurbsCurve placed(const NurbsCurve& curve, bool mirror, double mirror_angle, double turn);
};

inline CornerFan::CornerFan(
    double radius,
    double height,
    size_t strips,
    double lift,
    double drop,
    double foot,
    double thickness,
    const std::string& name,
    size_t samples
)
    : WoodSession(name),
      radius(radius),
      height(height),
      strips(std::max<size_t>(strips, 1)),
      lift(lift),
      drop(drop),
      foot(foot),
      thickness(thickness),
      samples(std::max<size_t>(samples, 8)) {

    if (radius <= 0.0 || height <= 0.0 || foot <= 0.0 || foot >= 1.0 || thickness <= 0.0)
        throw std::invalid_argument("A corner fan needs a radius, a height, a foot between 0 and 1 and a thickness");

    // profile: the parabola from three points, the corner, the middle lifted above the chord and the top
    const std::array<Point, 3> profile = compute_profile();

    // first strip: the parabolic cylinder through the profile, between the box face y = 0 and the first fold plane
    const std::array<NurbsCurve, 4> first = compute_first(profile);

    // strips: the first mirrored in every fold plane in turn, so every fold is a parabola in its plane
    add_strips(first);

    // contacts: every two strips that share their fold plane, found by the session's contact detection
    compute_face_contacts(0);
}

inline double CornerFan::wedge() const {

    return 0.5 * Tolerance::PI / static_cast<double>(strips);
}

inline Vector CornerFan::ruling() const {

    // square to the strip's middle in plan, rising so the two folds' tops differ by the drop across the strip
    const double middle = 0.5 * wedge();
    const Vector across(-std::sin(middle), std::cos(middle), 0.0);
    const double half = radius * std::tan(middle);
    const Vector direction = across + Vector(0.0, 0.0, 0.5 * drop / half);

    return direction.normalized();
}

inline std::array<Point, 3> CornerFan::compute_profile() const {

    const double middle = 0.5 * wedge();
    const Point corner(0.0, 0.0, 0.0);
    const Point top(radius * std::cos(middle), radius * std::sin(middle), height);
    const Point control = Point::mid_point(corner, top) + Vector(0.0, 0.0, lift);

    return {corner, control, top};
}

inline Point CornerFan::bezier(const std::array<Point, 3>& profile, double s) {

    const Vector a(profile[0][0], profile[0][1], profile[0][2]);
    const Vector b(profile[1][0], profile[1][1], profile[1][2]);
    const Vector c(profile[2][0], profile[2][1], profile[2][2]);
    const Vector point = a * ((1.0 - s) * (1.0 - s)) + b * (2.0 * s * (1.0 - s)) + c * (s * s);

    return Point(point[0], point[1], point[2]);
}

inline Point CornerFan::on_plane(const Point& point, double angle) const {

    // the vertical plane through the corner at angle, met along the ruling
    const Vector normal(-std::sin(angle), std::cos(angle), 0.0);
    const Vector direction = ruling();
    const Vector position(point[0], point[1], point[2]);
    const double along = -normal.dot(position) / normal.dot(direction);

    return point + direction * along;
}

inline std::array<NurbsCurve, 4> CornerFan::compute_first(const std::array<Point, 3>& profile) const {

    // the outer creases: the profile cut at foot by de Casteljau, its three control points carried along the rulings onto
    // the two planes, a parallel projection that keeps it a parabola
    const Point start = bezier(profile, foot);
    const Vector b(profile[1][0], profile[1][1], profile[1][2]);
    const Vector c(profile[2][0], profile[2][1], profile[2][2]);
    const Vector between = b * (1.0 - foot) + c * foot;
    const std::array<Point, 3> cut = {start, Point(between[0], between[1], between[2]), profile[2]};
    std::array<NurbsCurve, 4> creases;

    for (size_t plane = 0; plane < 2; plane++) {
        const double angle = plane == 0 ? 0.0 : wedge();
        std::vector<Point> controls;

        for (const Point& point : cut)
            controls.push_back(on_plane(point, angle));

        creases[plane] = NurbsCurve::create(false, 2, controls);
    }

    // the inner creases: the profile moved the thickness along the cylinder's normal, below the sheet, carried the same way
    const Vector direction = ruling();
    std::array<std::vector<Point>, 2> inner;

    for (size_t i = 0; i <= samples; i++) {
        const double s = foot + (1.0 - foot) * static_cast<double>(i) / static_cast<double>(samples);
        const Point point = bezier(profile, s);
        const Vector tangent = (profile[1] - profile[0]) * (2.0 * (1.0 - s)) + (profile[2] - profile[1]) * (2.0 * s);
        Vector normal = direction.cross(tangent).normalized();

        if (normal[2] > 0.0)
            normal = -normal;

        const Point offset = point + normal * thickness;
        inner[0].push_back(on_plane(offset, 0.0));
        inner[1].push_back(on_plane(offset, wedge()));
    }

    creases[2] = NurbsCurve::create_interpolated(inner[0], CurveNurbsKnotStyle::Uniform);
    creases[3] = NurbsCurve::create_interpolated(inner[1], CurveNurbsKnotStyle::Uniform);

    return creases;
}

inline NurbsCurve CornerFan::placed(const NurbsCurve& curve, bool mirror, double mirror_angle, double turn) {

    const Vector normal(-std::sin(mirror_angle), std::cos(mirror_angle), 0.0);
    const double c = std::cos(turn);
    const double s = std::sin(turn);
    NurbsCurve result = curve;

    for (int i = 0; i < curve.cv_count(); i++) {
        Point point = curve.get_cv(i);

        if (mirror)
            point = point - normal * (2.0 * normal.dot(Vector(point[0], point[1], point[2])));

        result.set_cv(i, Point(c * point[0] - s * point[1], s * point[0] + c * point[1], point[2]));
    }

    return result;
}

inline void CornerFan::add_strips(const std::array<NurbsCurve, 4>& first) {

    const std::shared_ptr<TreeNode> group = add_group("strips");

    for (size_t k = 0; k < strips; k++) {
        // an even strip the first turned by k wedges, an odd one the first mirrored in its fold plane and turned by k - 1
        const bool mirror = k % 2 == 1;
        const double turn = wedge() * static_cast<double>(k - k % 2);
        std::array<NurbsCurve, 4> curves;

        for (size_t c = 0; c < 4; c++)
            curves[c] = placed(first[c], mirror, wedge(), turn);

        const std::shared_ptr<PlateCurved> strip = std::make_shared<PlateCurved>(
            curves[0],
            curves[1],
            curves[2],
            curves[3],
            fmt::format("strip_{}", k)
        );
        add(strip, group);

        // its development, the proof it is cut flat
        const Development development(curves[0], curves[1], samples);
        _distortion = std::max(_distortion, development.distortion);
    }
}
