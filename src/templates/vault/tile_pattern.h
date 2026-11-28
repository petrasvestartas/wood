#pragma once
#include "src/templates/vault/vault.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Tile Remeshing, Regularization & Matching Metrics
// ═══════════════════════════════════════════════════════════════════════════

/// Distortion, matching, and planarity diagnostics of a field-guided free-form brick/tile pattern.
struct TilePatternMetrics {
    size_t quad_faces = 0; // Number of near-square base faces after QuadriFlow remeshing.
    size_t singularities = 0; // Number of irregular interior vertices (valence != 4).
    size_t full_bricks = 0; // Number of matched two-quad full bricks.
    size_t half_tiles = 0; // Number of unmatched single-quad half-tiles.
    size_t unmatched_interior = 0; // Number of unmatched non-boundary faces.
    size_t parallel_pairs = 0; // Count of side-by-side parallel brick joints (stack bond).
    size_t running_pairs = 0; // Count of 1-quad shifted running-bond brick pairs.
    size_t herringbone_pairs = 0; // Count of 90-degree stepped perpendicular herringbone pairs.
    double bip_objective = 0.0; // Binary Integer Program objective score achieved.
    double raw_edge_error = 0.0; // Mean relative edge length error |l - h| / h before ShapeUp.
    double mean_edge_error = 0.0; // Mean relative edge length error |l - h| / h after ShapeUp.
    double max_edge_error = 0.0; // Maximum relative edge length error |l - h| / h after ShapeUp.
    double raw_angle_error_deg = 0.0; // Mean quad corner deviation from 90 degrees before ShapeUp.
    double mean_angle_error_deg = 0.0; // Mean quad corner deviation from 90 degrees after ShapeUp.
    double max_angle_error_deg = 0.0; // Maximum quad corner deviation from 90 degrees after ShapeUp.
    double mean_surface_drift = 0.0; // Mean distance from regularized vertices to input shell in mm.
    double max_surface_drift = 0.0; // Maximum distance from regularized vertices to input shell in mm.
    double side_planarity = 0.0; // Worst side-face non-planarity in mm.
    double cap_planarity = 0.0; // Worst top/bottom cap non-planarity in mm.
};

/// Result of Binary Integer Program brick matching on the dual graph of a quad mesh.
struct BrickMatchingResult {
    std::vector<std::pair<size_t, size_t>> matched_pairs; // Matched adjacent face pairs (fa, fb).
    std::vector<size_t> unmatched_faces; // Faces left as single half-tiles (concentrated at borders).
    double objective = 0.0; // Achieved BIP objective value.
    size_t unmatched_interior = 0; // Count of unmatched interior (non-border) faces.
    size_t parallel_pairs = 0; // Count of active parallel 2x2 stack-bond pairs.
    size_t running_pairs = 0; // Count of active 1-step shifted running-bond pairs.
    size_t herringbone_pairs = 0; // Count of active 90-degree L-stepped herringbone pairs.
};

// ═══════════════════════════════════════════════════════════════════════════
// Pure-C++ Field-Guided QuadriFlow, ShapeUp & BIP Tile Matching
// ═══════════════════════════════════════════════════════════════════════════

/// Evaluates edge-length and 90-degree corner-angle distortion metrics on a quad mesh against target_edge.
void evaluate_quad_distortion(
    const Mesh& quad_mesh,
    double target_edge,
    double& mean_edge_error,
    double& max_edge_error,
    double& mean_angle_error_deg,
    double& max_angle_error_deg,
    size_t& singularity_count
);

/// Field-guided isotropic quad remeshing (4-RoSy principal-curvature hierarchy + extrinsic position field + max-flow integer jumps).
Mesh quadriflow_remesh(
    const Mesh& shell_mesh,
    double target_edge,
    double orientation_degrees = 0.0,
    bool align_boundary = true
);

/// Local-global ShapeUp regularization pushing polygon faces toward squares of side target_edge (Bouaziz et al. 2012 / igl::shapeup).
Mesh shapeup_regularize(
    const Mesh& quad_mesh,
    double target_edge,
    int iterations = 50,
    double shape_weight = 1.0,
    double close_weight = 0.06,
    double smooth_weight = 0.02
);

/// Solves the Binary Integer Program on the quad dual graph for staggered, running-bond, or herringbone brick matching (Du et al. 2023).
BrickMatchingResult match_brick_tiles(
    const Mesh& quad_mesh,
    int pattern = TileVaultGuide::RUNNING_BOND,
    double penalty_weight = 0.35,
    double reward_weight = 0.30,
    double border_weight = 0.60
);

/// Dissolves matched dual edges of quad_mesh into 6-vertex conformal brick faces while retaining unmatched half-tiles.
Mesh merge_matched_brick_mesh(const Mesh& quad_mesh, const BrickMatchingResult& matching);

