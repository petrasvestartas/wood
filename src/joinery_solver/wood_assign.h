// ═══════════════════════════════════════════════════════════════════════════
// wood/wood_assign.h — assign_joint and assign_insertion helpers.
//
// Session-branch equivalents of the CGAL-based rtree_util functions
// find_closest_plateside_to_indexedpoint and find_closest_plateside_to_line
// from the main branch (cmake/src/wood/include/rtree_util.h).
//
// Both functions:
//   1. Build a spatial R-tree over element AABBs for broad-phase culling.
//   2. For each query, iterate segments of the top and bottom face polylines
//      and find the closest one (per-segment Closest::line_point).
//   3. Write the result into the output vector (joint type int / direction
//      Vector) at the correct face/edge slot index.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once
#include <vector>
#include "wood_element.h"
// session_cpp headers via the "../src/xxx.h" pattern used across wood/.
#include "../src/point.h"
#include "../src/line.h"
#include "../src/vector.h"

namespace wood_session {

// ───────────────────────────────────────────────────────────────────────────
// assign_joint — assign integer joint-type indices to element face/edge slots
//               based on closest-point search against plate face polylines.
//
// Mirrors: collider::rtree_util::find_closest_plateside_to_indexedpoint
//          (cmake/src/wood/include/rtree_util.cpp:189–282)
//
// Parameters
// ----------
// elements      — timber plates (polylines[0]=bot, [1]=top, [2..N]=sides).
// points        — one query point per assignment.
// point_types   — integer joint type per point:
//                   t < 0  → assign to a face-plane slot (top or bottom)
//                   t > 0  → assign to a side-edge slot (2 + segment index)
//                   abs(t) is the value stored; 0 clears the slot.
//
// Output
// ------
// out_joint_types[element][slot] = abs(type), -1 when unassigned.
// Slot layout:  0 = bottom face, 1 = top face, 2..N = side edges
//               where N-2 == top_polyline.point_count()-1.
// ───────────────────────────────────────────────────────────────────────────
void assign_joint(
    const std::vector<WoodElement>&         elements,
    const std::vector<session_cpp::Point>&  points,
    const std::vector<int>&                 point_types,
    std::vector<std::vector<int>>&          out_joint_types
);

// ───────────────────────────────────────────────────────────────────────────
// assign_insertion — assign insertion direction vectors to element side slots
//                   based on a line's start point closest-point search.
//
// Mirrors: collider::rtree_util::find_closest_plateside_to_line
//          (cmake/src/wood/include/rtree_util.cpp:84–187)
//
// Parameters
// ----------
// elements — timber plates.
// lines    — one line per assignment.
//              line.start()               → search anchor (closest-point test)
//              line.end() - line.start()  → direction stored in output
//
// Output
// ------
// out_insertion_vectors[element][segment_idx + 2] = direction vector.
// Slot offset +2 skips top(1) and bottom(0) face-plane slots so that side
// edge i maps to slot i+2, matching the face index convention in WoodElement.
// All unassigned slots hold Vector(0,0,0).
// ───────────────────────────────────────────────────────────────────────────
void assign_insertion(
    const std::vector<WoodElement>&                elements,
    const std::vector<session_cpp::Line>&          lines,
    std::vector<std::vector<session_cpp::Vector>>& out_insertion_vectors
);

} // namespace wood_session
