#pragma once

#include "pch.h"

using namespace session_cpp;

namespace wood_session {

/// One planar face of a solid, the coplanar mesh faces around it merged, with its frame.
struct PlanarFace {
    std::vector<Point> points; // Outer loop, counter-clockwise about the normal.
    std::vector<std::vector<Point>> holes; // Inner loops, clockwise.
    Vector normal; // Outward unit normal.
    Vector x; // In-plane x along the first side.
    Vector y; // normal x x.
};

/// One round hole: its axis, start to end, and its radius.
struct Drill {
    Line
        axis; // The hole runs along it; where an end lies inside the solid the hole stops there with a flat bottom.
    double radius = 0.0; // Hole radius.
};

/// The shortest distance between two segments.
double segment_distance(const Point& p0, const Point& p1, const Point& q0, const Point& q1);

/// Joins overlapping coaxial drills of the same radius.
std::vector<Drill> merged_drills(std::vector<Drill> drills);

/// Builds an exact drilled BRep from planar mesh faces, returning empty for unsupported intersections.
std::optional<BRep> drilled_brep(const Mesh& mesh, const std::vector<Drill>& drills);

/// The drills merged as drilled_brep merges them, split by whether it can bore each exactly.
std::array<std::vector<Drill>, 2>
split_clear_drills(const Mesh& mesh, const std::vector<Drill>& drills);

/// Returns line intervals inside a closed planar mesh, measured in millimetres from the start.
std::vector<std::array<double, 2>> inside_stretches(const Mesh& mesh, const Line& line);

/// The planar faces of a mesh.
std::vector<PlanarFace> planar_faces(const Mesh& mesh);

/// Whether a point lies inside the closed solid of planar_faces, by the parity of the faces a ray from it crosses.
bool is_inside(const std::vector<PlanarFace>& faces, const Point& point);

}
