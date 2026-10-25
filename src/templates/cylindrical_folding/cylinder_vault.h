#pragma once
#include "wood_session.h"
#include "../development.h"
#include "primitives.h"

using namespace session_cpp;
using namespace wood_session;

/// Equal circular cylinders over a regular polygon.
class CylinderVault : public WoodSession {
public:
    /// How the cylinders sit on the polygon.
    enum class Kind { Groin, Cloister };

    const size_t sides; // The regular polygon's sides.
    const double edge; // mm, the polygon's side.
    const double
        radius; // mm, every cylinder; at least half the side (groin) or the apothem (cloister).
    const Kind kind; // How the cylinders sit on the polygon.
    const double
        wall; // mm, the walls the cloister vault stands on, a flat panel under every side; 0 for none.
    const double thickness; // mm, every panel, under the cylinder.

private:
    double _distortion = 0.0; // The largest relative length error of any panel's development.
    double _twist = 0.0; // The largest twist of any panel's generatrices out of the tangent planes.

public:
    /// The vault as the session named name.
    CylinderVault(
        size_t sides = 4,
        double edge = 12000.0,
        double radius = 6000.0,
        Kind kind = Kind::Groin,
        double wall = 0.0,
        double thickness = 120.0,
        const std::string& name = "cylinder_vault"
    );

    /// Not copied, as a session is not.
    CylinderVault(const CylinderVault&) = delete;

    /// Not assigned, as it is not copied.
    CylinderVault& operator=(const CylinderVault&) = delete;

    /// The largest relative length error of any panel's development, the proof every panel is cut flat.
    double distortion() const {
        return _distortion;
    }

    /// The largest twist of any panel's generatrices out of the tangent planes, 0 on a cylinder.
    double twist() const {
        return _twist;
    }

private:
    /// One cylinder segment: its circle, its generatrices, the angles it spans and the vertical planes around its footprint.
    struct Segment {
        Point centre; // The circle's centre, on the cylinder's axis.
        Vector x; // The circle's direction at angle 90 degrees.
        Vector y; // The circle's direction at angle 0.
        Vector axis; // The generatrices' unit direction.
        double radius; // mm, the cylinder.
        double start; // radians, the first generatrix.
        double end; // radians, the last generatrix.
        std::vector<Plane>
            footprint; // The vertical planes around the segment, normals pointing in.
        std::vector<Point> corners; // The footprint's corners on the ground, counter-clockwise.
        Point from; // The side's first corner on the ground, the wall under the segment.
        Point to; // The side's second corner.
    };

    /// The polygon's corner i on the ground, counter-clockwise from the x axis, around the origin.
    Point corner(size_t i) const;

    /// The vertical plane through two points on the ground, its normal pointing toward a third.
    static Plane wall_plane(const Point& from, const Point& to, const Point& inside);

    /// Every segment of the kind over the polygon, lifted onto the walls.
    std::vector<Segment> compute_segments() const;

    /// The point of the segment's circle at the angle.
    static Point on_circle(const Segment& segment, double radius, double angle);

    /// Where the generatrix at the angle enters and leaves the footprint: the distances along the axis and the planes.
    std::array<std::pair<double, size_t>, 2>
    clip(const Segment& segment, double radius, double angle) const;

    /// mm, the generatrix at the angle inside the footprint.
    double generatrix_length(const Segment& segment, double radius, double angle) const;

    /// The planes the generatrix at the angle enters and leaves the footprint by.
    std::pair<size_t, size_t>
    generatrix_planes(const Segment& segment, double radius, double angle) const;

    /// The angle between end and other where the generatrix is a millimetre long, by bisection.
    double drawn_in(const Segment& segment, double radius, double end, double other) const;

    /// The angles the meridians of the cylinder of the radius change their plane at, the spans of the panel's multi-span arcs.
    std::vector<double> compute_spans(const Segment& segment, double radius) const;

    /// The meridian at the given end (0 the entry, 1 the exit).
    NurbsCurve compute_meridian(
        const Segment& segment, const std::vector<double>& spans, double radius, size_t end
    ) const;

    /// The footprints, in the group plan.
    void add_plan(const std::vector<Segment>& segments);

    /// Every segment's panel, lamella_<segment>, in the group lamellas; its development returned for the flat pattern.
    std::vector<Development> add_panels(const std::vector<Segment>& segments);

    /// The flat walls under the cloister vault's sides, wall_<segment>, in the group walls.
    void add_walls(const std::vector<Segment>& segments);

