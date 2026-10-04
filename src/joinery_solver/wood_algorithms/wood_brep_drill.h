#pragma once

#include "pch.h"

namespace wood_session {

/// One planar face of a solid, the coplanar mesh faces around it merged, with its frame.
struct PlanarFace {
    std::vector<session_cpp::Point> points; // Outer loop, counter-clockwise about the normal.
    std::vector<std::vector<session_cpp::Point>> holes; // Inner loops, clockwise.
    session_cpp::Vector normal; // Outward unit normal.
    session_cpp::Vector x; // In-plane x along the first side.
    session_cpp::Vector y; // normal x x.
};

/// One round hole: its axis, start to end, and its radius.
struct Drill {
    session_cpp::Line axis; // The hole runs along it; where an end lies inside the solid the hole stops there with a flat bottom.
    double radius = 0.0; // Hole radius.
};

/// The shortest distance between two segments.
double segment_distance(const session_cpp::Point& p0, const session_cpp::Point& p1, const session_cpp::Point& q0, const session_cpp::Point& q1);

/// The drills with every two of one radius on one axis whose spans meet or overlap joined into one, so two blind holes meeting in the middle make one through bore.
std::vector<Drill> merged_drills(std::vector<Drill> drills);

/// The closed solid of a mesh with planar faces as a BRep with exact round holes, coplanar faces merged and every edge shared one-to-one; empty when a drill passes within its radius of an edge or of another drill, or a face is not planar.
std::optional<session_cpp::BRep> drilled_brep(const session_cpp::Mesh& mesh, const std::vector<Drill>& drills);

/// The stretches where the line's infinite extension runs inside a closed mesh with planar faces, as parameter pairs in mm from its start, the line's own ends not clipping them.
std::vector<std::array<double, 2>> inside_stretches(const session_cpp::Mesh& mesh, const session_cpp::Line& line);

/// The planar faces of a mesh, every set of edge-adjacent coplanar mesh faces merged into one face with its outer loop and holes, each with its frame; empty when a face is not planar. Find them once for many is_inside tests.
std::vector<PlanarFace> planar_faces(const session_cpp::Mesh& mesh);

/// Whether a point lies inside the closed solid of planar_faces, by the parity of the faces a ray from it crosses.
bool is_inside(const std::vector<PlanarFace>& faces, const session_cpp::Point& point);

}
