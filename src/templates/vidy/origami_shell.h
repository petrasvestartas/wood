#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vidy {

// ═══════════════════════════════════════════════════════════════════════════
// Origami facet and edge descriptors
// ═══════════════════════════════════════════════════════════════════════════

/// One planar polygon facet of a double-layer origami shell with its four miter offset rings.
struct OrigamiFacet {
    size_t index = 0; // Facet index in 0..facets.size() - 1.
    size_t assembly_step = 0; // Sequential assembly rank in 0..facets.size() - 1.
    std::vector<size_t> vertices; // Base mesh vertex indices in outward counter-clockwise order.
    std::vector<int> edge_ids; // Topology edge index for each directed facet side k.
    std::vector<Plane> side_planes; // Outward-facing miter or boundary plane for each side k.
    Point centroid; // Centroid on the inner base mesh (h = 0).
    Vector normal; // Outward unit facet normal from h = 0 to h = total_offset.
    std::array<std::vector<Point>, 4> miter_rings; // Exact miter corners at the four layer heights.
};

/// One undirected topology edge of a double-layer origami shell with fold polarity and miter plane.
struct OrigamiEdge {
    size_t u = 0; // First vertex index in base_mesh.
    size_t v = 0; // Second vertex index in base_mesh.
    bool is_boundary = false; // True on naked perimeter or ground edges, false on interior creases.
    bool on_ground = false; // True when both vertices lie on the base ground plane z = ground_z.
    int facet_a = -1; // First incident facet index.
    int side_a = -1; // Directed edge index on facet_a.
    int facet_b = -1; // Second incident facet index, or -1 when naked.
    int side_b = -1; // Directed edge index on facet_b, or -1 when naked.
    bool positive_fold = true; // True when exterior fold is convex, false when valley.
    double dihedral_rad = 0.0; // Angle acos(n_a . n_b) between adjacent facet normals in radians.
    double tool_tilt_deg = 0.0; // Five-axis CNC bevel tilt |phi - 90| in degrees on interior folds.
    Plane miter_plane; // Shared angle-bisector or boundary plane on this edge.
};

// ═══════════════════════════════════════════════════════════════════════════
// OrigamiShell
// ═══════════════════════════════════════════════════════════════════════════

/// Double-layer CLT origami shell with selective chamfers, corner borders and stepped Vidy tenons.
class OrigamiShell : public WoodSession {
public:
    const double plate_thickness; // Single-layer CLT plate thickness in mm.
    const double total_offset; // Total double-layer shell depth from inner to outer face in mm.
    const bool with_joints; // Generates stepped tenons, rabbet lips and through-mortises when true.
    const bool with_borders; // Generates full-corner perpendicular cavity border boards when true.
    const double pin_pitch; // Centre-to-centre pitch of stepped tenon pins along creases in mm.
    const double corner_scale; // Scale multiplier on multi-valent saddle chamfers (1.0 = exact).

    /// Builds four miter shells, selective chamfers, border boards and Vidy joinery from input_mesh.
    explicit OrigamiShell(
        const Mesh& input_mesh,
        double plate_thickness = 40.0,
        double total_offset = 240.0,
        bool with_joints = true,
        bool with_borders = true,
        double pin_pitch = 450.0,
        double corner_scale = 1.0,
        const std::string& name = "origami_shell"
    );

    /// Not copied, as a session is not.
    OrigamiShell(const OrigamiShell&) = delete;

    /// Not assigned, as it is not copied.
    OrigamiShell& operator=(const OrigamiShell&) = delete;

    /// Parametric Yoshimura antiprismatic diamond-triangulated barrel vault mesh on z = 0.
    static Mesh yoshimura_arch_mesh(
        size_t bays = 4,
        size_t arch_segments = 6,
        double span = 12000.0,
        double bay_length = 2600.0,
        double rise = 4800.0,
        double pleat_depth = 520.0
    );

    /// Parametric Miura-ori herringbone planar-quad arch mesh with flat feet on z = 0.
    static Mesh miura_arch_mesh(
        size_t arch_folds = 6,
        size_t strips = 4,
        double span = 12000.0,
        double length = 9600.0,
        double rise = 4500.0,
        double pleat_depth = 520.0,
        double chevron_shift = 420.0
    );

    /// Base mesh vertex coordinates at h = 0.
    const std::vector<Point>& vertices() const;

    /// Incident facet count per base mesh vertex.
    const std::vector<size_t>& vertex_valences() const;

    /// Four offset heights {0, plate_thickness, total_offset - plate_thickness, total_offset}.
    std::array<double, 4> layer_heights() const;

    /// Clear hollow cavity depth total_offset - 2 * plate_thickness in mm.
    double cavity_gap() const;

    /// Oriented planar base mesh at h = 0.
    const Mesh& base_mesh() const;

    /// Cavity-facing surface mesh of the inner CLT layer at h = plate_thickness.
    const Mesh& inner_cavity_mesh() const;

    /// Cavity-facing surface mesh of the outer CLT layer at h = total_offset - plate_thickness.
    const Mesh& outer_cavity_mesh() const;

    /// Exterior surface mesh of the outer CLT layer at h = total_offset.
    const Mesh& outer_mesh() const;

    /// Quad connection ribbons spanning h = 0..total_offset across every miter and boundary plane.
    const Mesh& miter_planes_mesh() const;

    /// Every planar facet with its outward normal, side planes and four offset corner rings.
    const std::vector<OrigamiFacet>& facets() const;

