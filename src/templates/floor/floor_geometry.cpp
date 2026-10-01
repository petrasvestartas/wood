#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor::geometry {

const double TOLERANCE = 1e-9; // compas TOL.absolute: parallel planes and lines below it have no intersection

// ═══════════════════════════════════════════════════════════════════════════
// Planes
// ═══════════════════════════════════════════════════════════════════════════

Plane offset(const Plane& plane, double distance) {
    return Plane::from_point_normal(plane.origin() + plane.z_axis() * distance, plane.z_axis());
}

Plane rotate(const Plane& plane, double radians, const Vector& axis, const Point& point) {

    const Xform rotation = Xform::rotation_around_line(Line::from_points(point, point + axis), radians);

    return Plane::from_point_normal(plane.origin().transformed(rotation), plane.z_axis().transformed(rotation));
}

Plane level(double z) {
    return Plane::from_point_normal(Point(0.0, 0.0, z), Vector(0.0, 0.0, 1.0));
}

Plane edge_plane(const Line& edge, const Vector& normal_z) {
    return Plane::from_point_normal(edge.center(), direction(edge).cross(normal_z));
}

// ═══════════════════════════════════════════════════════════════════════════
// Intersections
// ═══════════════════════════════════════════════════════════════════════════

std::optional<Line> plane_plane(const Plane& plane0, const Plane& plane1) {

    const Vector& n0 = plane0.z_axis();
    const Vector& n1 = plane1.z_axis();

    if (std::abs(n0.dot(n1) - 1.0) <= TOLERANCE)
        return std::nullopt;

    const Vector d = n0.cross(n1);
    const Point o = plane0.origin();
    const std::optional<Point> x1 = line_plane(Line::from_points(o, o + d.cross(n0)), plane1);

    if (!x1)
        return std::nullopt;

    return Line::from_points(*x1, *x1 + d);
}

std::optional<Point> line_plane(const Line& line, const Plane& plane) {

    const Point a = line.start();
    const Vector ab = line.end() - a;
    const double cosa = plane.z_axis().dot(ab);

    if (std::abs(cosa) <= TOLERANCE)
        return std::nullopt;

    const double ratio = -plane.z_axis().dot(a - plane.origin()) / cosa;

    return a + ab * ratio;
}

std::optional<Point> plane_plane_plane(const Plane& plane0, const Plane& plane1, const Plane& plane2) {

    const std::optional<Line> line = plane_plane(plane0, plane1);

    if (!line)
        return std::nullopt;

    return line_plane(*line, plane2);
}

// ═══════════════════════════════════════════════════════════════════════════
// Polylines
// ═══════════════════════════════════════════════════════════════════════════

Line edge(const std::vector<Point>& polygon, size_t i) {
    return Line::from_points(polygon[i], polygon[(i + 1) % polygon.size()]);
}

Vector direction(const Line& line) {
    return line.to_vector().normalized();
}

Polyline closed(const std::vector<Point>& points) {

    std::vector<Point> loop = points;
    loop.push_back(points.front());

    return Polyline(loop);
}

Polyline extend_ends(const Polyline& polyline, double amount) {

    std::vector<Point> pts = polyline.get_points();
    const size_t n = pts.size();

    pts[0] = pts[0] + (pts[0] - pts[1]).normalized() * amount;
    pts[n - 1] = pts[n - 1] + (pts[n - 1] - pts[n - 2]).normalized() * amount;

    return Polyline(pts);
}

Polyline cut(const Polyline& polyline, const Plane& plane0, const Plane& plane1) {
    return polyline.cut_by_plane(plane0).cut_by_plane(plane1);
}

Polyline offset_polyline(const Polyline& polyline, double distance) {

    const std::vector<Point> pts = polyline.get_points();
    const size_t n = pts.size();
    const Vector z(0.0, 0.0, 1.0);

    std::vector<Plane> planes = {Plane::from_point_normal(pts[0], pts[1] - pts[0])};

    for (size_t i = 0; i + 1 < n; i++) {
        const Line line = Line::from_points(pts[i], pts[i + 1]);
        const Vector x = direction(line);
        const Vector y = z.cross(x);
        planes.push_back(offset(Plane::from_point_normal(line.center(), x.cross(y)), distance));
    }

    planes.push_back(Plane::from_point_normal(pts[n - 1], pts[n - 2] - pts[n - 1]));

    const Plane base = Plane::from_point_normal(pts[0], z.cross(pts[n - 1] - pts[0]));

    std::vector<Point> result;

    for (size_t i = 0; i + 1 < planes.size(); i++)
        result.push_back(plane_plane_plane(planes[i], planes[i + 1], base).value());

    return Polyline(result);
}

Polyline quadratic_points(const Point& p0, const Point& p1, const Point& p2, int divisions) {

    std::vector<Point> pts;

    for (int k = 0; k < divisions; k++) {
        const double t = static_cast<double>(k) / (divisions - 1);
        const double a = (1.0 - t) * (1.0 - t);
        const double b = 2.0 * (1.0 - t) * t;
        const double c = t * t;
        pts.push_back(Point(a * p0[0] + b * p1[0] + c * p2[0], a * p0[1] + b * p1[1] + c * p2[1], a * p0[2] + b * p1[2] + c * p2[2]));
    }

    return Polyline(pts);
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

    Outline outline{closed(pts_top), closed(pts_bottom)};

    if (flip)
        std::swap(outline.top, outline.bottom);

    return outline;
}

}
