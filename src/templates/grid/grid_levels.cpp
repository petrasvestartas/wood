#include "pch.h"
#include "src/templates/grid/grid_plan.h"
#include "wood_element_geometry.h"

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

/// The family of a plan edge: its pattern line's, else that of a pattern line parallel to it within a degree, else -1.
int compute_family(const Mesh& plan, std::pair<size_t, size_t> edge, const Pattern& pattern) {

    const double id = plan.edge_attribute(edge, "line").value_or(-1.0);
    if (!is_ring(id) && id >= 0.0)
        return pattern.families[static_cast<size_t>(id)];

    const Vector direction = compute_direction(*plan.vertex_point(edge.first), *plan.vertex_point(edge.second));
    for (size_t i = 0; i < pattern.lines.size(); i++)
        if (pattern.families[i] >= 0 && std::abs(pattern.lines[i].to_direction().dot(direction)) > std::cos(Tolerance::TO_RADIANS))
            return pattern.families[i];

    return -1;
}

/// The plan of one level: the pattern clipped to the rings and every ring's edges arranged into faces; faces outside the rings removed, with no rings the cells one family alone bounds; then family, boundary, column and floor attributes.
Mesh compute_plan(const Pattern& pattern, const std::vector<Polyline>& rings, const std::vector<Polyline>& extras, double tolerance, double merge, bool floor) {

    std::vector<Line> lines;
    std::vector<double> ids;
    for (size_t i = 0; i < pattern.lines.size(); i++)
        for (const Piece& piece : compute_pieces(pattern.lines[i], rings, true, tolerance)) {
            lines.push_back(piece.line);
            ids.push_back(static_cast<double>(i));
        }

    std::vector<Line> sides;
    std::vector<Polyline> all = rings;
    all.insert(all.end(), extras.begin(), extras.end());
    for (size_t r = 0; r < all.size(); r++) {
        const std::vector<Point> corners = to_loop(all[r]);
        for (size_t e = 0; e < corners.size(); e++) {
            sides.push_back(Line::from_points(corners[e], corners[(e + 1) % corners.size()]));
            ids.push_back(compute_ring_id(r, e));
        }
    }

    Mesh plan = compute_arrangement(lines, sides, ids, tolerance, merge);
    for (const size_t face : plan.faces()) {
        const std::vector<size_t> loop = *plan.face_vertices(face);
        std::set<int> families;
        for (size_t i = 0; i < loop.size(); i++)
            families.insert(compute_family(plan, {loop[i], loop[(i + 1) % loop.size()]}, pattern));
        if (rings.empty() ? families.size() == 1 && *families.begin() >= 0 : !is_inside(rings, compute_interior(to_loop(*plan.face_polygon(face)))))
            plan.remove_face(face);
        else
            plan.set_face_attribute(face, "floor", floor ? 1.0 : 0.0);
    }

    for (const std::pair<size_t, size_t>& edge : plan.edges()) {
        const bool boundary = plan.edge_faces(edge.first, edge.second).value_or(std::vector<size_t>()).size() < 2;
        plan.set_edge_attribute(edge, "family", compute_family(plan, edge, pattern));
        plan.set_edge_attribute(edge, "boundary", boundary ? 1.0 : 0.0);
        for (const size_t vertex : {edge.first, edge.second}) {
            plan.set_vertex_attribute(vertex, "column", 1.0);
            if (boundary)
                plan.set_vertex_attribute(vertex, "boundary", 1.0);
        }
    }

    return plan;
}

/// The section of a solid by the horizontal plane through z, its rings at z 0.
std::vector<Polyline> compute_section(const Mesh& solid, double z) {

    std::vector<Polyline> rings;
    for (const Polyline& ring : solid.section_by_plane(Plane::from_point_normal(Point(0.0, 0.0, z), Vector(0.0, 0.0, 1.0))))
        rings.push_back(compute_lifted(ring, 0.0));

    return rings;
}

/// A level from its section rings and the cores inside them, every ring counter-clockwise at z 0, its plan computed with column 0 inside a core.
Level compute_level(double z, const std::vector<Polyline>& rings, const std::vector<Polyline>& cores, const std::vector<Polyline>& extras, const Pattern& pattern, double tolerance, double merge, bool floor) {

    Level level;
    level.z = z;
    for (const Polyline& core : cores) {
        std::vector<Point> corners = to_loop(compute_lifted(core, 0.0));
        if (compute_area(corners) < 0.0)
            std::reverse(corners.begin(), corners.end());
        if (rings.empty() || is_inside(rings, corners[0]))
            level.cores.push_back(to_polyline(corners));
    }

    level.plan = compute_plan(pattern, rings, extras, tolerance, merge, floor);
    for (const size_t vertex : level.plan.vertices())
        if (is_inside(level.cores, *level.plan.vertex_point(vertex)))
            level.plan.set_vertex_attribute(vertex, "column", 0.0);

    return level;
}

