#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor::geometry {

const double TOLERANCE = 1e-9; // parallel planes and lines below it have no intersection
const double EXTENSION = 1000.0; // how far parabola ends are pushed out before the panel planes trim them

// ═══════════════════════════════════════════════════════════════════════════
// Planes
// ═══════════════════════════════════════════════════════════════════════════

Plane rotate(const Plane& plane, double radians, const Vector& axis, const Point& point) {
    return plane.transformed(Xform::rotation_around_line(Line::from_points(point, point + axis), radians));
}

Plane level(double z) {
    return Plane::xy_plane() + Vector(0.0, 0.0, z);
}

Plane edge_plane(const Line& edge, const Vector& normal_z) {
    return Plane::from_point_normal(edge.center(), edge.to_direction().cross(normal_z));
}

// ═══════════════════════════════════════════════════════════════════════════
// Intersections
// ═══════════════════════════════════════════════════════════════════════════

std::optional<Line> plane_plane(const Plane& plane0, const Plane& plane1) {

    Line line;

    if (!Intersection::plane_plane(plane0, plane1, line))
        return std::nullopt;

    return line.to_vector().dot(plane0.z_axis().cross(plane1.z_axis())) < 0.0 ? Line::from_points(line.end(), line.start()) : line;
}

std::optional<Point> line_plane(const Line& line, const Plane& plane) {

    Point point;

    if (std::abs(plane.z_axis().dot(line.to_direction())) <= TOLERANCE || !Intersection::line_plane(line, plane, point, false))
        return std::nullopt;

    return point;
}

std::optional<Point> plane_plane_plane(const Plane& plane0, const Plane& plane1, const Plane& plane2) {

    Point point;

    if (!Intersection::plane_plane_plane(plane0, plane1, plane2, point))
        return std::nullopt;

    return point;
}

// ═══════════════════════════════════════════════════════════════════════════
// Polylines
// ═══════════════════════════════════════════════════════════════════════════

Line edge(const std::vector<Point>& polygon, size_t i) {
    return Line::from_points(polygon[i], polygon[(i + 1) % polygon.size()]);
}

Polyline trim(const Polyline& polyline, const Plane& plane0, const Plane& plane1) {

    std::vector<Point> pts = polyline.get_points();
    const size_t n = pts.size();
    const Point middle = polyline.center();

    pts[0] = pts[0] + (pts[0] - pts[1]).normalized() * EXTENSION;
    pts[n - 1] = pts[n - 1] + (pts[n - 1] - pts[n - 2]).normalized() * EXTENSION;

    // each cut keeps the side of the original's middle, not the extended one's, which a short member puts past the second plane
    return Polyline(pts).cut_by_plane(plane0, signed_distance(middle, plane0) >= 0.0).cut_by_plane(plane1, signed_distance(middle, plane1) >= 0.0);
}

/// The points pushed out at both ends by EXTENSION.
static std::vector<Point> extended(const Polyline& polyline) {

    std::vector<Point> pts = polyline.get_points();
    const size_t n = pts.size();
    pts[0] = pts[0] + (pts[0] - pts[1]).normalized() * EXTENSION;
    pts[n - 1] = pts[n - 1] + (pts[n - 1] - pts[n - 2]).normalized() * EXTENSION;

    return pts;
}

/// The first segment of the points the plane crosses.
static size_t crossed(const std::vector<Point>& pts, const Plane& plane) {

    for (size_t i = 0; i + 1 < pts.size(); i++)
        if ((signed_distance(pts[i], plane) >= 0.0) != (signed_distance(pts[i + 1], plane) >= 0.0))
            return i;

    throw std::runtime_error("trim_alike: a plane misses the extended polyline");
}

std::vector<Polyline> trim_alike(const std::vector<Polyline>& polylines, const Plane& plane0, const Plane& plane1) {

    const std::vector<Point> first = extended(polylines[0]);
    const size_t a = crossed(first, plane0);
    const size_t b = crossed(first, plane1);
    const std::array<size_t, 2> ends = {std::min(a, b), std::max(a, b)};
    const std::array<const Plane*, 2> planes = a <= b ? std::array<const Plane*, 2>{&plane0, &plane1} : std::array<const Plane*, 2>{&plane1, &plane0};
    std::vector<Polyline> trimmed;

    for (const Polyline& polyline : polylines) {

        if (polyline.point_count() != polylines[0].point_count())
            throw std::invalid_argument("trim_alike: polylines of different vertex counts");

        const std::vector<Point> pts = extended(polyline);
        std::vector<Point> kept = {line_plane(Line::from_points(pts[ends[0]], pts[ends[0] + 1]), *planes[0]).value()};
        kept.insert(kept.end(), pts.begin() + ends[0] + 1, pts.begin() + ends[1] + 1);
        kept.push_back(line_plane(Line::from_points(pts[ends[1]], pts[ends[1] + 1]), *planes[1]).value());
        trimmed.push_back(Polyline(kept));
    }

    return trimmed;
}

