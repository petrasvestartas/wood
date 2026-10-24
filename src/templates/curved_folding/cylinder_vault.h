#pragma once
#include "wood_session.h"
#include "development.h"
#include "primitives.h"

using namespace session_cpp;
using namespace wood_session;

/// Engel's structure systems through interpenetration of cylindrical surfaces (Structure Systems pp. 238-240): equal circular cylinders
/// over a regular polygon, one segment per side or per corner, meeting their neighbours in the vertical planes of the polygon's
/// symmetry. Two equal cylinders mirrored in a plane meet in that plane, so every crease is a planar curve, an ellipse arc, straight in
/// plan and curved in elevation, and the segments on both sides of it are each other's mirror image: the same planar curved crease the
/// reflection fold makes. Three kinds: the groin vault, every segment's generatrices running from its side toward the centre, in one
/// plane, sloping toward the centre or rising toward it (p. 238 left, pp. 239-240 over triangles, pentagons, hexagons and octagons, and
/// the cross-shaped plan of upright segments); the diagonal vault, a segment per corner with its generatrices across the corner's
/// diagonal, meeting on the lines from the sides' middles to the centre (p. 238 right); and the cloister vault, the generatrices along
/// the sides.
///
/// Every segment is cut along its generatrices into lamellas, each a PlateCurved: its outer face the exact cylinder between its two
/// end curves, the sections of the segment by the two vertical planes its generatrices run between, a side of the polygon or a
/// crease; its inner face the thickness under it, the cylinder of the smaller radius, whose sections by the same planes are its inner
/// meridians, so two lamellas across a crease share their side face; its caps the generatrices at its two ends, shared with the next
/// lamella of the segment. A generatrix is straight across the lamella, so every lamella is a strip bent about one axis, cut from a
/// flat panel, as glulam or curved cross-laminated timber is; its development is checked and drawn in the flat pattern beside the model.
///
/// A vault may be placed more than once, at a centre and a scale each: Engel's composition of cylindrical surfaces crossing diagonally
/// over a square grid plan (p. 241), a field of groin vaults with a larger one in the middle, is one groin vault placed on every bay of
/// the grid and once more, at twice the scale, over the four bays in the middle.
///
/// Fields: the polygon, the cylinders, the kind, the lamellas per segment, the thickness and the placements; the plan, the lamellas and
/// the flat pattern are in the session.
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
        Diagonal, // A segment per corner, its generatrices across the corner's diagonal, its profile the arc from the corner up to the centre; the segments meet on the lines from the sides' middles to the centre.
        Cloister // A segment per side, its generatrices along the side, its profile the arc from the side up to the centre; the segments meet on the lines from the corners to the centre.
    };

    const size_t sides; // The regular polygon's sides.
    const double edge; // mm, the polygon's side.
    const double radius; // mm, every cylinder; the arch over a side is a half circle when it is half the side (groin), the crown is above the centre (diagonal, cloister).
    const double rise; // radians, the groin vault's generatrices rising toward the centre, negative sloping; 0 for the other kinds.
    const Kind kind; // How the cylinders sit on the polygon.
    const size_t lamellas; // Lamellas across every segment, more where its generatrices change the plane they end on.
    const double thickness; // mm, every lamella, under the cylinder.
    const std::vector<Placement> placements; // Every copy of the vault; one at the origin at scale 1 when none is given.

    /// The vault as the session named name.
    CylinderVault(
        size_t sides = 4,
        double edge = 12000.0,
        double radius = 6000.0,
        double rise = 0.0,
        Kind kind = Kind::Groin,
        size_t lamellas = 12,
        double thickness = 120.0,
        const std::string& name = "cylinder_vault",
        const std::vector<Placement>& placements = {}
    );

    /// The placements of Engel's field (p. 241): a vault on every bay of a columns by rows grid of bays, except the middle 2 by 2, over which one vault stands at twice the scale; columns and rows even.
    static std::vector<Placement> field(size_t columns, size_t rows, double bay);

    /// Not copied, as a session is not.
    CylinderVault(const CylinderVault&) = delete;

    /// Not assigned, as it is not copied.
    CylinderVault& operator=(const CylinderVault&) = delete;

    /// The largest relative length error of any lamella's development, the proof every lamella is cut flat.
    double distortion() const {
        return _distortion;
    }

    /// The largest twist of any lamella's generatrices out of the tangent planes, 0 on a cylinder.
    double twist() const {
        return _twist;
    }

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
    };

    double _distortion = 0.0;
    double _twist = 0.0;

    /// The polygon's corner i on the ground, counter-clockwise from the x axis, for the placement.
    Point corner(const Placement& placement, size_t i) const;

    /// The vertical plane through two points on the ground, its normal pointing toward a third.
    static Plane wall(const Point& from, const Point& to, const Point& inside);

    /// Every segment of the kind over the polygon.
    std::vector<Segment> compute_segments() const;

    /// The point of the segment's circle at the angle.
    static Point on_circle(const Segment& segment, double radius, double angle);

    /// Where the generatrix at the angle enters and leaves the footprint: the distances along the axis from the circle, and the planes.
    std::array<std::pair<double, size_t>, 2> clip(const Segment& segment, double angle) const;

    /// The angles the segment is cut at: the lamellas spread evenly, and every angle where a generatrix changes the plane it starts or
    /// ends on, so no lamella spans two planes.
    std::vector<double> compute_divisions(const Segment& segment) const;

    /// The footprints, in the group plan.
    void add_plan(const std::vector<Segment>& segments);

    /// Every lamella, lamella_<segment>_<k>, in the group lamellas; its development and its place on the development returned for the flat pattern.
    std::vector<std::pair<Development, Point>> add_lamellas(const std::vector<Segment>& segments);

    /// The flat cutting pattern beside the model, a row per segment, in the group flat.
    void add_flat(const std::vector<std::pair<Development, Point>>& developments, double beside);
};

