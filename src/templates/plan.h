#pragma once
#include <numeric>
#include "wood_session.h"
#include "../src/clipper2/clipper.h"

namespace wood_grid {

// ═══════════════════════════════════════════════════════════════════════════
// Loops
// ═══════════════════════════════════════════════════════════════════════════

/// The corners of a closed polyline, the closing point dropped.
inline std::vector<session_cpp::Point> to_loop(const session_cpp::Polyline& polyline) {

    std::vector<session_cpp::Point> points = polyline.get_points();
    if (points.size() > 1 && points.front().distance(points.back()) < session_cpp::Tolerance::APPROXIMATION)
        points.pop_back();

    return points;
}

/// A closed polyline through corners.
inline session_cpp::Polyline to_polyline(std::vector<session_cpp::Point> points) {
    points.push_back(points.front());
    return session_cpp::Polyline(points);
}

/// Signed plan area of a loop, positive counter-clockwise seen from above.
inline double compute_area(const std::vector<session_cpp::Point>& points) {

    double area = 0.0;
    for (size_t i = 0; i < points.size(); i++)
        area += points[i][0] * points[(i + 1) % points.size()][1] - points[(i + 1) % points.size()][0] * points[i][1];

    return area / 2.0;
}

/// Unit plan direction from a to b.
inline session_cpp::Vector compute_direction(const session_cpp::Point& a, const session_cpp::Point& b) {
    return session_cpp::Vector(b[0] - a[0], b[1] - a[1], 0.0).normalized();
}

/// Plan distance between two points.
inline double compute_distance(const session_cpp::Point& a, const session_cpp::Point& b) {
    return std::hypot(b[0] - a[0], b[1] - a[1]);
}

/// The point at height z.
inline session_cpp::Point compute_lift(const session_cpp::Point& point, double z) {
    return session_cpp::Point(point[0], point[1], z);
}

/// Every corner at height z.
inline session_cpp::Polyline compute_lift(const session_cpp::Polyline& polyline, double z) {

    std::vector<session_cpp::Point> points;
    for (const session_cpp::Point& point : polyline.get_points())
        points.push_back(compute_lift(point, z));

    return session_cpp::Polyline(points);
}

// ═══════════════════════════════════════════════════════════════════════════
// Sections
// ═══════════════════════════════════════════════════════════════════════════

/// Rings of the cap faces of solid cut at the horizontal plane through z: outer rings counter-clockwise seen from above, face_holes as clockwise holes, all dropped to z 0; empty above the solid.
inline std::vector<session_cpp::Polyline> compute_section(const session_cpp::Mesh& solid, double z, double tolerance) {

    const session_cpp::Mesh below = solid.cut_by_plane(session_cpp::Plane::from_point_normal(session_cpp::Point(0.0, 0.0, z), session_cpp::Vector(0.0, 0.0, -1.0)));
    const auto flat = [&](const std::vector<size_t>& ring) {
        std::vector<session_cpp::Point> points;
        for (const size_t key : ring)
            points.push_back(compute_lift(*below.vertex_point(key), 0.0));
        return points;
    };

    std::vector<session_cpp::Polyline> rings;
    for (const size_t face : below.faces()) {
        const std::vector<size_t> ring = *below.face_vertices(face);
        if (std::any_of(ring.begin(), ring.end(), [&](size_t key) { return std::abs((*below.vertex_point(key))[2] - z) > tolerance; }))
            continue;

        std::vector<session_cpp::Point> outer = flat(ring);
        if (compute_area(outer) < 0.0)
            std::reverse(outer.begin(), outer.end());
        rings.push_back(to_polyline(outer));

        if (!below.get_face_holes().count(face))
            continue;

        for (const std::vector<size_t>& hole : below.get_face_holes().at(face)) {
            std::vector<session_cpp::Point> inner = flat(hole);
            if (compute_area(inner) > 0.0)
                std::reverse(inner.begin(), inner.end());
            rings.push_back(to_polyline(inner));
        }
    }

    return rings;
}

/// Rings as Clipper paths on a 0.001 grid.
inline Clipper2Lib::Paths64 to_paths(const std::vector<session_cpp::Polyline>& rings) {

    Clipper2Lib::Paths64 paths;
    for (const session_cpp::Polyline& ring : rings) {
        paths.emplace_back();
        for (const session_cpp::Point& point : to_loop(ring))
            paths.back().emplace_back(std::llround(point[0] * 1000.0), std::llround(point[1] * 1000.0));
    }

    return paths;
}

/// Clipper paths as rings at z 0, outer rings counter-clockwise, holes clockwise.
inline std::vector<session_cpp::Polyline> from_paths(const Clipper2Lib::Paths64& paths) {

    std::vector<session_cpp::Polyline> rings;
    for (const Clipper2Lib::Path64& path : paths) {
        if (path.size() < 3)
            continue;

        std::vector<session_cpp::Point> points;
        for (const Clipper2Lib::Point64& point : path)
            points.emplace_back(point.x / 1000.0, point.y / 1000.0, 0.0);
        rings.push_back(to_polyline(points));
    }

    return rings;
}

/// Intersection (0), union (1) or difference (2) of two ring lists with holes kept, output rings oriented as compute_section; Clipper2.
inline std::vector<session_cpp::Polyline> compute_regions(const std::vector<session_cpp::Polyline>& a, const std::vector<session_cpp::Polyline>& b, int clip) {

    const Clipper2Lib::ClipType type = clip == 0 ? Clipper2Lib::ClipType::Intersection : clip == 1 ? Clipper2Lib::ClipType::Union : Clipper2Lib::ClipType::Difference;

    return from_paths(Clipper2Lib::BooleanOp(type, Clipper2Lib::FillRule::NonZero, to_paths(a), to_paths(b)));
}

/// Rings inflated by distance with mitre joins, holes kept; Clipper2 InflatePaths.
inline std::vector<session_cpp::Polyline> compute_offset(const std::vector<session_cpp::Polyline>& rings, double distance) {
    return from_paths(Clipper2Lib::InflatePaths(to_paths(rings), distance * 1000.0, Clipper2Lib::JoinType::Miter, Clipper2Lib::EndType::Polygon, 100.0));
}

/// Even-odd inside test over all rings, so a ring inside a hole is an island.
inline bool compute_inside(const std::vector<session_cpp::Polyline>& rings, const session_cpp::Point& point) {

    int count = 0;
    for (const session_cpp::Polyline& ring : rings)
        count += ring.point_in_polygon_2d(point) ? 1 : 0;

    return count % 2 == 1;
}

/// Loop with side i moved out by distances[i], the loop counter-clockwise seen from above; the mitre of two sides at every corner.
inline std::vector<session_cpp::Point> compute_offset(const std::vector<session_cpp::Point>& points, const std::vector<double>& distances) {

    const size_t count = points.size();
    std::vector<session_cpp::Vector> outward;
    for (size_t i = 0; i < count; i++)
        outward.push_back(compute_direction(points[i], points[(i + 1) % count]).cross(session_cpp::Vector(0.0, 0.0, 1.0)));

    std::vector<session_cpp::Point> result;
    for (size_t i = 0; i < count; i++) {
        const size_t before = (i + count - 1) % count;
        const double cosine = outward[before].dot(outward[i]);
        if (1.0 - cosine * cosine < 1e-9) {
            result.push_back(points[i] + outward[i] * std::max(distances[before], distances[i]));
            continue;
        }

        const double alpha = (distances[before] - cosine * distances[i]) / (1.0 - cosine * cosine);
        const double beta = (distances[i] - cosine * distances[before]) / (1.0 - cosine * cosine);
        result.push_back(points[i] + outward[before] * alpha + outward[i] * beta);
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Arrangement
// ═══════════════════════════════════════════════════════════════════════════

/// A line id of a ring edge: 1000000 + 1000 * ring + edge.
inline double compute_ring_id(size_t ring, size_t edge) {
    return 1000000.0 + 1000.0 * ring + edge;
}

/// True for the id of a ring edge.
inline bool is_ring(double id) {
    return id >= 1000000.0;
}

/// Every line split at every crossing and T-junction with another, collinear overlaps taken by the ring or the earlier line first, then split points within merge welded onto the earlier one in priority (ring corners, ring crossings, the rest) and dangling pieces dropped; ids follow their source line.
inline std::pair<std::vector<session_cpp::Line>, std::vector<double>> compute_crossings(const std::vector<session_cpp::Line>& lines, const std::vector<double>& ids, double tolerance, double merge) {

    struct Segment { session_cpp::Point a; session_cpp::Point b; double id; size_t rank; bool alive; };
    std::vector<Segment> segments;
    for (size_t i = 0; i < lines.size(); i++)
        segments.push_back({compute_lift(lines[i].start(), 0.0), compute_lift(lines[i].end(), 0.0), ids[i], i, true});

    const auto wins = [&](const Segment& a, const Segment& b) { return is_ring(a.id) != is_ring(b.id) ? is_ring(a.id) : a.rank < b.rank; };
    const auto parameter = [](const Segment& s, const session_cpp::Point& p) { return (p - s.a).dot(compute_direction(s.a, s.b)); };

    for (size_t i = 0; i < segments.size(); i++)
        for (size_t j = 0; j < segments.size(); j++) {
            if (i == j || !segments[i].alive || !segments[j].alive || wins(segments[j], segments[i]))
                continue;

            const session_cpp::Vector di = compute_direction(segments[i].a, segments[i].b);
            const session_cpp::Vector dj = compute_direction(segments[j].a, segments[j].b);
            if (std::abs(di.cross(dj)[2]) > 1e-6 || std::abs((segments[j].a - segments[i].a).cross(di)[2]) > tolerance)
                continue;

            const double length = compute_distance(segments[j].a, segments[j].b);
            const double low = std::max(0.0, std::min(parameter(segments[j], segments[i].a), parameter(segments[j], segments[i].b)));
            const double high = std::min(length, std::max(parameter(segments[j], segments[i].a), parameter(segments[j], segments[i].b)));
            if (high - low <= tolerance)
                continue;

            const Segment loser = segments[j];
            segments[j].alive = false;
            if (low > tolerance)
                segments.push_back({loser.a, loser.a + dj * low, loser.id, loser.rank, true});
            if (length - high > tolerance)
                segments.push_back({loser.a + dj * high, loser.b, loser.id, loser.rank, true});
        }

    struct Stop { session_cpp::Point point; int order; };
    std::vector<Stop> stops;
    std::vector<std::vector<std::pair<double, size_t>>> params(segments.size());
    for (size_t i = 0; i < segments.size(); i++) {
        if (!segments[i].alive)
            continue;

        const int order = is_ring(segments[i].id) ? 0 : 2;
        params[i].emplace_back(0.0, stops.size());
        stops.push_back({segments[i].a, order});
        params[i].emplace_back(compute_distance(segments[i].a, segments[i].b), stops.size());
        stops.push_back({segments[i].b, order});
    }

    for (size_t i = 0; i < segments.size(); i++)
        for (size_t j = i + 1; j < segments.size(); j++) {
            if (!segments[i].alive || !segments[j].alive)
                continue;

            const session_cpp::Vector u = segments[i].b - segments[i].a;
            const session_cpp::Vector v = segments[j].b - segments[j].a;
            const double denominator = u.cross(v)[2];
            if (std::abs(denominator) < 1e-9 * u.magnitude() * v.magnitude())
                continue;

            const session_cpp::Vector w = segments[j].a - segments[i].a;
            const double t = w.cross(v)[2] / denominator;
            const double s = w.cross(u)[2] / denominator;
            if (t < -tolerance / u.magnitude() || t > 1.0 + tolerance / u.magnitude() || s < -tolerance / v.magnitude() || s > 1.0 + tolerance / v.magnitude())
                continue;

            const session_cpp::Point point = segments[i].a + u * std::clamp(t, 0.0, 1.0);
            params[i].emplace_back(std::clamp(t, 0.0, 1.0) * u.magnitude(), stops.size());
            params[j].emplace_back(std::clamp(s, 0.0, 1.0) * v.magnitude(), stops.size());
            stops.push_back({point, is_ring(segments[i].id) || is_ring(segments[j].id) ? 1 : 2});
        }

    std::vector<size_t> order(stops.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return stops[a].order < stops[b].order; });

    std::vector<session_cpp::Point> welded;
    std::vector<size_t> canonical(stops.size());
    for (const size_t index : order) {
        size_t found = welded.size();
        for (size_t k = 0; k < welded.size() && found == welded.size(); k++)
            if (compute_distance(welded[k], stops[index].point) <= merge)
                found = k;
        if (found == welded.size())
            welded.push_back(stops[index].point);
        canonical[index] = found;
    }

    std::set<std::pair<size_t, size_t>> seen;
    std::vector<std::pair<size_t, size_t>> pieces;
    std::vector<double> piece_ids;
    for (size_t i = 0; i < segments.size(); i++) {
        if (!segments[i].alive)
            continue;

        std::sort(params[i].begin(), params[i].end());
        for (size_t k = 0; k + 1 < params[i].size(); k++) {
            const std::pair<size_t, size_t> piece = std::minmax(canonical[params[i][k].second], canonical[params[i][k + 1].second]);
            if (piece.first == piece.second || seen.count(piece))
                continue;

            seen.insert(piece);
            pieces.push_back(piece);
            piece_ids.push_back(segments[i].id);
        }
    }

    for (bool pruned = true; pruned;) {
        std::vector<int> degree(welded.size(), 0);
        for (const std::pair<size_t, size_t>& piece : pieces) {
            degree[piece.first]++;
            degree[piece.second]++;
        }

        pruned = false;
        for (size_t k = 0; k < pieces.size(); k++)
            if (degree[pieces[k].first] < 2 || degree[pieces[k].second] < 2) {
                pieces.erase(pieces.begin() + k);
                piece_ids.erase(piece_ids.begin() + k);
                pruned = true;
                break;
            }
    }

    std::pair<std::vector<session_cpp::Line>, std::vector<double>> result;
    for (size_t k = 0; k < pieces.size(); k++) {
        result.first.push_back(session_cpp::Line::from_points(welded[pieces[k].first], welded[pieces[k].second]));
        result.second.push_back(piece_ids[k]);
    }

    return result;
}

/// Integer plan key of a point on the tolerance grid.
inline std::pair<long long, long long> compute_key(const session_cpp::Point& point, double tolerance) {
    return {std::llround(point[0] / tolerance), std::llround(point[1] / tolerance)};
}

/// Mesh::from_lines of split lines with the outer face deleted, edge attribute line from the split's ids and vertex attributes line_a, line_b from the two lowest ids meeting there; slivers below tolerance squared dropped.
inline session_cpp::Mesh compute_arrangement(const std::vector<session_cpp::Line>& lines, const std::vector<double>& ids, double tolerance) {

    session_cpp::Mesh plan = session_cpp::Mesh::from_lines(lines, true, tolerance * 0.1);
    for (const size_t face : plan.faces())
        if (std::abs(compute_area(to_loop(*plan.face_polygon(face)))) < tolerance * tolerance)
            plan.remove_face(face);

    std::map<std::pair<std::pair<long long, long long>, std::pair<long long, long long>>, double> lookup;
    for (size_t i = 0; i < lines.size(); i++)
        lookup[std::minmax(compute_key(lines[i].start(), tolerance), compute_key(lines[i].end(), tolerance))] = ids[i];

    std::map<size_t, std::set<double>> meeting;
    for (const std::pair<size_t, size_t>& edge : plan.edges()) {
        const auto found = lookup.find(std::minmax(compute_key(*plan.vertex_point(edge.first), tolerance), compute_key(*plan.vertex_point(edge.second), tolerance)));
        const double id = found == lookup.end() ? -1.0 : found->second;
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

/// One plane per face of a closed mesh, normal out; exact for the swept and lofted solids the builders make.
inline std::vector<session_cpp::Plane> compute_planes(const session_cpp::Mesh& solid) {

    const session_cpp::Point centre = solid.centroid();
    std::vector<session_cpp::Plane> planes;
    for (const size_t face : solid.faces()) {
        const std::vector<session_cpp::Point> points = to_loop(*solid.face_polygon(face));
        session_cpp::Vector normal = session_cpp::Vector::average_normal(points).normalized();
        const session_cpp::Point origin = session_cpp::Point::centroid(points);
        if ((origin - centre).dot(normal) < 0.0)
            normal = -normal;
        planes.push_back(session_cpp::Plane::from_point_normal(origin, normal));
    }

    return planes;
}

/// Vertical plane along the side of a convex plan polygon the ray from origin along direction leaves through, normal out of the polygon; none when the ray never leaves.
inline std::optional<session_cpp::Plane> compute_exit(const std::vector<session_cpp::Point>& polygon, const session_cpp::Point& origin, const session_cpp::Vector& direction) {

    std::optional<session_cpp::Plane> exit;
    double best = std::numeric_limits<double>::max();
    for (size_t i = 0; i < polygon.size(); i++) {
        const session_cpp::Vector normal = compute_direction(polygon[i], polygon[(i + 1) % polygon.size()]).cross(session_cpp::Vector(0.0, 0.0, 1.0));
        const double speed = direction.dot(normal);
        if (speed <= 1e-9)
            continue;

        const double t = (compute_lift(polygon[i], origin[2]) - origin).dot(normal) / speed;
        if (t < best) {
            best = t;
            exit = session_cpp::Plane::from_point_normal(origin + direction * t, normal);
        }
    }

    return exit;
}

/// Long plan rectangle of half-width about the line through origin along direction: the footprint of a member that runs through.
inline std::vector<session_cpp::Point> compute_strip(const session_cpp::Point& origin, const session_cpp::Vector& direction, double half) {

    const session_cpp::Vector side = direction.cross(session_cpp::Vector(0.0, 0.0, 1.0));
    const session_cpp::Vector along = direction * 1e7;
    const session_cpp::Point centre = compute_lift(origin, 0.0);

    return {centre - along + side * half, centre + along + side * half, centre + along - side * half, centre - along - side * half};
}

/// Plane through origin bisecting unit plan directions a and b, normal towards a; perpendicular to a when b is opposite.
inline session_cpp::Plane compute_bisector(const session_cpp::Point& origin, const session_cpp::Vector& a, const session_cpp::Vector& b) {

    const session_cpp::Vector normal = a - b;

    return session_cpp::Plane::from_point_normal(origin, normal.magnitude() < 1e-6 ? a : normal.normalized());
}

/// Closed polygon about centre whose side j is perpendicular to directions[j] at distances[j]; the direction polygon of a node.
inline session_cpp::Polyline compute_polygon(const std::vector<session_cpp::Vector>& directions, const session_cpp::Point& centre, const std::vector<double>& distances) {

    std::vector<session_cpp::Point> points;
    for (size_t j = 0; j < directions.size(); j++) {
        const session_cpp::Vector& a = directions[j];
        const session_cpp::Vector& b = directions[(j + 1) % directions.size()];
        const double cosine = a.dot(b);
        const double alpha = (distances[j] - cosine * distances[(j + 1) % directions.size()]) / (1.0 - cosine * cosine);
        const double beta = (distances[(j + 1) % directions.size()] - cosine * distances[j]) / (1.0 - cosine * cosine);
        points.push_back(centre + a * alpha + b * beta);
    }

    return to_polyline(points);
}

/// Unit plan directions of the edges at a plan vertex and their opposites, counter-clockwise, closer than 1 degree merged; one edge adds its perpendicular, none gives x and y.
inline std::vector<session_cpp::Vector> compute_directions(const session_cpp::Mesh& plan, size_t vertex) {

    const session_cpp::Point origin = *plan.vertex_point(vertex);
    std::vector<double> angles;
    for (const size_t other : plan.vertex_neighbors(vertex).value_or(std::vector<size_t>())) {
        const session_cpp::Point point = *plan.vertex_point(other);
        angles.push_back(std::atan2(point[1] - origin[1], point[0] - origin[0]));
        angles.push_back(std::atan2(origin[1] - point[1], origin[0] - point[0]));
    }

    if (angles.empty())
        angles = {-session_cpp::Tolerance::HALF_PI, 0.0, session_cpp::Tolerance::HALF_PI, session_cpp::Tolerance::PI};
    std::sort(angles.begin(), angles.end());

    std::vector<double> merged;
    for (const double angle : angles)
        if (merged.empty() || angle - merged.back() > session_cpp::Tolerance::TO_RADIANS)
            merged.push_back(angle);
    if (merged.size() > 1 && merged.front() + session_cpp::Tolerance::TWO_PI - merged.back() <= session_cpp::Tolerance::TO_RADIANS)
        merged.pop_back();

    if (merged.size() == 2) {
        merged.push_back(merged[0] + session_cpp::Tolerance::HALF_PI);
        merged.push_back(merged[0] - session_cpp::Tolerance::HALF_PI);
        std::sort(merged.begin(), merged.end());
    }

    std::vector<session_cpp::Vector> directions;
    for (const double angle : merged)
        directions.emplace_back(std::cos(angle), std::sin(angle), 0.0);

    return directions;
}

} // namespace wood_grid