    /// Every classified mesh topology edge with fold polarity and miter plane.
    const std::vector<OrigamiEdge>& edges() const;

    /// Facet indices ordered in collision-free sequential assembly order.
    const std::vector<size_t>& assembly_order() const;

    /// Inner room-side CLT plates (h = 0 to plate_thickness) in facet order.
    const std::vector<std::shared_ptr<FacePlanarElement>>& inner_plates() const;

    /// Outer CLT plates (h = total_offset - plate_thickness to total_offset) in facet order.
    const std::vector<std::shared_ptr<FacePlanarElement>>& outer_plates() const;

    /// All main folded inner and outer CLT shell plates.
    const std::vector<std::shared_ptr<FacePlanarElement>>& shell_plates() const;

    /// Perpendicular cavity-closing border boards along all naked and ground boundary edges.
    const std::vector<std::shared_ptr<FacePlanarElement>>& border_plates() const;

    /// Perpendicular cavity-closing border boards belonging to facet_idx.
    const std::vector<std::shared_ptr<FacePlanarElement>>&
    border_plates_for_facet(size_t facet_idx) const;

    /// Every CLT plate in the origami shell (inner, outer and cavity border boards).
    const std::vector<std::shared_ptr<FacePlanarElement>>& all_plates() const;

    /// Single rigid-body insertion unit vector of plate (facet_idx, layer 0=inner / 1=outer).
    Vector plate_insertion_vector(size_t facet_idx, size_t layer) const;

    /// Copies all shell and border plates translated by delta into group_name on target.
    void copy_plates_to(
        WoodSession& target, const std::string& group_name, const Vector& delta = {0.0, 0.0, 0.0}
    ) const;

    /// Total number of jointed interior crease seams across the shell.
    size_t total_jointed_seams() const;

    /// Total number of stepped cavity-bridging Double_Pins across the shell.
    size_t total_double_pins() const;

    /// Total number of Single_Pins across the shell.
    size_t total_single_pins() const;

    /// Total number of integral through-tenon pins (Double_Pins + Single_Pins).
    size_t total_pins() const;

    /// Total number of rectangular through-mortise hole loops cut across all plates.
    size_t total_mortise_slots() const;

private:
    double _ground_z = 0.0; // Lowest z coordinate of the input mesh.
    std::array<double, 4> _plan_bounds = {
        0.0, 0.0, 0.0, 0.0
    }; // Outer plan bounds {xmin, xmax, ymin, ymax}.
    std::vector<Point> _vertices; // Welded base mesh vertex coordinates.
    std::vector<size_t> _vertex_valences; // Incident facet count per vertex.
    Mesh _base_mesh; // Oriented planar base mesh at h = 0.
    std::vector<OrigamiFacet> _facets; // Per-facet topology, planes and four miter corner rings.
    std::vector<OrigamiEdge> _edges; // Classified undirected mesh topology edges.
    std::vector<size_t> _assembly_order; // Sequential assembly order of facet indices.
    Mesh _inner_cavity_mesh; // Offset mesh at h = plate_thickness.
    Mesh _outer_cavity_mesh; // Offset mesh at h = total_offset - plate_thickness.
    Mesh _outer_mesh; // Exterior offset mesh at h = total_offset.
    Mesh _miter_planes_mesh; // Quad miter ribbons spanning h = 0..total_offset.
    std::vector<std::shared_ptr<FacePlanarElement>> _inner_plates; // Inner CLT layer plates.
    std::vector<std::shared_ptr<FacePlanarElement>> _outer_plates; // Outer CLT layer plates.
    std::vector<std::shared_ptr<FacePlanarElement>> _shell_plates; // Inner and outer shell plates.
    std::vector<std::shared_ptr<FacePlanarElement>> _border_plates; // Cavity-closing border boards.
    std::vector<std::vector<std::shared_ptr<FacePlanarElement>>>
        _facet_border_plates; // Border boards indexed by facet f.
    std::vector<std::shared_ptr<FacePlanarElement>> _all_plates; // All shell and border plates.
    std::array<std::vector<Vector>, 2>
        _insertion_vectors; // Rigid-body insertion vector [layer][f].
    size_t _total_jointed_seams = 0; // Jointed interior crease seam count.
    size_t _total_double_pins = 0; // Stepped Double_Pin count.
    size_t _total_single_pins = 0; // Single_Pin count.
    size_t _total_mortise_slots = 0; // Through-mortise slot count across all plates.

    /// Extracts welded vertices, consistently oriented outward faces and envelope bounds.
    void extract_oriented_facets(const Mesh& input_mesh);

    /// Builds undirected topology edges, fold polarities and per-facet edge index tables.
    void compute_edges();

    /// Computes side miter and boundary planes plus the four offset corner rings per facet.
    void compute_miter_rings();

    /// Builds an offset surface mesh from ring_index of every facet.
    Mesh compute_offset_mesh(size_t ring_index) const;

    /// Builds quad connection ribbons spanning ring 0 to ring 3 across non-ground edges.
    Mesh compute_miter_planes_mesh() const;

    /// Computes a collision-free facet assembly sequence and one insertion vector per plate.
    void compute_assembly_and_vectors();

    /// Builds the inner and outer CLT shell plates with selective chamfers and Vidy joinery.
    void compute_shell_plates();

    /// Builds full-corner perpendicular cavity-closing border boards along all boundary edges.
    void compute_border_plates();

    /// Adds the guide meshes and inner, outer and border plates to the session tree.
    void populate_scene();
};

}
