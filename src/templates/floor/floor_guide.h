#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

// The geometry of the timber floor: FloorGuide computes every member of every quarter as two face loops;
// floor.h builds the model from it.

namespace wood_floor {

// ═══════════════════════════════════════════════════════════════════════════
// The tables FloorGuide computes per quarter
// ═══════════════════════════════════════════════════════════════════════════

/// The planes of a quarter in pairs, one pair per member: the first plane is the member's base face, the second the face it is offset to.
class ConstructionPlanes {
public:
    std::vector<std::array<Plane, 2>> outer_ribs; // Along the two bay edges, the band offset inwards by outer_ribs.
    std::vector<std::array<Plane, 2>> inner_beams; // Along the two seams and the oculus edge; the oculus one tilted by the oculus plane angle.
    std::vector<std::array<Plane, 2>> inner_ribs; // From the column head chamfer to the inner beam corners.
    std::vector<std::array<Plane, 2>> wedges; // The column head fan: side 0, the middle one tilted by wedge_plane_angle, side 1.
    std::vector<std::array<Plane, 2>> tsections; // Beside the ribs, tsections thick: outer rib 0, inner rib 0 outer and central face, inner rib 1 central and outer face, outer rib 1.
};

/// One plan quad per member at the floor datum, index i the footprint of member i of that family.
class ConstructionQuads {
public:
    std::vector<Polyline> outer_ribs; // Two.
    std::vector<Polyline> inner_beams; // Three: seam 0, oculus edge, seam 1.
    std::vector<Polyline> inner_ribs; // Two.
    std::vector<Polyline> wedges; // Three.
    std::vector<Polyline> tsections; // Six.
};

/// The central panel of one quarter by rule A: its ruling, the one sweep of both inner ribs, and the soffit, +t and +2t traces on the two inner ribs' central faces.
class CentralPanel {
public:
    Vector ruling; // u: the panel's horizontal ruling, along which rib 0's central trace projects onto rib 1's.
    Vector rib_sweep; // r: the horizontal direction both inner ribs are swept along from their outer to their central face.
    std::array<std::array<Polyline, 3>, 2> traces; // Per inner rib, its central face's soffit, +t and +2t, offset in the panel's own cross-section.
};


// ═══════════════════════════════════════════════════════════════════════════
// FloorGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The floor guide, a session ready to draw: the corners and the parameters, and the geometry every member is built from, computed by compute() on construction and again after a parameter changes and drawn into the session itself, grouped by quarter. It works for any convex four-corner bay: every method takes the quarter q, the quarter at corner q. A Floor builds the model from it.
class FloorGuide : public WoodSession {
public:
    std::array<Point, 4> corners; // Counter-clockwise at z 0.

    // the parameters, each with its default; after changing one, compute() again
    double size_oculus = 1000.0; // Distance of every oculus point from the centre along its seam: a square diamond on a rectangular bay.
    double size_column_head = 220.0; // Side of the square column shaft and of the head polygon at the corner.
    double size_column_head_chamfer = 120.0; // Where the chamfer vertices sit on the shaft faces; also the capitel width.
    double size_outer_ribs = 100.0; // Outer rib thickness.
    double size_inner_ribs = 60.0; // Inner rib thickness.
    double size_inner_beams = 60.0; // Seam and oculus beam thickness; also the ring beam width at the datum.
    double size_wedge = 240.0; // Side wedge block thickness; the middle block is middle_wedge_factor times it.
    double size_tsections = 27.0; // Flange plane offset and bed layer thickness.
    double height = 650.0; // Rib depth where the parabola starts, a wedge thickness past the column face.
    double rise = 453.0; // Parabola rise from there to the seam.
    double wedge_plane_angle = -10.0; // Degrees the chamfer fan plane leans about its top edge.
    double oculus_plane_angle = 5.0; // Degrees the oculus bearing plane leans about its top edge.
    double column_head_depth = 730.0; // Depth of the carved head and of the capitel.
    double bay_height = 3500.0; // Storey: the floor top above the slab, the column top.
    double middle_wedge_factor = 1.25; // The middle block in wedge thicknesses.

    // what compute() derives from them
    Point centre; // The vertex centroid, where the bimedians cross and bisect each other.
    std::array<Point, 4> oculus_points; // Point q on seam q, size_oculus from the centre.
    double soffit = 0.0; // The level of every inner and ring beam's soffit: the deepest end of a rib that ends on one, so every rib end meets its beam in full.

    /// The guide of the corners with the default parameters, computed.
    explicit FloorGuide(const std::array<Point, 4>& corners);

    /// Computes everything from the corners and the parameters, quarter by quarter in the order of the methods below, and redraws it; throws naming the failure when the corners are not counter-clockwise and convex at z 0, an oculus point leaves its seam, the rise leaves (0, height), an oculus point lies in an outer rib band or the ring would leave a quarter's oculus beam uncovered.
    void compute();

