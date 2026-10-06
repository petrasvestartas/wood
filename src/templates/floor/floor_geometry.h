#pragma once
#include "src/templates/floor/floor.h"

namespace wood_floor::geometry {

// ═══════════════════════════════════════════════════════════════════════════
// Guide
// ═══════════════════════════════════════════════════════════════════════════

/// Why the guide's corners and oculus make no floor, empty when they do: the corners counter-clockwise and convex at z 0, every oculus corner between the centre and its edge midpoint, and the ring covering every quarter beam face, sin(oculus corner angle) >= sin(seam angle) at every oculus corner.
std::string invalid(const FloorGuide& guide);

// ═══════════════════════════════════════════════════════════════════════════
// Planes
// ═══════════════════════════════════════════════════════════════════════════

/// The plane turned by radians about the axis through point.
session_cpp::Plane rotate(const session_cpp::Plane& plane, double radians, const session_cpp::Vector& axis, const session_cpp::Point& point);

/// The world xy plane lifted to z.
session_cpp::Plane level(double z);

/// The vertical plane through the midpoint of a plan edge, its normal edge x normal_z.
session_cpp::Plane edge_plane(const session_cpp::Line& edge, const session_cpp::Vector& normal_z);

// ═══════════════════════════════════════════════════════════════════════════
// Intersections
// ═══════════════════════════════════════════════════════════════════════════

/// The line two planes share, by Intersection::plane_plane, pointing along cross(n0, n1); empty when the normals agree.
std::optional<session_cpp::Line> plane_plane(const session_cpp::Plane& plane0, const session_cpp::Plane& plane1);

/// The point an infinite line meets a plane, by Intersection::line_plane; empty when parallel.
std::optional<session_cpp::Point> line_plane(const session_cpp::Line& line, const session_cpp::Plane& plane);

/// The point three planes share, by Intersection::plane_plane_plane; empty when two of them are parallel.
std::optional<session_cpp::Point> plane_plane_plane(const session_cpp::Plane& plane0, const session_cpp::Plane& plane1, const session_cpp::Plane& plane2);

// ═══════════════════════════════════════════════════════════════════════════
// Polylines
// ═══════════════════════════════════════════════════════════════════════════

/// The polygon side from point i to the next, closing to the first.
session_cpp::Line edge(const std::vector<session_cpp::Point>& polygon, size_t i);

/// The polyline with both end segments pushed out by EXTENSION, then cut by the two planes, each cut keeping the side the original polyline's points average on.
session_cpp::Polyline trim(const session_cpp::Polyline& polyline, const session_cpp::Plane& plane0, const session_cpp::Plane& plane1);

/// Polylines of one vertex count trimmed alike, so the quads between them stay quads: each pushed out like trim, then cut at both planes on the segment the first one first crosses there, that segment's line on the others carried to the plane, keeping the vertices between.
std::vector<session_cpp::Polyline> trim_alike(const std::vector<session_cpp::Polyline>& polylines, const session_cpp::Plane& plane0, const session_cpp::Plane& plane1);

/// The polyline offset by distance in its vertical plane, square to every segment, its ends on the end normals.
session_cpp::Polyline offset_polyline(const session_cpp::Polyline& polyline, double distance);

/// The points of a closed loop without its closing point.
std::vector<session_cpp::Point> open_points(const session_cpp::Polyline& polyline);

/// The signed distance of a point from a plane, positive on the normal side.
double signed_distance(const session_cpp::Point& point, const session_cpp::Plane& plane);

/// The lowest corner of a member outline on a plane it ends on, at most 0.
double end_level(const Outline& outline, const session_cpp::Plane& end);

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

/// The points of the region two closed loops on a plane share, by the kernel's boolean intersection; the first loop when they share none.
std::vector<session_cpp::Point> overlap(const session_cpp::Polyline& a, const session_cpp::Polyline& b, const session_cpp::Plane& plane);

/// The polygon with everything below z removed, the crossing edges cut at z.
std::vector<session_cpp::Point> above(const std::vector<session_cpp::Point>& points, double z);

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════

/// A quarter member reference.
MemberRef quarter_member(size_t quarter, Family family, size_t index);

// ═══════════════════════════════════════════════════════════════════════════
// Central panel
// ═══════════════════════════════════════════════════════════════════════════

/// Rule A for one quarter: the inner ribs' one sweep r so that rib 0's central trace projected along one ruling u lands on rib 1's, and the central traces by the layers.
CentralPanel central_panel(const ConstructionPlanes& cp, const std::vector<std::array<session_cpp::Polyline, 3>>& parabolas, const FloorGuide& guide);

// ═══════════════════════════════════════════════════════════════════════════
// Outlines
// ═══════════════════════════════════════════════════════════════════════════

/// The member bounded by a ring of side planes between a bottom and a top plane: each outline is the corners of consecutive side planes on its plane, a missing corner skipped.
Outline loft_planes(const std::vector<session_cpp::Plane>& planes, const session_cpp::Plane& bottom, const session_cpp::Plane& top, bool flip = false);

// ═══════════════════════════════════════════════════════════════════════════
// Screws
// ═══════════════════════════════════════════════════════════════════════════

/// The screw relationships of the floor: per quarter the outer ribs into the seam beams, the mitres, the inner rib ends, then the ring corners and the ring into the quarters' oculus beams, each with its screw axes.
std::vector<Relationship> screw_relationships(const FloorGuide& guide);

}
