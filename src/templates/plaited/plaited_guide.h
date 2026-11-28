#pragma once
#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_plaited {

// ═══════════════════════════════════════════════════════════════════════════
// Pattern and guide tables
// ═══════════════════════════════════════════════════════════════════════════

/// Decomposition rule turning each extrados polygon into four-sided plaited cells.
enum class PlaitedPattern {
    Abeille, // Four-way checkerboard on quad meshes.
    Sphera, // Three-way rhombille on hexagonal meshes.
    LeonardoStar, // Universal n-kite star on arbitrary polygon meshes.
};

/// One four-sided plaited unit on the extrados: edges 0 and 2 tilt inward, edges 1 and 3 tilt outward.
struct PlaitedCell {
    std::array<Point, 4> corners; // Counter-clockwise extrados corners around normal.
    std::array<size_t, 4> vertex_ids = {0, 0, 0, 0}; // Topological vertex keys for seam pairing.
    std::array<int, 4> seam_ids = {-1, -1, -1, -1}; // Seam index of each local edge 0..3.
    std::array<std::pair<size_t, size_t>, 4> mesh_edges; // Original mesh edge key when on perimeter, else {SIZE_MAX, SIZE_MAX}.
    Point centre; // Extrados centroid of the four corners.
    Vector normal; // Unit outward surface normal from intrados to extrados.
    int source_face = -1; // Owning face index in the input mesh.
};

/// One shared seam between two adjacent plaited cells, or a naked boundary edge when cell_b is -1.
struct PlaitedSeam {
    Point start; // Extrados start point along cell_a's half-edge.
    Point end; // Extrados end point along cell_a's half-edge.
    Point midpoint; // Midpoint of start and end on the extrados.
    Vector tangent; // Unit vector from start to end.
    Vector normal; // Unit outward surface normal perpendicular to tangent.
    Plane plane_for_a; // Bevel plane turned inward toward cell_a.
    Plane plane_for_b; // Exact opposite bevel plane turned inward toward cell_b.
    Plane relief_for_a; // Radial step plane turned inward toward cell_a.
    Plane relief_for_b; // Exact opposite radial step plane turned inward toward cell_b.
    int cell_a = -1; // First owning cell index.
    int edge_a = -1; // Local edge 0..3 on cell_a.
    int sign_a = -1; // -1 for inward side cheek, +1 for outward bearing end on cell_a.
    int cell_b = -1; // Second owning cell index, or -1 on a naked edge.
    int edge_b = -1; // Local edge 0..3 on cell_b, or -1 on a naked edge.
    int sign_b = 1; // Opposite of sign_a on interior seams.
};

/// The Leonardo reciprocal bar axis of one plaited cell, running from bearing end 3 to bearing end 1.
struct PlaitedAxis {
    Line line; // Reference axis between the midpoints of edges 3 and 1.
    Vector direction; // Unit axis vector from start to end.
    Vector up; // Unit surface normal perpendicular to direction.
    Point centre; // Midpoint of line.
};

// ═══════════════════════════════════════════════════════════════════════════
// Surface meshes for flat floors, arches and doubly curved shells
// ═══════════════════════════════════════════════════════════════════════════

/// Flat quad mesh centred on the origin at height z, optionally sheared along x by skew radians.
Mesh flat_quad_mesh(
    double width,
    double depth,
    int nx,
    int ny,
    double skew = 0.0,
    double z = 0.0
);

/// Flat rectangular honeycomb of whole hexagons of circumradius radius centred on the origin.
Mesh flat_hex_grid_mesh(
    int cols,
    int rows,
    double radius,
    double z = 0.0
);

/// Flat hexagonal patch of concentric hexagon rings of circumradius radius centred on the origin.
Mesh flat_hex_mesh(int rings, double radius, double z = 0.0);

/// Circular arch or barrel quad mesh of span, rise and depth, optionally skewed or twisted.
Mesh arch_quad_mesh(
    double span,
    double rise,
    double depth,
    int courses,
    int rings,
    double skew = 0.0,
    double twist = 0.0
);

/// Cylindrical barrel vault tiled by whole hexagons across the arc and along the depth.
Mesh barrel_hex_mesh(
    double span,
    double rise,
    double depth,
    int courses,
    int rings
);

