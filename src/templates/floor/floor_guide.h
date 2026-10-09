#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Per-quarter tables
// ═══════════════════════════════════════════════════════════════════════════

/// The planes of a quarter in pairs, one pair per member: the first plane is the member's base face, the second the face it is offset to.
///
/// - `outer_ribs[2]`: along the two bay edges.
/// - `inner_beams[3]`: seam 0, the oculus edge, seam 1.
/// - `inner_ribs[2]`: from the column head chamfer to the inner beam corners.
/// - `wedges[3]`: the column head fan, side 0, middle, side 1.
/// - `tsections[6]`: the flanges beside the ribs, outer rib 0 to outer rib 1.
class ConstructionPlanes {
public:
    std::array<std::array<Plane, 2>, 2> outer_ribs; // Along the two bay edges, the band offset inwards by outer_ribs.
    std::array<std::array<Plane, 2>, 3> inner_beams; // Along the two seams and the oculus edge; the oculus one tilted by the oculus plane angle.
    std::array<std::array<Plane, 2>, 2> inner_ribs; // From the column head chamfer to the inner beam corners.
    std::array<std::array<Plane, 2>, 3> wedges; // The column head fan: side 0, the middle one tilted by wedge_plane_angle, side 1.
    std::array<std::array<Plane, 2>, 6> tsections; // Beside the ribs, tsections thick: outer rib 0, inner rib 0 outer and central face, inner rib 1 central and outer face, outer rib 1.
};

/// One plan quad per member at the floor datum, index i the footprint of member i of that family.
///
/// - `outer_ribs[2]`, `inner_beams[3]` (seam 0, oculus edge, seam 1), `inner_ribs[2]`, `wedges[3]`, `tsections[6]`: a closed quad each.
class ConstructionQuads {
public:
    std::array<Polyline, 2> outer_ribs;
    std::array<Polyline, 3> inner_beams; // Seam 0, oculus edge, seam 1.
    std::array<Polyline, 2> inner_ribs;
    std::array<Polyline, 3> wedges;
    std::array<Polyline, 6> tsections;
};

/// The central panel of one quarter by rule A: its ruling, the one sweep of both inner ribs, and the soffit, +t and +2t traces on the two inner ribs' central faces.
///
/// - `ruling`: the panel's horizontal ruling direction.
/// - `rib_sweep`: the direction both inner ribs are swept along, outer face to central face.
/// - `traces[2][3]`: per inner rib, its central face's soffit, +t and +2t.
class CentralPanel {
public:
    Vector ruling; // u: the panel's horizontal ruling, along which rib 0's central trace projects onto rib 1's.
    Vector rib_sweep; // r: the horizontal direction both inner ribs are swept along from their outer to their central face.
    std::array<std::array<Polyline, 3>, 2> traces; // Per inner rib, its central face's soffit, +t and +2t, offset in the panel's own cross-section.
};


// ═══════════════════════════════════════════════════════════════════════════
// FloorGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The floor guide, a session ready to draw: the corners and the parameters, and the geometry every member is built from, computed once on construction and drawn into the session itself, grouped by quarter. It works for any convex four-corner bay: every method takes the quarter q, the quarter at corner q. A Floor builds the model from it.
///
/// Public fields:
/// - `corners[4]`, and the parameters `size_oculus`, `size_column_head`, `size_column_head_chamfer`, `size_outer_ribs`, `size_inner_ribs`, `size_inner_beams`, `size_wedge`, `size_tsections`, `height`, `rise`, `wedge_plane_angle`, `oculus_plane_angle`, `column_head_depth`, `bay_height`, `middle_wedge_factor`.
/// - `centre`, `oculus_points[4]`, `soffit`: what the constructor derives first.
///
/// Per quarter q, read through its method:
/// - `construction_planes(q)`: a ConstructionPlanes, a plane pair per member.
/// - `construction_quads(q)`: a ConstructionQuads, a plan quad per member.
/// - `rib_starts(q)[2]`, `boundary_parabolas(q)[4][3]`, `central_panel(q)`, `bed_top_planes(q)[3]`.
/// - the members as two face loops each: `outer_ribs(q)[2]`, `inner_ribs(q)[2]`, `inner_beams(q)[3]`, `wedges(q)[3]`, `tsections(q)[6]`, `bed_rails(q)[3][2]`, `beds(q)[3][n]`, `column_cutters(q)[6]`.
/// - `oculus()[9]`: four ring beams, four bottom wedges, the central plate.
class FloorGuide : public WoodSession {
public:
    const std::array<Point, 4> corners; // Counter-clockwise at z 0.

