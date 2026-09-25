#pragma once
#include "src/templates/grid/grid.h"

namespace wood_grid::plan {

// ═══════════════════════════════════════════════════════════════════════════
// Loops
// ═══════════════════════════════════════════════════════════════════════════

/// The corners of a closed polyline, the closing point dropped.
std::vector<Point> to_loop(const Polyline& polyline);

/// A closed polyline through corners.
Polyline to_polyline(std::vector<Point> points);

/// Signed plan area of a loop, positive counter-clockwise seen from above.
double compute_area(const std::vector<Point>& points);

/// Unit plan direction from a to b.
Vector compute_direction(const Point& a, const Point& b);

/// Plan distance between two points.
double compute_distance(const Point& a, const Point& b);

/// The point at height z.
Point compute_lift(const Point& point, double z);

/// Every corner of a polyline at height z.
Polyline compute_lifted(const Polyline& polyline, double z);

/// The plan corners of a mesh vertex ring at z 0.
std::vector<Point> compute_flat(const Mesh& mesh, const std::vector<size_t>& ring);

/// A point just inside a simple plan polygon: its first side's midpoint nudged inwards.
Point compute_interior(const std::vector<Point>& points);

/// True when a counter-clockwise plan loop turns left at every corner.
bool is_convex(const std::vector<Point>& points);

// ═══════════════════════════════════════════════════════════════════════════
// Regions
// ═══════════════════════════════════════════════════════════════════════════

/// Rings of the cap faces of solid cut at the horizontal plane through z: outer rings counter-clockwise seen from above, face holes as clockwise holes, all at z 0; empty above the solid.
std::vector<Polyline> compute_section(const Mesh& solid, double z, double tolerance);

/// Intersection (0), union (1) or difference (2) of two ring lists with holes kept, output rings oriented as compute_section; Clipper2.
std::vector<Polyline> compute_regions(const std::vector<Polyline>& a, const std::vector<Polyline>& b, int clip);

/// Even-odd inside test over all rings, so a ring inside a hole is an island.
bool is_inside(const std::vector<Polyline>& rings, const Point& point);

/// Where the lines of two sides through a corner meet once each is moved out along its outward normal by its distance: the mitre; along the first normal by the larger distance when the sides are parallel.
Point compute_corner(const Point& corner, const Vector& before, double a, const Vector& after, double b);

/// Loop with side i moved out by distances[i], the loop counter-clockwise seen from above; the mitre of two sides at every corner.
std::vector<Point> compute_offset(const std::vector<Point>& points, const std::vector<double>& distances);

/// A piece of a line between its crossings with rings, and the ring and side each end stops on, -1 at the line's own ends.
struct Piece {
    Line line; // The piece.
    std::array<int, 2> ring = {-1, -1}; // Ring index at the start and the end.
    std::array<int, 2> side = {-1, -1}; // Side index at the start and the end.
};

/// A line cut where it crosses the rings: the pieces whose midpoints lie inside, or outside when inside is false; the whole line when there are no rings.
std::vector<Piece> compute_pieces(const Line& line, const std::vector<Polyline>& rings, bool inside, double tolerance);

// ═══════════════════════════════════════════════════════════════════════════
// Arrangement
// ═══════════════════════════════════════════════════════════════════════════

/// A line id of a ring edge: 1000000 + 1000 * ring + edge.
double compute_ring_id(size_t ring, size_t edge);

/// True for the id of a ring edge.
bool is_ring(double id);

/// Every line split at every crossing with another, collinear overlaps given to rings and earlier lines, split points within merge welded onto ring corners first, then ring crossings, then the rest, dangling pieces dropped; ids follow their source line.
std::pair<std::vector<Line>, std::vector<double>> compute_crossings(const std::vector<Line>& lines, const std::vector<double>& ids, double tolerance, double merge);

/// Mesh::from_lines of split lines with the outer face deleted, edge attribute line from the split's ids and vertex attributes line_a, line_b from the two lowest ids meeting there; slivers below tolerance squared dropped.
Mesh compute_arrangement(const std::vector<Line>& lines, const std::vector<double>& ids, double tolerance);

// ═══════════════════════════════════════════════════════════════════════════
// Planes
// ═══════════════════════════════════════════════════════════════════════════

/// Vertical plane along the side of a plan polygon the ray from origin along direction leaves through, normal out of the polygon; for a concave polygon the plane at its farthest reach, so no flange is run through; none when the ray never leaves.
std::optional<Plane> compute_exit(const std::vector<Point>& polygon, const Point& origin, const Vector& direction);

/// Reach of a plan polygon past a point along a plan direction: its farthest corner.
double compute_reach(const std::vector<Point>& polygon, const Point& point, const Vector& direction);

/// Long plan rectangle of half-width about the line through origin along direction: the footprint of a member that runs through.
std::vector<Point> compute_strip(const Point& origin, const Vector& direction, double half);

/// Plane through origin bisecting unit plan directions a and b, normal towards a; perpendicular to a when b is opposite.
Plane compute_bisector(const Point& origin, const Vector& a, const Vector& b);

/// Closed polygon about centre whose side j is perpendicular to directions[j] at distances[j]; the direction polygon of a node.
Polyline compute_polygon(const std::vector<Vector>& directions, const Point& centre, const std::vector<double>& distances);

/// Unit plan directions of the edges at a plan vertex and their opposites, counter-clockwise, closer than 1 degree merged; one edge adds its perpendicular, none gives x and y.
std::vector<Vector> compute_directions(const Mesh& plan, size_t vertex);

} // namespace wood_grid::plan