// ═══════════════════════════════════════════════════════════════════════════
// Drawn lines
// ═══════════════════════════════════════════════════════════════════════════

/// Degrees between a segment and the horizontal plane.
double compute_tilt(const Line& line) {
    return std::atan2(std::abs(line.end()[2] - line.start()[2]), compute_distance(line.start(), line.end())) * Tolerance::TO_DEGREES;
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

/// True when z lies at elevation k within tolerance.
bool is_at(const std::vector<double>& elevations, size_t k, double z, double tolerance) {
    return std::abs(elevations[k] - z) <= tolerance;
}

/// What is drawn at one level: its plan lines, the floors and the walls.
struct Drawn {
    std::vector<Line> lines; // The horizontal lines there, then the floor edges and the wall top edges, all at z 0.
    std::vector<double> ids; // Per line: its index among the drawn lines, -1 for a floor or wall edge.
    std::vector<Polyline> floors; // The horizontal surfaces there, counter-clockwise at z 0.
    std::vector<std::pair<Line, double>> walls; // The top edge of every vertical surface ending there, with 2 for a core, 1 otherwise.
};

/// The lines of level k as drawn: the horizontal lines there, the edges of the floors there and the top edges of the walls ending there, with the floors and walls beside.
Drawn compute_drawn(const std::vector<Line>& lines, const std::vector<Polyline>& surfaces, const std::vector<double>& elevations, size_t k, double tolerance, double angle) {

    Drawn drawn;
    for (const Line& line : lines)
        if (compute_tilt(line) <= angle && is_at(elevations, k, line.start()[2], tolerance)) {
            drawn.lines.push_back(Line::from_points(compute_lift(line.start(), 0.0), compute_lift(line.end(), 0.0)));
            drawn.ids.push_back(static_cast<double>(drawn.lines.size() - 1));
        }

    for (const Polyline& surface : surfaces) {
        const std::pair<double, double> range = compute_z_range(surface);
        const bool level = std::abs(wood_session::compute_newell(surface.get_points())[2]) > std::cos(angle * Tolerance::TO_RADIANS);
        if (level && is_at(elevations, k, range.first, tolerance)) {
            std::vector<Point> corners = to_loop(compute_lifted(surface, 0.0));
            if (compute_area(corners) < 0.0)
                std::reverse(corners.begin(), corners.end());
            drawn.floors.push_back(to_polyline(corners));
            for (size_t e = 0; e < corners.size(); e++) {
                drawn.lines.push_back(Line::from_points(corners[e], corners[(e + 1) % corners.size()]));
                drawn.ids.push_back(-1.0);
            }
        }

        std::vector<Point> top;
        for (const Point& point : to_loop(surface))
            if (std::abs(point[2] - range.second) <= tolerance)
                top.push_back(compute_lift(point, 0.0));
        if (level || !is_at(elevations, k, range.second, tolerance) || top.size() < 2 || range.second - range.first <= tolerance)
            continue;

        drawn.walls.emplace_back(Line::from_points(top.front(), top.back()), surface.name.find("core") != std::string::npos ? 2.0 : 1.0);
        drawn.lines.push_back(drawn.walls.back().first);
        drawn.ids.push_back(-1.0);
    }

    return drawn;
}

/// The plan of level k as drawn: floor 1 inside a drawn floor, family -1, boundary where fewer than two floors meet, role 0 on floor and wall edges nobody drew a line on, wall 1 under a drawn wall (2 when named core), column 1 under every end of a vertical line.
Mesh compute_drawn_plan(const std::vector<Line>& lines, const std::vector<Polyline>& surfaces, const std::vector<double>& elevations, size_t k, double tolerance, double angle) {

    const Drawn drawn = compute_drawn(lines, surfaces, elevations, k, tolerance, angle);
    Mesh plan = compute_arrangement(drawn.lines, {}, drawn.ids, tolerance, tolerance);
    for (const size_t face : plan.faces())
        plan.set_face_attribute(face, "floor", is_inside(drawn.floors, compute_interior(to_loop(*plan.face_polygon(face)))) ? 1.0 : 0.0);

    for (const std::pair<size_t, size_t>& edge : plan.edges()) {
        const Point middle = compute_lift(*plan.vertex_point(edge.first), 0.0) + (compute_lift(*plan.vertex_point(edge.second), 0.0) - compute_lift(*plan.vertex_point(edge.first), 0.0)) * 0.5;
        int floored = 0;
        for (const size_t face : plan.edge_faces(edge.first, edge.second).value_or(std::vector<size_t>()))
            floored += plan.face_attribute(face, "floor").value_or(0.0) == 1.0 ? 1 : 0;

        plan.set_edge_attribute(edge, "family", -1.0);
        plan.set_edge_attribute(edge, "boundary", floored < 2 ? 1.0 : 0.0);
        if (plan.edge_attribute(edge, "line").value_or(-1.0) < 0.0)
            plan.set_edge_attribute(edge, "role", 0.0);
        for (const std::pair<Line, double>& wall : drawn.walls)
            if (wall.first.closest_point(middle).second.distance(middle) <= tolerance)
                plan.set_edge_attribute(edge, "wall", wall.second);
    }

    for (const Line& line : lines)
        for (const Point& end : {line.start(), line.end()}) {
            if (compute_tilt(line) < 90.0 - angle || !is_at(elevations, k, end[2], tolerance))
                continue;

            std::optional<size_t> found;
            for (const size_t vertex : plan.vertices())
                if (compute_distance(*plan.vertex_point(vertex), end) <= tolerance)
                    found = vertex;
            plan.set_vertex_attribute(found ? *found : plan.add_vertex(compute_lift(end, 0.0)), "column", 1.0);
        }

    return plan;
}

} // namespace wood_grid::levels