    // the parameters, given to the constructor
    const double size_oculus; // Distance of every oculus point from the centre along its seam: a square diamond on a rectangular bay.
    const double size_column_head; // Side of the square column shaft and of the head polygon at the corner.
    const double size_column_head_chamfer; // Where the chamfer vertices sit on the shaft faces; also the capitel width.
    const double size_outer_ribs; // Outer rib thickness.
    const double size_inner_ribs; // Inner rib thickness.
    const double size_inner_beams; // Seam and oculus beam thickness; also the ring beam width at the datum.
    const double size_wedge; // Side wedge block thickness; the middle block is middle_wedge_factor times it.
    const double size_tsections; // Flange plane offset and bed layer thickness.
    const double height; // Rib depth where the parabola starts, a wedge thickness past the column face.
    const double rise; // Parabola rise from there to the seam.
    const double wedge_plane_angle; // Degrees the chamfer fan plane leans about its top edge.
    const double oculus_plane_angle; // Degrees the oculus bearing plane leans about its top edge.
    const double column_head_depth; // Depth of the carved head and of the capitel.
    const double bay_height; // Storey: the floor top above the slab, the column top.
    const double middle_wedge_factor; // The middle block in wedge thicknesses.

    // what the constructor derives from them
    Point centre; // The vertex centroid, where the bimedians cross and bisect each other.
    std::array<Point, 4> oculus_points; // Point q on seam q, size_oculus from the centre.
    double soffit = 0.0; // The level of every inner and ring beam's soffit: the deepest end of a rib that ends on one, so every rib end meets its beam in full.

    /// The guide of the corners, counter-clockwise and convex at z 0, and the parameters: computes every table, step by step, and draws them.
    explicit FloorGuide(
        const std::array<Point, 4>& corners,
        double size_oculus = 1000.0,
        double size_column_head = 220.0,
        double size_column_head_chamfer = 120.0,
        double size_outer_ribs = 100.0,
        double size_inner_ribs = 60.0,
        double size_inner_beams = 60.0,
        double size_wedge = 240.0,
        double size_tsections = 27.0,
        double height = 650.0,
        double rise = 453.0,
        double wedge_plane_angle = -10.0,
        double oculus_plane_angle = 5.0,
        double column_head_depth = 730.0,
        double bay_height = 3500.0,
        double middle_wedge_factor = 1.25);

    /// Depth at every seam and at the oculus: height minus rise.
    double static_h() const;

    /// The midpoint of edge k, corner k to corner k + 1.
    Point midpoint(size_t k) const;

    /// The interior angle at corner k in degrees.
    double corner_angle(size_t k) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Floor plan geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// Quarter q in plan: corner q, the midpoint of edge q, oculus point q, oculus point q - 1, the midpoint of edge q - 1. Line 0 runs along edge q, line 1 is seam q, line 2 the oculus edge, line 3 seam q - 1, line 4 along edge q - 1.
    std::vector<Point> quarter_polygon(size_t q) const;

    /// The column head polygon at corner q, the ribs start from it: the corner, two shaft corners and the two chamfer points, in the column's frame.
    std::vector<Point> quarter_column_polygon(size_t q) const;

    /// The column's frame at corner q: origin the corner, x and y the edge directions at a right corner, symmetric about the corner bisector otherwise.
    Plane column_frame(size_t q) const;

    /// The support's plane at corner q, on the slab under the column axis, half a column head along both frame axes from the corner.
    Plane support_plane(size_t q) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Construction planes and quads
    // ═══════════════════════════════════════════════════════════════════════════

    /// Quarter q's plane pairs, one per member: outer ribs on the bay edges, inner beams on the seams and the tilted oculus edge, inner ribs from the column head to the beam corners, the wedge fan, and the t-sections beside the ribs.
    const ConstructionPlanes& construction_planes(size_t q) const;

    /// Quarter q's member quads where each member's four planes meet the datum.
    const ConstructionQuads& construction_quads(size_t q) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // 3D geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// Per outer rib of quarter q, how far along its axis the parabola starts: the wedge where both ends land level, else solved so they do.
    const std::array<double, 2>& rib_starts(size_t q) const;

    /// Quarter q's parabolas along the outer and inner rib axes (outer 0, outer 1, inner 0, inner 1), each with its +tsections and +2 tsections offsets.
    const std::array<std::array<Polyline, 3>, 4>& boundary_parabolas(size_t q) const;

    /// Quarter q's central panel between the inner ribs.
    const CentralPanel& central_panel(size_t q) const;

