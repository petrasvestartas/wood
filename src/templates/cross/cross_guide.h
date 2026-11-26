#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_cross {

// ═══════════════════════════════════════════════════════════════════════════
// VdaMeshGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The inputs of a vda mesh: the mesh, its plates and the connectors across its edges.
struct VdaMeshGuide {
    Mesh mesh = default_mesh(); // The input mesh, welded at 0.01 by the template.
    double face_thickness = 20.0; // Plate thickness along the face normal.
    std::vector<double> face_positions = {0.0}; // Offsets of the plate layers along the face normal, sorted.
    std::vector<int> edge_divisions = {2}; // Connectors per interior edge: one value for all, or one per edge.
    std::vector<double> edge_division_length = {}; // Connector spacing along an edge, used instead of edge_divisions when given.
    std::vector<Line> insertion_lines = {}; // Lines that turn the connectors of the edge they touch.
    double connector_width = 200.0; // Connector size across the edge, in the plane of the faces.
    double connector_height = 200.0; // Connector size along the average face normal.
    double connector_thickness = 20.0; // Connector size along the edge.

    /// A fifteen-face dome of hexagons and pentagons.
    static Mesh default_mesh();

    /// The dome with 40 mm plates and 300 x 300 x 40 connectors, two per edge.
    static VdaMeshGuide dome();

    /// The dome made to go together: 40 mm plates, 260 x 260 x 40 connectors turned along the slide of every edge's later plate.
    static VdaMeshGuide dome_assembly();
};

}
