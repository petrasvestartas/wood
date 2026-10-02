#pragma once
#include "src/templates/floor/floor.h"

namespace wood_floor::geometry {

// ═══════════════════════════════════════════════════════════════════════════
// Planes
// ═══════════════════════════════════════════════════════════════════════════

/// The plane moved by distance along its normal.
session_cpp::Plane offset(const session_cpp::Plane& plane, double distance);

/// The plane turned by radians about the axis through point.
session_cpp::Plane rotate(const session_cpp::Plane& plane, double radians, const session_cpp::Vector& axis, const session_cpp::Point& point);

/// The world xy plane lifted to z.
session_cpp::Plane level(double z);

/// The vertical plane through the midpoint of a plan edge, its normal edge x normal_z.
session_cpp::Plane edge_plane(const session_cpp::Line& edge, const session_cpp::Vector& normal_z);

// ═══════════════════════════════════════════════════════════════════════════
// Intersections
// ═══════════════════════════════════════════════════════════════════════════

/// The line two planes share, from plane0 towards cross(n0, n1) with that unscaled length; empty when the normals agree.
std::optional<session_cpp::Line> plane_plane(const session_cpp::Plane& plane0, const session_cpp::Plane& plane1);

/// The point an infinite line meets a plane; empty when parallel.
std::optional<session_cpp::Point> line_plane(const session_cpp::Line& line, const session_cpp::Plane& plane);

/// The point three planes share; empty when two of them are parallel.
std::optional<session_cpp::Point> plane_plane_plane(const session_cpp::Plane& plane0, const session_cpp::Plane& plane1, const session_cpp::Plane& plane2);

// ═══════════════════════════════════════════════════════════════════════════
// Polylines
// ═══════════════════════════════════════════════════════════════════════════

/// The polygon side from point i to the next, closing to the first.
session_cpp::Line edge(const std::vector<session_cpp::Point>& polygon, size_t i);

/// The unit direction of a line.
session_cpp::Vector direction(const session_cpp::Line& line);

/// The polyline cut by two planes in turn, each keeping the side of the remaining middle.
session_cpp::Polyline cut(const session_cpp::Polyline& polyline, const session_cpp::Plane& plane0, const session_cpp::Plane& plane1);

/// The polyline with both end segments pushed out by EXTENSION, then cut by the two planes.
session_cpp::Polyline trim(const session_cpp::Polyline& polyline, const session_cpp::Plane& plane0, const session_cpp::Plane& plane1);

/// The polyline offset by distance in its vertical plane, square to every segment, its ends on the end normals.
session_cpp::Polyline offset_polyline(const session_cpp::Polyline& polyline, double distance);

/// The area centroid of a closed planar polyline.
session_cpp::Point area_centroid(const session_cpp::Polyline& polyline);

/// The area of a closed planar polyline.
double polygon_area(const session_cpp::Polyline& polyline);

/// The closed polygon of the points lifted by lift.
session_cpp::Polyline lifted(const std::vector<session_cpp::Point>& points, double lift);

/// The plane lifted by lift.
session_cpp::Plane lifted(const session_cpp::Plane& plane, double lift);

/// The line lifted by lift.
session_cpp::Line lifted(const session_cpp::Line& line, double lift);

/// The polygon with everything below z removed, the crossing edges cut at z.
std::vector<session_cpp::Point> above(const std::vector<session_cpp::Point>& points, double z);

// ═══════════════════════════════════════════════════════════════════════════
// Central panel
// ═══════════════════════════════════════════════════════════════════════════

/// Rule A for one quarter: the inner ribs' one sweep r so that rib 0's central trace projected along one ruling u lands on rib 1's, and the central traces by the layers.
CentralPanel central_panel(const ConstructionPlanes& cp, const std::vector<std::array<session_cpp::Polyline, 3>>& parabolas, const FloorSizes& sizes, CentralLayers layers);

// ═══════════════════════════════════════════════════════════════════════════
// Outlines
// ═══════════════════════════════════════════════════════════════════════════

/// The member bounded by a ring of side planes between a bottom and a top plane: each outline is the corners of consecutive side planes on its plane, a missing corner skipped.
Outline loft_planes(const std::vector<session_cpp::Plane>& planes, const session_cpp::Plane& bottom, const session_cpp::Plane& top, bool flip = false);

// ═══════════════════════════════════════════════════════════════════════════
// Screws
// ═══════════════════════════════════════════════════════════════════════════

/// The screw relationships of the floor: per quarter the outer ribs into the seam beams, the mitres, the inner rib ends, then the ring corners and the ring into the quarters' oculus beams, each with its screw axes.
std::vector<Relationship> screw_relationships(const Floor& floor);

}