    /// Quarter q's bed panel tops, one per wedge, normal up: the plane fitted to each panel's deepest quad.
    const std::array<Plane, 3>& bed_top_planes(size_t q) const;

    /// The cutter levels at corner q: the datum, the outer rib bottoms, and minus column_head_depth.
    std::array<double, 3> column_levels(size_t q) const;

    /// The plane each outer rib of quarter q ends on at its seam: the far face of the seam beam, which runs on through the outer rib band to the bay's outer face.
    std::array<Plane, 2> rib_seam_ends(size_t q) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Members
    // ═══════════════════════════════════════════════════════════════════════════

    /// Quarter q's three bed rows as rails, each row its bottom rails and its top rails: the lower and upper layer on the panel's two side planes trimmed alike, so every segment of the four makes one bed.
    const std::array<std::array<std::array<Polyline, 2>, 2>, 3>& bed_rails(size_t q) const;

    /// Quarter q's bed plates in three rows, each row trimmed alike so every plate stays a quad.
    const std::array<std::vector<std::array<Polyline, 2>>, 3>& beds(size_t q) const;

    /// Quarter q's six t-sections beside the ribs.
    const std::array<std::array<Polyline, 2>, 6>& tsections(size_t q) const;

    /// Quarter q's two outer ribs along the bay edges: each its parabola trimmed by its end planes on its first face, and swept to its second.
    const std::array<std::array<Polyline, 2>, 2>& outer_ribs(size_t q) const;

    /// Quarter q's two inner ribs, swept along the central panel's rib sweep.
    const std::array<std::array<Polyline, 2>, 2>& inner_ribs(size_t q) const;

    /// Quarter q's three column blocks between the ribs at the column head, standing on the beds.
    const std::array<std::array<Polyline, 2>, 3>& wedges(size_t q) const;

    /// Quarter q's three inner beams: seam 0, the oculus edge, seam 1.
    const std::array<std::array<Polyline, 2>, 3>& inner_beams(size_t q) const;

    /// The oculus: four ring beams, each between its edge's tilted plane and ring inner plane from the previous beam's inner plane to the next beam's tilted plane (a pinwheel), four bottom wedges and the inner plate.
    const std::array<std::array<Polyline, 2>, 9>& oculus() const;

    /// The ring's inner face on oculus edge q: the oculus beam's back face moved back by twice inner_beams.
    Plane ring_inner(size_t q) const;

    /// The six plates that carve the column head at corner q: the three fan faces down to the middle level, and three below it down to the head's depth.
    const std::array<std::array<Polyline, 2>, 6>& column_cutters(size_t q) const;

    /// The column's carved face on fan plane i of corner q (0 side 0, 1 the chamfer, 2 side 1) between the datum and the middle level: datum corners, then middle-level corners.
    std::vector<Point> column_face(size_t q, size_t i) const;

    /// A member bounded by a ring of side planes between a bottom and a top plane: corner i of each loop where sides i and i + 1 meet its plane; flip swaps the two loops.
    static std::array<Polyline, 2> loft(
        const std::vector<Plane>& sides,
        const Plane& bottom,
        const Plane& top,
        bool flip = false
    );

    /// The distance between the area centroids of a member's two loops.
    static double thickness(const std::array<Polyline, 2>& loops);

    /// The middle of a member: the mean of its two loops' area centroids.
    static Point body(const std::array<Polyline, 2>& loops);

    /// The lowest corner of a member on a plane it ends on, at most 0.
    static double end_level(const std::array<Polyline, 2>& loops, const Plane& end);

private:
    // per quarter, filled by the constructor
    std::array<ConstructionPlanes, 4> _construction_planes;
    std::array<std::array<double, 2>, 4> _rib_starts;
    std::array<ConstructionQuads, 4> _construction_quads;
    std::array<std::array<std::array<Polyline, 3>, 4>, 4> _boundary_parabolas; // Per rib (outer 0, outer 1, inner 0, inner 1), soffit, +t and +2t.
    std::array<CentralPanel, 4> _central_panel;
    std::array<std::array<Plane, 3>, 4> _bed_top_planes; // Per bed row: beside rib 0, the central panel, beside rib 1.
    double _rib_bottom = 0.0; // The middle cutter level, one for every column.
    // the members, each as its two face loops, filled by the constructor
    std::array<std::array<std::array<Polyline, 2>, 2>, 4> _outer_ribs;
    std::array<std::array<std::array<Polyline, 2>, 2>, 4> _inner_ribs;
    std::array<std::array<std::array<Polyline, 2>, 6>, 4> _tsections;
    std::array<std::array<std::array<std::array<Polyline, 2>, 2>, 3>, 4> _bed_rails;
    std::array<std::array<std::vector<std::array<Polyline, 2>>, 3>, 4> _beds;
    std::array<std::array<std::array<Polyline, 2>, 3>, 4> _wedges;
    std::array<std::array<std::array<Polyline, 2>, 3>, 4> _inner_beams;
    std::array<std::array<std::array<Polyline, 2>, 6>, 4> _column_cutters;
    std::array<std::array<Polyline, 2>, 9> _oculus; // Four ring beams, four bottom wedges, the inner plate.

