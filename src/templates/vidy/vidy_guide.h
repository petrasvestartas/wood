#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vidy {

// ═══════════════════════════════════════════════════════════════════════════
// Facet and edge descriptors
// ═══════════════════════════════════════════════════════════════════════════

/// Structural role of one planar facet in a Théâtre de Vidy bay.
enum class VidyFacetRole {
    LeftWall, // Vertical V-pleated planar quad along the negative-x wall.
    Roof, // Antiprismatic planar roof triangle spanning across the arch.
    RightWall, // Vertical V-pleated planar quad along the positive-x wall.
};

/// Fabrication classification of one mesh topology edge (after Robeller's Vidy DLFP table).
enum class VidyEdgeKind {
    Regular, // Interior fold crease within a bay receiving double-layer tenon joinery (regl).
    BaySeparation, // Vertical XZ miter plane separating adjacent prefabricated bays (vcon).
    GableEnd, // Naked front or rear gable perimeter mitered on a vertical XZ plane (vend).
    Ground, // Naked horizontal wall foot mitered flush on the z = 0 slab plane (grnd).
};

/// One planar facet of the Vidy pavilion together with its four mitered offset rings.
struct VidyFacet {
    size_t bay = 0; // Longitudinal prefabricated bay index in 0..bays-1.
    size_t local_index = 0; // Local facet index in 0..13 within its bay (0..3 walls, 4..13 roof).
    VidyFacetRole role = VidyFacetRole::Roof; // Structural role: LeftWall, Roof or RightWall.
    std::vector<size_t> vertices; // Base mesh vertex indices (3 for roof triangle, 4 for wall quad).
    std::vector<int> edge_ids; // Topology edge index for each directed facet side k.
    std::vector<Plane> side_planes; // Outward-facing miter or boundary plane for each directed side k.
    Point centroid; // Centroid on the inner base mesh (h = 0).
    Vector normal; // Outward unit facet normal from interior room (h = 0) to exterior (h = total_offset).
    std::array<std::vector<Point>, 4> miter_rings; // Exact miter corners at h = {0, t_plate, t_offs - t_plate, t_offs}.
};

/// One undirected topology edge of the Vidy base mesh with fold polarity and tool-tilt angle.
struct VidyEdge {
    size_t u = 0; // First vertex index in base_mesh.
    size_t v = 0; // Second vertex index in base_mesh.
    VidyEdgeKind kind = VidyEdgeKind::Regular; // Edge type: Regular, BaySeparation, GableEnd or Ground.
    int facet_a = -1; // First incident facet index.
    int side_a = -1; // Directed edge index on facet_a.
    int facet_b = -1; // Second incident facet index, or -1 when naked.
    int side_b = -1; // Directed edge index on facet_b, or -1 when naked.
    bool positive_fold = true; // True when exterior fold is convex (pos), false when valley (neg).
    bool in_situ_eave = false; // True on wall-to-roof eaves assembled vertically along +Z on site (situ).
    double tool_tilt_deg = 0.0; // Five-axis CNC bevel tilt |phi - 90| in degrees on Regular folds.
    Plane miter_plane; // Shared angle-bisector, XZ separation, or ground miter plane on this edge.
};

// ═══════════════════════════════════════════════════════════════════════════
// VidyGuide
// ═══════════════════════════════════════════════════════════════════════════

/// Geometric guide for the Théâtre de Vidy double-layer antiprismatic folded roof and V-pleated walls.
class VidyGuide : public WoodSession {
public:
    const size_t bays; // Number of longitudinal prefabricated bays (11 in the Lausanne pavilion).
    const double bay_length; // Longitudinal pitch per bay along y in mm (2609.62 mm).
    const double span_mid; // Clear transverse span between even bay-axis wall bases at y = 0 in mm.
    const double span_end; // Clear transverse span between even bay-axis wall bases at gable ends in mm.
    const double wall_pleat; // Outward V-pleat amplitude of odd mid-bay wall vertices in mm.
    const double eave_height_mid; // Even bay-axis eave height at y = 0 in mm.
    const double eave_height_end; // Even bay-axis eave height at gable ends in mm.
    const double crown_height_mid; // Odd mid-bay roof ridge apex height at y = 0 in mm.
    const double crown_height_end; // Odd mid-bay roof ridge apex height at end bays in mm.
    const double knee_half_span; // Transverse x distance of the two arch haunch knees from axis in mm.
    const double knee_rise; // Vertical rise of the left arch knee above the even-station eave in mm.
    const double asymmetry; // Auditorium roof asymmetry raising right knee and shifting crown in mm.
    const double plate_thickness; // Single-layer 5-ply CLT thickness t_plate in mm (45 mm).
    const double total_offset; // Total double-layer shell depth t_offs from inner to outer face in mm (300 mm).

