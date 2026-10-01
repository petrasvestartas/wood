#pragma once
#include "src/templates/floor/floor.h"

namespace wood_floor::geometry {

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Planes
// ═══════════════════════════════════════════════════════════════════════════

/// The plane moved by distance along its normal.
Plane offset(const Plane& plane, double distance);

/// The plane turned by radians about the axis through point.
Plane rotate(const Plane& plane, double radians, const Vector& axis, const Point& point);

/// The world xy plane lifted to z.
Plane level(double z);

/// The vertical plane through the midpoint of a plan edge, its normal edge x normal_z.
Plane edge_plane(const Line& edge, const Vector& normal_z);

// ═══════════════════════════════════════════════════════════════════════════
// Intersections
// ═══════════════════════════════════════════════════════════════════════════

/// The line two planes share, from plane0 towards cross(n0, n1) with that unscaled length; empty when the normals agree.
std::optional<Line> plane_plane(const Plane& plane0, const Plane& plane1);

/// The point an infinite line meets a plane; empty when parallel.
std::optional<Point> line_plane(const Line& line, const Plane& plane);

/// The point three planes share; empty when two of them are parallel.
std::optional<Point> plane_plane_plane(const Plane& plane0, const Plane& plane1, const Plane& plane2);

// ═══════════════════════════════════════════════════════════════════════════
// Polylines
// ═══════════════════════════════════════════════════════════════════════════

/// The polygon side from point i to the next, closing to the first.
Line edge(const std::vector<Point>& polygon, size_t i);

/// The unit direction of a line.
Vector direction(const Line& line);

/// The points closed by repeating the first.
Polyline closed(const std::vector<Point>& points);

/// The polyline with both end segments extended outwards by amount.
Polyline extend_ends(const Polyline& polyline, double amount);

/// The polyline cut by two planes in turn, each keeping the side of the remaining middle.
Polyline cut(const Polyline& polyline, const Plane& plane0, const Plane& plane1);

/// The polyline offset by distance in its vertical plane, square to every segment, its ends on the end normals.
Polyline offset_polyline(const Polyline& polyline, double distance);

/// Divisions points along the quadratic Bezier curve from p0 over control p1 to p2.
Polyline quadratic_points(const Point& p0, const Point& p1, const Point& p2, int divisions = 7);

// ═══════════════════════════════════════════════════════════════════════════
// Outlines
// ═══════════════════════════════════════════════════════════════════════════

/// The member bounded by a ring of side planes between a bottom and a top plane: each outline is the corners of consecutive side planes on its plane, a missing corner skipped.
Outline loft_planes(const std::vector<Plane>& planes, const Plane& bottom, const Plane& top, bool flip = false);

}
