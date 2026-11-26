#pragma once
#include "session.h"
#include "intersection.h"
#include "mesh.h"
#include "plane.h"
#include "polyline.h"
#include "tolerance.h"
#include "line.h"
#include "xform.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <utility>
#include <vector>

using namespace session_cpp;

/// The boundary frame and the box beam shared by the reciprocal templates.
namespace wood_reciprocal {

// ═══════════════════════════════════════════════════════════════════════════
// Mesh topology
// ═══════════════════════════════════════════════════════════════════════════

/// The sorted vertex pair of the edge u-v.
std::pair<size_t, size_t> edge_key(size_t u, size_t v);

/// Every edge mapped to the (face, local edge) pairs that own it.
std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>>
edge_owners(const std::vector<std::vector<size_t>>& faces);

/// The start vertex of a half-edge.
size_t half_edge_start(
    const std::vector<std::vector<size_t>>& faces, const std::pair<int, int>& half_edge
);

/// The end vertex of a half-edge.
size_t
half_edge_end(const std::vector<std::vector<size_t>>& faces, const std::pair<int, int>& half_edge);

/// The naked half-edges in boundary-loop order, each in its owning face's winding.
std::vector<std::pair<int, int>> naked_half_edges(
    const std::vector<std::vector<size_t>>& faces,
    const std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>>& owners
);

/// The [start, end) ranges of the boundary loops in the ordered naked half-edges.
std::vector<std::pair<size_t, size_t>> loop_ranges(
    const std::vector<std::pair<int, int>>& naked, const std::vector<std::vector<size_t>>& faces
);

/// Whether the loop over the range closes on its first half-edge.
bool loop_closed(
    const std::vector<std::pair<int, int>>& naked,
    const std::vector<std::vector<size_t>>& faces,
    const std::pair<size_t, size_t>& range
);

/// The faces of the mesh as vertex keys, with every vertex point and one normal per face.
struct MeshFaces {
    std::vector<std::vector<size_t>> faces; // Vertex keys per face, in winding order.
    std::vector<Vector> normals; // One normal per face, (0, 0, 1) for a degenerate one.
    std::map<size_t, Point> points; // Vertex key -> point.
    std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>>
        owners; // Edge -> its (face, local edge) pairs.

    /// The faces of the mesh in key order with their normals.
    static MeshFaces of(const Mesh& mesh);
};

// ═══════════════════════════════════════════════════════════════════════════
// Vectors
// ═══════════════════════════════════════════════════════════════════════════

/// The part of v perpendicular to the unit axis, unit length, any perpendicular when v is along it.
Vector perpendicular_part(const Vector& v, const Vector& axis);

/// The plane turned so that the point lies on its positive side.
Plane facing(const Plane& plane, const Point& inside);

/// The long face of a box beam through point along dir whose normal points most along towards.
Plane beam_face_towards(
    const Point& point,
    const Vector& dir,
    const Vector& up,
    double width,
    double height,
    const Vector& towards
);

// ═══════════════════════════════════════════════════════════════════════════
// Frame
// ═══════════════════════════════════════════════════════════════════════════

/// A plane a beam end is cut by, valid only where the hit lies on the positive side of every bound.
struct CutFace {
    Plane plane; // The cutting plane.
    std::vector<Plane>
        bounds; // Half-spaces the hit must lie in; empty for a plane that cuts everywhere.
};

/// The plane as a cut valid everywhere.
std::vector<CutFace> unbounded(const Plane& plane);

/// One straight beam per naked edge: axis, up, end planes, and its inner face at each boundary vertex.
struct BoundaryFrame {
    std::vector<std::pair<int, int>> naked; // (face, local edge) per frame beam, in loop order.
    std::vector<Vector> directions; // Unit axis per frame beam, start vertex to end vertex.
    std::vector<Vector> ups; // Up per frame beam.
    std::vector<Plane> cut_from; // End plane at the start vertex per frame beam.
    std::vector<Plane> cut_to; // End plane at the end vertex per frame beam.
    std::map<size_t, std::vector<CutFace>>
        inner_faces_at_vertex; // Boundary vertex -> the inner faces of the frame beams meeting there.
};

/// The up per naked half-edge, reflected across every miter and its closure twist put at the bends.
std::vector<Vector> boundary_up_directions(
    const std::vector<std::pair<int, int>>& naked,
    const std::vector<std::vector<size_t>>& faces,
    const std::map<size_t, Point>& points,
    const std::vector<Vector>& owner_normals
);

/// The miter plane at a boundary vertex, the bisector of the arriving and leaving directions.
Plane miter_plane(const Point& vertex, const Vector& arriving, const Vector& leaving);

/// The boundary frame of the mesh faces for beams width by height, ups from boundary_ups where given, butted at the corners whose sections do not mirror when butt, else mitered.
BoundaryFrame boundary_frame(
    const std::vector<std::vector<size_t>>& faces,
    const std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>>& owners,
    const std::map<size_t, Point>& points,
    const std::vector<Vector>& face_normals,
    double width,
    double height,
    const std::map<std::pair<size_t, size_t>, Vector>& boundary_ups,
    bool butt,
    const std::map<std::pair<size_t, size_t>, int>& through_priority
);

/// The frame's inner faces at the vertex a beam along dir can be cut by, the near-parallel ones left out.
std::vector<CutFace>
frame_cuts(const BoundaryFrame& frame, size_t vertex, const Vector& dir);

// ═══════════════════════════════════════════════════════════════════════════
// Frame ups
// ═══════════════════════════════════════════════════════════════════════════

/// One up per naked edge from its owning face's normal, turned to point up.
std::map<std::pair<size_t, size_t>, Vector> owner_normal_ups(const Mesh& mesh);

/// The naked half-edges of the mesh in loop order, split into sides at bends over corner_angle degrees.
struct MeshBoundary {
    std::vector<std::pair<size_t, size_t>> keys; // Edge key per naked half-edge.
    std::vector<Point> starts; // Start point per naked half-edge.
    std::vector<Point> ends; // End point per naked half-edge.
    std::vector<Vector> directions; // Unit direction per naked half-edge.
    std::vector<Vector> normals; // Owning face normal, turned up, per naked half-edge.
    std::vector<int> sides; // Side index per naked half-edge.
    std::vector<int> side_rank; // Rank of every side within its loop, 0 first.

