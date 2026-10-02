#pragma once

#include "pch.h"

namespace wood_session {

/// One round hole: its axis, start to end, and its radius.
struct Drill {
    session_cpp::Line axis; // The hole runs along it; where an end lies inside the solid the hole stops there with a flat bottom.
    double radius = 0.0; // Hole radius.
};

/// The shortest distance between two segments.
double segment_distance(const session_cpp::Point& p0, const session_cpp::Point& p1, const session_cpp::Point& q0, const session_cpp::Point& q1);

/// The drills with every two of one radius on one axis whose spans meet or overlap joined into one, again until none do, so two blind holes bored from opposite faces to the same middle make one through bore.
std::vector<Drill> merged_drills(std::vector<Drill> drills);

/// The closed solid of a mesh with exact round holes, with no drills its clean planar BRep: its planar faces, coplanar mesh faces merged and holes kept as inner loops, every side split at the vertices of neighbouring faces lying on it so each edge is shared one-to-one, with an exact circle or ellipse loop where a drill crosses one, an exact cylindrical face along every stretch of a drill inside the solid, and a flat disc where a drill ends inside it; empty when a drill passes within its radius of an edge of the mesh or of another drill, or crosses a face that is not planar, where a mesh hole is the only answer.
std::optional<session_cpp::BRep> drilled_brep(const session_cpp::Mesh& mesh, const std::vector<Drill>& drills);

/// Where the line's infinite extension runs inside a closed mesh with planar faces: the parameter pairs along it, in mm from its start, of every stretch from an entry crossing to the exit after it; the line's own ends do not clip them.
std::vector<std::array<double, 2>> inside_stretches(const session_cpp::Mesh& mesh, const session_cpp::Line& line);

/// Whether a point lies inside a closed mesh, by the parity of the faces a ray from it crosses.
bool is_inside(const session_cpp::Mesh& mesh, const session_cpp::Point& point);

}
