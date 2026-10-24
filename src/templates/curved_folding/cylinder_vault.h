#pragma once
#include "wood_session.h"
#include "development.h"
#include "primitives.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel's structure systems through interpenetration of cylindrical surfaces (Structure Systems pp. 238-241): equal circular cylinders
/// over a regular polygon, one segment per side, meeting their neighbours in the vertical planes through the corners and the centre.
/// Two equal cylinders mirrored in a plane meet in that plane, so every crease is a planar curve, an ellipse arc, straight in plan and
/// curved in elevation, and the segments on both sides of it are each other's mirror image: the same planar curved crease the
/// reflection fold makes. Two kinds: the groin vault, every segment's generatrices running from its side toward the centre, in one
/// plane, sloping toward the centre or rising toward it (p. 238 left, pp. 239-240 over triangles, pentagons, hexagons and octagons, and
/// the cross-shaped plan of upright segments); and the pavilion or cloister vault, the generatrices along the sides, the segments
/// rising from the walls to a peak over the centre (p. 238 right, on four walls).
///
/// Every segment is one panel, a PlateCurved: its outer face the exact cylinder between its two end curves, the sections of the segment
/// by the vertical planes its generatrices run between, one multi-span rational arc each, a span per plane with a C0 knot where the
/// generatrices change from one plane to the next (the groin's crease, two half ellipses meeting at the apex); its inner face the
/// thickness under it, the cylinder of the smaller radius, whose sections by the same planes are its inner meridians, so two panels
/// across a crease share their side face; its caps the first and the last generatrix, drawn in from the corner or the apex to where
/// they are a millimetre long, so the solid has no zero-length edge. Every panel is bent about one axis and cut from a flat panel, as
/// glulam or curved cross-laminated timber is; its development is checked and drawn in the flat pattern beside the model.
///
/// A vault may be placed more than once, at a centre and a scale each: Engel's composition of cylindrical surfaces crossing diagonally
/// over a square grid plan (p. 241), a field of groin vaults with a larger one in the middle, is one groin vault placed on every bay of
/// the grid and once more, at twice the scale, over the four bays in the middle.
///
/// Fields: the polygon, the cylinders, the kind, the walls, the thickness and the placements; the plan, the panels and the flat pattern
/// are in the session.
class CylinderVault : public WoodSession {
public:
    /// Where a copy of the vault stands and how large it is.
    struct Placement {
        Point centre; // The polygon's centre on the ground.
        double scale; // The polygon's and the cylinders' size against the given side and radius.
    };

    /// How the cylinders sit on the polygon.
    enum class Kind {
        Groin, // A segment per side, its generatrices from the side toward the centre, tilted by the rise; the segments meet on the lines from the corners to the centre.
        Cloister // A segment per side, its generatrices along the side, its profile the arc from the side up to the peak over the centre; the segments meet on the lines from the corners to the centre.
    };

    const size_t sides; // The regular polygon's sides.
    const double edge; // mm, the polygon's side.
    const double radius; // mm, every cylinder; the arch over a side is a half circle when it is half the side (groin), the peak is above the centre (cloister).
    const double rise; // radians, the groin vault's generatrices rising toward the centre, negative sloping; 0 for the cloister.
    const Kind kind; // How the cylinders sit on the polygon.
    const double wall; // mm, the walls the cloister vault stands on, a flat panel under every side; 0 for none.
    const double thickness; // mm, every panel, under the cylinder.
    const std::vector<Placement> placements; // Every copy of the vault; one at the origin at scale 1 when none is given.

