#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Face normals
// ═══════════════════════════════════════════════════════════════════════════

/// Newell's unit normal of the face through points given by its corner indices.
Vector newell_normal(const std::vector<Point>& points, const std::vector<size_t>& face);

// ═══════════════════════════════════════════════════════════════════════════
// Mitered plates
// ═══════════════════════════════════════════════════════════════════════════

/// One plate per mesh face mitered at every fold, its sharp corners cut back, free edges in edge_planes cut flush.
std::vector<std::shared_ptr<FacePlanarElement>> mitered_plates(
    const Mesh& mesh,
    double thickness,
    double chamfer_bottom,
    double chamfer_top,
    double chamfer_angle,
    const std::vector<Plane>& edge_planes = {}
);

// ═══════════════════════════════════════════════════════════════════════════
// Coplanar faces
// ═══════════════════════════════════════════════════════════════════════════

/// The root of face f's group of coplanar faces, halving the path to it on the way.
size_t coplanar_root(std::vector<size_t>& group, size_t f);

/// The boundary loop of a group of coplanar faces, a straight corner dropped unless another group turns there.
std::vector<size_t> coplanar_loop(
    const std::vector<Point>& points,
    const std::vector<std::vector<size_t>>& faces,
    const std::vector<size_t>& members,
    const std::map<std::pair<size_t, size_t>, size_t>& owner,
    const std::map<size_t, std::set<size_t>>& users,
    std::vector<size_t>& group
);

/// The mesh with every group of neighbouring coplanar faces merged into one face, its straight corners dropped.
Mesh merged_coplanar(const Mesh& mesh);

// ═══════════════════════════════════════════════════════════════════════════
// Exact offset
// ═══════════════════════════════════════════════════════════════════════════

/// One plate per mesh face by the exact face offset.
class OffsetPlates {
private:
    /// A face at a vertex with the vertices before and after it on that face.
    struct Corner {
        size_t face; // The face.
        size_t before; // The vertex before on the face.
        size_t after; // The vertex after on the face.
    };

    /// A plane n . x = c of a vertex's cone.
    struct Cut {
        Vector normal; // The unit normal n.
        double c; // The plane's offset along n.
        std::optional<size_t>
            face; // The face it moves; none for a free edge or the placeholder of an open fan.
    };

    /// The planes of one fan of faces around a vertex.
    struct Cone {
        size_t vertex; // The vertex.
        std::vector<size_t>
            faces; // The fan's faces in order, each next across the edge out of the last.
        std::vector<Cut> cuts; // Every face's moved plane, an open fan closed by one placeholder.
        std::vector<Vector>
            edges; // Unit edges from the vertex, edges[i] between cut i and the next round the cycle.
        std::optional<Cut> free_in; // An open fan's free edge in.
        std::optional<Cut> free_out; // An open fan's free edge out.
    };

    const double _thickness; // mm, every plate.
    const std::vector<Plane> _edge_planes; // Planes that cut the free edges lying in them.
    std::vector<Point> _points; // The merged mesh's vertices.
    std::vector<std::vector<size_t>> _faces; // The merged mesh's faces.
    std::vector<Vector> _normals; // Every face's normal, the side its plate's inner face lies on.
    std::vector<std::vector<Corner>> _corners; // Per vertex the faces at it.
    std::map<std::pair<size_t, size_t>, std::vector<Point>>
        _chains; // Per face and vertex its inner points.
    std::vector<std::shared_ptr<FacePlanarElement>> _plates; // One plate per face.

public:
    /// The plates of mesh thickness thick, a free edge lying in one of edge_planes cut by it.
    OffsetPlates(const Mesh& mesh, double thickness, const std::vector<Plane>& edge_planes);

    /// One plate per face, named `plate_<i>`.
    const std::vector<std::shared_ptr<FacePlanarElement>>& plates() const;

private:
    /// Per vertex the faces at it with the vertex before and after it on each.
    std::vector<std::vector<Corner>> compute_corners() const;

    /// The fans of faces around vertex v, each started at a face with a free edge in when there is one.
    std::vector<std::vector<size_t>> compute_fans(size_t v) const;

    /// The cone of fan at vertex v: every face's plane moved by the thickness and an open fan's free edges.
    Cone compute_cone(size_t v, const std::vector<size_t>& fan) const;

    /// Every fan face's inner points by the triangulation most faces take, its points nearest the vertex.
    std::vector<std::vector<Point>> compute_split(const Cone& cone) const;

    /// Face i's inner points clipped to every edge plane its vertex stands on, so a foot ends flush.
    std::vector<Point> compute_foot(const Cone& cone, size_t i, std::vector<Point> inner) const;

    /// One plate per face, its inner outline the chains of its corners over its outline.
    std::vector<std::shared_ptr<FacePlanarElement>> compute_plates() const;

    /// The plane of the free edge a b of face f.
    Cut free_edge(size_t a, size_t b, size_t f) const;

    /// Whether face i's inner outline, with the rays along its edges, is simple and turns as its corner does.
    bool valid(const Cone& cone, const std::vector<Point>& inner, size_t i) const;

    /// The cone's edge from the vertex between cuts a and b.
    static Vector between(const Cone& cone, size_t a, size_t b);

    /// Where cut i meets the cuts a and b, the placeholder taken as the free edge on its side.
    static std::optional<Point> point_of(const Cone& cone, size_t i, size_t a, size_t b);

    /// Face i's inner points in a triangulation, one where every two consecutive planes sharing a triangle with it meet.
    static std::optional<std::vector<Point>>
    chain(const Cone& cone, const std::vector<std::array<size_t, 3>>& triangles, size_t i);

    /// Whether two cuts are one plane.
    static bool same(const Cut& a, const Cut& b);

    /// The point where three planes meet, none when two are parallel.
    static std::optional<Point> meet(const Cut& a, const Cut& b, const Cut& c);

    /// The triangulations of the polygon of corners first to last, each as triangles of corner indices.
    static std::vector<std::vector<std::array<size_t, 3>>>
    triangulations(size_t first, size_t last);

    /// The coordinates of offset in the plane frame u, w.
    static std::array<double, 2> flat(const Vector& offset, const Vector& u, const Vector& w);

    /// The signed area of a polygon in a plane frame.
    static double area(const std::vector<std::array<double, 2>>& polygon);

    /// Twice the signed area of the triangle a b c, positive when c lies left of a b.
    static double side(
        const std::array<double, 2>& a,
        const std::array<double, 2>& b,
        const std::array<double, 2>& c
    );

    /// The height of point over plane along its unit normal.
    static double height(const Point& point, const Plane& plane, const Vector& normal);
};

/// One plate per mesh face by the exact face offset, a free edge lying in one of edge_planes cut by it.
std::vector<std::shared_ptr<FacePlanarElement>>
offset_plates(const Mesh& mesh, double thickness, const std::vector<Plane>& edge_planes = {});

// ═══════════════════════════════════════════════════════════════════════════
// Report
// ═══════════════════════════════════════════════════════════════════════════

/// The folding examples' line on the plates: their count, the shortest edge, and whether each is thickness thick and planar.
std::string
plate_report(const std::vector<std::shared_ptr<FacePlanarElement>>& plates, double thickness);