    /// A member's two faces: the plane and its copy moved by distance along the normal.
    static std::array<Plane, 2> pair(const Plane& plane, double distance);

    /// The constructor's steps, in its order; compute_rib_starts also sizes the column blocks inside compute_construction_planes.
    ConstructionPlanes compute_construction_planes(size_t q) const;
    Point compute_centre() const;
    std::array<Point, 4> compute_oculus_points() const;
    ConstructionQuads compute_construction_quads(size_t q) const;
    std::array<double, 2> compute_rib_starts(const ConstructionPlanes& cp) const;
    std::array<std::array<Polyline, 3>, 4> compute_boundary_parabolas(size_t q) const;
    CentralPanel compute_central_panel(size_t q) const;
    std::array<Plane, 3> compute_bed_top_planes(size_t q) const;
    double compute_rib_bottom() const;
    double compute_soffit() const;
    std::array<std::array<Polyline, 2>, 2> compute_outer_ribs(size_t q) const;
    std::array<std::array<Polyline, 2>, 2> compute_inner_ribs(size_t q) const;
    std::array<std::array<Polyline, 2>, 6> compute_tsections(size_t q) const;
    std::array<std::array<std::array<Polyline, 2>, 2>, 3> compute_bed_rails(size_t q) const;
    std::array<std::vector<std::array<Polyline, 2>>, 3> compute_beds(size_t q) const;
    std::array<std::array<Polyline, 2>, 3> compute_wedges(size_t q) const;
    std::array<std::array<Polyline, 2>, 3> compute_inner_beams(size_t q) const;
    std::array<std::array<Polyline, 2>, 9> compute_oculus() const;
    std::array<std::array<Polyline, 2>, 6> compute_column_cutters(size_t q) const;

    /// Outer rib k's axis on the datum, along its base face from its fan plane to its seam plane; the block far faces do not touch it.
    static Line outer_rib_axis(const ConstructionPlanes& cp, size_t k);

    /// The outer parabola over a rib axis: from -height at distance along it, controlled at its midpoint at -static_h, to the seam at -static_h.
    Polyline outer_parabola(const Line& axis, double distance) const;

    /// The z where an outer rib's soffit meets its fan plane, and the rib start that lands it on a level, by the secant from the wedge.
    double fan_end(
        const Line& axis,
        double distance,
        const Plane& fan,
        const Plane& seam
    ) const;
    double rib_start_at_level(
        const Line& axis,
        const Plane& fan,
        const Plane& seam,
        double level
    ) const;

    /// Rule A: the root of the closure nearest the reference, scanned without crossing a rib face and refined by bisection; the closure for one sweep; which side of each rib face a sweep crosses; the bisection; the sweep at degrees from the reference.
    static Vector rib_sweep(
        const std::array<Polyline, 2>& shadows,
        const std::array<Vector, 2>& normals,
        double thickness,
        const Vector& reference
    );
    static double closure(
        const std::array<Polyline, 2>& shadows,
        const std::array<Vector, 2>& normals,
        double thickness,
        const Vector& r
    );
    static bool sweep_sides(
        const std::array<Vector, 2>& normals,
        const Vector& reference,
        double degrees,
        std::array<bool, 2>& sides
    );
    static double bisect(
        const std::array<Polyline, 2>& shadows,
        const std::array<Vector, 2>& normals,
        double thickness,
        const Vector& reference,
        double lo,
        double hi
    );
    static Vector turned(const Vector& reference, double degrees);

    /// A rib: its trace trimmed by the two end planes on its first face, and on its second face the trace swept along the rib with its end corners on the end planes; shared by outer_ribs and inner_ribs.
    static std::array<Polyline, 2> rib(
        const Polyline& trace,
        const Plane& face1,
        const Vector& sweep,
        const Plane& cut_plane0,
        const Plane& cut_plane1,
        bool inner
    );

    /// Draws the construction into the session by quarter, under the names the Floor gives the members: quarter_q holds plan_q and a group per family with a group per member, holding its plan quad, its two face planes and for a rib its parabolas.
    void draw();
};

}
