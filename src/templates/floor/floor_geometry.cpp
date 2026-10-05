#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor::geometry {

const double TOLERANCE = 1e-9; // compas TOL.absolute: parallel planes and lines below it have no intersection
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

Polyline cut(const Polyline& polyline, const Plane& plane0, const Plane& plane1) {
    return polyline.cut_by_plane(plane0).cut_by_plane(plane1);
}

Polyline trim(const Polyline& polyline, const Plane& plane0, const Plane& plane1) {

    std::vector<Point> pts = polyline.get_points();
    const size_t n = pts.size();

    pts[0] = pts[0] + (pts[0] - pts[1]).normalized() * EXTENSION;
    pts[n - 1] = pts[n - 1] + (pts[n - 1] - pts[n - 2]).normalized() * EXTENSION;

    return cut(Polyline(pts), plane0, plane1);
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

    return points.size() < 3 ? 0.0 : 0.5 * wood_session::compute_newell(points).magnitude();
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