    /// Builds the Stage 1 base DCEL mesh + bay axes and Stage 2 double-layer miter offset meshes + miter planes.
    explicit VidyGuide(
        size_t bays = 11,
        double bay_length = 2609.62,
        double span_mid = 19115.80,
        double span_end = 16285.82,
        double wall_pleat = 656.0,
        double eave_height_mid = 7482.78,
        double eave_height_end = 6512.31,
        double crown_height_mid = 10347.67,
        double crown_height_end = 9723.09,
        double knee_half_span = 6099.83,
        double knee_rise = 1800.0,
        double asymmetry = 630.0,
        double plate_thickness = 45.0,
        double total_offset = 300.0,
        const std::string& name = "vidy_guide"
    );

    /// Base mesh vertex coordinates across all 2 * bays + 1 stations.
    const std::vector<Point>& vertices() const;

    /// Clear insulation cavity thickness between inner and outer CLT layers (total_offset - 2 * plate_thickness).
    double cavity_gap() const;

    /// Stage 1 base polygonal mesh (h = 0, room-facing surface: 10 roof triangles + 4 wall quads per bay).
    const Mesh& base_mesh() const;

    /// Stage 2 cavity-facing surface mesh of the inner CLT layer (h = plate_thickness).
    const Mesh& inner_cavity_mesh() const;

    /// Stage 2 cavity-facing surface mesh of the outer CLT layer (h = total_offset - plate_thickness).
    const Mesh& outer_cavity_mesh() const;

    /// Stage 2 exterior surface mesh of the outer CLT layer (h = total_offset).
    const Mesh& outer_mesh() const;

    /// Stage 2 quad connection ribbons spanning h = 0..total_offset across every miter and separation plane.
    const Mesh& miter_planes_mesh() const;

    /// Transverse bay-axis grid lines 0..bays on z = 0 separating the prefabricated bays.
    const std::vector<Line>& bay_axes() const;

    /// Every planar facet with its outward normal, side miter planes and four offset corner rings.
    const std::vector<VidyFacet>& facets() const;

    /// Every classified mesh topology edge with fold polarity, miter plane and 5-axis CNC tool-tilt angle.
    const std::vector<VidyEdge>& edges() const;

    /// Four exact offset heights {0, plate_thickness, total_offset - plate_thickness, total_offset}.
    std::array<double, 4> layer_heights() const;

private:
    std::vector<Point> _vertices; // Base mesh vertex coordinates across 2 * bays + 1 stations.
    std::vector<Line> _bay_axes; // Transverse bay-axis lines on z = 0.
    Mesh _base_mesh; // Base polygonal mesh (h = 0) with outward normals.
    std::vector<VidyFacet> _facets; // Per-facet topology, planes and 4 miter corner rings.
    std::vector<VidyEdge> _edges; // Classified undirected mesh edges.
    Mesh _inner_cavity_mesh; // Offset mesh at h = plate_thickness.
    Mesh _outer_cavity_mesh; // Offset mesh at h = total_offset - plate_thickness.
    Mesh _outer_mesh; // Exterior offset mesh at h = total_offset.
    Mesh _miter_planes_mesh; // Quad miter ribbons spanning h = 0..total_offset along every fold/separation plane.

    void build_stations_and_facets();
    void build_edges_and_miter_rings();
    Mesh build_offset_mesh(size_t ring_index) const;
    Mesh build_miter_planes_mesh() const;
    void draw();
};

} // namespace wood_vidy
