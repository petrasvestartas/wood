#pragma once

#include "pch.h"

namespace wood_session {

/// One round hole: its axis, start to end, and its radius.
struct Drill {
    session_cpp::Line axis; // The hole runs along it; where an end lies inside the solid the hole stops there with a flat bottom.
    double radius = 0.0; // Hole radius.
};

/// The closed solid of a mesh with exact round holes, with no drills its clean planar BRep: its planar faces, coplanar mesh faces merged and holes kept as inner loops, with an exact circle or ellipse loop where a drill crosses one, an exact cylindrical face along every stretch of a drill inside the solid, and a flat disc where a drill ends inside it; empty when a drill passes within its radius of an edge of the mesh or of another drill, or crosses a face that is not planar, where a mesh hole is the only answer.
std::optional<session_cpp::BRep> drilled_brep(const session_cpp::Mesh& mesh, const std::vector<Drill>& drills);

/// Whether a point lies inside a closed mesh, by the parity of the faces a ray from it crosses.
bool is_inside(const session_cpp::Mesh& mesh, const session_cpp::Point& point);

}
