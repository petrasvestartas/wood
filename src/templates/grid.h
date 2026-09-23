#pragma once
#include "wood_session.h"
#include "wood_profile.h"
#include "src/templates/plan.h"

namespace wood_grid {

// ═══════════════════════════════════════════════════════════════════════════
// Pattern
// ═══════════════════════════════════════════════════════════════════════════

/// Plan lines a building is drawn on, at z 0, each in a parallel family: 0 and 1 the two directions a rectangular system names, 2 a third family, -1 a free line.
struct Pattern {
    std::vector<session_cpp::Line> lines; // Segments in plan, as long as the pattern extends; a footprint beyond them gets no members there.
    std::vector<int> families; // One per line.

    /// Lines along x at the running sums of ys (family 0) and along y at the running sums of xs (family 1), the y axis leaning skew degrees towards x.
    static Pattern orthogonal(const std::vector<double>& xs, const std::vector<double>& ys, double skew = 0.0);

    /// Rays from the first radius to the last (family 0) and ring chords at every radius (family 1) over sweep degrees; a first radius of 0 gives a centre point.
    static Pattern radial(const std::vector<double>& radii, int sectors, double sweep = 360.0);

    /// Three families of lines at 0, 60 and 120 degrees, side apart, over nx by ny rhombi.
    static Pattern triangular(double side, int nx, int ny);

    /// Edges of pointy-top hexagons of side in nx columns and ny rows, every edge a free line (family -1).
    static Pattern hexagonal(double side, int nx, int ny);

    /// Lines as drawn with a family per line, all free when families is empty.
    static Pattern from_lines(const std::vector<session_cpp::Line>& lines, const std::vector<int>& families = {});

    /// A copy moved by xform: the pattern's origin and rotation under the building.
    Pattern transformed(const session_cpp::Xform& xform) const;
};

/// Bay widths over length at spacing, Branch's rule: whole bays from the start while the rest is at least remainder long, the rest as the last bay.
inline std::vector<double> compute_bays(double length, double spacing, double remainder = 304.8) {

    std::vector<double> bays(static_cast<size_t>(std::floor(length / spacing + 1e-9)), spacing);
    const double rest = length - spacing * bays.size();
    if (rest >= remainder)
        bays.push_back(rest);
    else if (rest > 1e-9 && !bays.empty())
        bays.back() += rest;

    return bays;
}

// ═══════════════════════════════════════════════════════════════════════════
// Framing
// ═══════════════════════════════════════════════════════════════════════════

/// Section per role, each a profile in its own frame (loop 0 outer counter-clockwise centred on the axis, x width, y depth up; loops 1.. holes); an empty role falls back as noted.
struct Profiles {
    std::vector<session_cpp::Polyline> column = wood_session::profile_rectangle(300.0, 300.0);
    std::vector<session_cpp::Polyline> girder = wood_session::profile_rectangle(200.0, 600.0);
    std::vector<session_cpp::Polyline> beam; // Members on free lines and under span -1; empty takes girder.
    std::vector<session_cpp::Polyline> purlin; // Purlin rows and stations; empty takes beam.
    std::vector<session_cpp::Polyline> edge_girder; // Perimeter members on girder-family lines; empty takes girder.
    std::vector<session_cpp::Polyline> edge_beam; // Perimeter members on any other line or ring edge; empty takes purlin under system 2, else beam.
    std::vector<session_cpp::Polyline> brace; // Workflow C braces; empty takes beam.
};

/// How every level is framed and jointed: the structural method, the joint choices and the sizes; per-bay and per-member changes are attributes on the level plans.
struct Framing {
    int system = 1; // 0 point supported (deck on columns or heads, no members), 1 post and beam (girders on the span family, the deck spans between them), 2 purlin on girder (girders plus purlin rows at spacing).
    int span = 0; // Pattern family the girders run on; -1 every line carries a beam (two-way, hexagonal, irregular).
    double spacing = 3000.0; // Largest purlin spacing under system 2; ceil(cell / spacing) intervals per unclipped cell, one row on every interior cross line.
    int edge = 1; // Perimeter members on the section rings and hole rings: 1 built, 0 none.
    int node = 0; // Column joint: 0 head (capital under the members), 1 flush (column top at the datum, members into its faces, deck over all), 2 through (column datum to datum, deck notched, members into its faces).
    double drop = 0.0; // Girder top below the datum: 0 flush with the purlins, 203.2 hung as Branch, the purlin depth stacked.
    double deck = 200.0; // Deck thickness above the datum.
    double wall = 200.0; // Facade and core wall thickness, centred on the line.
    double head = 300.0; // Head height under node 0.
    double reach = 400.0; // Head top half-width, and how far an open member end runs past its node when nothing butts into it.
    double panel = 0.0; // Largest deck strip width across the deck span; 0 one deck per bay.
    double merge = 1000.0; // Column points closer than this weld to the earlier one in a plan: ring vertices, then ring crossings, then interior crossings.
    double taper = 30.0; // Largest lean in degrees of a perimeter column following a moving section; beyond it the vertex is a transfer.
    double angle = 10.0; // Tilt tolerance in degrees: horizontal within angle, vertical within 90 - angle.
    double tolerance = 1.0; // Weld distance, coplanarity and clash tolerance.
    bool facade = false; // A wall under every perimeter member.
    Profiles profiles; // Sections per role.
};

// ═══════════════════════════════════════════════════════════════════════════
// Building
// ═══════════════════════════════════════════════════════════════════════════

/// One level: its datum, the section rings at it, the open holes, the cores, and the plan the pattern fills into the section; faces are bays, edges member lines, vertices column points, every meaning a double attribute.
struct Level {
    double z = 0.0; // Datum: the framing top, the deck underside.
    std::vector<session_cpp::Polyline> rings; // Section at z, the union of the slices just below and just above: outer rings counter-clockwise seen from above, holes clockwise, all at z 0.
    std::vector<session_cpp::Polyline> holes; // Open holes (atria, courtyards drawn as holes): no deck, no walls, edge members round them when they are in the arrangement.
    std::vector<session_cpp::Polyline> cores; // Core rings on the wall centre line, counter-clockwise: a void face, a wall per side, a deck hole, a support for the members that reach them.
    session_cpp::Mesh plan; // Arrangement of the pattern and the rings inside the section; a hole or core ring that meets no line is a face hole of its bay.
};

/// A building as its levels: the same pipeline from a massing, a footprint or drawn lines; elements per storey from a Framing.
struct Building {
    std::vector<Level> levels; // Ascending; storey k spans levels[k] to levels[k + 1]; levels[0] is the ground and carries the column feet.
    std::vector<session_cpp::Line> braces; // Tilted lines from from_lines, built as beams cut by what they meet.
    Pattern pattern; // The lines the plans were drawn on: the unclipped cells purlin stations are spaced over.
    double tolerance = 1.0; // Weld distance the plans were built with.

    /// A. A closed massing sliced at elevations: sections just below and just above each, their union filled with pattern, cores as rings through every level; columns follow the sections within taper.
    static Building from_solid(const session_cpp::Mesh& massing, const std::vector<double>& elevations, const Pattern& pattern, const std::vector<session_cpp::Polyline>& cores = {}, double tolerance = 1.0, double merge = 1000.0);

    /// A. The same for a BRep through its tessellation, facet degrees per curved face.
    static Building from_solid(const session_cpp::BRep& massing, const std::vector<double>& elevations, const Pattern& pattern, const std::vector<session_cpp::Polyline>& cores = {}, double tolerance = 1.0, double merge = 1000.0, double facet = 15.0);

    /// B. Footprint rings (outer counter-clockwise, holes clockwise; empty means every bounded cell of the pattern) over storeys of heights, the same section on every level, cores on every level.
    static Building from_footprint(const std::vector<session_cpp::Polyline>& footprint, const std::vector<double>& heights, const Pattern& pattern, const std::vector<session_cpp::Polyline>& cores = {}, double tolerance = 1.0, double merge = 1000.0);

    /// C. Members and surfaces as drawn: horizontal lines and floors make the plan of their level, vertical lines its column points, vertical surfaces its walls (2 when named core, 1 otherwise), tilted lines braces; duplicates in either direction merged, lines split at every node and crossing.
    static Building from_lines(const std::vector<session_cpp::Line>& lines, const std::vector<session_cpp::Polyline>& surfaces, double tolerance = 1.0, double angle = 10.0);

    /// Every element of storey k with its joints resolved, world space, in plan order so instance_by_key() dedups them: columns and walls standing in the storey, then the heads, members, stations and decks of the level that caps it, then its braces.
    std::vector<std::shared_ptr<session_cpp::Element>> to_elements(const Framing& framing, size_t storey) const;

    /// Every storey's elements added to session under a group per storey named storey_k.
    void to_session(wood_session::WoodSession& session, const Framing& framing) const;
};

} // namespace wood_grid

