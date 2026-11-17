#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Cavity border boards
// ═══════════════════════════════════════════════════════════════════════════

/// Geometry and clearances of one perpendicular cavity-closing border board on a planar facet edge.
struct CavityBorderSpec {
    Plane facet_plane; // Inner facet reference plane, z_axis from inner to outer layer.
    Plane boundary_plane; // Outward-facing side or separation plane of the boundary edge.
    Plane start_plane; // Outward-facing side plane of the preceding facet edge.
    Plane end_plane; // Outward-facing side plane of the following facet edge.
    std::array<Point, 2> inner_edge; // Edge start and end on the inner plate's cavity face.
    std::array<Point, 2> outer_edge; // Edge start and end on the outer plate's cavity face.
    double inner_level = 0.0; // Board start height along facet_plane.z_axis().
    double outer_level = 0.0; // Board end height along facet_plane.z_axis().
    double thickness = 45.0; // Board thickness inward from boundary_plane.
    double outer_gap = 0.0; // Setback of the board's outer face inward from boundary_plane.
    double start_setback = 0.0; // Perpendicular clearance inward from start_plane.
    double end_setback = 0.0; // Perpendicular clearance inward from end_plane.
    double min_length = 80.0; // Shortest longitudinal board span kept.
};

/// A perpendicular cavity-closing border board between two shell layers, null when shorter than min_length.
inline std::shared_ptr<FacePlanarElement>
compute_cavity_border_board(const CavityBorderSpec& spec, const std::string& name) {

    const Vector span = spec.inner_edge[1] - spec.inner_edge[0];

    if (spec.outer_level - spec.inner_level < 5.0 || span.magnitude() < spec.min_length)
        return nullptr;

    const Vector along = span.normalized();
    const Point mid = Point::centroid(
        {spec.inner_edge[0], spec.inner_edge[1], spec.outer_edge[0], spec.outer_edge[1]}
    );
    const Plane board_outer = spec.boundary_plane.translate_by_normal(-spec.outer_gap);
    const Plane board_inner =
        spec.boundary_plane.translate_by_normal(-(spec.outer_gap + spec.thickness));
    const Plane cavity_inner = spec.facet_plane.translate_by_normal(spec.inner_level);
    const Plane cavity_outer = spec.facet_plane.translate_by_normal(spec.outer_level);
    const Plane cross = Plane::from_point_normal(mid, along);
    const std::optional<Point> b0 =
        Intersection::plane_plane_plane(board_inner, cavity_inner, cross);
    const std::optional<Point> b1 =
        Intersection::plane_plane_plane(board_inner, cavity_outer, cross);
    const std::optional<Point> t0 =
        Intersection::plane_plane_plane(board_outer, cavity_inner, cross);
    const std::optional<Point> t1 =
        Intersection::plane_plane_plane(board_outer, cavity_outer, cross);

    if (!b0 || !b1 || !t0 || !t1)
        return nullptr;

    double s_min =
        std::max((spec.inner_edge[0] - mid).dot(along), (spec.outer_edge[0] - mid).dot(along)) +
        15.0;
    double s_max =
        std::min((spec.inner_edge[1] - mid).dot(along), (spec.outer_edge[1] - mid).dot(along)) -
        15.0;
    const std::array<Point, 4> section = {*b0, *b1, *t0, *t1};
    const double start_dot = along.dot(spec.start_plane.z_axis());
    const double end_dot = along.dot(spec.end_plane.z_axis());

    if (start_dot < -1e-6)
        for (const Point& corner : section)
            s_min = std::max(
                s_min, (-spec.start_setback - spec.start_plane.signed_distance(corner)) / start_dot
            );

    if (end_dot > 1e-6)
        for (const Point& corner : section)
            s_max = std::min(
                s_max, (-spec.end_setback - spec.end_plane.signed_distance(corner)) / end_dot
            );

    if (s_max - s_min < spec.min_length)
        return nullptr;

    const Plane start_cap = Plane::from_point_normal(mid + along * s_min, -along);
    const Plane end_cap = Plane::from_point_normal(mid + along * s_max, along);
    const std::vector<Plane> sides = {cavity_inner, end_cap, cavity_outer, start_cap};
    const Polyline bottom = Polyline::from_planes(sides, board_inner);
    const Polyline top = Polyline::from_planes(sides, board_outer);

    return std::make_shared<FacePlanarElement>(bottom, top, name);
}