namespace wood_grid {

// ═══════════════════════════════════════════════════════════════════════════
// Pattern constructors
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
    const int rays = sweep >= 360.0 - 1e-9 ? sectors : sectors + 1;

    Pattern pattern;
    for (int j = 0; j < rays; j++) {
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
        pattern.lines.push_back(Line::from_points(levels::compute_rhombic(side, std::min(k, nx), k - std::min(k, nx)), levels::compute_rhombic(side, std::max(0, k - ny), k - std::max(0, k - ny))));
        pattern.families.push_back(2);
    }

    return pattern;
}

Pattern Pattern::hexagonal(double side, int nx, int ny) {

    std::vector<Line> lines;
    for (int j = 0; j < ny; j++)
        for (int i = 0; i < nx; i++) {
            const Point centre(std::sqrt(3.0) * side * (i + 0.5 * (j % 2)), 1.5 * side * j, 0.0);
            for (int k = 0; k < 6; k++)
                lines.push_back(Line::from_points(centre + (levels::compute_polar(side, 30.0 + 60.0 * k) - Point(0.0, 0.0, 0.0)), centre + (levels::compute_polar(side, 90.0 + 60.0 * k) - Point(0.0, 0.0, 0.0))));
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
    building.tolerance = tolerance;
    building.levels.push_back(levels::compute_level(elevations[0], rings, cores, {}, pattern, tolerance, merge, false));
    for (size_t k = 1; k < elevations.size(); k++) {
        building.levels.push_back(building.levels[0]);
        building.levels.back().z = elevations[k];
        for (const size_t face : building.levels.back().plan.faces())
            building.levels.back().plan.set_face_attribute(face, "floor", 1.0);
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
        below[k] = k > 0 ? levels::compute_section(solid, elevations[k] - tolerance) : std::vector<Polyline>();
        above[k] = k + 1 < count ? levels::compute_section(solid, elevations[k] + tolerance) : std::vector<Polyline>();
    }

    Building building;
    building.tolerance = tolerance;
    for (size_t k = 0; k < count; k++) {
        std::vector<Polyline> rings = below[k].empty() ? above[k] : above[k].empty() ? below[k] : BooleanPolyline::compute_regions(below[k], above[k], 1);
        for (Polyline& ring : rings)
            ring.merge_collinear(1e-6);

        std::vector<Polyline> extras;
        for (size_t storey = k > 0 ? k - 1 : k; storey <= k && storey + 1 < count; storey++)
            for (const Polyline& ring : BooleanPolyline::compute_regions(above[storey], below[storey + 1], 0))
                if (plan::compute_area(plan::to_loop(ring)) > 0.0)
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
        Level level;
        level.z = elevations[k];
        level.plan = levels::compute_drawn_plan(lines, surfaces, elevations, k, tolerance, angle);
        building.levels.push_back(level);
    }

    for (const Line& line : lines)
        if (levels::compute_tilt(line) > angle && levels::compute_tilt(line) < 90.0 - angle)
            building.braces.push_back(line);

    return building;
}

} // namespace wood_grid