    /// The boundary of the mesh split into sides at bends over corner_angle degrees.
    static MeshBoundary of(const Mesh& mesh, double corner_angle);
};

/// One up per naked edge, one tilt per side from the plane fitted through it, or from side_vectors.
std::map<std::pair<size_t, size_t>, Vector> side_tilt_boundary_ups(
    const Mesh& mesh, double corner_angle, const std::vector<Vector>& side_vectors
);

/// The through priority per naked edge: the sides of every loop alternate 1, 0, 1, 0.
std::map<std::pair<size_t, size_t>, int>
through_side_priority(const Mesh& mesh, double corner_angle);

// ═══════════════════════════════════════════════════════════════════════════
// Box beam
// ═══════════════════════════════════════════════════════════════════════════

/// One box beam: axis and up, its solid, and its four long faces as closed outlines.
struct ReciprocalBeam {
    Vector direction; // Unit axis.
    Vector up; // Unit up, across the height.
    Mesh mesh; // The closed solid.
    Polyline right; // The face at +direction x up.
    Polyline left; // The face at -direction x up.
    Polyline
        bottom; // The face at -up, start left, start right, end right, end left, creases between.
    Polyline top; // The face at +up, corner i above corner i of bottom.
};

/// Whether the point lies within every bound of the face, a hair of tolerance for a hit on a miter.
bool within_bounds(const CutFace& face, const Point& point);

/// Where the ray from origin along out first leaves the cut faces within their bounds, and through which face.
std::pair<Point, int>
ray_exit(const Point& origin, const Vector& out, const std::vector<CutFace>& cuts);

/// One end of a beam face: its corners from left to right and the planes the two corner rays left through.
struct EndCorners {
    std::vector<Point> corners; // Left corner, the crease when there is one, right corner.
    int left_plane = -1; // The plane the left corner ray left through.
    int right_plane = -1; // The plane the right corner ray left through.
};

/// One end of a beam face, with the crease between two exit planes as a middle corner when allowed.
EndCorners end_corners(
    const Point& left,
    const Point& right,
    const Vector& out,
    const std::vector<CutFace>& cuts,
    const Plane& face_plane,
    bool crease_allowed
);

/// The solid between two outlines of equal count: bottom, top and one quad per side.
Mesh outline_solid(const std::vector<Point>& bottom, const std::vector<Point>& top);

/// The corners as a closed outline.
Polyline closed_outline(const std::vector<Point>& corners);

/// The box beam from from_ref to to_ref, width across and height along up, each end cut at its faces.
ReciprocalBeam cut_beam(
    const Point& from_ref,
    const Point& to_ref,
    const Vector& dir,
    const Vector& up,
    double width,
    double height,
    const std::vector<CutFace>& cuts_from,
    const std::vector<CutFace>& cuts_to
);

/// One straight beam per frame half-edge, cut at its two end planes.
std::vector<ReciprocalBeam> frame_beams(
    const BoundaryFrame& frame,
    const std::vector<std::vector<size_t>>& faces,
    const std::map<size_t, Point>& points,
    double width,
    double height
);

}