/// Generates constant-bed-height geodesic coordinate courses with orthogonal running-bond bricks (Adiels, Ander & Williams, IASS 2017).
Mesh geodesic_brick_pattern(
    const Mesh& shell_mesh,
    double bed_height,
    double brick_length,
    bool polar_from_holes = true
);

/// Builds the 3D tile or planar-sided block elements for guide and records quality metrics when non-null.
std::vector<std::shared_ptr<Element>> tile_vault(
    const TileVaultGuide& guide,
    TilePatternMetrics* metrics = nullptr,
    Mesh* out_quad_mesh = nullptr,
    Mesh* out_brick_mesh = nullptr
);

// ═══════════════════════════════════════════════════════════════════════════
// TileVault Session
// ═══════════════════════════════════════════════════════════════════════════

/// Free-form brick or tile shell session combining QuadriFlow remeshing, ShapeUp regularization, and BIP tile matching.
class TileVault : public WoodSession {
public:
    const TileVaultGuide guide; // Shell geometry, tile dimensions, bond pattern, and materialisation mode.

    /// Remeshes the shell, regularizes squares, solves BIP brick matching, extrudes tiles/blocks, and computes contacts.
    explicit TileVault(
        const TileVaultGuide& guide = TileVaultGuide::funicular_running_blocks(),
        const std::string& name = "vault_tile_pattern"
    );

    /// Returns the ShapeUp-regularized near-square quad mesh.
    const Mesh& quad_mesh() const;

    /// Returns the merged 6-vertex brick polygon mesh.
    const Mesh& brick_mesh() const;

    /// Returns full remeshing, ShapeUp, BIP matching, and planarity metrics.
    const TilePatternMetrics& metrics() const;

private:
    Mesh _quad_mesh; // Near-square field-aligned quad mesh.
    Mesh _brick_mesh; // Merged brick polygon mesh.
    TilePatternMetrics _metrics; // Quality and optimization diagnostics.
};

// ═══════════════════════════════════════════════════════════════════════════
// Calgary 2025 Trivalent Dual & Variable-Thickness Shell Materialisation
// ═══════════════════════════════════════════════════════════════════════════

/// Quality, thickness, and planarity diagnostics of a Calgary 2025 trivalent dual shell.
struct CalgaryDualMetrics {
    size_t dual_faces = 0; // Number of trivalent dual block polygons.
    size_t support_blocks = 0; // Number of blocks resting on the four pier support borders.
    size_t chamfer_cuts = 0; // Number of interior trivalent corner chamfer cuts applied.
    double min_thickness = 0.0; // Minimum interpolated vertex thickness in mm.
    double max_thickness = 0.0; // Maximum interpolated vertex thickness in mm.
    double side_planarity = 0.0; // Worst shared side-interface non-planarity in mm.
    double top_cap_planarity = 0.0; // Worst extrados cap non-planarity in mm.
};

/// Remeshes guide.shell preserving borders, builds its NURBS-smoothed trivalent dual, and interpolates vertex thicknesses.
Mesh calgary_dual_mesh(
    const CalgaryVaultGuide& guide,
    std::vector<double>* out_vertex_thickness = nullptr,
    std::set<std::pair<size_t, size_t>>* out_support_edges = nullptr
);

/// Builds variable-thickness planar-sided flat-top stereotomic blocks (with optional chamfers/bevels) for guide.
std::vector<std::shared_ptr<Element>> calgary_vault(
    const CalgaryVaultGuide& guide,
    CalgaryDualMetrics* metrics = nullptr,
    Mesh* out_dual_mesh = nullptr
);

/// Calgary 2025 asymmetric four-pier trivalent dual shell session with variable thickness and planarized contacts.
class CalgaryVault : public WoodSession {
public:
    const CalgaryVaultGuide guide; // Shell geometry, variable thickness parameters, and chamfer/bevel settings.

    /// Remeshes the thrust shell, builds the smoothed trivalent dual, extrudes variable-thickness blocks, and computes contacts.
    explicit CalgaryVault(
        const CalgaryVaultGuide& guide = CalgaryVaultGuide::variable_thickness_blocks(),
        const std::string& name = "vault_calgary_dual"
    );

    /// Returns the NURBS-smoothed boundary-inclusive trivalent dual mesh.
    const Mesh& dual_mesh() const;

    /// Returns block count, thickness range, chamfer count, and planarity diagnostics.
    const CalgaryDualMetrics& metrics() const;

private:
    Mesh _dual_mesh; // Boundary-inclusive trivalent dual polygon mesh.
    CalgaryDualMetrics _metrics; // Thickness and planarity diagnostics.
};

}
