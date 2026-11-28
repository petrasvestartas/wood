#pragma once
#include "pch.h"
#include "wood_element_node_block.h"

using namespace laika_cpp;

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Planarization
// ═══════════════════════════════════════════════════════════════════════════

/// Moves points minimally until every polygon lies in one plane, leaving pinned vertices fixed.
std::vector<Point> planarized(
    const std::vector<Point>& points,
    const std::vector<std::vector<size_t>>& polygons,
    double tolerance = 1e-7,
    const std::vector<bool>& pinned = {}
);

/// Returns the largest distance from any polygon corner to its fitted plane.
double planarity(
    const std::vector<Point>& points,
    const std::vector<std::vector<size_t>>& polygons
);

// ═══════════════════════════════════════════════════════════════════════════
// Volumetric NGon Solids
// ═══════════════════════════════════════════════════════════════════════════

/// Planarity residuals of an extruded NGon block assembly.
struct NgonSolidMetrics {
    double side_planarity = 0.0; // Worst corner deviation over all side faces in mm.
    double cap_planarity = 0.0; // Worst corner deviation over all top and bottom caps in mm.
};

/// Extrudes every face of mesh into a volumetric block with optional planarized sides, shifted top/bottom facets, and bevels.
std::vector<std::shared_ptr<wood_session::NodeBlockElement>> ngon_solids(
    const Mesh& mesh,
    double thickness,
    bool planar_sides = true,
    double flatten_caps = 1.0,
    const Vector& facet_shift = Vector(0.0, 0.0, 0.0),
    double bevel_degrees = 0.0,
    bool trim_base = false,
    double base_z = 0.0,
    NgonSolidMetrics* metrics = nullptr
);

}
