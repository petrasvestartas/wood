#pragma once
#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Cavity border boards
// ═══════════════════════════════════════════════════════════════════════════

/// Geometry and corner-plane rules of one perpendicular cavity-closing border board.
struct CavityBorderSpec {
    Plane facet_plane; // Inner facet reference plane, z_axis from inner to outer layer.
    Plane boundary_plane; // Outward-facing side or separation plane of the boundary edge.
    Plane start_plane; // Outward-facing side plane of the preceding facet edge.
    Plane end_plane; // Outward-facing side plane of the following facet edge.
    double inner_level = 0.0; // Board start height along facet_plane.z_axis().
    double outer_level = 0.0; // Board end height along facet_plane.z_axis().
    double thickness = 45.0; // Board thickness inward from boundary_plane.
    double outer_gap = 0.0; // Setback of the board's outer face inward from boundary_plane.
    bool miter_start = false; // Miters start end against an adjacent border board on start_plane.
    double start_neighbor_gap = 0.0; // Neighbor board outer_gap when miter_start is true.
    double start_neighbor_thickness = 45.0; // Neighbor board thickness when miter_start is true.
    bool miter_end = false; // Miters end against an adjacent border board on end_plane.
    double end_neighbor_gap = 0.0; // Neighbor board outer_gap when miter_end is true.
    double end_neighbor_thickness = 45.0; // Neighbor board thickness when miter_end is true.
    double start_setback = 0.0; // Normal clearance inward from the resolved start cap plane.
    double end_setback = 0.0; // Normal clearance inward from the resolved end cap plane.
    std::optional<Plane>
        start_bisector; // Optional outward bisector to non-adjacent facet at start.
    std::optional<Plane> end_bisector; // Optional outward bisector to non-adjacent facet at end.
    double min_length = 80.0; // Shortest longitudinal board span kept.
};

/// Outward dihedral bisector plane through corner_vertex between facet_normal and other_normal.
std::optional<Plane> compute_cavity_bisector_plane(
    const Point& corner_vertex, const Vector& facet_normal, const Vector& other_normal
);

/// Tightens primary cap against an optional bisector cap across all four longitudinal board rails.
Plane tighten_cavity_cap(
    const Plane& cap,
    const std::optional<Plane>& bisector,
    double setback,
    const Plane& cavity_inner,
    const Plane& cavity_outer,
    const Plane& board_inner,
    const Plane& board_outer,
    const Vector& u_out
);

/// Outward-facing end-cap plane of a cavity border board at neighbor_plane, mitered when miter is true.
std::optional<Plane> compute_cavity_border_cap(
    const Plane& cavity_mid,
    const Plane& boundary_plane,
    double outer_gap,
    double thickness,
    const Plane& neighbor_plane,
    bool miter,
    double neighbor_gap,
    double neighbor_thickness,
    double setback
);

/// Resolved start and end cap planes of a cavity border board after miter and bisector tightening.
std::optional<std::pair<Plane, Plane>> resolve_cavity_border_caps(
    const CavityBorderSpec& spec,
    const Plane& cavity_inner,
    const Plane& cavity_outer,
    const Plane& cavity_mid,
    const Plane& board_inner,
    const Plane& board_outer
);

/// Perpendicular cavity-closing border board reaching its corner planes, or null if < min_length.
std::shared_ptr<FacePlanarElement>
compute_cavity_border_board(const CavityBorderSpec& spec, const std::string& name);
