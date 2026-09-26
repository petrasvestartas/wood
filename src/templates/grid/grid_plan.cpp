#include "pch.h"
#include "src/templates/grid/grid_plan.h"

using namespace session_cpp;

namespace wood_grid::plan {

// ═══════════════════════════════════════════════════════════════════════════
// Loops
// ═══════════════════════════════════════════════════════════════════════════

std::vector<Point> to_loop(const Polyline& polyline) {

    std::vector<Point> points = polyline.get_points();
    if (points.size() > 1 && points.front().distance(points.back()) < Tolerance::APPROXIMATION)
        points.pop_back();

    return points;
}

Polyline to_polyline(std::vector<Point> points) {

    points.push_back(points.front());

    return Polyline(points);
}

double compute_area(const std::vector<Point>& points) {

    double area = 0.0;
    for (size_t i = 0; i < points.size(); i++)
        area += points[i][0] * points[(i + 1) % points.size()][1] - points[(i + 1) % points.size()][0] * points[i][1];

    return area / 2.0;
}

Vector compute_direction(const Point& a, const Point& b) {
    return Vector(b[0] - a[0], b[1] - a[1], 0.0).normalized();
}

double compute_distance(const Point& a, const Point& b) {
    return std::hypot(b[0] - a[0], b[1] - a[1]);
}

Point compute_lift(const Point& point, double z) {
    return Point(point[0], point[1], z);
}

Polyline compute_lifted(const Polyline& polyline, double z) {

    std::vector<Point> points;
    for (const Point& point : polyline.get_points())
        points.push_back(compute_lift(point, z));

    return Polyline(points);
}

std::vector<Point> compute_flat(const Mesh& mesh, const std::vector<size_t>& ring) {

    std::vector<Point> points;
    for (const size_t key : ring)
        points.push_back(compute_lift(*mesh.vertex_point(key), 0.0));

    return points;
}

Point compute_interior(const std::vector<Point>& points) {

    const Vector inward = Vector(0.0, 0.0, 1.0).cross(compute_direction(points[0], points[1])) * (compute_area(points) < 0.0 ? -0.1 : 0.1);

    return compute_lift(points[0] + (points[1] - points[0]) * 0.5 + inward, 0.0);
}

bool is_convex(const std::vector<Point>& points) {

    const size_t count = points.size();
    for (size_t i = 0; i < count; i++)
        if (compute_direction(points[i], points[(i + 1) % count]).cross(compute_direction(points[(i + 1) % count], points[(i + 2) % count]))[2] < -1e-9)
            return false;

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Regions
// ═══════════════════════════════════════════════════════════════════════════

bool is_inside(const std::vector<Polyline>& rings, const Point& point) {

    int count = 0;
    for (const Polyline& ring : rings)
        count += ring.point_in_polygon_2d(point) ? 1 : 0;

    return count % 2 == 1;
}

Polyline compute_wall_ring(const Polyline& core, double wall) {

    Polyline ring = core;
    if (!Intersection::offset_in_3d(ring, Plane::xy_plane(), wall / 2.0))
        return core;

    return ring;
}

Point compute_corner(const Point& corner, const Vector& before, double a, const Vector& after, double b) {

    const double cosine = before.dot(after);
    if (1.0 - cosine * cosine < 1e-9)
        return corner + before * std::max(a, b);

    return corner + before * ((a - cosine * b) / (1.0 - cosine * cosine)) + after * ((b - cosine * a) / (1.0 - cosine * cosine));
}

/// A crossing of a line with a ring side: the parameter along the line, the ring and the side.
struct Crossing {
    double t = 0.0; // Parameter along the line.
    int ring = -1; // Ring index.
    int side = -1; // Side index.
};

/// True when a comes before b along the line.
bool is_before(const Crossing& a, const Crossing& b) {
    return a.t < b.t;
}

std::vector<Piece> compute_pieces(const Line& line, const std::vector<Polyline>& rings, bool inside, double tolerance) {

    if (rings.empty())
        return {Piece{line}};

    std::vector<Crossing> crossings = {{0.0, -1, -1}, {1.0, -1, -1}};
    for (size_t r = 0; r < rings.size(); r++) {
        const std::vector<Point> corners = to_loop(rings[r]);
        for (size_t i = 0; i < corners.size(); i++) {
            double t = 0.0;
            double s = 0.0;
            const Line side = Line::from_points(corners[i], corners[(i + 1) % corners.size()]);
            if (Intersection::line_line_parameters(line, side, t, s, tolerance, true, false) && line.point_at(t).distance(side.point_at(s)) <= tolerance)
                crossings.push_back({t, static_cast<int>(r), static_cast<int>(i)});
        }
    }
    std::sort(crossings.begin(), crossings.end(), is_before);

    std::vector<Piece> pieces;
    for (size_t k = 0; k + 1 < crossings.size(); k++)
        if ((crossings[k + 1].t - crossings[k].t) * line.length() > tolerance && is_inside(rings, line.point_at((crossings[k].t + crossings[k + 1].t) / 2.0)) == inside)
            pieces.push_back({Line::from_points(line.point_at(crossings[k].t), line.point_at(crossings[k + 1].t)), {crossings[k].ring, crossings[k + 1].ring}, {crossings[k].side, crossings[k + 1].side}});

    return pieces;
}

// ═══════════════════════════════════════════════════════════════════════════
// Arrangement
// ═══════════════════════════════════════════════════════════════════════════

double compute_ring_id(size_t ring, size_t edge) {
    return 1000000.0 + 1000.0 * ring + edge;
}

bool is_ring(double id) {
    return id >= 1000000.0;
}

Mesh compute_arrangement(const std::vector<Line>& lines, const std::vector<Line>& rings, const std::vector<double>& ids, double tolerance, double merge) {

    Mesh plan = Mesh::from_arrangement(lines, rings, tolerance, merge);
    std::map<size_t, std::set<double>> meeting;
    for (const std::pair<size_t, size_t>& edge : plan.edges()) {
        const double source = *plan.edge_attribute(edge, "line");
        const double id = source < 0.0 ? -1.0 : ids[static_cast<size_t>(source)];
        plan.set_edge_attribute(edge, "line", id);
        meeting[edge.first].insert(id);
        meeting[edge.second].insert(id);
    }

    for (const std::pair<const size_t, std::set<double>>& entry : meeting) {
        plan.set_vertex_attribute(entry.first, "line_a", *entry.second.begin());
        plan.set_vertex_attribute(entry.first, "line_b", entry.second.size() > 1 ? *std::next(entry.second.begin()) : *entry.second.begin());
    }

    return plan;
}

// ═══════════════════════════════════════════════════════════════════════════
// Planes
// ═══════════════════════════════════════════════════════════════════════════

std::optional<Plane> compute_exit(const std::vector<Point>& polygon, const Point& origin, const Vector& direction) {

    if (!is_convex(polygon))
        return Plane::from_point_normal(origin + direction * compute_reach(polygon, origin, direction), direction);

    std::optional<Plane> exit;
    double best = std::numeric_limits<double>::max();
    for (size_t i = 0; i < polygon.size(); i++) {
        const Vector normal = compute_direction(polygon[i], polygon[(i + 1) % polygon.size()]).cross(Vector(0.0, 0.0, 1.0));
        const double speed = direction.dot(normal);
        if (speed <= 1e-9)
            continue;

        const double t = (compute_lift(polygon[i], origin[2]) - origin).dot(normal) / speed;
        if (t < best) {
            best = t;
            exit = Plane::from_point_normal(origin + direction * t, normal);
        }
    }

    return exit;
}

double compute_reach(const std::vector<Point>& polygon, const Point& point, const Vector& direction) {

    double reach = 0.0;
    for (const Point& corner : polygon)
        reach = std::max(reach, (compute_lift(corner, 0.0) - compute_lift(point, 0.0)).dot(direction));

    return reach;
}

std::vector<Point> compute_strip(const Point& origin, const Vector& direction, double half) {

    const Vector side = direction.cross(Vector(0.0, 0.0, 1.0));
    const Vector along = direction * 1e7;
    const Point centre = compute_lift(origin, 0.0);

    return {centre - along + side * half, centre + along + side * half, centre + along - side * half, centre - along - side * half};
}

Plane compute_bisector(const Point& origin, const Vector& a, const Vector& b) {

    const Vector normal = a - b;

    return Plane::from_point_normal(origin, normal.magnitude() < 1e-6 ? a : normal.normalized());
}

Polyline compute_polygon(const std::vector<Vector>& directions, const Point& centre, const std::vector<double>& distances) {

    std::vector<Point> points;
    for (size_t j = 0; j < directions.size(); j++)
        points.push_back(compute_corner(centre, directions[j], distances[j], directions[(j + 1) % directions.size()], distances[(j + 1) % directions.size()]));

    return to_polyline(points);
}

std::vector<Vector> compute_directions(const Mesh& plan, size_t vertex) {

    const Point origin = *plan.vertex_point(vertex);
    std::vector<double> angles;
    for (const size_t other : plan.vertex_neighbors(vertex).value_or(std::vector<size_t>())) {
        const Point point = *plan.vertex_point(other);
        angles.push_back(std::atan2(point[1] - origin[1], point[0] - origin[0]));
        angles.push_back(std::atan2(origin[1] - point[1], origin[0] - point[0]));
    }
    if (angles.empty())
        angles = {-Tolerance::HALF_PI, 0.0, Tolerance::HALF_PI, Tolerance::PI};
    std::sort(angles.begin(), angles.end());

    std::vector<double> merged;
    for (const double angle : angles)
        if (merged.empty() || angle - merged.back() > Tolerance::TO_RADIANS)
            merged.push_back(angle);
    if (merged.size() > 1 && merged.front() + Tolerance::TWO_PI - merged.back() <= Tolerance::TO_RADIANS)
        merged.pop_back();
    if (merged.size() == 2) {
        merged.push_back(merged[0] + Tolerance::HALF_PI);
        merged.push_back(merged[0] - Tolerance::HALF_PI);
        std::sort(merged.begin(), merged.end());
    }

    std::vector<Vector> directions;
    for (const double angle : merged)
        directions.emplace_back(std::cos(angle), std::sin(angle), 0.0);

    return directions;
}

} // namespace wood_grid::plan