inline CylinderVault::CylinderVault(size_t sides, double edge, double radius, double rise, Kind kind, size_t lamellas, double thickness, const std::string& name, const std::vector<Placement>& placements)
    : WoodSession(name),
      sides(std::max<size_t>(sides, 3)),
      edge(edge),
      radius(radius),
      rise(rise),
      kind(kind),
      lamellas(std::max<size_t>(lamellas, 2)),
      thickness(thickness),
      placements(placements.empty() ? std::vector<Placement>{{Point(0.0, 0.0, 0.0), 1.0}} : placements) {

    if (edge <= 0.0 || radius <= thickness || thickness <= 0.0)
        throw std::invalid_argument("A cylinder vault needs a side, a radius larger than its thickness and a thickness");

    if (kind != Kind::Groin && std::abs(rise) > 1e-12)
        throw std::invalid_argument("Only the groin vault's generatrices rise or slope toward the centre");

    // segments: every cylinder with its circle, its generatrices and its footprint
    const std::vector<Segment> segments = compute_segments();

    // plan: the footprints on the ground
    add_plan(segments);

    // lamellas: every segment cut along its generatrices, each strip a curved plate between its two end planes
    const std::vector<std::pair<Development, Point>> developments = add_lamellas(segments);

    // flat: the cutting pattern beside the model, every lamella unfolded
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

inline Plane CylinderVault::wall(const Point& from, const Point& to, const Point& inside) {

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
        const double circumradius = 0.5 * edge / std::sin(Tolerance::PI / static_cast<double>(sides));
        const double apothem = 0.5 * edge / std::tan(Tolerance::PI / static_cast<double>(sides));
        const Point from = corner(placement, i);
        const Point to = corner(placement, i + 1);
        const Point middle = Point::mid_point(from, to);
        const Vector along = (to - from).normalized();
        const Vector outward = (middle - centre).normalized();
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
            segment.corners = {from, to, centre};
        } else if (kind == Kind::Cloister) {
            // the arc from the side up to the crown over the centre, the generatrices along the side
            if (radius < apothem - 1e-6)
                throw std::invalid_argument("A cloister vault's radius is at least the polygon's apothem");

            const double crown = radius - std::sqrt(std::max(0.0, radius * radius - apothem * apothem));
            segment.axis = along;
            segment.centre = centre + Vector(0.0, 0.0, crown - radius);
            segment.x = outward;
            segment.y = Vector(0.0, 0.0, 1.0);
            segment.start = 0.0;
            segment.end = std::atan2(apothem, radius - crown);
            segment.corners = {from, to, centre};
        } else {
            // the arc from the corner up to the crown over the centre, the generatrices across the corner's diagonal, over the kite between the two neighbouring sides' middles
            if (radius < circumradius - 1e-6)
                throw std::invalid_argument("A diagonal vault's radius is at least the polygon's circumradius");

            const Point before = Point::mid_point(corner(placement, i + sides - 1), from);
            const Vector radial = (from - centre).normalized();
            const double crown = radius - std::sqrt(std::max(0.0, radius * radius - circumradius * circumradius));
            segment.axis = Vector(0.0, 0.0, 1.0).cross(radial);
            segment.centre = centre + Vector(0.0, 0.0, crown - radius);
            segment.x = radial;
            segment.y = Vector(0.0, 0.0, 1.0);
            segment.start = 0.0;
            segment.end = std::atan2(circumradius, radius - crown);
            segment.corners = {before, from, middle, centre};
        }

        segment.radius = radius;

        // the vertical planes around the footprint, normals pointing in
        for (size_t c = 0; c < segment.corners.size(); c++)
            segment.footprint.push_back(wall(segment.corners[c], segment.corners[(c + 1) % segment.corners.size()], Point::centroid(segment.corners)));

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

inline std::vector<double> CylinderVault::compute_divisions(const Segment& segment) const {

    // the even divisions, then every angle where the entry or exit plane changes, found between fine samples by bisection
    std::vector<double> divisions;

    for (size_t k = 0; k <= lamellas; k++)
        divisions.push_back(segment.start + (segment.end - segment.start) * static_cast<double>(k) / static_cast<double>(lamellas));

    const size_t fine = 1024;
    const auto planes = [&](double angle) {
        const std::array<std::pair<double, size_t>, 2> ends = clip(segment, angle);
        return std::make_pair(ends[0].second, ends[1].second);
    };

    for (size_t k = 0; k < fine; k++) {
        double low = segment.start + (segment.end - segment.start) * static_cast<double>(k) / static_cast<double>(fine);
        double high = segment.start + (segment.end - segment.start) * static_cast<double>(k + 1) / static_cast<double>(fine);

        if (planes(low) == planes(high))
            continue;

        const std::pair<size_t, size_t> at_low = planes(low);

        for (int step = 0; step < 50; step++) {
            const double middle = 0.5 * (low + high);
            (planes(middle) == at_low ? low : high) = middle;
        }

        divisions.push_back(0.5 * (low + high));
    }

    std::sort(divisions.begin(), divisions.end());
    std::vector<double> distinct;

    for (const double division : divisions)
        if (distinct.empty() || division - distinct.back() > 1e-6)
            distinct.push_back(division);

    return distinct;
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

inline std::vector<std::pair<Development, Point>> CylinderVault::add_lamellas(const std::vector<Segment>& segments) {

    const std::shared_ptr<TreeNode> group = add_group("lamellas");
    std::vector<std::pair<Development, Point>> developments;

    for (size_t i = 0; i < segments.size(); i++) {
        const Segment& segment = segments[i];
        const std::vector<double> divisions = compute_divisions(segment);
        const auto length = [&](double angle) {
            const std::array<std::pair<double, size_t>, 2> ends = clip(segment, angle);
            return ends[1].first - ends[0].first;
        };

        for (size_t k = 0; k + 1 < divisions.size(); k++) {
            // the lamella's angles, each end drawn in to where its generatrix is at least the thickness long, so its cap is a face
            double from = divisions[k];
            double to = divisions[k + 1];

            if (length(from) < thickness && length(to) < thickness)
                continue;

            for (double* end : {&from, &to})
                if (length(*end) < thickness) {
                    double short_angle = *end;
                    double long_angle = end == &from ? to : from;

                    for (int step = 0; step < 60; step++) {
                        const double middle = 0.5 * (short_angle + long_angle);
                        (length(middle) < thickness ? short_angle : long_angle) = middle;
                    }

                    *end = long_angle;
                }

            if (to - from < 1e-6)
                continue;

            // the planes its generatrices run between, and the exact arcs of the outer and the inner cylinder projected along the generatrices onto them
            const std::array<std::pair<double, size_t>, 2> ends = clip(segment, 0.5 * (from + to));
            const Xform to_start = Xform::project_to_plane_by_axis(segment.footprint[ends[0].second], segment.axis);
            const Xform to_end = Xform::project_to_plane_by_axis(segment.footprint[ends[1].second], segment.axis);
            const NurbsCurve outer = Primitives::arc(on_circle(segment, segment.radius, from), on_circle(segment, segment.radius, 0.5 * (from + to)), on_circle(segment, segment.radius, to));
            const NurbsCurve inner = Primitives::arc(on_circle(segment, segment.radius - thickness, from), on_circle(segment, segment.radius - thickness, 0.5 * (from + to)), on_circle(segment, segment.radius - thickness, to));
            const NurbsCurve outer_a = outer.transformed(to_start);
            const NurbsCurve outer_b = outer.transformed(to_end);
            add(std::make_shared<PlateCurved>(outer_a, outer_b, inner.transformed(to_start), inner.transformed(to_end), fmt::format("lamella_{}_{}", i, k)), group);

            // its development, the proof it is cut flat, and its place on the segment's development: the arc length from the first generatrix, the distance along the generatrix from the circle
            developments.emplace_back(Development(outer_a, outer_b, 32), Point(segment.radius * (from - segment.start), clip(segment, from)[0].first, static_cast<double>(i)));
            _distortion = std::max(_distortion, developments.back().first.distortion);
            _twist = std::max(_twist, developments.back().first.twist);
        }
    }

    return developments;
}

inline void CylinderVault::add_flat(const std::vector<std::pair<Development, Point>>& developments, double beside) {

    const std::shared_ptr<TreeNode> flat = add_group("flat");

    // the pattern starts beside the model's largest x; a row per segment, each as long along y as the longest generatrix
    double largest = -std::numeric_limits<double>::infinity();
    double lowest = std::numeric_limits<double>::infinity();
    double highest = -std::numeric_limits<double>::infinity();

    for (const std::shared_ptr<Element>& element : *objects.elements)
        if (const std::shared_ptr<PlateCurved> lamella = std::dynamic_pointer_cast<PlateCurved>(element))
            for (const NurbsCurve* curve : {&lamella->outer_a, &lamella->outer_b})
                for (size_t k = 0; k <= 16; k++)
                    largest = std::max(largest, curve->point_at(curve->domain_start() + (curve->domain_end() - curve->domain_start()) * static_cast<double>(k) / 16.0)[0]);

    for (const std::pair<Development, Point>& development : developments)
        for (size_t k = 0; k < development.first.outline.point_count(); k++) {
            lowest = std::min(lowest, development.second[1] + development.first.outline.get_point(k)[1]);
            highest = std::max(highest, development.second[1] + development.first.outline.get_point(k)[1]);
        }

    const double pitch = highest - lowest + beside;
    size_t index = 0;

    for (const std::pair<Development, Point>& development : developments) {
        Polyline outline = development.first.outline.transformed(Xform::translation(largest + beside + development.second[0], development.second[2] * pitch + development.second[1] - lowest, 0.0));
        outline.name = fmt::format("flat_{}", index++);
        add_polyline(outline, flat);
    }
}