    /// The flat cutting pattern beside the model, a row per segment, in the group flat.
    void add_flat(const std::vector<Development>& developments, double beside);
};

inline CylinderVault::CylinderVault(
    size_t sides,
    double edge,
    double radius,
    Kind kind,
    double wall,
    double thickness,
    const std::string& name
)
    : WoodSession(name), sides(std::max<size_t>(sides, 3)), edge(edge), radius(radius), kind(kind),
      wall(wall), thickness(thickness) {

    if (edge <= 0.0 || radius <= thickness || thickness <= 0.0 || wall < 0.0)
        throw std::invalid_argument("A cylinder vault needs a side, a radius larger than its "
                                    "thickness, a thickness and no negative wall");

    if (kind != Kind::Cloister && wall > 0.0)
        throw std::invalid_argument("Only the cloister vault stands on walls");

    // segments: every cylinder with its circle, its generatrices and its footprint
    const std::vector<Segment> segments = compute_segments();

    // plan: the footprints on the ground
    add_plan(segments);

    // panels: every segment one curved plate between the planes its generatrices run between
    const std::vector<Development> developments = add_panels(segments);

    // walls: a flat panel under every side the cloister vault stands on
    if (wall > 0.0)
        add_walls(segments);

    // flat: the cutting pattern beside the model, every panel unfolded
    add_flat(developments, 0.25 * edge);

    // contacts: every two panels that share a face, found by the session's contact detection
    compute_face_contacts(0);
}

inline Point CylinderVault::corner(size_t i) const {

    const double circumradius = 0.5 * edge / std::sin(Tolerance::PI / static_cast<double>(sides));
    const double angle =
        2.0 * Tolerance::PI * static_cast<double>(i % sides) / static_cast<double>(sides);

    return Point(circumradius * std::cos(angle), circumradius * std::sin(angle), 0.0);
}

inline Plane CylinderVault::wall_plane(const Point& from, const Point& to, const Point& inside) {

    Vector normal = (to - from).cross(Vector(0.0, 0.0, 1.0));
    normal.normalize_self();

    if (normal.dot(inside - from) < 0.0)
        normal = -normal;

    return Plane::from_point_normal(from, normal);
}

inline std::vector<CylinderVault::Segment> CylinderVault::compute_segments() const {

    std::vector<Segment> segments;
    const Point centre(0.0, 0.0, 0.0);
    const double apothem = 0.5 * edge / std::tan(Tolerance::PI / static_cast<double>(sides));

    for (size_t i = 0; i < sides; i++) {
        const Point from = corner(i);
        const Point to = corner(i + 1);
        const Point middle = Point::mid_point(from, to);
        const Vector along = (to - from).normalized();
        const Vector outward = (middle - centre).normalized();
        Segment segment;

        if (kind == Kind::Groin) {
            if (radius < 0.5 * edge - 1e-6)
                throw std::invalid_argument("A groin vault's radius is at least half its side");

            segment.axis = -outward;
            segment.centre =
                middle -
                Vector(0.0, 0.0, std::sqrt(std::max(0.0, radius * radius - 0.25 * edge * edge)));
            segment.x = along;
            segment.y = Vector(0.0, 0.0, 1.0);
            segment.start = -std::asin(0.5 * edge / radius);
            segment.end = -segment.start;
        } else {
            if (radius < apothem - 1e-6)
                throw std::invalid_argument(
                    "A cloister vault's radius is at least the polygon's apothem"
                );

            const double peak =
                radius - std::sqrt(std::max(0.0, radius * radius - apothem * apothem));
            segment.axis = along;
            segment.centre = centre + Vector(0.0, 0.0, wall + peak - radius);
            segment.x = outward;
            segment.y = Vector(0.0, 0.0, 1.0);
            segment.start = 0.0;
            segment.end = std::atan2(apothem, radius - peak);
        }

        segment.radius = radius;
        segment.corners = {from, to, centre};
        segment.from = from;
        segment.to = to;
        const size_t count = segment.corners.size();
        const Point inside = Point::centroid(segment.corners);

        for (size_t c = 0; c < count; c++)
            segment.footprint.push_back(
                wall_plane(segment.corners[c], segment.corners[(c + 1) % count], inside)
            );

        segments.push_back(segment);
    }

    return segments;
}

inline Point CylinderVault::on_circle(const Segment& segment, double radius, double angle) {

    return segment.centre + segment.x * (radius * std::sin(angle)) +
           segment.y * (radius * std::cos(angle));
}

