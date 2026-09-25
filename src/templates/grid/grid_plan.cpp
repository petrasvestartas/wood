#include "pch.h"
#include "src/templates/grid/grid_plan.h"
#include "wood_element_geometry.h"
#include "../src/clipper2/clipper.h"

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

/// True when every vertex of a mesh face ring lies at height z within tolerance.
bool is_level(const Mesh& mesh, const std::vector<size_t>& ring, double z, double tolerance) {

    for (const size_t key : ring)
        if (std::abs((*mesh.vertex_point(key))[2] - z) > tolerance)
            return false;

    return true;
}

bool is_convex(const std::vector<Point>& points) {

    const size_t count = points.size();
    for (size_t i = 0; i < count; i++)
        if (compute_direction(points[i], points[(i + 1) % count]).cross(compute_direction(points[(i + 1) % count], points[(i + 2) % count]))[2] < -1e-9)
            return false;

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Sections
// ═══════════════════════════════════════════════════════════════════════════

std::vector<Polyline> compute_section(const Mesh& solid, double z, double tolerance) {

    const Mesh below = solid.cut_by_plane(Plane::from_point_normal(Point(0.0, 0.0, z), Vector(0.0, 0.0, -1.0)));

    std::vector<Polyline> rings;
    for (const size_t face : below.faces()) {
        const std::vector<size_t> ring = *below.face_vertices(face);
        if (!is_level(below, ring, z, tolerance * 1e-3))
            continue;

        std::vector<Point> outer = compute_flat(below, ring);
        if (std::abs(compute_area(outer)) < tolerance * tolerance)
            continue;
        if (compute_area(outer) < 0.0)
            std::reverse(outer.begin(), outer.end());
        rings.push_back(to_polyline(outer));

        if (!below.get_face_holes().count(face))
            continue;

        for (const std::vector<size_t>& hole : below.get_face_holes().at(face)) {
            std::vector<Point> inner = compute_flat(below, hole);
            if (compute_area(inner) > 0.0)
                std::reverse(inner.begin(), inner.end());
            rings.push_back(to_polyline(inner));
        }
    }

    return rings;
}

/// Rings as Clipper paths on a 0.001 grid.
Clipper2Lib::Paths64 to_paths(const std::vector<Polyline>& rings) {

    Clipper2Lib::Paths64 paths;
    for (const Polyline& ring : rings) {
        paths.emplace_back();
        for (const Point& point : to_loop(ring))
            paths.back().emplace_back(std::llround(point[0] * 1000.0), std::llround(point[1] * 1000.0));
    }

    return paths;
}

/// Clipper paths as rings at z 0, outer rings counter-clockwise, holes clockwise.
std::vector<Polyline> from_paths(const Clipper2Lib::Paths64& paths) {

    std::vector<Polyline> rings;
    for (const Clipper2Lib::Path64& path : paths) {
        if (path.size() < 3)
            continue;

        std::vector<Point> points;
        for (const Clipper2Lib::Point64& point : path)
            points.emplace_back(point.x / 1000.0, point.y / 1000.0, 0.0);
        rings.push_back(to_polyline(points));
    }

    return rings;
}

std::vector<Polyline> compute_regions(const std::vector<Polyline>& a, const std::vector<Polyline>& b, int clip) {

    const Clipper2Lib::ClipType type = clip == 0 ? Clipper2Lib::ClipType::Intersection : clip == 1 ? Clipper2Lib::ClipType::Union : Clipper2Lib::ClipType::Difference;

    return from_paths(Clipper2Lib::BooleanOp(type, Clipper2Lib::FillRule::NonZero, to_paths(a), to_paths(b)));
}

bool is_inside(const std::vector<Polyline>& rings, const Point& point) {

    int count = 0;
    for (const Polyline& ring : rings)
        count += ring.point_in_polygon_2d(point) ? 1 : 0;

    return count % 2 == 1;
}

Point compute_interior(const std::vector<Point>& points) {

    const size_t count = points.size();
    size_t left = 0;
    for (size_t i = 1; i < count; i++)
        if (points[i][0] < points[left][0] || (points[i][0] == points[left][0] && points[i][1] < points[left][1]))
            left = i;

    const Point& a = points[(left + count - 1) % count];
    const Point& b = points[left];
    const Point& c = points[(left + 1) % count];
    const Polyline ear = to_polyline({a, b, c});

    std::optional<size_t> deepest;
    double depth = 0.0;
    for (size_t i = 0; i < count; i++) {
        if (i == left || i == (left + count - 1) % count || i == (left + 1) % count || !ear.point_in_polygon_2d(points[i]))
            continue;

        const double away = std::abs((points[i] - a).cross(c - a)[2]);
        if (!deepest || away > depth) {
            deepest = i;
            depth = away;
        }
    }

    if (!deepest)
        return Point((a[0] + b[0] + c[0]) / 3.0, (a[1] + b[1] + c[1]) / 3.0, 0.0);

    return Point((b[0] + points[*deepest][0]) / 2.0, (b[1] + points[*deepest][1]) / 2.0, 0.0);
}

// ═══════════════════════════════════════════════════════════════════════════
// Corners
// ═══════════════════════════════════════════════════════════════════════════

Point compute_corner(const Point& corner, const Vector& before, double a, const Vector& after, double b) {

    const double cosine = before.dot(after);
    if (1.0 - cosine * cosine < 1e-9)
        return corner + before * std::max(a, b);

    const double alpha = (a - cosine * b) / (1.0 - cosine * cosine);
    const double beta = (b - cosine * a) / (1.0 - cosine * cosine);

    return corner + before * alpha + after * beta;
}

std::vector<Vector> compute_normals(const std::vector<Point>& points) {

    std::vector<Vector> normals;
    for (size_t i = 0; i < points.size(); i++)
        normals.push_back(compute_direction(points[i], points[(i + 1) % points.size()]).cross(Vector(0.0, 0.0, 1.0)));

    return normals;
}

std::vector<Point> compute_offset(const std::vector<Point>& points, const std::vector<double>& distances) {

    const size_t count = points.size();
    const std::vector<Vector> outward = compute_normals(points);

    std::vector<Point> result;
    for (size_t i = 0; i < count; i++)
        result.push_back(compute_corner(points[i], outward[(i + count - 1) % count], distances[(i + count - 1) % count], outward[i], distances[i]));

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Arrangement
// ═══════════════════════════════════════════════════════════════════════════

double compute_ring_id(size_t ring, size_t edge, bool core) {
    return (core ? 2000000.0 : 1000000.0) + 1000.0 * ring + edge;
}

bool is_ring(double id) {
    return id >= 1000000.0;
}

bool is_core_id(double id) {
    return id >= 2000000.0;
}

/// A piece of an input line while the crossings are computed.
struct Segment {
    Point a; // Start at z 0.
    Point b; // End at z 0.
    double id = 0.0; // Id of the source line.
    size_t rank = 0; // Position of the source line in the input, the earlier the stronger.
    bool alive = true; // False once a collinear stronger segment took it.
};

/// A point every piece ends on: a segment end or a crossing.
struct Stop {
    Point point; // At z 0.
    int order = 0; // Weld priority: 0 a ring corner, 1 a ring crossing, 2 the rest, 3 on a core ring, which welds within tolerance alone.
};

/// The stops of a set of segments and, per segment, its (distance, stop) pairs.
struct Stops {
    std::vector<Stop> stops; // Every segment end, then every crossing.
    std::vector<std::vector<std::pair<double, size_t>>> params; // Per segment, the distance along it of each of its stops.
};

/// The welded stops: the points kept, their orders, and the kept index of every stop.
struct Welds {
    std::vector<Point> points; // One per kept stop.
    std::vector<int> orders; // The order of each kept stop.
    std::vector<size_t> canonical; // Kept index per input stop.
};

/// Pieces of the segments between consecutive stops as welded vertex pairs, with the id of their segment.
struct Pieces {
    std::vector<std::pair<size_t, size_t>> pairs; // Each once, lower index first.
    std::vector<double> ids; // Source line id per piece.
};

/// True when segment a takes a collinear overlap from b: rings first, then the earlier line.
bool is_stronger(const Segment& a, const Segment& b) {

    if (is_ring(a.id) != is_ring(b.id))
        return is_ring(a.id);

    return a.rank < b.rank;
}

/// Distance along a segment from its start to the foot of a point.
double compute_parameter(const Segment& segment, const Point& point) {
    return (point - segment.a).dot(compute_direction(segment.a, segment.b));
}

/// Collinear overlaps resolved: the weaker segment loses the overlapped stretch and keeps the rest as new pieces.
void compute_overlaps(std::vector<Segment>& segments, double tolerance) {

    for (size_t i = 0; i < segments.size(); i++)
        for (size_t j = 0; j < segments.size(); j++) {
            if (i == j || !segments[i].alive || !segments[j].alive || is_stronger(segments[j], segments[i]))
                continue;

            const Vector di = compute_direction(segments[i].a, segments[i].b);
            const Vector dj = compute_direction(segments[j].a, segments[j].b);
            if (std::abs(di.cross(dj)[2]) > 1e-6 || std::abs((segments[j].a - segments[i].a).cross(di)[2]) > tolerance)
                continue;

            const double length = compute_distance(segments[j].a, segments[j].b);
            const double low = std::max(0.0, std::min(compute_parameter(segments[j], segments[i].a), compute_parameter(segments[j], segments[i].b)));
            const double high = std::min(length, std::max(compute_parameter(segments[j], segments[i].a), compute_parameter(segments[j], segments[i].b)));
            if (high - low <= tolerance)
                continue;

            const Segment loser = segments[j];
            segments[j].alive = false;
            if (low > tolerance)
                segments.push_back({loser.a, loser.a + dj * low, loser.id, loser.rank, true});
            if (length - high > tolerance)
                segments.push_back({loser.a + dj * high, loser.b, loser.id, loser.rank, true});
        }
}

/// True when a point lies on a live core ring segment within tolerance.
bool is_on_core(const Point& point, const std::vector<Segment>& segments, double tolerance) {

    for (const Segment& segment : segments) {
        if (!segment.alive || !is_core_id(segment.id))
            continue;

        const double at = compute_parameter(segment, point);
        if (at >= -tolerance && at <= compute_distance(segment.a, segment.b) + tolerance && std::abs((point - segment.a).cross(compute_direction(segment.a, segment.b))[2]) <= tolerance)
            return true;
    }

    return false;
}

/// The weld order of a segment end: on a core ring 3 (its own ends, and the end of a line a core took a stretch from), another ring 0, a pattern line 2.
int compute_order(const Segment& segment, const Point& end, const std::vector<Segment>& segments, double tolerance) {

    if (is_core_id(segment.id) || is_on_core(end, segments, tolerance))
        return 3;

    return is_ring(segment.id) ? 0 : 2;
}

/// The stops of every live segment: its ends, then every crossing with a later one.
Stops compute_stops(const std::vector<Segment>& segments, double tolerance) {

    Stops result;
    result.params.resize(segments.size());
    for (size_t i = 0; i < segments.size(); i++) {
        if (!segments[i].alive)
            continue;

        result.params[i].emplace_back(0.0, result.stops.size());
        result.stops.push_back({segments[i].a, compute_order(segments[i], segments[i].a, segments, tolerance)});
        result.params[i].emplace_back(compute_distance(segments[i].a, segments[i].b), result.stops.size());
        result.stops.push_back({segments[i].b, compute_order(segments[i], segments[i].b, segments, tolerance)});
    }

    for (size_t i = 0; i < segments.size(); i++)
        for (size_t j = i + 1; j < segments.size(); j++) {
            if (!segments[i].alive || !segments[j].alive)
                continue;

            const Vector u = segments[i].b - segments[i].a;
            const Vector v = segments[j].b - segments[j].a;
            const double denominator = u.cross(v)[2];
            if (std::abs(denominator) < 1e-9 * u.magnitude() * v.magnitude())
                continue;

            const Vector w = segments[j].a - segments[i].a;
            const double t = w.cross(v)[2] / denominator;
            const double s = w.cross(u)[2] / denominator;
            if (t < -tolerance / u.magnitude() || t > 1.0 + tolerance / u.magnitude() || s < -tolerance / v.magnitude() || s > 1.0 + tolerance / v.magnitude())
                continue;

            const int order = is_core_id(segments[i].id) || is_core_id(segments[j].id) ? 3 : is_ring(segments[i].id) || is_ring(segments[j].id) ? 1 : 2;
            result.params[i].emplace_back(std::clamp(t, 0.0, 1.0) * u.magnitude(), result.stops.size());
            result.params[j].emplace_back(std::clamp(s, 0.0, 1.0) * v.magnitude(), result.stops.size());
            result.stops.push_back({segments[i].a + u * std::clamp(t, 0.0, 1.0), order});
        }

    return result;
}

/// Stops within merge of an earlier stop in priority order welded onto it; a core stop welds within tolerance alone and never takes another stop, so a core ring never pulls a column point onto itself.
Welds compute_welds(const std::vector<Stop>& stops, double tolerance, double merge) {

    Welds welds;
    welds.canonical.resize(stops.size());
    for (int order = 0; order < 4; order++)
        for (size_t index = 0; index < stops.size(); index++) {
            if (stops[index].order != order)
                continue;

            size_t found = welds.points.size();
            for (size_t k = 0; k < welds.points.size() && found == welds.points.size(); k++)
                if (compute_distance(welds.points[k], stops[index].point) <= (order == 3 || welds.orders[k] == 3 ? tolerance : merge))
                    found = k;
            if (found == welds.points.size()) {
                welds.points.push_back(stops[index].point);
                welds.orders.push_back(order);
            }
            welds.canonical[index] = found;
        }

    return welds;
}

/// Pieces of every live segment between consecutive stops as welded vertex pairs, each once, with the id of its segment.
Pieces compute_pieces(const std::vector<Segment>& segments, Stops& stops, const std::vector<size_t>& canonical) {

    std::set<std::pair<size_t, size_t>> seen;
    Pieces pieces;
    for (size_t i = 0; i < segments.size(); i++) {
        if (!segments[i].alive)
            continue;

        std::sort(stops.params[i].begin(), stops.params[i].end());
        for (size_t k = 0; k + 1 < stops.params[i].size(); k++) {
            const std::pair<size_t, size_t> piece = std::minmax(canonical[stops.params[i][k].second], canonical[stops.params[i][k + 1].second]);
            if (piece.first == piece.second || seen.count(piece))
                continue;

            seen.insert(piece);
            pieces.pairs.push_back(piece);
            pieces.ids.push_back(segments[i].id);
        }
    }

    return pieces;
}

/// Pieces with a dangling end removed until every end is shared, so every piece left bounds a face.
void compute_pruned(Pieces& pieces, size_t vertices) {

    const size_t rounds = pieces.pairs.size();
    for (size_t round = 0; round <= rounds; round++) {
        std::vector<int> degree(vertices, 0);
        for (const std::pair<size_t, size_t>& piece : pieces.pairs) {
            degree[piece.first]++;
            degree[piece.second]++;
        }

        Pieces kept;
        for (size_t k = 0; k < pieces.pairs.size(); k++)
            if (degree[pieces.pairs[k].first] >= 2 && degree[pieces.pairs[k].second] >= 2) {
                kept.pairs.push_back(pieces.pairs[k]);
                kept.ids.push_back(pieces.ids[k]);
            }

        const bool pruned = kept.pairs.size() != pieces.pairs.size();
        pieces = kept;
        if (!pruned)
            return;
    }
}

std::pair<std::vector<Line>, std::vector<double>> compute_crossings(const std::vector<Line>& lines, const std::vector<double>& ids, double tolerance, double merge) {

    std::vector<Segment> segments;
    for (size_t i = 0; i < lines.size(); i++)
        segments.push_back({compute_lift(lines[i].start(), 0.0), compute_lift(lines[i].end(), 0.0), ids[i], i, true});
    compute_overlaps(segments, tolerance);

    Stops stops = compute_stops(segments, tolerance);
    const Welds welds = compute_welds(stops.stops, tolerance, merge);
    Pieces pieces = compute_pieces(segments, stops, welds.canonical);
    compute_pruned(pieces, welds.points.size());

    std::pair<std::vector<Line>, std::vector<double>> result;
    for (size_t k = 0; k < pieces.pairs.size(); k++) {
        result.first.push_back(Line::from_points(welds.points[pieces.pairs[k].first], welds.points[pieces.pairs[k].second]));
        result.second.push_back(pieces.ids[k]);
    }

    return result;
}

/// Integer plan key of a point on the tolerance grid.
std::pair<long long, long long> compute_key(const Point& point, double tolerance) {
    return {std::llround(point[0] / tolerance), std::llround(point[1] / tolerance)};
}

Mesh compute_arrangement(const std::vector<Line>& lines, const std::vector<double>& ids, double tolerance) {

    Mesh plan = Mesh::from_lines(lines, true, tolerance * 0.1);
    for (const size_t face : plan.faces())
        if (std::abs(compute_area(to_loop(*plan.face_polygon(face)))) < tolerance * tolerance)
            plan.remove_face(face);

    std::map<std::pair<std::pair<long long, long long>, std::pair<long long, long long>>, double> lookup;
    for (size_t i = 0; i < lines.size(); i++)
        lookup[std::minmax(compute_key(lines[i].start(), tolerance), compute_key(lines[i].end(), tolerance))] = ids[i];

    std::map<size_t, std::set<double>> meeting;
    for (const std::pair<size_t, size_t>& edge : plan.edges()) {
        const std::pair<std::pair<long long, long long>, std::pair<long long, long long>> key = std::minmax(compute_key(*plan.vertex_point(edge.first), tolerance), compute_key(*plan.vertex_point(edge.second), tolerance));
        const double id = lookup.count(key) ? lookup.at(key) : -1.0;
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

std::vector<Plane> compute_planes(const Mesh& solid) {

    const Point centre = solid.centroid();
    std::vector<Plane> planes;
    for (const size_t face : solid.faces()) {
        const std::vector<Point> points = to_loop(*solid.face_polygon(face));
        Vector normal = wood_session::compute_newell(points);
        const Point origin = Point::centroid(points);
        if ((origin - centre).dot(normal) < 0.0)
            normal = -normal;
        planes.push_back(Plane::from_point_normal(origin, normal));
    }

    return planes;
}

std::optional<Plane> compute_exit(const std::vector<Point>& polygon, const Point& origin, const Vector& direction) {

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

Plane compute_bound(const std::vector<Point>& polygon, const Point& origin, const Vector& direction) {
    return Plane::from_point_normal(origin + direction * compute_reach(polygon, origin, direction), direction);
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