namespace wood_grid::build {

// ═══════════════════════════════════════════════════════════════════════════
// Records
// ═══════════════════════════════════════════════════════════════════════════

/// What the joint rules read at one level: its plan with the roles filled, the framing, the weld tolerance, and the column sections at its vertices, counter-clockwise at z 0.
struct Context {
    const Mesh& plan; // The level plan.
    const Framing& framing; // The framing the level is built with.
    double tolerance = 1.0; // Weld distance and coplanarity tolerance of the building.
    const std::map<size_t, std::vector<Point>>& standing; // Sections of the columns standing under the level at their head vertices: members butt into them under nodes 1 and 2.
    const std::map<size_t, std::vector<Point>>& rising; // Sections of the columns rising from the level at their feet: the deck notches under node 2.
};

/// A member end at a plan vertex: what the joint rules read.
struct Member {
    size_t other = 0; // The vertex at its far end.
    Vector direction; // Unit plan direction away from the vertex.
    int role = 0; // Edge role.
    int rank = 0; // Order at the node: perimeter 7, girder 6, beam 5, purlin 4, brace 1.
    double width = 0.0; // Profile width.
    double top = 0.0; // Top relative to the datum.
    double bottom = 0.0; // Bottom relative to the datum.
};

/// The end of a member at its vertex: how far its axis runs past the vertex before the cuts, and the cut planes, all at z 0.
struct End {
    double overrun = 0.0; // Length past the vertex the axis is built with.
    std::vector<Plane> planes; // Cut planes, each keeping the side its normal points to.
    bool bearing = false; // True when a column cuts the end: the member bears there, however short it is.
};

/// A purlin station over a bay at z 0 with the cut planes of both ends.
struct Station {
    Line line; // From one support to the next.
    std::vector<Plane> cuts; // The face of what each end lands on.
};

// ═══════════════════════════════════════════════════════════════════════════
// Rules
// ═══════════════════════════════════════════════════════════════════════════

/// The role of every edge without one: girders on the span family, purlins on the cross lines of system 2, beams on free lines and under span -1, perimeter members likewise, nothing under system 0; wall 1 on the perimeter when the framing asks for a facade.
void compute_roles(Mesh& plan, const Framing& framing);

/// Element name of a role: girder, beam, purlin, brace.
std::string compute_name(int role);

/// Profile of a role from the framing, the fallbacks of Profiles applied.
std::vector<Polyline> compute_profile(int role, const Framing& framing);

/// The member on the edge from vertex to other.
Member compute_member(const Context& context, size_t vertex, size_t other);

/// Every member at a vertex, counter-clockwise from x.
std::vector<Member> compute_members(const Context& context, size_t vertex);

/// Adds a plane unless an equal one is there.
void add_plane(std::vector<Plane>& planes, const Plane& plane);

/// Adds the exit face of a plan polygon for the ray from origin along direction, when the ray leaves it.
void add_exit(std::vector<Plane>& planes, const std::vector<Point>& polygon, const Point& origin, const Vector& direction);

/// Cut planes of the member on the edge from vertex to other at its vertex end: the column face there, then the through member's side, the mitre with an equal neighbour, or its own open end.
End compute_cuts(const Context& context, size_t vertex, size_t other);

/// Lowest member bottom at a vertex relative to the datum, 0 when only the deck arrives: the head top under node 0.
double compute_head_top(const Context& context, size_t vertex);

/// Farthest corner of any column section at a vertex along a plan direction, 0 without a column.
double compute_column_reach(const Context& context, size_t vertex, const Vector& direction);

/// Counter-clockwise corners of a plan face.
std::vector<size_t> compute_loop(const Mesh& plan, size_t face);

/// The sides of a loop, side i from corner i to i + 1.
std::vector<std::pair<size_t, size_t>> compute_sides(const std::vector<size_t>& loop);

/// Mean line direction of the plan edges in a family, opposite directions alike; none when no edge is in it.
std::optional<Vector> compute_family_direction(const Mesh& plan, const std::vector<std::pair<size_t, size_t>>& edges, int family);

/// How far the deck moves out from a plan edge: onto the outer face of the member or column on a perimeter edge, 0 on an interior edge.
double compute_side(const Context& context, std::pair<size_t, size_t> edge);

/// The four corners of the walls of a core ring in plan, pinwheel: each wall runs from the inner face of the wall before it to the outer face of the wall after it.
std::vector<std::vector<Point>> compute_core_quads(const Polyline& ring, double wall);

/// Deck loops of every floor face at z 0, largest first: perimeter sides out to the outer face of the member or column there, minus the columns rising through the deck, the core walls, and the decks built before it.
std::map<size_t, std::vector<Polyline>> compute_outlines(const Context& context, const std::vector<Polyline>& cores);

/// Loops of a deck cut into equal strips across the deck span, as many as compute_bays lays over its width at panel; one loop set when panel is 0.
std::vector<std::vector<Polyline>> compute_panels(const std::vector<Polyline>& loops, const Vector& span, double panel);

/// The vertical plane of a side of the wall band round a core, normal away from the core, through at.
Plane compute_wall_face(const std::vector<Polyline>& outer, int ring, int side, const Point& at);

/// Purlin stations of a system 2 face: parallel to its cross-family edges (else across its girders) at ceil(width / spacing) intervals, clipped to the face and outside the cores, each end cut on the member or wall it lands on.
std::vector<Station> compute_stations(const Context& context, size_t face, const std::vector<Polyline>& cores);

} // namespace wood_grid::build