#include "src/templates/grid_joints.h"

namespace wood_grid {

// ═══════════════════════════════════════════════════════════════════════════
// Patterns
// ═══════════════════════════════════════════════════════════════════════════

/// Running sums from 0 over steps.
inline std::vector<double> compute_sums(const std::vector<double>& steps) {

    std::vector<double> sums = {0.0};
    for (const double step : steps)
        sums.push_back(sums.back() + step);

    return sums;
}

inline Pattern Pattern::orthogonal(const std::vector<double>& xs, const std::vector<double>& ys, double skew) {

    const std::vector<double> x = compute_sums(xs);
    const std::vector<double> y = compute_sums(ys);
    const double lean = skew * session_cpp::Tolerance::TO_RADIANS;
    const auto point = [&](double u, double v) { return session_cpp::Point(u + v * std::sin(lean), v * std::cos(lean), 0.0); };

    Pattern pattern;
    for (const double v : y) {
        pattern.lines.push_back(session_cpp::Line::from_points(point(x.front(), v), point(x.back(), v)));
        pattern.families.push_back(0);
    }
    for (const double u : x) {
        pattern.lines.push_back(session_cpp::Line::from_points(point(u, y.front()), point(u, y.back())));
        pattern.families.push_back(1);
    }

    return pattern;
}

inline Pattern Pattern::radial(const std::vector<double>& radii, int sectors, double sweep) {

    const auto point = [&](double radius, int j) {
        const double angle = sweep * j / sectors * session_cpp::Tolerance::TO_RADIANS;
        return session_cpp::Point(radius * std::cos(angle), radius * std::sin(angle), 0.0);
    };
    const int chords = sweep >= 360.0 - 1e-9 ? sectors : sectors + 1;

    Pattern pattern;
    for (int j = 0; j < chords; j++) {
        pattern.lines.push_back(session_cpp::Line::from_points(point(radii.front(), j), point(radii.back(), j)));
        pattern.families.push_back(0);
    }
    for (const double radius : radii)
        for (int j = 0; j < sectors && radius > 0.0; j++) {
            pattern.lines.push_back(session_cpp::Line::from_points(point(radius, j), point(radius, j + 1)));
            pattern.families.push_back(1);
        }

    return pattern;
}

inline Pattern Pattern::triangular(double side, int nx, int ny) {

    const auto point = [&](double i, double j) { return session_cpp::Point(side * (i + 0.5 * j), side * std::sqrt(3.0) / 2.0 * j, 0.0); };

    Pattern pattern;
    for (int j = 0; j <= ny; j++) {
        pattern.lines.push_back(session_cpp::Line::from_points(point(0, j), point(nx, j)));
        pattern.families.push_back(0);
    }
    for (int i = 0; i <= nx; i++) {
        pattern.lines.push_back(session_cpp::Line::from_points(point(i, 0), point(i, ny)));
        pattern.families.push_back(1);
    }
    for (int k = 1; k < nx + ny; k++) {
        const int i0 = std::min(k, nx);
        const int i1 = std::max(0, k - ny);
        pattern.lines.push_back(session_cpp::Line::from_points(point(i0, k - i0), point(i1, k - i1)));
        pattern.families.push_back(2);
    }

    return pattern;
}

inline Pattern Pattern::hexagonal(double side, int nx, int ny) {

    std::vector<session_cpp::Line> lines;
    for (int j = 0; j < ny; j++)
        for (int i = 0; i < nx; i++) {
            const session_cpp::Point centre(std::sqrt(3.0) * side * (i + 0.5 * (j % 2)), 1.5 * side * j, 0.0);
            std::vector<session_cpp::Point> corners;
            for (int k = 0; k < 6; k++)
                corners.push_back(centre + session_cpp::Vector(std::cos((30.0 + 60.0 * k) * session_cpp::Tolerance::TO_RADIANS), std::sin((30.0 + 60.0 * k) * session_cpp::Tolerance::TO_RADIANS), 0.0) * side);
            for (int k = 0; k < 6; k++)
                lines.push_back(session_cpp::Line::from_points(corners[k], corners[(k + 1) % 6]));
        }

    return from_lines(lines);
}

inline Pattern Pattern::from_lines(const std::vector<session_cpp::Line>& lines, const std::vector<int>& families) {

    Pattern pattern;
    pattern.lines = lines;
    pattern.families = families.empty() ? std::vector<int>(lines.size(), -1) : families;

    return pattern;
}

inline Pattern Pattern::transformed(const session_cpp::Xform& xform) const {

    Pattern moved = *this;
    for (session_cpp::Line& line : moved.lines)
        line = line.transformed(xform);

    return moved;
}

// ═══════════════════════════════════════════════════════════════════════════
// Plans
// ═══════════════════════════════════════════════════════════════════════════

/// True when a ring touches or crosses any of the lines.
inline bool compute_touching(const session_cpp::Polyline& ring, const std::vector<session_cpp::Line>& lines, double tolerance) {

    const std::vector<session_cpp::Point> corners = to_loop(ring);
    for (size_t i = 0; i < corners.size(); i++) {
        const session_cpp::Line edge = session_cpp::Line::from_points(corners[i], corners[(i + 1) % corners.size()]);
        for (const session_cpp::Line& line : lines) {
            double t = 0.0;
            double s = 0.0;
            if (session_cpp::Intersection::line_line_parameters(edge, line, t, s, tolerance, true, false) && edge.point_at(t).distance(line.point_at(s)) <= tolerance)
                return true;
        }
    }

    return false;
}

/// Pattern line i clipped to the rings: the pieces whose midpoints lie inside, the whole line when there are no rings.
inline std::vector<session_cpp::Line> compute_clipped(const session_cpp::Line& line, const std::vector<session_cpp::Polyline>& rings, double tolerance) {

    if (rings.empty())
        return {line};

    std::vector<double> params = {0.0, 1.0};
    for (const session_cpp::Polyline& ring : rings) {
        const std::vector<session_cpp::Point> corners = to_loop(ring);
        for (size_t i = 0; i < corners.size(); i++) {
            double t = 0.0;
            double s = 0.0;
            const session_cpp::Line edge = session_cpp::Line::from_points(corners[i], corners[(i + 1) % corners.size()]);
            if (session_cpp::Intersection::line_line_parameters(line, edge, t, s, tolerance, true, false) && line.point_at(t).distance(edge.point_at(s)) <= tolerance)
                params.push_back(t);
        }
    }
    std::sort(params.begin(), params.end());

    std::vector<session_cpp::Line> pieces;
    for (size_t k = 0; k + 1 < params.size(); k++)
        if ((params[k + 1] - params[k]) * line.length() > tolerance && compute_inside(rings, line.point_at((params[k] + params[k + 1]) / 2.0)))
            pieces.push_back(session_cpp::Line::from_points(line.point_at(params[k]), line.point_at(params[k + 1])));

    return pieces;
}

/// The pattern family of a plan edge: that of the pattern line its midpoint lies on, -1 when it lies on none.
inline int compute_family(const session_cpp::Line& edge, const Pattern& pattern, double tolerance) {

    const session_cpp::Point middle = edge.point_at(0.5);
    for (size_t i = 0; i < pattern.lines.size(); i++) {
        const std::pair<double, session_cpp::Point> closest = pattern.lines[i].closest_point(compute_lift(middle, 0.0));
        if (closest.second.distance(compute_lift(middle, 0.0)) <= tolerance && std::abs(pattern.lines[i].to_direction().dot(edge.to_direction())) > 0.999)
            return pattern.families[i];
    }

    return -1;
}

/// The plan of one level: the pattern clipped to the section rings, the rings, the cores and extra rings arranged into faces; faces outside removed, core faces flagged, rings that touch nothing held back as face holes, then family, boundary, wall, column and floor attributes.
inline session_cpp::Mesh compute_plan(const Pattern& pattern, const std::vector<session_cpp::Polyline>& rings, const std::vector<session_cpp::Polyline>& cores, const std::vector<session_cpp::Polyline>& extras, double tolerance, double merge) {

    std::vector<session_cpp::Line> lines;
    std::vector<double> ids;
    for (size_t i = 0; i < pattern.lines.size(); i++)
        for (const session_cpp::Line& piece : compute_clipped(pattern.lines[i], rings, tolerance)) {
            lines.push_back(piece);
            ids.push_back(static_cast<double>(i));
        }

    std::vector<session_cpp::Polyline> held;
    std::vector<int> kinds;
    std::vector<session_cpp::Polyline> all = rings;
    all.insert(all.end(), cores.begin(), cores.end());
    all.insert(all.end(), extras.begin(), extras.end());
    for (size_t r = 0; r < all.size(); r++) {
        const int kind = r < rings.size() ? (compute_area(to_loop(all[r])) > 0.0 ? 0 : 1) : r < rings.size() + cores.size() ? 2 : 3;
        if (kind != 0 && !compute_touching(all[r], lines, tolerance)) {
            if (kind != 3) {
                held.push_back(all[r]);
                kinds.push_back(kind);
            }
            continue;
        }

        const std::vector<session_cpp::Point> corners = to_loop(all[r]);
        for (size_t e = 0; e < corners.size(); e++) {
            lines.push_back(session_cpp::Line::from_points(corners[e], corners[(e + 1) % corners.size()]));
            ids.push_back(compute_ring_id(r, e));
        }
    }

    const std::pair<std::vector<session_cpp::Line>, std::vector<double>> split = compute_crossings(lines, ids, tolerance, merge);
    session_cpp::Mesh plan = compute_arrangement(split.first, split.second, tolerance);
    const auto ring_of = [&](double id) { return static_cast<size_t>((id - 1000000.0) / 1000.0); };
    const auto is_core = [&](double id) { return is_ring(id) && ring_of(id) >= rings.size() && ring_of(id) < rings.size() + cores.size(); };

    for (const size_t face : plan.faces()) {
        const session_cpp::Point centre = *plan.face_centroid(face);
        if (!rings.empty() && !compute_inside(rings, centre)) {
            plan.remove_face(face);
            continue;
        }

        const bool core = std::any_of(cores.begin(), cores.end(), [&](const session_cpp::Polyline& ring) { return ring.point_in_polygon_2d(centre); });
        plan.set_face_attribute(face, "floor", core ? 0.0 : 1.0);
        plan.set_face_attribute(face, "core", core ? 1.0 : 0.0);
    }

    for (size_t h = 0; h < held.size(); h++)
        for (const size_t face : plan.faces()) {
            const std::vector<session_cpp::Point> corners = to_loop(held[h]);
            if (!plan.face_polygon(face)->point_in_polygon_2d(corners[0]))
                continue;

            std::vector<size_t> ring;
            for (const session_cpp::Point& corner : corners) {
                ring.push_back(plan.add_vertex(compute_lift(corner, 0.0)));
                plan.set_vertex_attribute(ring.back(), "wall", kinds[h] == 2 ? 2.0 : 0.0);
                plan.set_vertex_attribute(ring.back(), "column", 0.0);
            }
            std::vector<std::vector<size_t>> holes = plan.get_face_holes().count(face) ? plan.get_face_holes().at(face) : std::vector<std::vector<size_t>>();
            holes.push_back(ring);
            plan.set_face_holes(face, holes);
            break;
        }

    for (const std::pair<size_t, size_t>& edge : plan.edges()) {
        const double id = plan.edge_attribute(edge, "line").value_or(-1.0);
        const session_cpp::Line line = session_cpp::Line::from_points(*plan.vertex_point(edge.first), *plan.vertex_point(edge.second));
        const bool boundary = plan.edge_faces(edge.first, edge.second).value_or(std::vector<size_t>()).size() < 2;
        plan.set_edge_attribute(edge, "family", is_ring(id) || id < 0.0 ? compute_family(line, pattern, tolerance) : pattern.families[static_cast<size_t>(id)]);
        plan.set_edge_attribute(edge, "boundary", boundary ? 1.0 : 0.0);
        plan.set_edge_attribute(edge, "wall", is_core(id) ? 2.0 : 0.0);
        if (boundary) {
            plan.set_vertex_attribute(edge.first, "boundary", 1.0);
            plan.set_vertex_attribute(edge.second, "boundary", 1.0);
        }
    }

    for (const size_t vertex : plan.vertices()) {
        if (!plan.vertex_neighbors(vertex))
            continue;

        const session_cpp::Point point = *plan.vertex_point(vertex);
        const std::vector<size_t> around = *plan.vertex_neighbors(vertex);
        const bool on_core = std::any_of(around.begin(), around.end(), [&](size_t other) { return plan.edge_attribute({vertex, other}, "wall").value_or(0.0) == 2.0; });
        const bool in_core = std::any_of(cores.begin(), cores.end(), [&](const session_cpp::Polyline& ring) { return ring.point_in_polygon_2d(point); });
        plan.set_vertex_attribute(vertex, "column", on_core || in_core ? 0.0 : 1.0);
    }

    return plan;
}

// ═══════════════════════════════════════════════════════════════════════════
// Levels
// ═══════════════════════════════════════════════════════════════════════════

/// The clockwise rings: the holes.
inline std::vector<session_cpp::Polyline> compute_holes(const std::vector<session_cpp::Polyline>& rings) {

    std::vector<session_cpp::Polyline> holes;
    for (const session_cpp::Polyline& ring : rings)
        if (compute_area(to_loop(ring)) < 0.0)
            holes.push_back(ring);

    return holes;
}

/// The cores whose first corner lies inside the rings, every ring counter-clockwise.
inline std::vector<session_cpp::Polyline> compute_cores(const std::vector<session_cpp::Polyline>& cores, const std::vector<session_cpp::Polyline>& rings) {

    std::vector<session_cpp::Polyline> inside;
    for (const session_cpp::Polyline& core : cores) {
        std::vector<session_cpp::Point> corners = to_loop(core);
        if (compute_area(corners) < 0.0)
            std::reverse(corners.begin(), corners.end());
        if (rings.empty() || compute_inside(rings, corners[0]))
            inside.push_back(compute_lift(to_polyline(corners), 0.0));
    }

    return inside;
}

/// A level from its rings and cores, its plan computed; the ground has no floor unless asked.
inline Level compute_level(double z, const std::vector<session_cpp::Polyline>& rings, const std::vector<session_cpp::Polyline>& cores, const std::vector<session_cpp::Polyline>& extras, const Pattern& pattern, double tolerance, double merge, bool floor) {

    Level level;
    level.z = z;
    level.rings = rings;
    level.holes = compute_holes(rings);
    level.cores = compute_cores(cores, rings);
    level.plan = compute_plan(pattern, rings, level.cores, extras, tolerance, merge);
    if (!floor)
        for (const size_t face : level.plan.faces())
            level.plan.set_face_attribute(face, "floor", 0.0);

    return level;
}

/// True when two rings have the same corners within tolerance in some rotation and direction.
inline bool compute_same(const session_cpp::Polyline& a, const session_cpp::Polyline& b, double tolerance) {

    const std::vector<session_cpp::Point> first = to_loop(a);
    const std::vector<session_cpp::Point> second = to_loop(b);
    if (first.size() != second.size())
        return false;

    for (const session_cpp::Point& point : first)
        if (std::none_of(second.begin(), second.end(), [&](const session_cpp::Point& other) { return compute_distance(point, other) <= tolerance; }))
            return false;

    return true;
}

inline Building Building::from_footprint(const std::vector<session_cpp::Polyline>& footprint, const std::vector<double>& heights, const Pattern& pattern, const std::vector<session_cpp::Polyline>& cores, double tolerance, double merge) {

    std::vector<session_cpp::Polyline> rings;
    for (const session_cpp::Polyline& ring : footprint)
        rings.push_back(compute_lift(ring, 0.0));

    Building building;
    building.pattern = pattern;
    building.tolerance = tolerance;
    const std::vector<double> elevations = compute_sums(heights);
    building.levels.push_back(compute_level(elevations[0], rings, cores, {}, pattern, tolerance, merge, false));
    for (size_t k = 1; k < elevations.size(); k++) {
        building.levels.push_back(building.levels[0]);
        building.levels.back().z = elevations[k];
        for (const size_t face : building.levels.back().plan.faces())
            building.levels.back().plan.set_face_attribute(face, "floor", building.levels.back().plan.face_attribute(face, "core").value_or(0.0) == 1.0 ? 0.0 : 1.0);
    }

    return building;
}

inline Building Building::from_solid(const session_cpp::Mesh& massing, const std::vector<double>& elevations, const Pattern& pattern, const std::vector<session_cpp::Polyline>& cores, double tolerance, double merge) {

    session_cpp::Mesh solid = massing;
    solid.orient_outward();
    const size_t count = elevations.size();
    std::vector<std::vector<session_cpp::Polyline>> below(count);
    std::vector<std::vector<session_cpp::Polyline>> above(count);
    for (size_t k = 0; k < count; k++) {
        below[k] = k > 0 ? compute_section(solid, elevations[k] - tolerance, tolerance) : std::vector<session_cpp::Polyline>();
        above[k] = k + 1 < count ? compute_section(solid, elevations[k] + tolerance, tolerance) : std::vector<session_cpp::Polyline>();
    }

    Building building;
    building.pattern = pattern;
    building.tolerance = tolerance;
    for (size_t k = 0; k < count; k++) {
        std::vector<session_cpp::Polyline> rings = below[k].empty() ? above[k] : above[k].empty() ? below[k] : compute_regions(below[k], above[k], 1);
        for (session_cpp::Polyline& ring : rings)
            ring.merge_collinear(1e-6);

        std::vector<session_cpp::Polyline> extras;
        for (size_t storey = k > 0 ? k - 1 : k; storey <= k && storey + 1 < count; storey++)
            for (const session_cpp::Polyline& ring : compute_regions(above[storey], below[storey + 1], 0))
                if (compute_area(to_loop(ring)) > 0.0 && std::none_of(rings.begin(), rings.end(), [&](const session_cpp::Polyline& other) { return compute_same(ring, other, tolerance); }))
                    extras.push_back(ring);

        building.levels.push_back(compute_level(elevations[k], rings, cores, extras, pattern, tolerance, merge, k > 0));
    }

    return building;
}

inline Building Building::from_solid(const session_cpp::BRep& massing, const std::vector<double>& elevations, const Pattern& pattern, const std::vector<session_cpp::Polyline>& cores, double tolerance, double merge, double facet) {

    std::vector<std::vector<session_cpp::Point>> polygons;
    for (const session_cpp::Mesh& part : massing.face_meshes_q(true, facet, 0.1))
        for (const session_cpp::Polyline& polygon : part.face_outlines())
            polygons.push_back(to_loop(polygon));

    return from_solid(session_cpp::Mesh::from_polylines(polygons, tolerance * 0.01), elevations, pattern, cores, tolerance, merge);
}

/// Degrees between a segment and the horizontal plane.
inline double compute_tilt(const session_cpp::Point& a, const session_cpp::Point& b) {
    return std::atan2(std::abs(b[2] - a[2]), compute_distance(a, b)) * session_cpp::Tolerance::TO_DEGREES;
}

inline Building Building::from_lines(const std::vector<session_cpp::Line>& lines, const std::vector<session_cpp::Polyline>& surfaces, double tolerance, double angle) {

    const auto horizontal = [&](const session_cpp::Polyline& surface) {
        const session_cpp::Vector normal = session_cpp::Vector::average_normal(surface).normalized();
        return std::acos(std::min(1.0, std::abs(normal[2]))) * session_cpp::Tolerance::TO_DEGREES <= angle;
    };
    const auto vertical = [&](const session_cpp::Polyline& surface) {
        const session_cpp::Vector normal = session_cpp::Vector::average_normal(surface).normalized();
        return std::acos(std::min(1.0, std::abs(normal[2]))) * session_cpp::Tolerance::TO_DEGREES >= 90.0 - angle;
    };
    const auto lowest = [](const session_cpp::Polyline& surface) {
        double z = std::numeric_limits<double>::max();
        for (const session_cpp::Point& point : surface.get_points())
            z = std::min(z, point[2]);
        return z;
    };
    const auto highest = [](const session_cpp::Polyline& surface) {
        double z = -std::numeric_limits<double>::max();
        for (const session_cpp::Point& point : surface.get_points())
            z = std::max(z, point[2]);
        return z;
    };

    std::vector<double> heights;
    for (const session_cpp::Line& line : lines) {
        heights.push_back(line.start()[2]);
        heights.push_back(line.end()[2]);
    }
    for (const session_cpp::Polyline& surface : surfaces) {
        heights.push_back(lowest(surface));
        heights.push_back(highest(surface));
    }
    std::sort(heights.begin(), heights.end());

    std::vector<double> elevations;
    for (const double z : heights)
        if (elevations.empty() || z - elevations.back() > tolerance)
            elevations.push_back(z);
    const auto level_of = [&](double z) { return static_cast<size_t>(std::lower_bound(elevations.begin(), elevations.end(), z - tolerance) - elevations.begin()); };

    Building building;
    building.tolerance = tolerance;
    for (size_t k = 0; k < elevations.size(); k++) {
        std::vector<session_cpp::Line> drawn;
        std::vector<double> ids;
        std::vector<session_cpp::Polyline> floors;
        std::vector<std::pair<session_cpp::Line, double>> walls;
        for (const session_cpp::Line& line : lines)
            if (compute_tilt(line.start(), line.end()) <= angle && level_of(line.start()[2]) == k) {
                drawn.push_back(session_cpp::Line::from_points(compute_lift(line.start(), 0.0), compute_lift(line.end(), 0.0)));
                ids.push_back(static_cast<double>(drawn.size() - 1));
            }

        size_t ring = 0;
        for (const session_cpp::Polyline& surface : surfaces) {
            if (horizontal(surface) && level_of(lowest(surface)) == k) {
                std::vector<session_cpp::Point> corners = to_loop(compute_lift(surface, 0.0));
                if (compute_area(corners) < 0.0)
                    std::reverse(corners.begin(), corners.end());
                floors.push_back(to_polyline(corners));
                for (size_t e = 0; e < corners.size(); e++) {
                    drawn.push_back(session_cpp::Line::from_points(corners[e], corners[(e + 1) % corners.size()]));
                    ids.push_back(compute_ring_id(ring, e));
                }
                ring++;
            }
            if (vertical(surface) && level_of(highest(surface)) == k) {
                std::vector<session_cpp::Point> top;
                for (const session_cpp::Point& point : to_loop(surface))
                    if (std::abs(point[2] - highest(surface)) <= tolerance)
                        top.push_back(compute_lift(point, 0.0));
                if (top.size() < 2)
                    continue;
                walls.emplace_back(session_cpp::Line::from_points(top.front(), top.back()), surface.name.find("core") != std::string::npos ? 2.0 : 1.0);
                drawn.push_back(walls.back().first);
                ids.push_back(compute_ring_id(ring++, 0));
            }
        }

        Level level;
        level.z = elevations[k];
        level.rings = floors;
        const std::pair<std::vector<session_cpp::Line>, std::vector<double>> split = compute_crossings(drawn, ids, tolerance, tolerance);
        level.plan = compute_arrangement(split.first, split.second, tolerance);
        for (const size_t face : level.plan.faces())
            level.plan.set_face_attribute(face, "floor", compute_inside(floors, *level.plan.face_centroid(face)) ? 1.0 : 0.0);

        for (const std::pair<size_t, size_t>& edge : level.plan.edges()) {
            const double id = level.plan.edge_attribute(edge, "line").value_or(-1.0);
            const session_cpp::Line line = session_cpp::Line::from_points(*level.plan.vertex_point(edge.first), *level.plan.vertex_point(edge.second));
            const std::vector<size_t> faces = level.plan.edge_faces(edge.first, edge.second).value_or(std::vector<size_t>());
            const bool boundary = std::count_if(faces.begin(), faces.end(), [&](size_t face) { return level.plan.face_attribute(face, "floor").value_or(0.0) == 1.0; }) < 2;
            level.plan.set_edge_attribute(edge, "family", -1.0);
            level.plan.set_edge_attribute(edge, "boundary", boundary ? 1.0 : 0.0);
            level.plan.set_edge_attribute(edge, "wall", 0.0);
            if (is_ring(id))
                level.plan.set_edge_attribute(edge, "role", 0.0);
            for (const std::pair<session_cpp::Line, double>& wall : walls)
                if (wall.first.closest_point(line.point_at(0.5)).second.distance(line.point_at(0.5)) <= tolerance)
                    level.plan.set_edge_attribute(edge, "wall", wall.second);
        }

        for (const size_t vertex : level.plan.vertices())
            level.plan.set_vertex_attribute(vertex, "column", 0.0);
        for (const session_cpp::Line& line : lines) {
            if (compute_tilt(line.start(), line.end()) < 90.0 - angle)
                continue;
            for (const session_cpp::Point& end : {line.start(), line.end()}) {
                if (level_of(end[2]) != k)
                    continue;
                std::optional<size_t> found;
                for (const size_t vertex : level.plan.vertices())
                    if (compute_distance(*level.plan.vertex_point(vertex), end) <= tolerance)
                        found = vertex;
                level.plan.set_vertex_attribute(found ? *found : level.plan.add_vertex(compute_lift(end, 0.0)), "column", 1.0);
            }
        }

        building.levels.push_back(level);
    }

    for (const session_cpp::Line& line : lines)
        if (compute_tilt(line.start(), line.end()) > angle && compute_tilt(line.start(), line.end()) < 90.0 - angle)
            building.braces.push_back(line);

    return building;
}

// ═══════════════════════════════════════════════════════════════════════════
// Roles and columns
// ═══════════════════════════════════════════════════════════════════════════

/// The role of every edge without one from the framing and the pattern: perimeter edges edge girders on span-family lines and edge beams elsewhere, girders on the span family, purlins on the cross lines of system 2, beams on free lines and under span -1, nothing inside cores; wall 1 on the perimeter when the framing asks for a facade.
inline void compute_roles(session_cpp::Mesh& plan, const Framing& framing) {

    for (const std::pair<size_t, size_t>& edge : plan.edges()) {
        const std::vector<size_t> faces = plan.edge_faces(edge.first, edge.second).value_or(std::vector<size_t>());
        const bool boundary = plan.edge_attribute(edge, "boundary").value_or(0.0) == 1.0;
        const bool core = plan.edge_attribute(edge, "wall").value_or(0.0) == 2.0 || std::all_of(faces.begin(), faces.end(), [&](size_t face) { return plan.face_attribute(face, "core").value_or(0.0) == 1.0; });
        if (boundary && framing.facade && plan.edge_attribute(edge, "wall").value_or(0.0) == 0.0)
            plan.set_edge_attribute(edge, "wall", 1.0);
        if (plan.edge_attribute(edge, "role"))
            continue;

        const int family = static_cast<int>(plan.edge_attribute(edge, "family").value_or(-1.0));
        int system = framing.system;
        int span = framing.span;
        for (const size_t face : faces) {
            system = static_cast<int>(plan.face_attribute(face, "system").value_or(system));
            span = static_cast<int>(plan.face_attribute(face, "span").value_or(span));
        }

        int role = 0;
        if (core)
            role = 0;
        else if (boundary)
            role = framing.edge == 0 ? 0 : span >= 0 && family == span ? 4 : 5;
        else if (system == 0)
            role = 0;
        else if (span < 0 || family < 0)
            role = 2;
        else if (family == span)
            role = 1;
        else if (system == 2)
            role = 3;
        plan.set_edge_attribute(edge, "role", role);
    }
}

/// A column of a storey: its foot on the lower level and its head on the upper one.
struct Stack {
    size_t lower = 0; // Vertex in the lower plan.
    size_t upper = 0; // Vertex in the upper plan.
    session_cpp::Point foot; // Plan point of the foot at z 0.
    session_cpp::Point head; // Plan point of the head at z 0.
};

/// Columns between levels k and k + 1: every upper vertex with column 1 matched to a lower one by line identity, then by position, then by the nearest vertex on the same pattern line or the section within taper; an upper vertex with no match is written column 2.
inline std::vector<Stack> compute_columns(Building& building, size_t k, const Framing& framing) {

    session_cpp::Mesh& lower = building.levels[k].plan;
    session_cpp::Mesh& upper = building.levels[k + 1].plan;
    const double rise = building.levels[k + 1].z - building.levels[k].z;
    const double lean = rise * std::tan(framing.taper * session_cpp::Tolerance::TO_RADIANS);
    const auto identity = [&](const session_cpp::Mesh& plan, size_t vertex) {
        return std::make_pair(plan.vertex_attribute(vertex, "line_a").value_or(-1.0), plan.vertex_attribute(vertex, "line_b").value_or(-1.0));
    };
    const auto usable = [&](size_t vertex) { return lower.vertex_attribute(vertex, "column").value_or(0.0) == 1.0; };

    std::vector<Stack> stacks;
    for (const size_t vertex : upper.vertices()) {
        if (upper.vertex_attribute(vertex, "column").value_or(0.0) != 1.0)
            continue;

        const session_cpp::Point head = compute_lift(*upper.vertex_point(vertex), 0.0);
        const std::pair<double, double> key = identity(upper, vertex);
        std::optional<size_t> match;
        double best = std::numeric_limits<double>::max();
        for (const size_t other : lower.vertices()) {
            if (!usable(other))
                continue;

            const double distance = compute_distance(*lower.vertex_point(other), head);
            const bool same_line = key.first >= 0.0 && (identity(lower, other).first == key.first || identity(lower, other).second == key.second || identity(lower, other).first == key.second || identity(lower, other).second == key.first);
            const bool section = upper.vertex_attribute(vertex, "boundary").value_or(0.0) == 1.0 && lower.vertex_attribute(other, "boundary").value_or(0.0) == 1.0;
            const double score = identity(lower, other) == key && distance <= lean ? distance : distance <= building.tolerance ? distance + 1.0 : (same_line || section) && distance <= lean ? distance + lean + 2.0 : std::numeric_limits<double>::max();
            if (score < best) {
                best = score;
                match = other;
            }
        }

        if (!match) {
            upper.set_vertex_attribute(vertex, "column", 2.0);
            continue;
        }

        stacks.push_back({*match, vertex, compute_lift(*lower.vertex_point(*match), 0.0), head});
    }

    return stacks;
}

/// The plan section of the column at a vertex, counter-clockwise: the direction polygon at the profile's support per direction, x along the through member, for a rectangle; the profile turned to the through member otherwise.
inline std::vector<session_cpp::Point> compute_column_polygon(const session_cpp::Mesh& plan, size_t vertex, const Framing& framing) {

    const std::vector<Member> members = compute_members(plan, vertex, framing);
    const std::pair<std::optional<size_t>, std::optional<size_t>> through = compute_through(members, plan.vertex_attribute(vertex, "through"));
    const double turn = through.first ? std::atan2(members[*through.first].direction[1], members[*through.first].direction[0]) : 0.0;
    const session_cpp::Point centre = compute_lift(*plan.vertex_point(vertex), 0.0);
    const std::vector<session_cpp::Polyline>& profile = framing.profiles.column;

    if (to_loop(profile[0]).size() != 4) {
        std::vector<session_cpp::Point> points;
        for (const session_cpp::Point& point : to_loop(profile[0]))
            points.push_back(centre + session_cpp::Vector(point[0] * std::cos(turn) - point[1] * std::sin(turn), point[0] * std::sin(turn) + point[1] * std::cos(turn), 0.0));
        return points;
    }

    const std::vector<session_cpp::Vector> directions = compute_directions(plan, vertex);
    std::vector<double> distances;
    for (const session_cpp::Vector& direction : directions) {
        const double angle = std::atan2(direction[1], direction[0]) - turn;
        distances.push_back(wood_session::compute_support(profile, session_cpp::Vector(std::cos(angle), std::sin(angle), 0.0)));
    }

    return to_loop(compute_polygon(directions, centre, distances));
}

// ═══════════════════════════════════════════════════════════════════════════
// Stations
// ═══════════════════════════════════════════════════════════════════════════

/// Mean line direction of the edges of a loop in a family, none when the loop has none.
inline std::optional<session_cpp::Vector> compute_family_direction(const session_cpp::Mesh& plan, const std::vector<size_t>& loop, int family) {

    session_cpp::Vector sum(0.0, 0.0, 0.0);
    for (size_t i = 0; i < loop.size(); i++) {
        if (static_cast<int>(plan.edge_attribute({loop[i], loop[(i + 1) % loop.size()]}, "family").value_or(-1.0)) != family)
            continue;

        const session_cpp::Vector direction = compute_direction(*plan.vertex_point(loop[i]), *plan.vertex_point(loop[(i + 1) % loop.size()]));
        const double doubled = 2.0 * std::atan2(direction[1], direction[0]);
        sum += session_cpp::Vector(std::cos(doubled), std::sin(doubled), 0.0);
    }

    if (sum.magnitude() < 1e-9)
        return std::nullopt;

    const double angle = std::atan2(sum[1], sum[0]) / 2.0;

    return session_cpp::Vector(std::cos(angle), std::sin(angle), 0.0);
}

/// What a purlin station ends on: a plan edge, a held-back core ring edge, or nothing.
struct Support {
    int kind = 0; // 0 nothing, 1 a plan edge, 2 a core ring edge.
    std::pair<size_t, size_t> edge; // The plan edge under kind 1.
    session_cpp::Vector along; // Unit direction of the supporting line.
};

/// A purlin station: its line at z 0 and what each end lands on.
struct Station {
    session_cpp::Line line; // From the first support to the second.
    Support first; // Support at the start.
    Support second; // Support at the end.
};

/// Purlin stations of a system 2 face: stations parallel to its cross-family edges (else perpendicular to its girders) at ceil(cell / spacing) intervals over the unclipped cell between the bounding cross lines of the pattern, clipped to the face and its holes, each end remembering what it lands on; pieces shorter than the purlin width dropped.
inline std::vector<Station> compute_stations(const session_cpp::Mesh& plan, size_t face, const Framing& framing, const Pattern& pattern) {

    const std::vector<size_t> loop = compute_loop(plan, face);
    const int span = static_cast<int>(plan.face_attribute(face, "span").value_or(framing.span));
    const double spacing = plan.face_attribute(face, "spacing").value_or(framing.spacing);
    std::optional<session_cpp::Vector> girder = compute_family_direction(plan, loop, span);
    if (!girder)
        return {};

    std::optional<session_cpp::Vector> station;
    for (int family = 0; family < 3 && !station; family++)
        if (family != span)
            station = compute_family_direction(plan, loop, family);
    const session_cpp::Vector along = station.value_or(session_cpp::Vector(0.0, 0.0, 1.0).cross(*girder));
    const session_cpp::Vector across = along.cross(session_cpp::Vector(0.0, 0.0, 1.0));
    const session_cpp::Point origin(0.0, 0.0, 0.0);
    const auto offset = [&](const session_cpp::Point& point) { return (compute_lift(point, 0.0) - origin).dot(across); };

    double low = std::numeric_limits<double>::max();
    double high = -low;
    for (const size_t key : loop) {
        low = std::min(low, offset(*plan.vertex_point(key)));
        high = std::max(high, offset(*plan.vertex_point(key)));
    }

    double cell_low = -std::numeric_limits<double>::max();
    double cell_high = std::numeric_limits<double>::max();
    for (size_t i = 0; i < pattern.lines.size(); i++) {
        if (pattern.families[i] == span || std::abs(pattern.lines[i].to_direction().dot(along)) < 0.999)
            continue;

        const double at = offset(pattern.lines[i].start());
        if (at <= low + framing.tolerance)
            cell_low = std::max(cell_low, at);
        if (at >= high - framing.tolerance)
            cell_high = std::min(cell_high, at);
    }
    cell_low = cell_low == -std::numeric_limits<double>::max() ? low : cell_low;
    cell_high = cell_high == std::numeric_limits<double>::max() ? high : cell_high;

    struct Crossing { double t; Support support; };
    std::vector<std::vector<size_t>> rings = {loop};
    if (plan.get_face_holes().count(face))
        for (const std::vector<size_t>& hole : plan.get_face_holes().at(face))
            rings.push_back(hole);
    const double width = wood_session::compute_size(compute_profile(3, framing)).first;

    std::vector<Station> stations;
    const int intervals = std::max(1, static_cast<int>(std::ceil((cell_high - cell_low) / spacing - 1e-9)));
    for (int k = 1; k < intervals; k++) {
        const double at = cell_low + (cell_high - cell_low) * k / intervals;
        if (at <= low + framing.tolerance || at >= high - framing.tolerance)
            continue;

        const session_cpp::Point base = origin + across * at;
        std::vector<Crossing> crossings;
        for (size_t r = 0; r < rings.size(); r++)
            for (size_t i = 0; i < rings[r].size(); i++) {
                const session_cpp::Point a = compute_lift(*plan.vertex_point(rings[r][i]), 0.0);
                const session_cpp::Point b = compute_lift(*plan.vertex_point(rings[r][(i + 1) % rings[r].size()]), 0.0);
                const double da = (a - origin).dot(across) - at;
                const double db = (b - origin).dot(across) - at;
                if ((da >= 0.0) == (db >= 0.0))
                    continue;

                const session_cpp::Point hit = a + (b - a) * (da / (da - db));
                Support support;
                support.kind = r == 0 ? 1 : plan.vertex_attribute(rings[r][i], "wall").value_or(0.0) == 2.0 ? 2 : 0;
                support.edge = {rings[r][i], rings[r][(i + 1) % rings[r].size()]};
                support.along = compute_direction(a, b);
                crossings.push_back({(hit - base).dot(along), support});
            }

        std::sort(crossings.begin(), crossings.end(), [](const Crossing& a, const Crossing& b) { return a.t < b.t; });
        for (size_t c = 0; c + 1 < crossings.size(); c += 2)
            if (crossings[c + 1].t - crossings[c].t > width)
                stations.push_back({session_cpp::Line::from_points(base + along * crossings[c].t, base + along * crossings[c + 1].t), crossings[c].support, crossings[c + 1].support});
    }

    return stations;
}

/// The cut plane of a station end on its support: the far side of the member on a plan edge when the heights overlap, the outer face of a core wall; none on an open hole or a stacked girder.
inline std::vector<session_cpp::Plane> compute_station_cuts(const session_cpp::Mesh& plan, const Support& support, const session_cpp::Point& at, const session_cpp::Vector& inward, const Member& purlin, const Framing& framing) {

    double half = 0.0;
    if (support.kind == 2 || (support.kind == 1 && plan.edge_attribute(support.edge, "wall").value_or(0.0) == 2.0))
        half = framing.wall / 2.0;
    else if (support.kind == 1 && plan.edge_attribute(support.edge, "role").value_or(0.0) > 0.0) {
        const Member member = compute_member(plan, support.edge.first, support.edge.second, framing);
        half = compute_overlap(purlin, member, framing.tolerance) ? member.width / 2.0 : 0.0;
    }

    std::vector<session_cpp::Plane> planes;
    if (half > 0.0)
        if (const std::optional<session_cpp::Plane> plane = compute_exit(compute_strip(at, support.along, half), at, inward))
            planes.push_back(*plane);

    return planes;
}

// ═══════════════════════════════════════════════════════════════════════════
// Builders
// ═══════════════════════════════════════════════════════════════════════════

/// The kept length of an axis under cut planes, negative when nothing is left.
inline double compute_kept(const session_cpp::Point& start, const session_cpp::Point& end, const std::vector<session_cpp::Plane>& planes) {

    const session_cpp::Vector direction = (end - start).normalized();
    double low = 0.0;
    double high = start.distance(end);
    for (const session_cpp::Plane& plane : planes) {
        const double speed = direction.dot(plane.z_axis());
        const double offset = (plane.origin() - start).dot(plane.z_axis());
        if (std::abs(speed) < 1e-9) {
            if (offset > 0.0)
                return -1.0;
            continue;
        }

        if (speed > 0.0)
            low = std::max(low, offset / speed);
        else
            high = std::min(high, offset / speed);
    }

    return high - low;
}

/// Beams of a profile along an axis at height z with cuts: one, or two side by side for a double profile; none when a stub shorter than its width is left.
inline std::vector<std::shared_ptr<session_cpp::Element>> to_beam(const session_cpp::Point& start, const session_cpp::Point& end, double z, const std::vector<session_cpp::Polyline>& profile, const std::vector<session_cpp::Plane>& cuts, const std::string& name) {

    std::vector<std::shared_ptr<session_cpp::Element>> beams;
    const double width = wood_session::compute_size(profile).first;
    if (compute_kept(start, end, cuts) < width)
        return beams;

    std::vector<std::vector<session_cpp::Polyline>> parts = {profile};
    if (profile.size() > 1 && compute_area(to_loop(profile[1])) > 0.0)
        parts = {{profile[0]}, {profile[1]}};

    const session_cpp::Vector side = session_cpp::Vector(0.0, 0.0, 1.0).cross((end - start).normalized());
    for (const std::vector<session_cpp::Polyline>& part : parts) {
        const std::vector<session_cpp::Point> corners = to_loop(part[0]);
        const double centre = session_cpp::Point::centroid(corners)[0];
        std::vector<session_cpp::Polyline> centred;
        for (const session_cpp::Polyline& ring : part)
            centred.push_back(ring.translated(session_cpp::Vector(-centre, 0.0, 0.0)));

        std::shared_ptr<wood_session::Beam> beam = std::make_shared<wood_session::Beam>(session_cpp::Polyline({compute_lift(start, z) + side * centre, compute_lift(end, z) + side * centre}), centred, std::vector<session_cpp::Vector>{session_cpp::Vector(0.0, 0.0, 1.0)}, name);
        beam->cuts = cuts;
        beams.push_back(beam);
    }

    return beams;
}

/// The member on a plan edge of the level at z: its role's profile, its top at the datum less its drop, its ends from compute_cuts at both vertices.
inline std::vector<std::shared_ptr<session_cpp::Element>> to_beam(const session_cpp::Mesh& plan, std::pair<size_t, size_t> edge, const Framing& framing, double z, const std::map<size_t, std::vector<session_cpp::Point>>& columns) {

    const Member member = compute_member(plan, edge.first, edge.second, framing);
    const End first = compute_cuts(plan, edge.first, edge.second, framing, columns);
    const End second = compute_cuts(plan, edge.second, edge.first, framing, columns);
    const session_cpp::Point a = compute_lift(*plan.vertex_point(edge.first), 0.0);
    const session_cpp::Point b = compute_lift(*plan.vertex_point(edge.second), 0.0);
    std::vector<session_cpp::Plane> cuts = first.planes;
    cuts.insert(cuts.end(), second.planes.begin(), second.planes.end());

    return to_beam(a - member.direction * first.overrun, b + member.direction * second.overrun, z + (member.top + member.bottom) / 2.0, compute_profile(plan, edge, framing), cuts, compute_name(member.role));
}

/// The purlin on a station of a face at z: the purlin profile, top at the datum, each end cut on its support.
inline std::vector<std::shared_ptr<session_cpp::Element>> to_purlin(const session_cpp::Mesh& plan, const Station& station, const Framing& framing, double z) {

    const std::vector<session_cpp::Polyline> profile = compute_profile(3, framing);
    const std::pair<double, double> size = wood_session::compute_size(profile);
    const Member purlin{0, station.line.to_direction(), 3, compute_rank(3), size.first, 0.0, -size.second};
    std::vector<session_cpp::Plane> cuts = compute_station_cuts(plan, station.first, station.line.start(), station.line.to_direction(), purlin, framing);
    for (const session_cpp::Plane& plane : compute_station_cuts(plan, station.second, station.line.end(), -station.line.to_direction(), purlin, framing))
        cuts.push_back(plane);

    return to_beam(station.line.start(), station.line.end(), z - size.second / 2.0, profile, cuts, "purlin");
}

/// The column of a stack: the polygon at its head vertex swept from the foot at z_foot to z_top.
inline std::shared_ptr<session_cpp::Element> to_column(const Stack& stack, const std::vector<session_cpp::Point>& polygon, double z_foot, double z_top) {

    const session_cpp::Vector shift = compute_lift(stack.foot, 0.0) - compute_lift(stack.head, 0.0);
    std::vector<session_cpp::Point> section;
    for (const session_cpp::Point& point : polygon)
        section.push_back(compute_lift(point + shift, z_foot));

    return std::make_shared<wood_session::Column>(session_cpp::Line::from_points(compute_lift(stack.foot, z_foot), compute_lift(stack.head, z_top)), to_polyline(section), "column");
}

/// The head at a vertex under node 0: the direction polygon at the column supports on the head bottom, at reach on the head top.
inline std::shared_ptr<session_cpp::Element> to_head(const session_cpp::Mesh& plan, size_t vertex, const Framing& framing, double z_bottom, double z_top) {

    const session_cpp::Point centre = *plan.vertex_point(vertex);
    const std::vector<session_cpp::Vector> directions = compute_directions(plan, vertex);
    const std::vector<session_cpp::Point> bottom = compute_column_polygon(plan, vertex, framing);
    const session_cpp::Polyline top = compute_polygon(directions, centre, std::vector<double>(directions.size(), framing.reach));

    return std::make_shared<wood_session::Block>(std::vector<session_cpp::Polyline>{compute_lift(to_polyline(bottom), z_bottom), compute_lift(top, z_top)}, "head");
}

/// The deck plates of a face at z: loop 0 of the outline, holes and notches as features, one plate per panel strip.
inline std::vector<std::shared_ptr<session_cpp::Element>> to_deck(const session_cpp::Mesh& plan, size_t face, const Framing& framing, double z, const std::map<size_t, std::vector<session_cpp::Point>>& columns, session_cpp::Vector span) {

    const double thickness = plan.face_attribute(face, "thickness").value_or(framing.deck);
    std::vector<std::shared_ptr<session_cpp::Element>> decks;
    for (const std::vector<session_cpp::Polyline>& loops : compute_panels(compute_outline(plan, face, framing, columns), span, framing.panel)) {
        if (loops.empty() || compute_area(to_loop(loops[0])) <= 0.0)
            continue;

        std::shared_ptr<wood_session::Plate> deck = std::make_shared<wood_session::Plate>(compute_lift(loops[0], z), compute_lift(loops[0], z + thickness), "deck");
        if (loops.size() > 1) {
            for (const session_cpp::Polyline& ring : loops) {
                deck->features.bottom.push_back(compute_lift(ring, z));
                deck->features.top.push_back(compute_lift(ring, z + thickness));
            }
            deck->invalidate_geometry();
        }
        decks.push_back(deck);
    }

    return decks;
}

/// The facade wall under a perimeter edge over a storey: between the column faces, chamfered along the head faces under node 0, from z_bottom to the member bottom or the deck, thickness centred on the line.
inline std::shared_ptr<session_cpp::Element> to_wall(const session_cpp::Mesh& plan, std::pair<size_t, size_t> edge, const Framing& framing, double z_bottom, double z, const std::map<size_t, std::vector<session_cpp::Point>>& columns, const std::set<size_t>& heads) {

    const session_cpp::Point a = compute_lift(*plan.vertex_point(edge.first), 0.0);
    const session_cpp::Point b = compute_lift(*plan.vertex_point(edge.second), 0.0);
    const session_cpp::Vector along = compute_direction(a, b);
    const session_cpp::Vector normal = along.cross(session_cpp::Vector(0.0, 0.0, 1.0));
    const Member member = compute_member(plan, edge.first, edge.second, framing);
    const double z_top = z + (member.role > 0 ? member.bottom : 0.0);
    const auto rise = [&](size_t vertex, const session_cpp::Point& base, double inward) {
        const double support = columns.count(vertex) ? compute_reach(columns.at(vertex), base, along * inward) : 0.0;
        const auto at = [&](double offset, double height) { return compute_lift(base + along * (offset * inward), height); };
        if (!heads.count(vertex))
            return std::vector<session_cpp::Point>{at(support, z_bottom), at(support, z_top)};

        const double head_top = z + compute_head_top(plan, vertex, framing);
        std::vector<session_cpp::Point> points = {at(support, z_bottom), at(support, head_top - framing.head), at(framing.reach, head_top)};
        if (z_top > head_top + framing.tolerance)
            points.push_back(at(framing.reach, z_top));
        return points;
    };

    std::vector<session_cpp::Point> outline = rise(edge.first, a, 1.0);
    const std::vector<session_cpp::Point> back = rise(edge.second, b, -1.0);
    outline.insert(outline.end(), back.rbegin(), back.rend());
    const session_cpp::Polyline middle = to_polyline(outline);

    return std::make_shared<wood_session::Plate>(middle.translated(normal * (-framing.wall / 2.0)), middle.translated(normal * (framing.wall / 2.0)), plan.edge_attribute(edge, "wall").value_or(0.0) == 2.0 ? "core_wall" : "wall");
}

/// Plan intersection of two lines given by a point and a direction.
inline session_cpp::Point compute_meet(const session_cpp::Point& p, const session_cpp::Vector& d, const session_cpp::Point& q, const session_cpp::Vector& e) {

    const double denominator = d.cross(e)[2];
    if (std::abs(denominator) < 1e-9)
        return p;

    return p + d * ((q - p).cross(e)[2] / denominator);
}

/// The core walls of a ring over a storey, pinwheel: each wall runs from the inner face of the wall before it to the outer face of the wall after it, from z_bottom to z_top.
inline std::vector<std::shared_ptr<session_cpp::Element>> to_core(const session_cpp::Polyline& ring, const Framing& framing, double z_bottom, double z_top) {

    const std::vector<session_cpp::Point> corners = to_loop(ring);
    const size_t count = corners.size();
    const double half = framing.wall / 2.0;
    const auto direction = [&](size_t i) { return compute_direction(corners[i % count], corners[(i + 1) % count]); };
    const auto normal = [&](size_t i) { return direction(i).cross(session_cpp::Vector(0.0, 0.0, 1.0)); };

    std::vector<std::shared_ptr<session_cpp::Element>> walls;
    for (size_t i = 0; i < count; i++) {
        const size_t before = (i + count - 1) % count;
        const size_t after = (i + 1) % count;
        const session_cpp::Point start_inner = corners[i] - normal(before) * half;
        const session_cpp::Point end_outer = corners[after] + normal(after) * half;
        const std::vector<session_cpp::Point> quad = {
            compute_meet(corners[i] + normal(i) * half, direction(i), start_inner, direction(before)),
            compute_meet(corners[i] + normal(i) * half, direction(i), end_outer, direction(after)),
            compute_meet(corners[i] - normal(i) * half, direction(i), end_outer, direction(after)),
            compute_meet(corners[i] - normal(i) * half, direction(i), start_inner, direction(before))
        };
        walls.push_back(std::make_shared<wood_session::Plate>(compute_lift(to_polyline(quad), z_bottom), compute_lift(to_polyline(quad), z_top), "core_wall"));
    }

    return walls;
}

/// The plan vertex of a level within tolerance of a point, none otherwise.
inline std::optional<size_t> compute_vertex(const session_cpp::Mesh& plan, const session_cpp::Point& point, double tolerance) {

    for (const size_t vertex : plan.vertices())
        if (compute_distance(*plan.vertex_point(vertex), point) <= tolerance)
            return vertex;

    return std::nullopt;
}

/// A brace of a storey: the brace profile along the line, cut by the column faces at both ends, the deck top at the foot and the members or the deck at the head.
inline std::vector<std::shared_ptr<session_cpp::Element>> to_brace(const session_cpp::Line& line, const Building& building, size_t storey, const Framing& framing, const std::map<size_t, std::vector<session_cpp::Point>>& lower_columns, const std::map<size_t, std::vector<session_cpp::Point>>& upper_columns) {

    const Level& lower = building.levels[storey];
    const Level& upper = building.levels[storey + 1];
    const session_cpp::Point foot = line.start()[2] < line.end()[2] ? line.start() : line.end();
    const session_cpp::Point head = line.start()[2] < line.end()[2] ? line.end() : line.start();
    const session_cpp::Vector direction = compute_direction(foot, head);
    const std::vector<session_cpp::Polyline> profile = compute_profile(6, framing);
    const std::optional<size_t> under = compute_vertex(lower.plan, foot, building.tolerance);
    const double z_foot = lower.z + (under && framing.node != 2 ? compute_floor(lower.plan, *under, framing) : 0.0);

    std::vector<session_cpp::Plane> cuts = {session_cpp::Plane::from_point_normal(session_cpp::Point(0.0, 0.0, z_foot), session_cpp::Vector(0.0, 0.0, 1.0))};
    double top = upper.z;
    if (const std::optional<size_t> vertex = compute_vertex(upper.plan, head, building.tolerance)) {
        top = upper.z + compute_head_top(upper.plan, *vertex, framing);
        if (upper_columns.count(*vertex))
            if (const std::optional<session_cpp::Plane> plane = compute_exit(upper_columns.at(*vertex), compute_lift(head, 0.0), -direction))
                cuts.push_back(*plane);
    }
    cuts.push_back(session_cpp::Plane::from_point_normal(session_cpp::Point(0.0, 0.0, top), session_cpp::Vector(0.0, 0.0, -1.0)));
    if (under && lower_columns.count(*under))
        if (const std::optional<session_cpp::Plane> plane = compute_exit(lower_columns.at(*under), compute_lift(foot, 0.0), direction))
            cuts.push_back(*plane);

    const session_cpp::Vector along = (head - foot).normalized();
    const session_cpp::Vector up = along.cross(direction.cross(session_cpp::Vector(0.0, 0.0, 1.0))).normalized();
    std::shared_ptr<wood_session::Beam> beam = std::make_shared<wood_session::Beam>(session_cpp::Polyline({foot - along * framing.reach, head + along * framing.reach}), profile, std::vector<session_cpp::Vector>{up}, "brace");
    beam->cuts = cuts;

    return {beam};
}

// ═══════════════════════════════════════════════════════════════════════════
// Storeys
// ═══════════════════════════════════════════════════════════════════════════

/// Deck thickness at a level vertex: the thickest floor face there, 0 without a floor.
inline double compute_floor(const session_cpp::Mesh& plan, size_t vertex, const Framing& framing) {

    double thickness = 0.0;
    for (const size_t face : plan.vertex_faces(vertex).value_or(std::vector<size_t>()))
        if (plan.face_attribute(face, "floor").value_or(0.0) == 1.0)
            thickness = std::max(thickness, plan.face_attribute(face, "thickness").value_or(framing.deck));

    return thickness;
}

/// Direction the deck of a face spans: across the girders under system 1, along them otherwise.
inline session_cpp::Vector compute_span(const session_cpp::Mesh& plan, size_t face, const Framing& framing) {

    const int span = static_cast<int>(plan.face_attribute(face, "span").value_or(framing.span));
    const int system = static_cast<int>(plan.face_attribute(face, "system").value_or(framing.system));
    const session_cpp::Vector girder = compute_family_direction(plan, compute_loop(plan, face), span).value_or(session_cpp::Vector(1.0, 0.0, 0.0));

    return system == 1 ? session_cpp::Vector(0.0, 0.0, 1.0).cross(girder) : girder;
}

inline std::vector<std::shared_ptr<session_cpp::Element>> Building::to_elements(const Framing& framing, size_t storey) const {

    Building working = *this;
    for (Level& level : working.levels)
        compute_roles(level.plan, framing);

    const Level& lower = working.levels[storey];
    Level& upper = working.levels[storey + 1];
    const std::vector<Stack> stacks = compute_columns(working, storey, framing);
    const std::vector<Stack> next = storey + 2 < working.levels.size() ? compute_columns(working, storey + 1, framing) : std::vector<Stack>();

    std::map<size_t, std::vector<session_cpp::Point>> columns;
    std::set<size_t> heads;
    for (const Stack& stack : stacks) {
        columns[stack.upper] = compute_column_polygon(upper.plan, stack.upper, framing);
        heads.insert(stack.upper);
    }
    for (const Stack& stack : next)
        if (!columns.count(stack.lower))
            for (const session_cpp::Point& point : compute_column_polygon(working.levels[storey + 2].plan, stack.upper, framing))
                columns[stack.lower].push_back(point + (stack.foot - stack.head));
    std::map<size_t, std::vector<session_cpp::Point>> below;
    for (const Stack& stack : stacks)
        for (const session_cpp::Point& point : columns.at(stack.upper))
            below[stack.lower].push_back(point + (stack.foot - stack.head));
    const std::map<size_t, std::vector<session_cpp::Point>> plan_columns = framing.node == 0 ? std::map<size_t, std::vector<session_cpp::Point>>() : columns;
    const std::map<size_t, std::vector<session_cpp::Point>> plan_below = framing.node == 0 ? std::map<size_t, std::vector<session_cpp::Point>>() : below;

    std::vector<std::shared_ptr<session_cpp::Element>> elements;
    const auto foot_of = [&](size_t vertex) { return lower.z + (framing.node == 2 ? 0.0 : compute_floor(lower.plan, vertex, framing)); };
    for (const Stack& stack : stacks) {
        const double top = upper.z + (framing.node == 0 ? compute_head_top(upper.plan, stack.upper, framing) - framing.head : 0.0);
        elements.push_back(to_column(stack, columns.at(stack.upper), foot_of(stack.lower), top));
    }

    for (const session_cpp::Polyline& core : upper.cores) {
        const double bottom = lower.z + (storey == 0 ? 0.0 : lower.plan.faces().empty() ? 0.0 : framing.deck);
        for (const std::shared_ptr<session_cpp::Element>& wall : to_core(core, framing, bottom, upper.z + framing.deck))
            elements.push_back(wall);
    }

    for (const std::pair<size_t, size_t>& edge : upper.plan.edges())
        if (upper.plan.edge_attribute(edge, "wall").value_or(0.0) >= 1.0)
            elements.push_back(to_wall(upper.plan, edge, framing, lower.z + (framing.node == 2 ? 0.0 : std::max(compute_floor(lower.plan, edge.first, framing), compute_floor(lower.plan, edge.second, framing))), upper.z, columns, heads));

    if (framing.node == 0)
        for (const Stack& stack : stacks) {
            const double top = upper.z + compute_head_top(upper.plan, stack.upper, framing);
            elements.push_back(to_head(upper.plan, stack.upper, framing, top - framing.head, top));
        }

    for (const std::pair<size_t, size_t>& edge : upper.plan.edges())
        if (upper.plan.edge_attribute(edge, "role").value_or(0.0) > 0.0)
            for (const std::shared_ptr<session_cpp::Element>& beam : to_beam(upper.plan, edge, framing, upper.z, plan_columns))
                elements.push_back(beam);

    for (const size_t face : upper.plan.faces()) {
        if (upper.plan.face_attribute(face, "floor").value_or(0.0) != 1.0 || static_cast<int>(upper.plan.face_attribute(face, "system").value_or(framing.system)) != 2)
            continue;
        for (const Station& station : compute_stations(upper.plan, face, framing, pattern))
            for (const std::shared_ptr<session_cpp::Element>& purlin : to_purlin(upper.plan, station, framing, upper.z))
                elements.push_back(purlin);
    }

    for (const size_t face : upper.plan.faces())
        if (upper.plan.face_attribute(face, "floor").value_or(0.0) == 1.0)
            for (const std::shared_ptr<session_cpp::Element>& deck : to_deck(upper.plan, face, framing, upper.z, plan_columns, compute_span(upper.plan, face, framing)))
                elements.push_back(deck);

    if (storey == 0)
        for (const size_t face : lower.plan.faces())
            if (lower.plan.face_attribute(face, "floor").value_or(0.0) == 1.0)
                for (const std::shared_ptr<session_cpp::Element>& deck : to_deck(lower.plan, face, framing, lower.z, {}, compute_span(lower.plan, face, framing)))
                    elements.push_back(deck);

    for (const session_cpp::Line& line : braces)
        if (std::min(line.start()[2], line.end()[2]) >= lower.z - tolerance && std::max(line.start()[2], line.end()[2]) <= upper.z + tolerance)
            for (const std::shared_ptr<session_cpp::Element>& brace : to_brace(line, working, storey, framing, plan_below, plan_columns))
                elements.push_back(brace);

    return elements;
}

inline void Building::to_session(wood_session::WoodSession& session, const Framing& framing) const {

    for (size_t storey = 0; storey + 1 < levels.size(); storey++) {
        const std::shared_ptr<session_cpp::TreeNode> group = session.add_group(fmt::format("storey_{}", storey));
        for (const std::shared_ptr<session_cpp::Element>& element : to_elements(framing, storey))
            session.add(element, group);
    }
}

} // namespace wood_grid
