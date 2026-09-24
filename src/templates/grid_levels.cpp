#include "pch.h"
#include "src/templates/grid.h"
#include "src/templates/grid_plan.h"

using namespace session_cpp;

namespace wood_grid::levels {

using namespace wood_grid::plan;

// ═══════════════════════════════════════════════════════════════════════════
// Patterns
// ═══════════════════════════════════════════════════════════════════════════

/// Running sums from 0 over steps.
std::vector<double> compute_sums(const std::vector<double>& steps) {

    std::vector<double> sums = {0.0};
    for (const double step : steps)
        sums.push_back(sums.back() + step);

    return sums;
}

/// The plan point of grid coordinates (u, v) with the v axis leaning lean radians towards x.
Point compute_skewed(double u, double v, double lean) {
    return Point(u + v * std::sin(lean), v * std::cos(lean), 0.0);
}

/// The plan point at a radius and an angle in degrees.
Point compute_polar(double radius, double degrees) {
    return Point(radius * std::cos(degrees * Tolerance::TO_RADIANS), radius * std::sin(degrees * Tolerance::TO_RADIANS), 0.0);
}

/// The plan point of rhombic grid coordinates (i, j) on a triangular pattern of side.
Point compute_rhombic(double side, double i, double j) {
    return Point(side * (i + 0.5 * j), side * std::sqrt(3.0) / 2.0 * j, 0.0);
}

// ═══════════════════════════════════════════════════════════════════════════
// Plans
// ═══════════════════════════════════════════════════════════════════════════

/// True when a ring edge lies along a line: parallel within a degree, within tolerance of it, sharing more than tolerance of length.
bool is_along(const Line& edge, const Line& line, double tolerance) {

    const Vector direction = line.to_direction();
    if (std::abs(direction.cross(edge.to_direction())[2]) > std::sin(Tolerance::TO_RADIANS) || std::abs((edge.start() - line.start()).cross(direction)[2]) > tolerance)
        return false;

    const double s0 = (edge.start() - line.start()).dot(direction);
    const double s1 = (edge.end() - line.start()).dot(direction);

    return std::min(std::max(s0, s1), line.length()) - std::max(std::min(s0, s1), 0.0) > tolerance;
}

/// True when a ring touches, crosses or runs along any of the lines.
bool is_touching(const Polyline& ring, const std::vector<Line>& lines, double tolerance) {

    const std::vector<Point> corners = to_loop(ring);
    for (size_t i = 0; i < corners.size(); i++) {
        const Line edge = Line::from_points(corners[i], corners[(i + 1) % corners.size()]);
        for (const Line& line : lines) {
            double t = 0.0;
            double s = 0.0;
            if (is_along(edge, line, tolerance) || (Intersection::line_line_parameters(edge, line, t, s, tolerance, true, false) && edge.point_at(t).distance(line.point_at(s)) <= tolerance))
                return true;
        }
    }

    return false;
}

/// A pattern line clipped to the rings: the pieces whose midpoints lie inside, the whole line when there are no rings.
std::vector<Line> compute_clipped(const Line& line, const std::vector<Polyline>& rings, double tolerance) {

    if (rings.empty())
        return {line};

    std::vector<double> params = {0.0, 1.0};
    for (const Polyline& ring : rings) {
        const std::vector<Point> corners = to_loop(ring);
        for (size_t i = 0; i < corners.size(); i++) {
            double t = 0.0;
            double s = 0.0;
            const Line edge = Line::from_points(corners[i], corners[(i + 1) % corners.size()]);
            if (Intersection::line_line_parameters(line, edge, t, s, tolerance, true, false) && line.point_at(t).distance(edge.point_at(s)) <= tolerance)
                params.push_back(t);
        }
    }
    std::sort(params.begin(), params.end());

    std::vector<Line> pieces;
    for (size_t k = 0; k + 1 < params.size(); k++)
        if ((params[k + 1] - params[k]) * line.length() > tolerance && is_inside(rings, line.point_at((params[k] + params[k + 1]) / 2.0)))
            pieces.push_back(Line::from_points(line.point_at(params[k]), line.point_at(params[k + 1])));

    return pieces;
}

/// The pattern family of a plan edge: that of the pattern line its midpoint lies on, else of a pattern line parallel to it within a degree, else -1.
int compute_family(const Line& edge, const Pattern& pattern, double tolerance) {

    const Point middle = compute_lift(edge.point_at(0.5), 0.0);
    for (size_t i = 0; i < pattern.lines.size(); i++)
        if (pattern.lines[i].closest_point(middle).second.distance(middle) <= tolerance && std::abs(pattern.lines[i].to_direction().dot(edge.to_direction())) > 0.999)
            return pattern.families[i];

    for (size_t i = 0; i < pattern.lines.size(); i++)
        if (pattern.families[i] >= 0 && std::abs(pattern.lines[i].to_direction().dot(edge.to_direction())) > std::cos(Tolerance::TO_RADIANS))
            return pattern.families[i];

    return -1;
}

/// The family of a plan edge from its line id: the pattern line's, or compute_family for a ring edge or a welded one.
int compute_edge_family(const Mesh& plan, std::pair<size_t, size_t> edge, const Pattern& pattern, double tolerance) {

    const double id = plan.edge_attribute(edge, "line").value_or(-1.0);
    if (!is_ring(id) && id >= 0.0)
        return pattern.families[static_cast<size_t>(id)];

    return compute_family(Line::from_points(*plan.vertex_point(edge.first), *plan.vertex_point(edge.second)), pattern, tolerance);
}

/// True when a point lies inside any of the rings.
bool is_in_any(const std::vector<Polyline>& rings, const Point& point) {

    for (const Polyline& ring : rings)
        if (ring.point_in_polygon_2d(point))
            return true;

    return false;
}

/// The lines a plan is arranged from, and the rings held back from it.
struct Lines {
    std::vector<Line> lines; // Pattern pieces, then the edges of the rings that touch a line.
    std::vector<double> ids; // Per line: the pattern index, or a ring id.
    std::vector<Polyline> held; // Hole and core rings that touch no line: face holes of their bay.
    std::vector<int> kinds; // Per held ring: 1 an open hole, 2 a core.
};

/// The lines of a plan: the pattern clipped to the rings with its indices as ids, then the edges of every ring that touches a line with ring ids (core rings in the core band); a hole or core ring that touches nothing is held back.
Lines compute_plan_lines(const Pattern& pattern, const std::vector<Polyline>& rings, const std::vector<Polyline>& cores, const std::vector<Polyline>& extras, double tolerance) {

    Lines result;
    for (size_t i = 0; i < pattern.lines.size(); i++)
        for (const Line& piece : compute_clipped(pattern.lines[i], rings, tolerance)) {
            result.lines.push_back(piece);
            result.ids.push_back(static_cast<double>(i));
        }

    std::vector<Polyline> all = rings;
    all.insert(all.end(), cores.begin(), cores.end());
    all.insert(all.end(), extras.begin(), extras.end());
    for (size_t r = 0; r < all.size(); r++) {
        const int kind = r < rings.size() ? (compute_area(to_loop(all[r])) > 0.0 ? 0 : 1) : r < rings.size() + cores.size() ? 2 : 3;
        if (kind != 0 && !is_touching(all[r], result.lines, tolerance)) {
            if (kind != 3) {
                result.held.push_back(all[r]);
                result.kinds.push_back(kind);
            }
            continue;
        }

        const std::vector<Point> corners = to_loop(all[r]);
        for (size_t e = 0; e < corners.size(); e++) {
            result.lines.push_back(Line::from_points(corners[e], corners[(e + 1) % corners.size()]));
            result.ids.push_back(compute_ring_id(r, e, kind == 2));
        }
    }

    return result;
}

/// Faces outside the rings removed, faces inside a core flagged core with no floor, and with no rings the cells one family alone bounds removed (nothing spans them).
void compute_plan_faces(Mesh& plan, const Pattern& pattern, const std::vector<Polyline>& rings, const std::vector<Polyline>& cores, double tolerance) {

    for (const size_t face : plan.faces()) {
        const Point centre = compute_interior(to_loop(*plan.face_polygon(face)));
        if (!rings.empty() && !is_inside(rings, centre)) {
            plan.remove_face(face);
            continue;
        }

        const bool core = is_in_any(cores, centre);
        plan.set_face_attribute(face, "floor", core ? 0.0 : 1.0);
        plan.set_face_attribute(face, "core", core ? 1.0 : 0.0);
    }

    for (const size_t face : plan.faces()) {
        const std::vector<size_t> loop = *plan.face_vertices(face);
        std::set<int> families;
        for (size_t i = 0; i < loop.size(); i++)
            families.insert(compute_edge_family(plan, {loop[i], loop[(i + 1) % loop.size()]}, pattern, tolerance));
        if (rings.empty() && families.size() == 1 && *families.begin() >= 0)
            plan.remove_face(face);
    }
}

/// Every held-back ring added as a face hole of the face an interior point of it lies in, its vertices with wall 2 for a core and column 0.
void compute_plan_holes(Mesh& plan, const Lines& lines) {

    for (size_t h = 0; h < lines.held.size(); h++)
        for (const size_t face : plan.faces()) {
            const std::vector<Point> corners = to_loop(lines.held[h]);
            if (!plan.face_polygon(face)->point_in_polygon_2d(compute_interior(corners)))
                continue;

            std::vector<size_t> ring;
            for (const Point& corner : corners) {
                ring.push_back(plan.add_vertex(compute_lift(corner, 0.0)));
                plan.set_vertex_attribute(ring.back(), "wall", lines.kinds[h] == 2 ? 2.0 : 0.0);
                plan.set_vertex_attribute(ring.back(), "column", 0.0);
            }
            std::vector<std::vector<size_t>> holes = plan.get_face_holes().count(face) ? plan.get_face_holes().at(face) : std::vector<std::vector<size_t>>();
            holes.push_back(ring);
            plan.set_face_holes(face, holes);
            break;
        }
}

/// Edge attributes family, boundary and wall, vertex attributes boundary and column: a column at every vertex except on or inside a core.
void compute_plan_attributes(Mesh& plan, const Pattern& pattern, const std::vector<Polyline>& cores, double tolerance) {

    for (const std::pair<size_t, size_t>& edge : plan.edges()) {
        const double id = plan.edge_attribute(edge, "line").value_or(-1.0);
        const bool boundary = plan.edge_faces(edge.first, edge.second).value_or(std::vector<size_t>()).size() < 2;
        plan.set_edge_attribute(edge, "family", compute_edge_family(plan, edge, pattern, tolerance));
        plan.set_edge_attribute(edge, "boundary", boundary ? 1.0 : 0.0);
        plan.set_edge_attribute(edge, "wall", is_core_id(id) ? 2.0 : 0.0);
        if (boundary) {
            plan.set_vertex_attribute(edge.first, "boundary", 1.0);
            plan.set_vertex_attribute(edge.second, "boundary", 1.0);
        }
    }

    for (const size_t vertex : plan.vertices()) {
        if (!plan.vertex_neighbors(vertex))
            continue;

        bool on_core = false;
        for (const size_t other : *plan.vertex_neighbors(vertex))
            if (plan.edge_attribute({vertex, other}, "wall").value_or(0.0) == 2.0)
                on_core = true;
        plan.set_vertex_attribute(vertex, "column", on_core || is_in_any(cores, *plan.vertex_point(vertex)) ? 0.0 : 1.0);
    }
}

/// The plan of one level: the pattern clipped to the section rings, the rings, the cores and extra rings arranged into faces; faces outside removed, core faces flagged, rings that touch nothing held back as face holes, then family, boundary, wall, column and floor attributes.
Mesh compute_plan(const Pattern& pattern, const std::vector<Polyline>& rings, const std::vector<Polyline>& cores, const std::vector<Polyline>& extras, double tolerance, double merge) {

    const Lines lines = compute_plan_lines(pattern, rings, cores, extras, tolerance);
    const std::pair<std::vector<Line>, std::vector<double>> split = compute_crossings(lines.lines, lines.ids, tolerance, merge);

    Mesh plan = compute_arrangement(split.first, split.second, tolerance);
    compute_plan_faces(plan, pattern, rings, cores, tolerance);
    compute_plan_holes(plan, lines);
    compute_plan_attributes(plan, pattern, cores, tolerance);

    return plan;
}

// ═══════════════════════════════════════════════════════════════════════════
// Levels
// ═══════════════════════════════════════════════════════════════════════════

/// The cores whose first corner lies inside the rings, every ring counter-clockwise at z 0.
std::vector<Polyline> compute_cores(const std::vector<Polyline>& cores, const std::vector<Polyline>& rings) {

    std::vector<Polyline> inside;
    for (const Polyline& core : cores) {
        std::vector<Point> corners = to_loop(core);
        if (compute_area(corners) < 0.0)
            std::reverse(corners.begin(), corners.end());
        if (rings.empty() || is_inside(rings, corners[0]))
            inside.push_back(compute_lifted(to_polyline(corners), 0.0));
    }

    return inside;
}

/// True when a point lies on one of the lines within tolerance.
bool is_on_line(const Point& point, const std::vector<Line>& lines, double tolerance) {

    const Point flat = compute_lift(point, 0.0);
    for (const Line& line : lines)
        if (line.closest_point(flat).second.distance(flat) <= tolerance)
            return true;

    return false;
}

/// The corners of a ring and every point where a line crosses a side at more than 30 degrees, in order; a corner is flagged when it turns by more than 30 degrees or lies on a line, a crossing always.
std::vector<std::pair<Point, bool>> compute_anchors(const std::vector<Point>& corners, const std::vector<Line>& lines, double tolerance) {

    const double sharp = std::sin(30.0 * Tolerance::TO_RADIANS);
    const size_t count = corners.size();
    std::vector<std::pair<Point, bool>> points;
    for (size_t i = 0; i < count; i++) {
        const Line side = Line::from_points(corners[i], corners[(i + 1) % count]);
        const Vector after = compute_direction(corners[i], corners[(i + 1) % count]);
        const Vector before = compute_direction(corners[(i + count - 1) % count], corners[i]);
        points.emplace_back(corners[i], std::abs(before.cross(after)[2]) > sharp || before.dot(after) < 0.0 || is_on_line(corners[i], lines, tolerance));

        std::vector<double> params;
        for (const Line& line : lines) {
            double t = 0.0;
            double s = 0.0;
            if (std::abs(after.cross(line.to_direction())[2]) > sharp && Intersection::line_line_parameters(side, line, t, s, tolerance, true, false) && side.point_at(t).distance(line.point_at(s)) <= tolerance)
                params.push_back(t);
        }
        std::sort(params.begin(), params.end());

        for (const double t : params)
            if (t * side.length() > tolerance && (1.0 - t) * side.length() > tolerance && compute_distance(points.back().first, side.point_at(t)) > tolerance)
                points.emplace_back(compute_lift(side.point_at(t), 0.0), true);
    }

    return points;
}

/// A section ring with the pattern crossings as corners and its smooth corners dropped wherever the chord between the flagged points on either side stays within merge of them: a faceted curve takes its corners from the pattern, a polygon keeps its own; unchanged when nothing is flagged.
Polyline compute_resampled(const Polyline& ring, const Pattern& pattern, double tolerance, double merge) {

    const std::vector<std::pair<Point, bool>> points = compute_anchors(to_loop(ring), pattern.lines, tolerance);
    std::vector<size_t> anchors;
    for (size_t i = 0; i < points.size(); i++)
        if (points[i].second)
            anchors.push_back(i);
    if (anchors.empty())
        return ring;

    std::vector<Point> kept;
    for (size_t k = 0; k < anchors.size(); k++) {
        const size_t from = anchors[k];
        const size_t to = anchors[(k + 1) % anchors.size()];
        const size_t run = (to + points.size() - from - 1) % points.size();
        const Line chord = Line::from_points(points[from].first, points[to].first);
        bool close = true;
        for (size_t j = 1; j <= run; j++)
            close = close && chord.closest_point(points[(from + j) % points.size()].first).second.distance(points[(from + j) % points.size()].first) <= merge;

        kept.push_back(points[from].first);
        for (size_t j = 1; j <= run && !close; j++)
            kept.push_back(points[(from + j) % points.size()].first);
    }

    return kept.size() < 3 ? ring : to_polyline(kept);
}

/// The section of a solid at z with every ring resampled on the pattern.
std::vector<Polyline> compute_slice(const Mesh& solid, double z, const Pattern& pattern, double tolerance, double merge) {

    std::vector<Polyline> rings;
    for (const Polyline& ring : compute_section(solid, z, tolerance))
        rings.push_back(compute_resampled(ring, pattern, tolerance, merge));

    return rings;
}

/// A level from its rings and cores, its plan computed; the ground has no floor unless asked.
Level compute_level(double z, const std::vector<Polyline>& rings, const std::vector<Polyline>& cores, const std::vector<Polyline>& extras, const Pattern& pattern, double tolerance, double merge, bool floor) {

    Level level;
    level.z = z;
    for (const Polyline& ring : rings)
        level.rings.push_back(compute_resampled(ring, pattern, tolerance, merge));
    level.cores = compute_cores(cores, level.rings);
    level.plan = compute_plan(pattern, level.rings, level.cores, extras, tolerance, merge);
    if (!floor)
        for (const size_t face : level.plan.faces())
            level.plan.set_face_attribute(face, "floor", 0.0);

    return level;
}

/// True when a ring has the same corners as one of the rings within tolerance, in any rotation and direction.
bool is_known(const Polyline& ring, const std::vector<Polyline>& rings, double tolerance) {

    const std::vector<Point> corners = to_loop(ring);
    for (const Polyline& other : rings) {
        const std::vector<Point> others = to_loop(other);
        if (others.size() != corners.size())
            continue;

        size_t found = 0;
        for (const Point& corner : corners)
            for (const Point& candidate : others)
                if (compute_distance(corner, candidate) <= tolerance) {
                    found++;
                    break;
                }
        if (found == corners.size())
            return true;
    }

    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// Drawn lines
// ═══════════════════════════════════════════════════════════════════════════

/// Degrees between a segment and the horizontal plane.
double compute_tilt(const Point& a, const Point& b) {
    return std::atan2(std::abs(b[2] - a[2]), compute_distance(a, b)) * Tolerance::TO_DEGREES;
}

/// Degrees between a surface normal and vertical: 0 for a floor, 90 for a wall.
double compute_slope(const Polyline& surface) {

    const Vector normal = wood_session::compute_newell(surface.get_points());

    return std::acos(std::min(1.0, std::abs(normal[2]))) * Tolerance::TO_DEGREES;
}

/// Lowest and highest z of a polyline.
std::pair<double, double> compute_z_range(const Polyline& polyline) {

    std::pair<double, double> range(std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
    for (const Point& point : polyline.get_points()) {
        range.first = std::min(range.first, point[2]);
        range.second = std::max(range.second, point[2]);
    }

    return range;
}

/// Every distinct z of the line ends and surface corners, ascending, closer than tolerance merged.
std::vector<double> compute_elevations(const std::vector<Line>& lines, const std::vector<Polyline>& surfaces, double tolerance) {

    std::vector<double> heights;
    for (const Line& line : lines) {
        heights.push_back(line.start()[2]);
        heights.push_back(line.end()[2]);
    }
    for (const Polyline& surface : surfaces) {
        heights.push_back(compute_z_range(surface).first);
        heights.push_back(compute_z_range(surface).second);
    }
    std::sort(heights.begin(), heights.end());

    std::vector<double> elevations;
    for (const double z : heights)
        if (elevations.empty() || z - elevations.back() > tolerance)
            elevations.push_back(z);

    return elevations;
}

/// The level of a height: the first elevation within tolerance below or at it.
size_t compute_level_of(const std::vector<double>& elevations, double z, double tolerance) {
    return static_cast<size_t>(std::lower_bound(elevations.begin(), elevations.end(), z - tolerance) - elevations.begin());
}

/// What is drawn at one level: its plan lines, the floors and the walls.
struct Drawn {
    std::vector<Line> lines; // The horizontal lines there, then the floor edges and the wall top edges, all at z 0.
    std::vector<double> ids; // Per line: its index among the drawn lines, -1 for a floor or wall edge.
    std::vector<Polyline> floors; // The horizontal surfaces there, counter-clockwise at z 0.
    std::vector<std::pair<Line, double>> walls; // The top edge of every vertical surface there with 2 for a core, 1 otherwise.
};

/// The plan lines of level k as drawn: the horizontal lines there, then the floor edges and the top edges of the walls there, with the floors and the walls beside.
Drawn compute_drawn(const std::vector<Line>& lines, const std::vector<Polyline>& surfaces, const std::vector<double>& elevations, size_t k, double tolerance, double angle) {

    Drawn drawn;
    for (const Line& line : lines)
        if (compute_tilt(line.start(), line.end()) <= angle && compute_level_of(elevations, line.start()[2], tolerance) == k) {
            drawn.lines.push_back(Line::from_points(compute_lift(line.start(), 0.0), compute_lift(line.end(), 0.0)));
            drawn.ids.push_back(static_cast<double>(drawn.lines.size() - 1));
        }

    for (const Polyline& surface : surfaces) {
        const std::pair<double, double> range = compute_z_range(surface);
        if (compute_slope(surface) <= angle && compute_level_of(elevations, range.first, tolerance) == k) {
            std::vector<Point> corners = to_loop(compute_lifted(surface, 0.0));
            if (compute_area(corners) < 0.0)
                std::reverse(corners.begin(), corners.end());
            drawn.floors.push_back(to_polyline(corners));
            for (size_t e = 0; e < corners.size(); e++) {
                drawn.lines.push_back(Line::from_points(corners[e], corners[(e + 1) % corners.size()]));
                drawn.ids.push_back(-1.0);
            }
        }

        if (compute_slope(surface) < 90.0 - angle || compute_level_of(elevations, range.second, tolerance) != k)
            continue;

        std::vector<Point> top;
        for (const Point& point : to_loop(surface))
            if (std::abs(point[2] - range.second) <= tolerance)
                top.push_back(compute_lift(point, 0.0));
        if (top.size() < 2)
            continue;

        drawn.walls.emplace_back(Line::from_points(top.front(), top.back()), surface.name.find("core") != std::string::npos ? 2.0 : 1.0);
        drawn.lines.push_back(drawn.walls.back().first);
        drawn.ids.push_back(-1.0);
    }

    return drawn;
}

/// Attributes of a drawn plan: floor 1 on the faces inside a drawn floor, family -1, boundary where fewer than two floors meet, role 0 on floor and wall edges nobody drew a line on, wall from the drawn walls, column 0 everywhere.
void compute_drawn_attributes(Mesh& plan, const Drawn& drawn, double tolerance) {

    for (const size_t face : plan.faces())
        plan.set_face_attribute(face, "floor", is_inside(drawn.floors, compute_interior(to_loop(*plan.face_polygon(face)))) ? 1.0 : 0.0);

    for (const std::pair<size_t, size_t>& edge : plan.edges()) {
        const Point middle = compute_lift(*plan.vertex_point(edge.first), 0.0) + (compute_lift(*plan.vertex_point(edge.second), 0.0) - compute_lift(*plan.vertex_point(edge.first), 0.0)) * 0.5;
        int floored = 0;
        for (const size_t face : plan.edge_faces(edge.first, edge.second).value_or(std::vector<size_t>()))
            if (plan.face_attribute(face, "floor").value_or(0.0) == 1.0)
                floored++;

        plan.set_edge_attribute(edge, "family", -1.0);
        plan.set_edge_attribute(edge, "boundary", floored < 2 ? 1.0 : 0.0);
        plan.set_edge_attribute(edge, "wall", 0.0);
        if (plan.edge_attribute(edge, "line").value_or(-1.0) < 0.0)
            plan.set_edge_attribute(edge, "role", 0.0);
        for (const std::pair<Line, double>& wall : drawn.walls)
            if (wall.first.closest_point(middle).second.distance(middle) <= tolerance)
                plan.set_edge_attribute(edge, "wall", wall.second);
    }

    for (const size_t vertex : plan.vertices())
        plan.set_vertex_attribute(vertex, "column", 0.0);
}

/// Column 1 on the plan vertex under every end of a vertical line at level k, the vertex added when no line meets there.
void compute_drawn_columns(Mesh& plan, const std::vector<Line>& lines, const std::vector<double>& elevations, size_t k, double tolerance, double angle) {

    for (const Line& line : lines) {
        if (compute_tilt(line.start(), line.end()) < 90.0 - angle)
            continue;

        for (const Point& end : {line.start(), line.end()}) {
            if (compute_level_of(elevations, end[2], tolerance) != k)
                continue;

            std::optional<size_t> found;
            for (const size_t vertex : plan.vertices())
                if (compute_distance(*plan.vertex_point(vertex), end) <= tolerance)
                    found = vertex;
            plan.set_vertex_attribute(found ? *found : plan.add_vertex(compute_lift(end, 0.0)), "column", 1.0);
        }
    }
}

} // namespace wood_grid::levels

namespace wood_grid {

// ═══════════════════════════════════════════════════════════════════════════
// Bays
// ═══════════════════════════════════════════════════════════════════════════

std::vector<double> compute_bays(double length, double spacing, double remainder) {

    std::vector<double> bays(static_cast<size_t>(std::floor(length / spacing + 1e-9)), spacing);
    const double rest = length - spacing * bays.size();
    if (rest >= remainder)
        bays.push_back(rest);
    else if (rest > 1e-9 && !bays.empty())
        bays.back() += rest;

    return bays;
}

// ═══════════════════════════════════════════════════════════════════════════
// Pattern constructors
// ═══════════════════════════════════════════════════════════════════════════

Pattern Pattern::orthogonal(const std::vector<double>& xs, const std::vector<double>& ys, double skew) {

    const std::vector<double> x = levels::compute_sums(xs);
    const std::vector<double> y = levels::compute_sums(ys);
    const double lean = skew * Tolerance::TO_RADIANS;

    Pattern pattern;
    for (const double v : y) {
        pattern.lines.push_back(Line::from_points(levels::compute_skewed(x.front(), v, lean), levels::compute_skewed(x.back(), v, lean)));
        pattern.families.push_back(0);
    }

    for (const double u : x) {
        pattern.lines.push_back(Line::from_points(levels::compute_skewed(u, y.front(), lean), levels::compute_skewed(u, y.back(), lean)));
        pattern.families.push_back(1);
    }

    return pattern;
}

Pattern Pattern::radial(const std::vector<double>& radii, int sectors, double sweep) {

    const double step = sweep / sectors;
    const int chords = sweep >= 360.0 - 1e-9 ? sectors : sectors + 1;

    Pattern pattern;
    for (int j = 0; j < chords; j++) {
        pattern.lines.push_back(Line::from_points(levels::compute_polar(radii.front(), step * j), levels::compute_polar(radii.back(), step * j)));
        pattern.families.push_back(0);
    }

    for (const double radius : radii)
        for (int j = 0; j < sectors && radius > 0.0; j++) {
            pattern.lines.push_back(Line::from_points(levels::compute_polar(radius, step * j), levels::compute_polar(radius, step * (j + 1))));
            pattern.families.push_back(1);
        }

    return pattern;
}

Pattern Pattern::triangular(double side, int nx, int ny) {

    Pattern pattern;
    for (int j = 0; j <= ny; j++) {
        pattern.lines.push_back(Line::from_points(levels::compute_rhombic(side, 0, j), levels::compute_rhombic(side, nx, j)));
        pattern.families.push_back(0);
    }

    for (int i = 0; i <= nx; i++) {
        pattern.lines.push_back(Line::from_points(levels::compute_rhombic(side, i, 0), levels::compute_rhombic(side, i, ny)));
        pattern.families.push_back(1);
    }

    for (int k = 1; k < nx + ny; k++) {
        const int i0 = std::min(k, nx);
        const int i1 = std::max(0, k - ny);
        pattern.lines.push_back(Line::from_points(levels::compute_rhombic(side, i0, k - i0), levels::compute_rhombic(side, i1, k - i1)));
        pattern.families.push_back(2);
    }

    return pattern;
}

Pattern Pattern::hexagonal(double side, int nx, int ny) {

    std::vector<Line> lines;
    for (int j = 0; j < ny; j++)
        for (int i = 0; i < nx; i++) {
            const Point centre(std::sqrt(3.0) * side * (i + 0.5 * (j % 2)), 1.5 * side * j, 0.0);
            std::vector<Point> corners;
            for (int k = 0; k < 6; k++)
                corners.push_back(centre + (levels::compute_polar(side, 30.0 + 60.0 * k) - Point(0.0, 0.0, 0.0)));
            for (int k = 0; k < 6; k++)
                lines.push_back(Line::from_points(corners[k], corners[(k + 1) % 6]));
        }

    return from_lines(lines);
}

Pattern Pattern::from_lines(const std::vector<Line>& lines, const std::vector<int>& families) {

    Pattern pattern;
    pattern.lines = lines;
    pattern.families = families.empty() ? std::vector<int>(lines.size(), -1) : families;

    return pattern;
}

Pattern Pattern::transformed(const Xform& xform) const {

    Pattern moved = *this;
    for (Line& line : moved.lines)
        line = line.transformed(xform);

    return moved;
}

// ═══════════════════════════════════════════════════════════════════════════
// Building constructors
// ═══════════════════════════════════════════════════════════════════════════

Building Building::from_footprint(const std::vector<Polyline>& footprint, const std::vector<double>& elevations, const Pattern& pattern, const std::vector<Polyline>& cores, double tolerance, double merge) {

    std::vector<Polyline> rings;
    for (const Polyline& ring : footprint)
        rings.push_back(plan::compute_lifted(ring, 0.0));

    Building building;
    building.pattern = pattern;
    building.tolerance = tolerance;
    building.levels.push_back(levels::compute_level(elevations[0], rings, cores, {}, pattern, tolerance, merge, false));
    for (size_t k = 1; k < elevations.size(); k++) {
        building.levels.push_back(building.levels[0]);
        building.levels.back().z = elevations[k];
        for (const size_t face : building.levels.back().plan.faces())
            building.levels.back().plan.set_face_attribute(face, "floor", building.levels.back().plan.face_attribute(face, "core").value_or(0.0) == 1.0 ? 0.0 : 1.0);
    }

    return building;
}

Building Building::from_solid(const Mesh& massing, const std::vector<double>& elevations, const Pattern& pattern, const std::vector<Polyline>& cores, double tolerance, double merge) {

    Mesh solid = massing;
    solid.orient_outward();
    const size_t count = elevations.size();
    std::vector<std::vector<Polyline>> below(count);
    std::vector<std::vector<Polyline>> above(count);
    for (size_t k = 0; k < count; k++) {
        below[k] = k > 0 ? levels::compute_slice(solid, elevations[k] - tolerance, pattern, tolerance, merge) : std::vector<Polyline>();
        above[k] = k + 1 < count ? levels::compute_slice(solid, elevations[k] + tolerance, pattern, tolerance, merge) : std::vector<Polyline>();
    }

    Building building;
    building.pattern = pattern;
    building.tolerance = tolerance;
    for (size_t k = 0; k < count; k++) {
        std::vector<Polyline> rings = below[k].empty() ? above[k] : above[k].empty() ? below[k] : plan::compute_regions(below[k], above[k], 1);
        for (Polyline& ring : rings)
            ring.merge_collinear(1e-6);

        std::vector<Polyline> extras;
        for (size_t storey = k > 0 ? k - 1 : k; storey <= k && storey + 1 < count; storey++)
            for (const Polyline& ring : plan::compute_regions(above[storey], below[storey + 1], 0))
                if (plan::compute_area(plan::to_loop(ring)) > 0.0 && !levels::is_known(ring, rings, tolerance))
                    extras.push_back(ring);

        building.levels.push_back(levels::compute_level(elevations[k], rings, cores, extras, pattern, tolerance, merge, k > 0));
    }

    return building;
}

Building Building::from_lines(const std::vector<Line>& lines, const std::vector<Polyline>& surfaces, double tolerance, double angle) {

    const std::vector<double> elevations = levels::compute_elevations(lines, surfaces, tolerance);

    Building building;
    building.tolerance = tolerance;
    for (size_t k = 0; k < elevations.size(); k++) {
        const levels::Drawn drawn = levels::compute_drawn(lines, surfaces, elevations, k, tolerance, angle);
        const std::pair<std::vector<Line>, std::vector<double>> split = plan::compute_crossings(drawn.lines, drawn.ids, tolerance, tolerance);

        Level level;
        level.z = elevations[k];
        level.rings = drawn.floors;
        level.plan = plan::compute_arrangement(split.first, split.second, tolerance);
        levels::compute_drawn_attributes(level.plan, drawn, tolerance);
        levels::compute_drawn_columns(level.plan, lines, elevations, k, tolerance, angle);
        building.levels.push_back(level);
    }

    for (const Line& line : lines)
        if (levels::compute_tilt(line.start(), line.end()) > angle && levels::compute_tilt(line.start(), line.end()) < 90.0 - angle)
            building.braces.push_back(line);

    return building;
}

Mesh to_mesh(const BRep& massing, double facet, double tolerance) {

    std::vector<std::vector<Point>> polygons;
    for (const Mesh& part : massing.face_meshes_q(true, facet, 0.1))
        for (const Polyline& polygon : part.face_outlines())
            polygons.push_back(plan::to_loop(polygon));

    return Mesh::from_polylines(polygons, tolerance * 0.01);
}

} // namespace wood_grid