/// Spherical cap of span and rise tiled by concentric rings of whole hexagons, optionally extended across a circular base.
Mesh spherical_cap_hex_mesh(double span, double rise, int rings, bool circular_rim = false);

/// Planar-faced Goldberg polyhedron dome of pentagons and hexagons clipped to a level circular base.
Mesh geodesic_dome_mesh(double radius, int frequency = 2, double clip_ratio = 0.42);

/// Doubly curved quad surface over width by depth: synclastic dome at saddle 0, hypar at saddle 1.
Mesh doubly_curved_quad_mesh(
    double width,
    double depth,
    double rise,
    int nx,
    int ny,
    double saddle = 0.0
);

// ═══════════════════════════════════════════════════════════════════════════
// PlaitedGuide
// ═══════════════════════════════════════════════════════════════════════════

/// Geometric guide of a plaited stereotomic or Leonardo reciprocal system on any polygon mesh.
class PlaitedGuide : public WoodSession {
public:
    const Mesh mesh; // Reference extrados mesh, face normals pointing outward.
    const PlaitedPattern pattern; // Cell decomposition rule: Abeille, Sphera or LeonardoStar.
    const double thickness; // Structural depth from extrados inward along -normal.
    const double bevel; // Signed joint tilt angle in radians around each interior seam line.
    const double relief_depth; // Intrados rib truncation depth in [0, thickness); 0 for full-depth bevels.
    const bool flip_chirality; // Reverses pinwheel swirl and checkerboard parity when true.
    const bool bevel_naked_edges; // Tilts naked perimeter edges by bevel when true, flush radial when false.
    const double opening; // Void fraction in [0, 0.85] narrowing blocks into open hot-wire bars.
    const bool flat_base; // Terminates springing edges horizontally on the lowest mesh height when true.

    /// Computes plaited cells, shared seam planes and reciprocal bar axes on mesh.
    PlaitedGuide(
        const Mesh& mesh,
        PlaitedPattern pattern = PlaitedPattern::Abeille,
        double thickness = 180.0,
        double bevel = 0.45,
        double relief_depth = 0.0,
        bool flip_chirality = false,
        bool bevel_naked_edges = false,
        double opening = 0.0,
        bool flat_base = false
    );

    /// Every four-sided plaited cell on the extrados.
    const std::vector<PlaitedCell>& cells() const;

    /// Every shared interior and naked boundary seam with its bevel and relief planes.
    const std::vector<PlaitedSeam>& seams() const;

    /// One reciprocal bar axis per plaited cell, from bearing end 3 to bearing end 1.
    const std::vector<PlaitedAxis>& axes() const;

    /// Active side plane of local edge k (0..3) facing inward into cell_index.
    const Plane& side_plane(size_t cell_index, size_t edge_index) const;

    /// Un-inset mesh seam plane of local edge k (0..3) facing inward into cell_index.
    const Plane& seam_plane(size_t cell_index, size_t edge_index) const;

    /// Radial relief plane of local edge k (0..3) facing inward into cell_index.
    const Plane& relief_plane(size_t cell_index, size_t edge_index) const;

    /// Horizontal ground plane at the lowest mesh height when flat_base is active.
    const std::optional<Plane>& base_plane() const;

    /// Neighbouring cell index across local edge k (0..3), or -1 when naked.
    int neighbor_cell(size_t cell_index, size_t edge_index) const;

private:
    std::optional<Plane> _base_plane; // Horizontal springing plane when flat_base is active.
    std::vector<PlaitedCell> _cells; // Extrados four-sided plaited cells.
    std::vector<PlaitedSeam> _seams; // Shared interior and naked boundary seams.
    std::vector<PlaitedAxis> _axes; // Reciprocal bar axes per cell.
    std::vector<std::array<Plane, 4>> _open_planes; // Inset hot-wire bar side planes when opening > 0.

    std::optional<Plane> compute_base_plane() const;
    std::vector<PlaitedCell> compute_cells() const;
    std::vector<PlaitedCell> compute_abeille_cells() const;
    std::vector<PlaitedCell> compute_sphera_cells() const;
    std::vector<PlaitedCell> compute_star_cells() const;
    std::vector<PlaitedSeam> compute_seams(std::vector<PlaitedCell>& cells) const;
    std::vector<PlaitedAxis> compute_axes() const;
    std::vector<std::array<Plane, 4>> compute_open_planes() const;
    void draw();
};

} // namespace wood_plaited