Polyline offset_polyline(const Polyline& polyline, double distance) {

    const std::vector<Point> pts = polyline.get_points();
    const size_t n = pts.size();
    const Vector z(0.0, 0.0, 1.0);

    std::vector<Plane> planes = {Plane::from_point_normal(pts[0], pts[1] - pts[0])};

    for (size_t i = 0; i + 1 < n; i++) {
        const Line line = Line::from_points(pts[i], pts[i + 1]);
        const Vector x = line.to_direction();
        const Vector y = z.cross(x);
        planes.push_back(Plane::from_point_normal(line.center(), x.cross(y)).translate_by_normal(distance));
    }

    planes.push_back(Plane::from_point_normal(pts[n - 1], pts[n - 2] - pts[n - 1]));

    const Plane base = Plane::from_point_normal(pts[0], z.cross(pts[n - 1] - pts[0]));

    std::vector<Point> result;

    for (size_t i = 0; i + 1 < planes.size(); i++)
        result.push_back(plane_plane_plane(planes[i], planes[i + 1], base).value());

    return Polyline(result);
}

std::vector<Point> open_points(const Polyline& polyline) {

    std::vector<Point> points = polyline.get_points();

    if (polyline.is_closed())
        points.pop_back();

    return points;
}

double signed_distance(const Point& point, const Plane& plane) {
    return (point - plane.origin()).dot(plane.z_axis());
}

double end_level(const Outline& outline, const Plane& end) {

    double level = 0.0;

    for (const Polyline& loop : {outline.top, outline.bottom})
        for (const Point& point : loop.get_points())
            if (std::abs(signed_distance(point, end)) <= 1e-6)
                level = std::min(level, point[2]);

    return level;
}

Point area_centroid(const Polyline& polyline) {

    const std::vector<Point> points = open_points(polyline);
    const Vector normal = wood_session::compute_newell(points).normalized();
    const Point origin = points[0];
    Vector sum(0.0, 0.0, 0.0);
    double area = 0.0;

    for (size_t i = 1; i + 1 < points.size(); i++) {
        const double weight = (points[i] - origin).cross(points[i + 1] - origin).dot(normal);
        sum += ((points[i] - origin) + (points[i + 1] - origin)) * (weight / 3.0);
        area += weight;
    }

    return origin + sum / area;
}

double polygon_area(const Polyline& polyline) {

    const std::vector<Point> points = open_points(polyline);
    Vector twice(0.0, 0.0, 0.0);

    for (size_t i = 1; i + 1 < points.size(); i++)
        twice += (points[i] - points[0]).cross(points[i + 1] - points[0]);

    return 0.5 * twice.magnitude();
}

Polyline lifted(const std::vector<Point>& points, double lift) {
    return Polyline(points).translated(Vector(0.0, 0.0, lift)).closed();
}

Plane lifted(const Plane& plane, double lift) {
    return plane + Vector(0.0, 0.0, lift);
}

Line lifted(const Line& line, double lift) {
    return line.transformed(Xform::translation(0.0, 0.0, lift));
}

std::vector<Point> overlap(const Polyline& a, const Polyline& b, const Plane& plane) {

    const std::vector<Point> loop = open_points(a);
    const std::vector<Polyline> shared = Polyline::boolean_op(a, b, plane, 0);

    if (shared.empty() || std::abs(polygon_area(shared.front()) - polygon_area(a)) <= 1e-6 * polygon_area(a))
        return loop;

    // the overlap in a's winding, from the corner nearest a's first, so a contact's top edge and normal read as a's do
    std::vector<Point> points = open_points(shared.front());

    if (wood_session::compute_newell(points).dot(wood_session::compute_newell(loop)) < 0.0)
        std::reverse(points.begin(), points.end());

    const auto nearest = std::min_element(points.begin(), points.end(), [&loop](const Point& p, const Point& q) { return p.distance(loop[0]) < q.distance(loop[0]); });
    std::rotate(points.begin(), nearest, points.end());

    return points;
}

std::vector<Point> above(const std::vector<Point>& points, double z) {

    std::vector<Point> result;
    const size_t n = points.size();

    for (size_t i = 0; i < n; i++) {
        const Point& a = points[i];
        const Point& b = points[(i + 1) % n];
        const bool a_in = a[2] >= z;
        const bool b_in = b[2] >= z;

        if (a_in)
            result.push_back(a);

        if (a_in != b_in)
            result.push_back(a + (b - a) * ((z - a[2]) / (b[2] - a[2])));
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════

MemberRef quarter_member(size_t quarter, Family family, size_t index) {
    return MemberRef{static_cast<int>(quarter), family, index, -1};
}

// ═══════════════════════════════════════════════════════════════════════════
// Outlines
// ═══════════════════════════════════════════════════════════════════════════

Outline loft_planes(const std::vector<Plane>& planes, const Plane& bottom, const Plane& top, bool flip) {

    const size_t n = planes.size();
    std::vector<Point> pts_bottom;
    std::vector<Point> pts_top;

    for (size_t i = 0; i < n; i++) {
        const std::optional<Point> rb = plane_plane_plane(planes[i], planes[(i + 1) % n], bottom);
        const std::optional<Point> rt = plane_plane_plane(planes[i], planes[(i + 1) % n], top);

        if (rb)
            pts_bottom.push_back(*rb);

        if (rt)
            pts_top.push_back(*rt);
    }

    Outline outline{Polyline(pts_top).closed(), Polyline(pts_bottom).closed()};

    if (flip)
        std::swap(outline.top, outline.bottom);

    return outline;
}

}