    /// Depth at every seam and at the oculus: height minus rise.
    double static_h() const;

    /// The midpoint of edge k, corner k to corner k + 1.
    Point midpoint(size_t k) const;

    /// The interior angle at corner k in degrees.
    double corner_angle(size_t k) const;

    // ═══════════════════════════════════════════════════════════════════════
    // Floor plan geometry
    // ═══════════════════════════════════════════════════════════════════════

    /// Quarter q in plan: corner q, the midpoint of edge q, oculus point q, oculus point q - 1, the midpoint of edge q - 1. Line 0 runs along edge q, line 1 is seam q, line 2 the oculus edge, line 3 seam q - 1, line 4 along edge q - 1.
    std::vector<Point> quarter_polygon(size_t q) const;

    /// The column head polygon at corner q, the ribs start from it: the corner, two shaft corners and the two chamfer points, in the column's frame.
    std::vector<Point> quarter_column_polygon(size_t q) const;

    /// The column's frame at corner q: origin the corner, x and y the edge directions at a right corner, symmetric about the corner bisector otherwise.
    Plane column_frame(size_t q) const;

    /// The support's plane at corner q, on the slab under the column axis, half a column head along both frame axes from the corner.
    Plane support_plane(size_t q) const;

    // ═══════════════════════════════════════════════════════════════════════
    // Beams: the plate edges as plane pairs, then their plan quads
    // ═══════════════════════════════════════════════════════════════════════

    /// Quarter q's plane pairs, one per member: outer ribs on the bay edges, inner beams on the seams and the tilted oculus edge, inner ribs from the column head to the beam corners, the wedge fan, and the t-sections beside the ribs.
    const ConstructionPlanes& construction_planes(size_t q) const;

    /// Quarter q's member quads where each member's four planes meet the datum.
    const ConstructionQuads& construction_quads(size_t q) const;

    // ═══════════════════════════════════════════════════════════════════════
    // 3D geometry
    // ═══════════════════════════════════════════════════════════════════════

    /// Per outer rib of quarter q, how far along its axis the parabola starts: the wedge where both ends land level, else solved so they do.
    std::array<double, 2> run_in(size_t q) const;

    /// Quarter q's parabolas along the outer and inner rib axes (outer 0, outer 1, inner 0, inner 1), each with its +tsections and +2 tsections offsets.
    const std::vector<std::array<Polyline, 3>>& boundary_parabolas(size_t q) const;

    /// Quarter q's central panel between the inner ribs.
    const CentralPanel& central_panel(size_t q) const;

    /// Quarter q's bed panel tops, one per wedge, normal up: the plane fitted to each panel's deepest quad.
    const std::vector<Plane>& bed_top_planes(size_t q) const;

    /// The cutter levels at corner q: the datum, the outer rib bottoms, and minus column_head_depth.
    std::array<double, 3> column_levels(size_t q) const;

    /// The plane each outer rib of quarter q ends on at its seam: the far face of the seam beam, which runs on through the outer rib band to the bay's outer face.
    std::array<Plane, 2> rib_seam_ends(size_t q) const;

    // ═══════════════════════════════════════════════════════════════════════
    // Members, each as its two face loops at the datum
    // ═══════════════════════════════════════════════════════════════════════

    /// Quarter q's three bed rows as rails, each row its bottom rails and its top rails: the lower and upper layer on the panel's two side planes trimmed alike, so every segment of the four makes one bed.
    std::vector<std::array<std::array<Polyline, 2>, 2>> bed_rails(size_t q) const;

    /// Quarter q's bed plates in three rows, each row trimmed alike so every plate stays a quad.
    std::vector<std::vector<std::array<Polyline, 2>>> beds(size_t q) const;

    /// Quarter q's six t-sections beside the ribs.
    std::vector<std::array<Polyline, 2>> tsections(size_t q) const;

    /// Quarter q's two outer ribs along the bay edges: each its parabola trimmed by its end planes on its first face, and swept to its second.
    std::vector<std::array<Polyline, 2>> outer_ribs(size_t q) const;

    /// Quarter q's two inner ribs, swept along the central panel's rib sweep.
    std::vector<std::array<Polyline, 2>> inner_ribs(size_t q) const;

    /// Quarter q's three column blocks between the ribs at the column head, standing on the beds.
    std::vector<std::array<Polyline, 2>> wedges(size_t q) const;

    /// Quarter q's three inner beams: seam 0, the oculus edge, seam 1.
    std::vector<std::array<Polyline, 2>> inner_beams(size_t q) const;

    /// The oculus: four ring beams, each between its edge's tilted plane and ring inner plane from the previous beam's inner plane to the next beam's tilted plane (a pinwheel), four bottom wedges and the inner plate.
    std::vector<std::array<Polyline, 2>> oculus() const;

