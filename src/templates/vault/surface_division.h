#pragma once
#include "pch.h"
#include "brep.h"
#include "mesh.h"
#include "nurbssurface.h"

using namespace session_cpp;

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Canonical Vault NURBS & Sewn Multi-Surface Builders
// ═══════════════════════════════════════════════════════════════════════════

/// Builds an arched anticlastic saddle vault NURBS surface over span_x by span_y.
NurbsSurface saddle_vault_surface(double span_x, double span_y, double arch_rise, double sag);

/// Builds a doubly-curved pillow dome NURBS surface over span_x by span_y reaching crown rise.
NurbsSurface pillow_dome_surface(double span_x, double span_y, double rise);

/// Builds an arched toroidal / annular vault NURBS surface over sector_degrees.
NurbsSurface torus_vault_surface(
    double major_radius,
    double half_width,
    double arch_rise,
    double sector_degrees = 360.0
);

/// Builds a curved funnel / catenoid column-vault NURBS surface from throat_radius up to outer_radius.
NurbsSurface funnel_vault_surface(double outer_radius, double throat_radius, double height);

/// Builds four sewn doubly-curved NURBS vault webs meeting along the four diagonal groin seams of a square bay.
std::vector<NurbsSurface> groin_vault_surfaces(double span, double crown_rise, double arch_rise);

/// Builds a multi-patch sewn funnel-canopy vault of lobes four-sided NURBS patches meeting along radial seams.
std::vector<NurbsSurface> sewn_funnel_surfaces(
    double outer_radius,
    double throat_radius,
    double height,
    double petal_rise,
    int lobes = 4
);

// ═══════════════════════════════════════════════════════════════════════════
// SurfaceDivision
// ═══════════════════════════════════════════════════════════════════════════

/// Sewn NURBS surface sheet and NGon subdivisions (quads, diamonds, hexagons, dual hexagons, bricks, Chebyshev net).
class SurfaceDivision {
public:
    const std::vector<NurbsSurface> surfaces; // Four-sided NURBS patches sewn along shared boundary curves.
    const double size_u; // Target cell step along the primary strip direction.
    const double size_v; // Target cell step along the secondary strip direction.

    /// Builds the sewn division grid over one or multiple four-sided NURBS surfaces.
    SurfaceDivision(const std::vector<NurbsSurface>& surfaces, double size_u, double size_v);

    /// Builds the sewn division grid from the underlying untrimmed surfaces of a BRep.
    static SurfaceDivision from_brep(const BRep& brep, double size_u, double size_v);

    /// Quad grid across the sewn sheet, with odd courses shifted by stagger when non-zero.
    Mesh quads(double stagger = 0.0) const;

    /// Rhombic diamond mesh across the sewn sheet, merging half-diamonds seamlessly across surface seams.
    Mesh diamonds() const;

    /// Hexagonal NGon mesh across the sewn sheet, merging half-hexagons seamlessly across surface seams.
    Mesh hexagons(double side = 1.0 / 3.0) const;

    /// Isotropic trivalent dual hexagonal mesh across multi-patch sewn seams and star singularities.
    Mesh dual_hexagons() const;

    /// Variable-count staggered brick courses adapting brick count to each course's arc length with conformal shared edges.
    Mesh bricks(double length, double stagger = 0.5) const;

    /// Equal-edge-length Chebyshev compass net walked from the surface centre at angle_degrees.
    Mesh chebyshev(double edge, double angle_degrees = 0.0) const;

private:
    /// Cumulative arc-length table along a boundary curve of a NURBS surface.
    struct ArcTable {
        std::vector<double> fractions; // Normalized [0, 1] curve parameter samples.
        std::vector<double> lengths; // Cumulative arc lengths from start.
        double total = 0.0; // Total curve length.

        /// Returns the normalized [0, 1] parameter at fraction s in [0, 1] of total arc length.
        double at_fraction(double s) const;
    };

    /// Sewn strip divisions and orientation of one patch in the sheet.
    struct Grid {
        int u = 2; // Even division count along surface u.
        int v = 2; // Even division count along surface v.
        bool flip_normal = false; // Whether the sheet normal opposes dS/du x dS/dv.
        bool transpose_uv = false; // Whether surface u/v are swapped relative to patch 0's strip frame.
    };

    /// One quad cell of the sewn base grid.
    struct Cell {
        size_t surface = 0; // Index in surfaces.
        std::array<size_t, 4> corners = {0, 0, 0, 0}; // Welded vertex indices CCW around sheet normal.
    };

    std::vector<std::array<std::pair<size_t, bool>, 4>> _sides; // Per patch side: (canonical side id, reversed).
    std::vector<ArcTable> _side_arcs; // Arc-length table per canonical sewn side.
    std::vector<Grid> _grids; // Division counts and orientation per patch.
    std::vector<Point> _points; // Welded grid vertex positions.
    std::vector<Vector> _normals; // Unit sheet normals per welded grid vertex.
    std::vector<int> _parities; // Global bipartite (i + j) % 2 parity per welded vertex.
    std::vector<Cell> _cells; // Sewn quad cells.

    std::vector<std::array<std::pair<size_t, bool>, 4>> compute_sides(
        std::vector<ArcTable>& out_arcs
    ) const;
    std::vector<Grid> compute_grids() const;
    void compute_cells();

    /// Evaluates surface s at continuous grid coordinates (gi, gj) in [0, u] x [0, v] using Coons arc-length blending.
    std::pair<Point, Vector> evaluate(size_t s, double gi, double gj) const;

    /// Evaluates surface s in strip-aligned coordinates (a, b), swapping u and v when transpose_uv is set.
    std::pair<Point, Vector> evaluate_strip(size_t s, double a, double b) const;
};

}