inline std::array<std::pair<double, size_t>, 2>
CylinderVault::clip(const Segment& segment, double radius, double angle) const {

    // the generatrix through the circle point clipped by every plane of the footprint: the latest entry and the earliest exit
    const Point point = on_circle(segment, radius, angle);
    std::pair<double, size_t> entry = {-std::numeric_limits<double>::infinity(), 0};
    std::pair<double, size_t> exit = {std::numeric_limits<double>::infinity(), 0};

    for (size_t p = 0; p < segment.footprint.size(); p++) {
        const Plane& plane = segment.footprint[p];
        const double speed = plane.z_axis().dot(segment.axis);
        const double distance = plane.z_axis().dot(plane.origin() - point);

        if (std::abs(speed) < 1e-12)
            continue;

        const double s = distance / speed;

        if (speed > 0.0 && s > entry.first)
            entry = {s, p};

        if (speed < 0.0 && s < exit.first)
            exit = {s, p};
    }

    return {entry, exit};
}

inline double
CylinderVault::generatrix_length(const Segment& segment, double radius, double angle) const {

    const std::array<std::pair<double, size_t>, 2> ends = clip(segment, radius, angle);

    return ends[1].first - ends[0].first;
}

inline std::pair<size_t, size_t>
CylinderVault::generatrix_planes(const Segment& segment, double radius, double angle) const {

    const std::array<std::pair<double, size_t>, 2> ends = clip(segment, radius, angle);

    return std::make_pair(ends[0].second, ends[1].second);
}

inline double
CylinderVault::drawn_in(const Segment& segment, double radius, double end, double other) const {

    double short_angle = end;
    double long_angle = other;

    for (int step = 0; step < 60; step++) {
        const double middle = 0.5 * (short_angle + long_angle);

        if (generatrix_length(segment, radius, middle) < 1.0)
            short_angle = middle;
        else
            long_angle = middle;
    }

    return long_angle;
}

inline std::vector<double>
CylinderVault::compute_spans(const Segment& segment, double radius) const {

    double first = segment.start;
    double last = segment.end;

    if (generatrix_length(segment, radius, first) < 1.0)
        first = drawn_in(segment, radius, first, last);

    if (generatrix_length(segment, radius, last) < 1.0)
        last = drawn_in(segment, radius, last, first);

    std::vector<double> spans = {first};
    const size_t fine = 1024;

    for (size_t k = 0; k < fine; k++) {
        double low = first + (last - first) * static_cast<double>(k) / static_cast<double>(fine);
        double high =
            first + (last - first) * static_cast<double>(k + 1) / static_cast<double>(fine);
        const std::pair<size_t, size_t> at_low = generatrix_planes(segment, radius, low);

        if (at_low == generatrix_planes(segment, radius, high))
            continue;

        for (int step = 0; step < 50; step++) {
            const double middle = 0.5 * (low + high);

            if (generatrix_planes(segment, radius, middle) == at_low)
                low = middle;
            else
                high = middle;
        }

        if (0.5 * (low + high) - spans.back() > 1e-6)
            spans.push_back(0.5 * (low + high));
    }

    if (last - spans.back() > 1e-6)
        spans.push_back(last);

    return spans;
}

inline NurbsCurve CylinderVault::compute_meridian(
    const Segment& segment, const std::vector<double>& spans, double radius, size_t end
) const {

    std::vector<Point> points;
    std::vector<double> weights;
    std::vector<double> knots = {0.0};
    std::vector<int> mults = {3};

    for (size_t s = 0; s + 1 < spans.size(); s++) {
        const double from = spans[s];
        const double to = spans[s + 1];
        const double middle = 0.5 * (from + to);
        const size_t plane = clip(segment, radius, middle)[end].second;
        const Xform onto = Xform::project_to_plane_by_axis(segment.footprint[plane], segment.axis);
        const Point a = on_circle(segment, radius, from);
        const Point b = on_circle(segment, radius, middle);
        const Point c = on_circle(segment, radius, to);
        const NurbsCurve arc = Primitives::arc(a, b, c).transformed(onto);

        for (int cv = s == 0 ? 0 : 1; cv < 3; cv++) {
            points.push_back(arc.get_cv(cv));
            weights.push_back(arc.weight(cv));
        }

        knots.push_back(static_cast<double>(s + 1) / static_cast<double>(spans.size() - 1));
        mults.push_back(s + 2 < spans.size() ? 2 : 3);
    }

    return NurbsCurve::create_from_parameters(points, weights, knots, mults, 2);
}

