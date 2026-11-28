#pragma once
#include "boolean_polyline.h"
#include "wood_session.h"
#include "src/templates/template_chamfer.h"

using namespace laika_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Crease descriptor
// ═══════════════════════════════════════════════════════════════════════════

/// One interior fold crease or cut seam between two adjacent planar faces.
struct FoldCrease {
    size_t face_a = 0; // First incident face index.
    size_t face_b = 0; // Second incident face index.
    Line line_3d; // Shared 3D edge segment.
    Line line_2d; // Unfolded 2D crease segment in the xy plane on face_a's sheet.
    double angle = 0.0; // Signed dihedral fold angle in radians (+ mountain, - valley, 0 coplanar).
    bool is_cut = false; // True when split across a cut seam, false when hinged flat.
};

// ═══════════════════════════════════════════════════════════════════════════
// Planar net unfolding
// ═══════════════════════════════════════════════════════════════════════════

/// Isometric hinge-tree unfolding of a planar-faced folded mesh into non-overlapping flat 2D sheets.
class FoldUnfolding {
private:
    /// Undirected interior edge shared by two faces of the oriented mesh.
    struct DualEdge {
        size_t face_a = 0; // First incident face index.
        size_t face_b = 0; // Second incident face index.
        size_t u = 0; // First mesh vertex index.
        size_t v = 0; // Second mesh vertex index.
        double length = 0.0; // Shared edge length in mm.
        double angle = 0.0; // Signed dihedral angle in radians.
        double score = 0.0; // Spanning-tree priority score.
    };

public:
    double gap = 400.0; // Spacing in mm between separate unfolded sheets along +x.
    size_t sheet_count = 0; // Number of disconnected unfolded flat pieces.
    std::vector<Polyline> faces; // Closed 2D face outlines in z = 0, one per mesh face.
    std::vector<Xform> transforms; // Rigid 3D-to-2D transforms mapping each 3D face onto z = 0.
    std::vector<size_t> sheet_of; // Disconnected piece index containing each face.
    std::vector<Polyline>
        sheets; // Closed outer and hole boundary rings of every unfolded flat piece.
    std::vector<FoldCrease>
        creases; // Interior edges classified into intact fold hinges and cut seams.
    double distortion =
        0.0; // Largest relative edge or diagonal length error across all unfolded faces.

    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// Construct an empty unfolding.
    FoldUnfolding() = default;

    /// Unfold mesh into minimum-count non-overlapping flat sheets in the xy plane.
    explicit FoldUnfolding(const Mesh& mesh, double gap = 400.0);

    // ═══════════════════════════════════════════════════════════════════════════
    // Queries and scene export
    // ═══════════════════════════════════════════════════════════════════════════

    /// Number of interior edges cut open to unfold without overlap or stretching.
    size_t cut_count() const;

    /// Number of intact interior fold hinges with non-zero dihedral angle.
    size_t hinge_count() const;

    /// Rigidly lay one 3D FacePlanarElement flat onto z = 0 in the first quadrant, preserving bevels and features.
    static std::shared_ptr<FacePlanarElement> flatten_plate(const FacePlanarElement& plate);

    /// Add sheet silhouettes and interior fold creases to group "flat" beside session's 3D model.
    void add_to(WoodSession& session, double beside = 0.0) const;

private:
    /// Compute outward unit normal and vertex centroid of every 3D mesh face.
    static void compute_face_frames(
        const std::vector<Point>& points,
        const std::vector<std::vector<size_t>>& loops,
        std::vector<Vector>& normals,
        std::vector<Point>& centroids
    );

    /// Build all undirected interior dual edges with signed dihedral angles and priority scores.
    static std::vector<DualEdge> compute_dual_edges(
        const std::vector<Point>& points,
        const std::vector<std::vector<size_t>>& loops,
        const std::vector<Vector>& normals
    );

    /// Grow one non-overlapping hinged sheet starting from seed, updating placed, transforms and faces.
    std::vector<size_t> grow_sheet(
        size_t seed,
        size_t piece_index,
        const std::vector<Point>& points,
        const std::vector<std::vector<size_t>>& loops,
        const std::vector<Vector>& normals,
        const std::vector<Point>& centroids,
        const std::vector<DualEdge>& dual_edges,
        const std::vector<std::vector<size_t>>& incident,
        std::vector<bool>& placed
    );

    /// Shift one sheet's faces to [cursor_x, ..] x [0, ..], record merged rings and return next cursor_x.
    double finalize_sheet(
        const std::vector<size_t>& sheet_faces,
        const std::vector<Point>& points,
        const std::vector<std::vector<size_t>>& loops,
        double cursor_x
    );

    /// Classify all dual edges into intact interior fold creases vs cut seams.
    void
    classify_creases(const std::vector<Point>& points, const std::vector<DualEdge>& dual_edges);

    /// Rigid transform mapping root face loop onto z = 0 with its first edge along +x and normal along +z.
    static Xform root_transform(
        const std::vector<Point>& points, const std::vector<size_t>& loop, const Vector& normal
    );

    /// Rigid transform unfolding child face across hinge (u, v) away from parent_2d onto z = 0.
    static Xform hinge_transform(
        const Point& u_3d,
        const Point& v_3d,
        const Point& child_centre_3d,
        const Vector& child_normal,
        const Xform& parent_xform,
        const Point& parent_centre_2d
    );

    /// Closed 2D polygon of loop transformed by xform and projected onto z = 0, wound counter-clockwise.
    static Polyline flat_polygon(
        const std::vector<Point>& points, const std::vector<size_t>& loop, const Xform& xform
    );

    /// Whether candidate overlaps any polygon already placed in sheet_polys by interior area.
    static bool overlaps_sheet(const Polyline& candidate, const std::vector<Polyline>& sheet_polys);

    /// Snap 2D polygon vertices to a 0.001 mm grid and union all faces of one sheet into boundary rings.
    static std::vector<Polyline> union_sheet(const std::vector<Polyline>& sheet_polys);

    /// Maximum relative chord-length error between 3D face vertices and their 2D unfolded image.
    static double face_distortion(
        const std::vector<Point>& points, const std::vector<size_t>& loop, const Xform& xform
    );
};
