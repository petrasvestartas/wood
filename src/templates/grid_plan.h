#pragma once
#include "pch.h"

namespace wood_grid::plan {

using namespace session_cpp;

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

/// The plan corners of a mesh face ring at z 0.
std::vector<Point> compute_flat(const Mesh& mesh, const std::vector<size_t>& ring);

/// True when a counter-clockwise plan loop turns left at every corner.
bool is_convex(const std::vector<Point>& points);

// ═══════════════════════════════════════════════════════════════════════════
// Sections
// ═══════════════════════════════════════════════════════════════════════════

/// Rings of the cap faces of solid cut at the horizontal plane through z: outer rings counter-clockwise seen from above, face_holes as clockwise holes, all dropped to z 0; empty above the solid.
std::vector<Polyline> compute_section(const Mesh& solid, double z, double tolerance);

/// Intersection (0), union (1) or difference (2) of two ring lists with holes kept, output rings oriented as compute_section; Clipper2.
std::vector<Polyline> compute_regions(const std::vector<Polyline>& a, const std::vector<Polyline>& b, int clip);

/// Even-odd inside test over all rings, so a ring inside a hole is an island.
bool is_inside(const std::vector<Polyline>& rings, const Point& point);

/// A point strictly inside a simple plan polygon: the centre of the ear at its leftmost corner, or halfway to the corner deepest inside that ear.
Point compute_interior(const std::vector<Point>& points);

// ═══════════════════════════════════════════════════════════════════════════
// Corners
// ═══════════════════════════════════════════════════════════════════════════

/// Where the lines of two sides through a corner meet once each is moved out along its outward normal by its distance: the mitre; along the first normal by the larger distance when the sides are parallel.
Point compute_corner(const Point& corner, const Vector& before, double a, const Vector& after, double b);

/// Outward plan normals of the sides of a counter-clockwise loop, side i from corner i to i + 1.
std::vector<Vector> compute_normals(const std::vector<Point>& points);

/// Loop with side i moved out by distances[i], the loop counter-clockwise seen from above; the mitre of two sides at every corner.
std::vector<Point> compute_offset(const std::vector<Point>& points, const std::vector<double>& distances);

// ═══════════════════════════════════════════════════════════════════════════
// Arrangement
// ═══════════════════════════════════════════════════════════════════════════

/// A line id of a ring edge: 1000000 + 1000 * ring + edge, 2000000 up for a core ring.
double compute_ring_id(size_t ring, size_t edge, bool core);

/// True for the id of a ring edge.
bool is_ring(double id);

/// True for the id of a core ring edge.
bool is_core_id(double id);

/// Every line split at every crossing with another, collinear overlaps given to rings and earlier lines, split points within merge welded in priority (core rings within tolerance alone), dangling pieces dropped; ids follow their source line.
std::pair<std::vector<Line>, std::vector<double>> compute_crossings(const std::vector<Line>& lines, const std::vector<double>& ids, double tolerance, double merge);

/// Mesh::from_lines of split lines with the outer face deleted, edge attribute line from the split's ids and vertex attributes line_a, line_b from the two lowest ids meeting there; slivers below tolerance squared dropped.
Mesh compute_arrangement(const std::vector<Line>& lines, const std::vector<double>& ids, double tolerance);

// ═══════════════════════════════════════════════════════════════════════════
// Planes
// ═══════════════════════════════════════════════════════════════════════════

/// One plane per face of a closed mesh with a Newell normal turned out of the solid; exact for the swept and lofted solids the builders make.
std::vector<Plane> compute_planes(const Mesh& solid);

/// Vertical plane along the side of a convex plan polygon the ray from origin along direction leaves through, normal out of the polygon; none when the ray never leaves.
std::optional<Plane> compute_exit(const std::vector<Point>& polygon, const Point& origin, const Vector& direction);

/// Reach of a plan polygon past a point along a plan direction: its farthest corner.
double compute_reach(const std::vector<Point>& polygon, const Point& point, const Vector& direction);

/// Vertical plane perpendicular to direction at the farthest reach of a plan polygon past origin, normal along direction: the support plane a member butts into when the polygon is concave.
Plane compute_bound(const std::vector<Point>& polygon, const Point& origin, const Vector& direction);

/// Long plan rectangle of half-width about the line through origin along direction: the footprint of a member that runs through.
std::vector<Point> compute_strip(const Point& origin, const Vector& direction, double half);

/// Plane through origin bisecting unit plan directions a and b, normal towards a; perpendicular to a when b is opposite.
Plane compute_bisector(const Point& origin, const Vector& a, const Vector& b);

/// Closed polygon about centre whose side j is perpendicular to directions[j] at distances[j]; the direction polygon of a node.
Polyline compute_polygon(const std::vector<Vector>& directions, const Point& centre, const std::vector<double>& distances);

/// Unit plan directions of the edges at a plan vertex and their opposites, counter-clockwise, closer than 1 degree merged; one edge adds its perpendicular, none gives x and y.
std::vector<Vector> compute_directions(const Mesh& plan, size_t vertex);

} // namespace wood_grid::plan