    /// The ring's inner face on oculus edge q: the oculus beam's back face moved back by twice inner_beams.
    Plane ring_inner(size_t q) const;

    /// The six plates that carve the column head at corner q: the three fan faces down to the middle level, and three below it down to the head's depth.
    std::vector<std::array<Polyline, 2>> column_cutters(size_t q) const;

    /// The column's carved face on fan plane i of corner q (0 side 0, 1 the chamfer, 2 side 1) between the datum and the middle level: datum corners, then middle-level corners.
    std::vector<Point> column_face(size_t q, size_t i) const;

    /// A member bounded by a ring of side planes between a bottom and a top plane: corner i of each loop where sides i and i + 1 meet its plane; flip swaps the two loops.
    static std::array<Polyline, 2> loft(const std::vector<Plane>& sides, const Plane& bottom, const Plane& top, bool flip = false);

    /// The distance between the area centroids of a member's two loops.
    static double thickness(const std::array<Polyline, 2>& loops);

    /// The middle of a member: the mean of its two loops' area centroids.
    static Point body(const std::array<Polyline, 2>& loops);

    /// The lowest corner of a member on a plane it ends on, at most 0.
    static double end_level(const std::array<Polyline, 2>& loops, const Plane& end);

private:
    std::vector<ConstructionPlanes> _construction_planes; // Per quarter, cached by compute().
    std::vector<ConstructionQuads> _construction_quads;
    std::vector<std::array<double, 2>> _run_in;
    std::vector<std::vector<std::array<Polyline, 3>>> _boundary_parabolas;
    std::vector<CentralPanel> _central_panel;
    std::vector<std::vector<Plane>> _bed_top_planes;
    std::array<double, 4> _rib_bottom = {}; // Per corner, the middle cutter level.

    /// Why the corners and the oculus make no floor, empty when they do.
    std::string invalid() const;

    /// The angle at oculus point k between the two oculus edges that meet there, and between its seam and the next quarter's oculus edge, in degrees.
    double oculus_corner_angle(size_t k) const;
    double oculus_seam_angle(size_t k) const;

    /// Why the oculus is too close to a bay edge, empty when it is not: every quarter's inner beam corners must lie inside the outer rib bands.
    std::string beam_corners_in_bands() const;

    /// A member's two faces: the plane and its copy moved by distance along the normal.
    static std::array<Plane, 2> pair(const Plane& plane, double distance);

    /// The steps of compute() for quarter q, in order.
    ConstructionPlanes compute_construction_planes(size_t q) const;
    ConstructionQuads compute_construction_quads(const ConstructionPlanes& cp) const;
    std::array<double, 2> compute_run_in(size_t q) const;
    void set_block_planes(size_t q);
    std::vector<std::array<Polyline, 3>> compute_boundary_parabolas(size_t q) const;
    CentralPanel compute_central_panel(size_t q) const;
    std::vector<Plane> compute_bed_top_planes(size_t q) const;

    /// The outer parabola over a rib quad: from -height at the run-in along the axis, controlled at the axis midpoint at -static_h, to the seam at -static_h.
    Polyline outer_parabola(const Polyline& quad, double run_in) const;

    /// The z where an outer rib's soffit meets its fan plane, and the run-in that lands it on a level, by the secant from the wedge.
    double fan_end(const Polyline& quad, double run_in, const Plane& fan, const Plane& seam) const;
    double run_in_to_level(const Polyline& quad, const Plane& fan, const Plane& seam, double level) const;

    /// Rule A: the root of the closure nearest the reference, scanned without crossing a rib face and refined by bisection; the closure for one sweep; which side of each rib face a sweep crosses; the bisection; the sweep at degrees from the reference.
    static Vector rib_sweep(const std::array<Polyline, 2>& shadows, const std::array<Vector, 2>& normals, double thickness, const Vector& reference);
    static double closure(const std::array<Polyline, 2>& shadows, const std::array<Vector, 2>& normals, double thickness, const Vector& r);
    static bool sweep_sides(const std::array<Vector, 2>& normals, const Vector& reference, double degrees, std::array<bool, 2>& sides);
    static double bisect(const std::array<Polyline, 2>& shadows, const std::array<Vector, 2>& normals, double thickness, const Vector& reference, double lo, double hi);
    static Vector turned(const Vector& reference, double degrees);

    /// A rib: its trace trimmed by the two end planes on its first face, and on its second face the trace swept along the rib with its end corners on the end planes; shared by outer_ribs and inner_ribs.
    static std::array<Polyline, 2> rib(const Polyline& trace, const Plane& face1, const Vector& sweep, const Plane& cut_plane0, const Plane& cut_plane1, bool inner);

    /// Draws the construction into the session by quarter, under the names the Floor gives the members: quarter_q holds plan_q and a group per family with a group per member, holding its plan quad, its two face planes and for a rib its parabolas.
    void draw();
};

}