inline void CylinderVault::add_plan(const std::vector<Segment>& segments) {

    const std::shared_ptr<TreeNode> plan = add_group("plan");

    for (size_t i = 0; i < segments.size(); i++) {
        std::vector<Point> loop = segments[i].corners;
        loop.push_back(loop.front());
        Polyline footprint(loop);
        footprint.name = fmt::format("footprint_{}", i);
        add_polyline(footprint, plan);
    }
}

inline std::vector<Development> CylinderVault::add_panels(const std::vector<Segment>& segments) {

    const std::shared_ptr<TreeNode> group = add_group("lamellas");
    std::vector<Development> developments;

    for (size_t i = 0; i < segments.size(); i++) {
        const Segment& segment = segments[i];
        const double inner = segment.radius - thickness;
        const std::vector<double> outer_spans = compute_spans(segment, segment.radius);
        const std::vector<double> inner_spans = compute_spans(segment, inner);
        const NurbsCurve outer_a = compute_meridian(segment, outer_spans, segment.radius, 0);
        const NurbsCurve outer_b = compute_meridian(segment, outer_spans, segment.radius, 1);
        const NurbsCurve inner_a = compute_meridian(segment, inner_spans, inner, 0);
        const NurbsCurve inner_b = compute_meridian(segment, inner_spans, inner, 1);
        const std::shared_ptr<PlateCurved> panel = std::make_shared<PlateCurved>(
            outer_a, outer_b, inner_a, inner_b, fmt::format("lamella_{}", i)
        );
        add(panel, group);

        developments.emplace_back(outer_a, outer_b, 96);
        _distortion = std::max(_distortion, developments.back().distortion);
        _twist = std::max(_twist, developments.back().twist);
    }

    return developments;
}

inline void CylinderVault::add_walls(const std::vector<Segment>& segments) {

    const std::shared_ptr<TreeNode> group = add_group("walls");

    const double circumradius = corner(0).distance(Point(0.0, 0.0, 0.0));
    const double inset =
        1.0 - thickness / std::cos(Tolerance::PI / static_cast<double>(sides)) / circumradius;

    for (size_t i = 0; i < segments.size(); i++) {
        // a flat panel under the side: its outer face the side's plane from the ground to the wall's top, its inner face the thickness inward
        const Segment& segment = segments[i];
        const Point from(segment.from[0] * inset, segment.from[1] * inset, 0.0);
        const Point to(segment.to[0] * inset, segment.to[1] * inset, 0.0);
        const Vector top(0.0, 0.0, wall);
        const NurbsCurve outer_a = NurbsCurve::create(false, 1, {segment.from, segment.to});
        const NurbsCurve outer_b =
            NurbsCurve::create(false, 1, {segment.from + top, segment.to + top});
        const NurbsCurve inner_a = NurbsCurve::create(false, 1, {from, to});
        const NurbsCurve inner_b = NurbsCurve::create(false, 1, {from + top, to + top});
        const std::shared_ptr<PlateCurved> panel = std::make_shared<PlateCurved>(
            outer_a, outer_b, inner_a, inner_b, fmt::format("wall_{}", i)
        );
        add(panel, group);
    }
}

inline void CylinderVault::add_flat(const std::vector<Development>& developments, double beside) {

    const std::shared_ptr<TreeNode> flat = add_group("flat");

    double largest = -std::numeric_limits<double>::infinity();
    double tallest = 0.0;

    for (const std::shared_ptr<Element>& element : *objects.elements)
        if (const std::shared_ptr<PlateCurved> panel =
                std::dynamic_pointer_cast<PlateCurved>(element))
            for (const NurbsCurve* curve : {&panel->outer_a, &panel->outer_b})
                for (size_t k = 0; k <= 16; k++) {
                    const double domain = curve->domain_end() - curve->domain_start();
                    const double t = curve->domain_start() + domain * static_cast<double>(k) / 16.0;
                    largest = std::max(largest, curve->point_at(t)[0]);
                }

    for (const Development& development : developments)
        for (size_t k = 0; k < development.outline.point_count(); k++)
            tallest = std::max(tallest, development.outline.get_point(k)[1]);

    for (size_t i = 0; i < developments.size(); i++) {
        const Xform place =
            Xform::translation(largest + beside, static_cast<double>(i) * (tallest + beside), 0.0);
        Polyline outline = developments[i].outline.transformed(place);
        outline.name = fmt::format("flat_{}", i);
        add_polyline(outline, flat);
    }
}
