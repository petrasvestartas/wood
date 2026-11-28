#pragma once
#include "session.h"
#include "nurbssurface.h"
#include "mesh.h"
#include "tolerance.h"
#include "line.h"
#include "intersection.h"
#include "xform.h"
#include "src/templates/reciprocal/reciprocal_boundary.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace laika_cpp;

/// Test surfaces for the reciprocal templates, the meshes on them and the frame ups they give.
namespace wood_reciprocal {

// ═══════════════════════════════════════════════════════════════════════════
// Surfaces
// ═══════════════════════════════════════════════════════════════════════════

/// The nx by ny quad grid over width by depth, lifted by height times two half sine waves.
Mesh sinusoidal_dome_mesh(int nx, int ny, double width, double depth, double height);

/// A hyperbolic paraboloid over width by depth, two opposite corners lifted by rise.
NurbsSurface hypar_surface(double width, double depth, double rise);

/// The height of the inner control points of a bicubic patch whose centre reaches rise.
double bicubic_inner_height(double rise);

/// A bicubic dome over a rectangle whose sides bow out by bulge, its centre at rise.
NurbsSurface pillow_dome_surface(double width, double depth, double rise, double bulge);

/// The point on the circle of radius at angle, in z = 0.
Point circle_point(double radius, double angle);

/// The counter-clockwise unit tangent of a circle at angle.
Vector circle_tangent(double angle);

/// A bicubic dome over the disc of radius, four cubic arcs as its boundary, its centre at rise.
NurbsSurface disc_dome_surface(double radius, double rise);

/// A Scherk-like saddle on a plan bowed out by bulge, its two low sides level when flat_low_edges.
NurbsSurface scherk_surface(
    double width,
    double depth,
    double height,
    double bulge,
    bool flat_low_edges = true,
    double reach = 1.2
);

/// One of the serialized Annen arches from the data directory.
NurbsSurface annen_surface(const std::string& json_path, size_t index);

/// The surface with every boundary control row projected onto the plane fitted through its boundary curve.
NurbsSurface flatten_surface_sides(const NurbsSurface& surface);

// ═══════════════════════════════════════════════════════════════════════════
// Meshes
// ═══════════════════════════════════════════════════════════════════════════

/// The cumulative length along one mid iso-curve, to pick parameters at equal spacing on the surface.
struct ArcLengthMap {
    std::vector<double> parameters; // Sampled parameters, ascending.
    std::vector<double> lengths; // Length from the first sample to each sampled parameter.
    double total = 0.0; // The length of the whole iso-curve.

    /// The map of the mid iso-curve of the surface along dir.
    static ArcLengthMap of(const NurbsSurface& surface, int dir, int samples = 400);

    /// The parameter at which the iso-curve has covered length.
    double parameter_at(double length) const;
};

/// Whether faces on the surface must be reversed so their normals point up at the domain centre.
bool faces_reversed_on(const NurbsSurface& surface);

/// The quad grid on the surface with cells about target_edge long, spaced at equal arc length.
Mesh quad_mesh_from_surface(const NurbsSurface& surface, double target_edge);

/// A quad mesh over the disc of radius: a central square and four patches out to the circle, lifted onto the dome.
Mesh disc_quad_mesh(
    const NurbsSurface& dome, double radius, double target_edge, double square_ratio = 0.5
);

/// A convex polygon clipped to the side of an axis-aligned line at limit, above it when keep_above.
std::vector<std::pair<double, double>> clip_polygon(
    const std::vector<std::pair<double, double>>& polygon, int axis, double limit, bool keep_above
);

/// Staggered pointy-top hexagons of edge about target_edge filling width by depth, clipped to it.
std::vector<std::vector<std::pair<double, double>>>
clipped_hex_lattice(double width, double depth, double target_edge);

/// The plan cells welded within a thousandth: their plan points at z = 0 and faces, reversed when asked.
std::pair<std::vector<Point>, std::vector<std::vector<size_t>>>
welded_cells(const std::vector<std::vector<std::pair<double, double>>>& cells, bool reversed);

/// A hexagonal mesh on the surface: the clipped lattice in its arc-length plane, lifted onto it.
Mesh hex_mesh_from_surface(const NurbsSurface& surface, double target_edge);

/// The sinusoidal dome as a hexagonal mesh: the clipped lattice over its plan, lifted by its height function.
Mesh
sinusoidal_dome_hex_mesh(double width, double depth, double height, double target_edge);

/// True when the first dual corner turns before the second about the normal.
bool angle_less(
    const std::pair<double, std::pair<std::string, Point>>& p,
    const std::pair<double, std::pair<std::string, Point>>& q
);

/// The dual of the triangulated quads: one cell per vertex through its triangle centroids, half cells along a straight side.
Mesh dual_hex_mesh(const Mesh& quads);

// ═══════════════════════════════════════════════════════════════════════════
// Frame ups
// ═══════════════════════════════════════════════════════════════════════════

/// The side of the domain a parameter pair lies nearest to: u start, u end, v start, v end.
int nearest_domain_side(const NurbsSurface& surface, double u, double v);

/// One plane per side of the surface, fitted through the side's boundary vertices, turned to its average face normal.
std::array<Plane, 4> side_planes(const Mesh& mesh, const NurbsSurface& surface);

/// One up per naked edge, one tilt per side of the surface out of its plane, or from side_vectors when given.
std::map<std::pair<size_t, size_t>, Vector> side_tilt_boundary_ups(
    const Mesh& mesh,
    const NurbsSurface& surface,
    const std::optional<std::array<Vector, 4>>& side_vectors = std::nullopt
);

/// The through priority per naked edge: 1 on the u sides of the domain when u_sides_through, 0 on the others.
std::map<std::pair<size_t, size_t>, int>
through_side_priority(const Mesh& mesh, const NurbsSurface& surface, bool u_sides_through = true);

}