    /// The vault as the session named name.
    CylinderVault(
        size_t sides = 4,
        double edge = 12000.0,
        double radius = 6000.0,
        double rise = 0.0,
        Kind kind = Kind::Groin,
        double wall = 0.0,
        double thickness = 120.0,
        const std::string& name = "cylinder_vault",
        const std::vector<Placement>& placements = {}
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

    /// The placements of Engel's field (p. 241): a vault on every bay of a columns by rows grid of bays, except the middle 2 by 2, over which one vault stands at twice the scale; columns and rows even.
    static std::vector<Placement> field(size_t columns, size_t rows, double bay);

private:
    /// One cylinder segment: the circle its generatrices start from, their direction, the angles the segment spans, and the vertical
    /// planes around its footprint on the plan, their normals pointing in.
    struct Segment {
        Point centre; // The circle's centre, on the cylinder's axis.
        Vector x; // The circle's direction at angle 90 degrees.
        Vector y; // The circle's direction at angle 0.
        Vector axis; // The generatrices' unit direction.
        double radius; // mm, the cylinder, the vault's radius at the placement's scale.
        double start; // radians, the first generatrix.
        double end; // radians, the last generatrix.
        std::vector<Plane> footprint; // The vertical planes around the segment, normals pointing in.
        std::vector<Point> corners; // The footprint's corners on the ground, counter-clockwise.
        Point from; // The side's first corner on the ground, the wall under the segment.
        Point to; // The side's second corner.
    };

    double _distortion = 0.0;
    double _twist = 0.0;

    /// The polygon's corner i on the ground, counter-clockwise from the x axis, for the placement.
    Point corner(const Placement& placement, size_t i) const;

    /// The vertical plane through two points on the ground, its normal pointing toward a third.
    static Plane wall_plane(const Point& from, const Point& to, const Point& inside);

    /// Every segment of the kind over the polygon, at every placement, lifted onto the walls.
    std::vector<Segment> compute_segments() const;

    /// The point of the segment's circle at the angle.
    static Point on_circle(const Segment& segment, double radius, double angle);

    /// Where the generatrix at the angle enters and leaves the footprint: the distances along the axis from the circle, and the planes.
    std::array<std::pair<double, size_t>, 2> clip(const Segment& segment, double angle) const;

    /// The angles the segment's meridians change their plane at, between its first and its last generatrix drawn in to where they are
    /// a millimetre long: the spans of the panel's multi-span arcs.
    std::vector<double> compute_spans(const Segment& segment) const;

    /// The arc of the given radius over the spans, each span projected along the generatrices onto its plane at the given end (0 the
    /// entry, 1 the exit): one rational quadratic NURBS with a C0 knot at every change of plane, the panel's meridian.
    NurbsCurve compute_meridian(const Segment& segment, const std::vector<double>& spans, double radius, size_t end) const;

    /// The footprints, in the group plan.
    void add_plan(const std::vector<Segment>& segments);

    /// Every segment's panel, lamella_<segment>, in the group lamellas; its development returned for the flat pattern.
    std::vector<Development> add_panels(const std::vector<Segment>& segments);

    /// The flat walls under the cloister vault's sides, wall_<segment>, in the group walls.
    void add_walls(const std::vector<Segment>& segments);

    /// The flat cutting pattern beside the model, a row per segment, in the group flat.
    void add_flat(const std::vector<Development>& developments, double beside);
};

inline CylinderVault::CylinderVault(size_t sides, double edge, double radius, double rise, Kind kind, double wall, double thickness, const std::string& name, const std::vector<Placement>& placements)
    : WoodSession(name),
      sides(std::max<size_t>(sides, 3)),
      edge(edge),
      radius(radius),
      rise(rise),
      kind(kind),
      wall(wall),
      thickness(thickness),
      placements(placements.empty() ? std::vector<Placement>{{Point(0.0, 0.0, 0.0), 1.0}} : placements) {

    if (edge <= 0.0 || radius <= thickness || thickness <= 0.0 || wall < 0.0)
        throw std::invalid_argument("A cylinder vault needs a side, a radius larger than its thickness, a thickness and no negative wall");

    if (kind != Kind::Groin && std::abs(rise) > 1e-12)
        throw std::invalid_argument("Only the groin vault's generatrices rise or slope toward the centre");

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
}

inline Point CylinderVault::corner(const Placement& placement, size_t i) const {

    const double circumradius = placement.scale * 0.5 * edge / std::sin(Tolerance::PI / static_cast<double>(sides));
    const double angle = 2.0 * Tolerance::PI * static_cast<double>(i % sides) / static_cast<double>(sides);

    return placement.centre + Vector(circumradius * std::cos(angle), circumradius * std::sin(angle), 0.0);
}

inline std::vector<CylinderVault::Placement> CylinderVault::field(size_t columns, size_t rows, double bay) {

    if (columns < 2 || rows < 2 || columns % 2 != 0 || rows % 2 != 0)
        throw std::invalid_argument("A field has an even number of columns and rows, the middle four bays under the large vault");

    std::vector<Placement> placements;

    for (size_t column = 0; column < columns; column++)
        for (size_t row = 0; row < rows; row++) {
            const bool middle = (column + 1 == columns / 2 || column == columns / 2) && (row + 1 == rows / 2 || row == rows / 2);

            if (!middle)
                placements.push_back({Point(bay * (static_cast<double>(column) + 0.5 - 0.5 * static_cast<double>(columns)), bay * (static_cast<double>(row) + 0.5 - 0.5 * static_cast<double>(rows)), 0.0), 1.0});
        }

    placements.push_back({Point(0.0, 0.0, 0.0), 2.0});

    return placements;
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

    for (const Placement& placement : placements)
        for (size_t i = 0; i < sides; i++) {
            const Point& centre = placement.centre;
            const double edge = placement.scale * this->edge;
            const double radius = placement.scale * this->radius;
            const double apothem = 0.5 * edge / std::tan(Tolerance::PI / static_cast<double>(sides));
            const Point from = corner(placement, i);
            const Point to = corner(placement, i + 1);
            const Point middle = Point::mid_point(from, to);
            const Vector along = (to - from).normalized();
            const Vector outward = (middle - centre).normalized();
            const Vector lift(0.0, 0.0, wall);
            Segment segment;

            if (kind == Kind::Groin) {
                // the arch over the side, a circle across the generatrices that rise by the rise toward the centre, its springings at the corners
                if (radius < 0.5 * edge - 1e-6)
                    throw std::invalid_argument("A groin vault's radius is at least half its side");

                const double below = std::sqrt(std::max(0.0, radius * radius - 0.25 * edge * edge)) / std::cos(rise);
                segment.axis = -outward * std::cos(rise) + Vector(0.0, 0.0, std::sin(rise));
                segment.centre = middle - Vector(0.0, 0.0, below);
                segment.x = along;
                segment.y = (Vector(0.0, 0.0, 1.0) - segment.axis * std::sin(rise)).normalized();
                segment.start = -std::asin(0.5 * edge / radius);
                segment.end = -segment.start;
            } else {
                // the arc from the side up to the peak over the centre, the generatrices along the side, on the wall
                if (radius < apothem - 1e-6)
                    throw std::invalid_argument("A cloister vault's radius is at least the polygon's apothem");

                const double peak = radius - std::sqrt(std::max(0.0, radius * radius - apothem * apothem));
                segment.axis = along;
                segment.centre = centre + lift + Vector(0.0, 0.0, peak - radius);
                segment.x = outward;
                segment.y = Vector(0.0, 0.0, 1.0);
                segment.start = 0.0;
                segment.end = std::atan2(apothem, radius - peak);
            }

            // the vertical planes around the footprint, normals pointing in, and the side under the segment
            segment.radius = radius;
            segment.corners = {from, to, centre};
            segment.from = from;
            segment.to = to;

            for (size_t c = 0; c < segment.corners.size(); c++)
                segment.footprint.push_back(wall_plane(segment.corners[c], segment.corners[(c + 1) % segment.corners.size()], Point::centroid(segment.corners)));

            segments.push_back(segment);
        }

    return segments;
}

inline Point CylinderVault::on_circle(const Segment& segment, double radius, double angle) {
    return segment.centre + segment.x * (radius * std::sin(angle)) + segment.y * (radius * std::cos(angle));
}

inline std::array<std::pair<double, size_t>, 2> CylinderVault::clip(const Segment& segment, double angle) const {

    // the generatrix through the circle point, clipped in plan by every plane of the footprint: the latest entry and the earliest exit, as distances along the generatrix
    const Point point = on_circle(segment, segment.radius, angle);
    const Point plan(point[0], point[1], 0.0);
    const Vector direction = Vector(segment.axis[0], segment.axis[1], 0.0).normalized();
    const double flat = Vector(segment.axis[0], segment.axis[1], 0.0).magnitude();
    std::pair<double, size_t> entry = {-std::numeric_limits<double>::infinity(), 0};
    std::pair<double, size_t> exit = {std::numeric_limits<double>::infinity(), 0};

    for (size_t p = 0; p < segment.footprint.size(); p++) {
        const Plane& plane = segment.footprint[p];
        const double speed = plane.z_axis().dot(direction);
        const double distance = plane.z_axis().dot(plane.origin() - plan);

        if (std::abs(speed) < 1e-12)
            continue;

        const double s = distance / speed / flat;

        if (speed > 0.0 && s > entry.first)
            entry = {s, p};

        if (speed < 0.0 && s < exit.first)
            exit = {s, p};
    }

    return {entry, exit};
}

inline std::vector<double> CylinderVault::compute_spans(const Segment& segment) const {

    const auto length = [&](double angle) {
        const std::array<std::pair<double, size_t>, 2> ends = clip(segment, angle);
        return ends[1].first - ends[0].first;
    };
    const auto planes = [&](double angle) {
        const std::array<std::pair<double, size_t>, 2> ends = clip(segment, angle);
        return std::make_pair(ends[0].second, ends[1].second);
    };

    // the first and the last generatrix drawn in to where they are a millimetre long, from the corner or the apex where they vanish
    double first = segment.start;
    double last = segment.end;

    for (double* end : {&first, &last})
        if (length(*end) < 1.0) {
            double short_angle = *end;
            double long_angle = end == &first ? last : first;

            for (int step = 0; step < 60; step++) {
                const double middle = 0.5 * (short_angle + long_angle);
                (length(middle) < 1.0 ? short_angle : long_angle) = middle;
            }

            *end = long_angle;
        }

    // every angle between them where the entry or the exit plane changes, found between fine samples by bisection
    std::vector<double> spans = {first};
    const size_t fine = 1024;

    for (size_t k = 0; k < fine; k++) {
        double low = first + (last - first) * static_cast<double>(k) / static_cast<double>(fine);
        double high = first + (last - first) * static_cast<double>(k + 1) / static_cast<double>(fine);

        if (planes(low) == planes(high))
            continue;

        const std::pair<size_t, size_t> at_low = planes(low);

        for (int step = 0; step < 50; step++) {
            const double middle = 0.5 * (low + high);
            (planes(middle) == at_low ? low : high) = middle;
        }

        if (0.5 * (low + high) - spans.back() > 1e-6)
            spans.push_back(0.5 * (low + high));
    }

    if (last - spans.back() > 1e-6)
        spans.push_back(last);

    return spans;
}

inline NurbsCurve CylinderVault::compute_meridian(const Segment& segment, const std::vector<double>& spans, double radius, size_t end) const {

    // every span of the arc as a rational quadratic Bezier projected along the generatrices onto the plane the span's generatrices end on
    std::vector<Point> points;
    std::vector<double> weights;
    std::vector<double> knots = {0.0};
    std::vector<int> mults = {3};

    for (size_t s = 0; s + 1 < spans.size(); s++) {
        const double from = spans[s];
        const double to = spans[s + 1];
        const Plane& plane = segment.footprint[clip(segment, 0.5 * (from + to))[end].second];
        const NurbsCurve arc = Primitives::arc(on_circle(segment, radius, from), on_circle(segment, radius, 0.5 * (from + to)), on_circle(segment, radius, to)).transformed(Xform::project_to_plane_by_axis(plane, segment.axis));

        for (int c = s == 0 ? 0 : 1; c < 3; c++) {
            points.push_back(arc.get_cv(c));
            weights.push_back(arc.weight(c));
        }

        knots.push_back((to - spans.front()) / (spans.back() - spans.front()));
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
        // the panel's four meridians: the outer and the inner arc over the segment's spans, on the planes the generatrices enter and leave by
        const std::vector<double> spans = compute_spans(segments[i]);
        const NurbsCurve outer_a = compute_meridian(segments[i], spans, segments[i].radius, 0);
        const NurbsCurve outer_b = compute_meridian(segments[i], spans, segments[i].radius, 1);
        const NurbsCurve inner_a = compute_meridian(segments[i], spans, segments[i].radius - thickness, 0);
        const NurbsCurve inner_b = compute_meridian(segments[i], spans, segments[i].radius - thickness, 1);
        add(std::make_shared<PlateCurved>(outer_a, outer_b, inner_a, inner_b, fmt::format("lamella_{}", i)), group);

        // its development, the proof it is cut flat
        developments.emplace_back(outer_a, outer_b, 96);
        _distortion = std::max(_distortion, developments.back().distortion);
        _twist = std::max(_twist, developments.back().twist);
    }

    return developments;
}

inline void CylinderVault::add_walls(const std::vector<Segment>& segments) {

    const std::shared_ptr<TreeNode> group = add_group("walls");

    for (size_t i = 0; i < segments.size(); i++) {
        // a flat panel under the side: its outer face the side's plane from the ground to the wall's top, its inner face the thickness inward
        const Segment& segment = segments[i];
        const Vector inward = (Point::centroid(segment.corners) - Point::mid_point(segment.from, segment.to)).normalized() * thickness;
        const Vector top(0.0, 0.0, wall);
        const NurbsCurve outer_a = NurbsCurve::create(false, 1, {segment.from, segment.to});
        const NurbsCurve outer_b = NurbsCurve::create(false, 1, {segment.from + top, segment.to + top});
        const NurbsCurve inner_a = NurbsCurve::create(false, 1, {segment.from + inward, segment.to + inward});
        const NurbsCurve inner_b = NurbsCurve::create(false, 1, {segment.from + inward + top, segment.to + inward + top});
        add(std::make_shared<PlateCurved>(outer_a, outer_b, inner_a, inner_b, fmt::format("wall_{}", i)), group);
    }
}

inline void CylinderVault::add_flat(const std::vector<Development>& developments, double beside) {

    const std::shared_ptr<TreeNode> flat = add_group("flat");

    // the pattern starts beside the model's largest x; a row per panel, each as high as the tallest development
    double largest = -std::numeric_limits<double>::infinity();
    double tallest = 0.0;

    for (const std::shared_ptr<Element>& element : *objects.elements)
        if (const std::shared_ptr<PlateCurved> panel = std::dynamic_pointer_cast<PlateCurved>(element))
            for (const NurbsCurve* curve : {&panel->outer_a, &panel->outer_b})
                for (size_t k = 0; k <= 16; k++)
                    largest = std::max(largest, curve->point_at(curve->domain_start() + (curve->domain_end() - curve->domain_start()) * static_cast<double>(k) / 16.0)[0]);

    for (const Development& development : developments)
        for (size_t k = 0; k < development.outline.point_count(); k++)
            tallest = std::max(tallest, development.outline.get_point(k)[1]);

    for (size_t i = 0; i < developments.size(); i++) {
        Polyline outline = developments[i].outline.transformed(Xform::translation(largest + beside, static_cast<double>(i) * (tallest + beside), 0.0));
        outline.name = fmt::format("flat_{}", i);
        add_polyline(outline, flat);
    }
}
