#pragma once
#include "wood_session.h"

// The geometry of the timber floor: FloorGuide computes every member of every quarter as two face loops;
// floor.h builds the model from it.

namespace wood_floor {

class ConstructionPlanes;
class ConstructionQuads;
class CentralPanel;

// ═══════════════════════════════════════════════════════════════════════════
// Vocabulary
// ═══════════════════════════════════════════════════════════════════════════

/// The member families of the floor: the six of a quarter in the order they are added, then the ring beams, the columns and the supports.
enum class Family {
    outer_ribs, // Variable beams under the two outer parabolas.
    inner_ribs, // Variable beams under the two inner parabolas.
    inner_beams, // Variable beams on the seams and the oculus edge.
    wedges, // The three column blocks at the column head, plates.
    tsections, // T-section plates beside the ribs.
    beds, // Bed plates in three rows.
    ring, // The four ring beams of the oculus.
    column, // The four columns.
    support, // The four supports.
};

const std::array<std::string, 6> FAMILY_NAMES = {"outer_ribs", "inner_ribs", "inner_beams", "wedges", "tsections", "beds"}; // The group and element name prefix of each quarter family, in Family order; the guide draws each member's construction under the same names.

/// The display colour of each quarter family, in Family order: pink, yellow, two neutral greys and the yellow and blue tints, the Block Research Group's primary blue kept for the connectors. The guide draws a member's construction in it.
const std::array<session_cpp::Color, 6> FAMILY_COLORS = {
    session_cpp::Color(232.0f / 255.0f, 71.0f / 255.0f, 139.0f / 255.0f, 1.0f, "outer_ribs"),
    session_cpp::Color(242.0f / 255.0f, 204.0f / 255.0f, 12.0f / 255.0f, 1.0f, "inner_ribs"),
    session_cpp::Color(124.0f / 255.0f, 124.0f / 255.0f, 124.0f / 255.0f, 1.0f, "inner_beams"),
    session_cpp::Color(168.0f / 255.0f, 168.0f / 255.0f, 168.0f / 255.0f, 1.0f, "wedges"),
    session_cpp::Color(245.0f / 255.0f, 216.0f / 255.0f, 144.0f / 255.0f, 1.0f, "tsections"),
    session_cpp::Color(166.0f / 255.0f, 211.0f / 255.0f, 246.0f / 255.0f, 1.0f, "beds"),
};

/// The two closed loops a member is lofted between, top then bottom, vertex i facing vertex i: a member's geometry before it is an element.
using Loops = std::array<session_cpp::Polyline, 2>;

/// The kinds of contact the floor's design puts between two members; each is the name of the contact interaction between them, and decides the connector that goes there.
enum class ContactKind {
    seam_wedge, // The two seam beams either side of a seam: a wedge.
    oculus_wedge, // A quarter's oculus beam and its ring beam: a wedge.
    column_plate, // A column and an outer rib: a rectangle plate; the two plates of a corner get a cross lap.
    seam_tie, // Two outer ribs end to end at a seam, when the seam beams stop at the rib band: a tie.
    block_dowels, // A column block and a rib: dowels.
};

const std::array<std::string, 5> CONTACT_NAMES = {"seam_wedge", "oculus_wedge", "column_plate", "seam_tie", "block_dowels"}; // The interaction name of each kind, in ContactKind order.

/// mm below the floor top where every seam tie's key starts; the screws of the tied ribs stay above it.
const double TIE_TOP = 138.5;

/// mm, the closest two screw axes may come.
const double SCREW_SPACING = 8.0;

// ═══════════════════════════════════════════════════════════════════════════
// FloorGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The floor guide, a session ready to draw: the corners and the parameters, and the geometry every member is built from, computed by compute() on construction and again after a parameter changes and drawn into the session itself, grouped by quarter. It works for any convex four-corner bay: every method takes the quarter q, the quarter at corner q. A Floor builds the model from it.
class FloorGuide : public wood_session::WoodSession {
public:
    const std::array<session_cpp::Point, 4> corners; // Counter-clockwise at z 0.

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
    bool seam_through_ribs = true; // Run the two seam beams of every seam on through the outer rib band to the bay's outer face: the outer ribs end on the beams, the rib screws go from the beam's seam face into the rib end, and no ties are made.

    // what compute() derives from them
    session_cpp::Point centre; // The vertex centroid, where the bimedians cross and bisect each other.
    std::array<session_cpp::Point, 4> oculus_points; // Point q on seam q, oculus_radius from the centre.
    double soffit = 0.0; // The level of every inner and ring beam's soffit: the deepest end of a rib that ends on one, so every rib end meets its beam in full.

    /// The guide of the corners with the default parameters, computed.
    explicit FloorGuide(const std::array<session_cpp::Point, 4>& corners);

    /// The guide of the rectangle of half spans half_x and half_y about the origin, corner 0 at (-half_x, -half_y), with the default parameters, computed.
    static FloorGuide rectangle(double half_x, double half_y);

    /// Computes everything from the corners and the parameters, quarter by quarter in the order of the methods below, and redraws it; throws naming the failure when the corners are not counter-clockwise and convex at z 0, an oculus point leaves its seam, the rise leaves (0, height), an oculus point lies in an outer rib band or the ring would leave a quarter's oculus beam uncovered.
    void compute();

    /// Depth at every seam and at the oculus: height minus rise.
    double static_h() const;

    /// The midpoint of edge k, corner k to corner k + 1.
    session_cpp::Point midpoint(size_t k) const;

    /// The interior angle at corner k in degrees.
    double corner_angle(size_t k) const;

    // ═══════════════════════════════════════════════════════════════════════
    // Floor plan geometry
    // ═══════════════════════════════════════════════════════════════════════

    /// Quarter q in plan: corner q, the midpoint of edge q, oculus point q, oculus point q - 1, the midpoint of edge q - 1. Line 0 runs along edge q, line 1 is seam q, line 2 the oculus edge, line 3 seam q - 1, line 4 along edge q - 1.
    std::vector<session_cpp::Point> quarter_polygon(size_t q) const;

    /// The column head polygon at corner q, the ribs start from it: the corner, two shaft corners and the two chamfer points, in the column's frame.
    std::vector<session_cpp::Point> quarter_column_polygon(size_t q) const;

    /// The column's frame at corner q: origin the corner, x and y the edge directions at a right corner, symmetric about the corner bisector otherwise.
    session_cpp::Plane column_frame(size_t q) const;

    /// The support's plane at corner q, on the slab under the column axis, half a column head along both frame axes from the corner.
    session_cpp::Plane support_plane(size_t q) const;

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
    const std::vector<std::array<session_cpp::Polyline, 3>>& boundary_parabolas(size_t q) const;

    /// Quarter q's central panel between the inner ribs.
    const CentralPanel& central_panel(size_t q) const;

    /// Quarter q's bed panel tops, one per wedge, normal up: the plane fitted to each panel's deepest quad.
    const std::vector<session_cpp::Plane>& bed_top_planes(size_t q) const;

    /// The cutter levels at corner q: the datum, the outer rib bottoms, and minus column_head_depth.
    std::array<double, 3> column_levels(size_t q) const;

    /// The plane each outer rib of quarter q ends on at its seam: the seam plane, or the seam beam's far face when the seam runs through the rib band.
    std::array<session_cpp::Plane, 2> rib_seam_ends(size_t q) const;

    // ═══════════════════════════════════════════════════════════════════════
    // Members, each as its two face loops at the datum
    // ═══════════════════════════════════════════════════════════════════════

    /// Quarter q's bed plates in three rows, each row trimmed alike so every plate stays a quad.
    std::vector<std::vector<Loops>> beds(size_t q) const;

    /// Quarter q's six t-sections beside the ribs.
    std::vector<Loops> tsections(size_t q) const;

    /// Quarter q's two outer ribs along the bay edges: each its parabola trimmed by its end planes on its first face, and swept to its second.
    std::vector<Loops> outer_ribs(size_t q) const;

    /// Quarter q's two inner ribs, swept along the central panel's rib sweep.
    std::vector<Loops> inner_ribs(size_t q) const;

    /// Quarter q's three column blocks between the ribs at the column head, standing on the beds.
    std::vector<Loops> wedges(size_t q) const;

    /// Quarter q's three inner beams: seam 0, the oculus edge, seam 1.
    std::vector<Loops> inner_beams(size_t q) const;

    /// The oculus: four ring beams, each between its edge's tilted plane and ring inner plane from the previous beam's inner plane to the next beam's tilted plane (a pinwheel), four bottom wedges and the inner plate.
    std::vector<Loops> oculus() const;

    /// The ring's inner face on oculus edge q: the oculus beam's back face moved back by twice inner_beams.
    session_cpp::Plane ring_inner(size_t q) const;

    /// The six plates that carve the column head at corner q: the three fan faces down to the middle level, and three below it down to the head's depth.
    std::vector<Loops> column_cutters(size_t q) const;

    /// The column cutters of corner q lifted to the floor as solid difference cuts of the column.
    std::vector<wood_session::SolidCut> column_cuts(size_t q) const;

    /// The column's carved face on fan plane i of corner q (0 side 0, 1 the chamfer, 2 side 1) between the datum and the middle level: datum corners, then middle-level corners.
    std::vector<session_cpp::Point> column_face(size_t q, size_t i) const;

    /// A member bounded by a ring of side planes between a bottom and a top plane: corner i of each loop where sides i and i + 1 meet its plane; flip swaps the two loops.
    static Loops loft(const std::vector<session_cpp::Plane>& sides, const session_cpp::Plane& bottom, const session_cpp::Plane& top, bool flip = false);

    /// The distance between the area centroids of a member's two loops.
    static double thickness(const Loops& loops);

    /// The middle of a member: the mean of its two loops' area centroids.
    static session_cpp::Point body(const Loops& loops);

    /// The lowest corner of a member on a plane it ends on, at most 0.
    static double end_level(const Loops& loops, const session_cpp::Plane& end);

private:
    std::vector<ConstructionPlanes> _construction_planes; // Per quarter, cached by compute().
    std::vector<ConstructionQuads> _construction_quads;
    std::vector<std::array<double, 2>> _run_in;
    std::vector<std::vector<std::array<session_cpp::Polyline, 3>>> _boundary_parabolas;
    std::vector<CentralPanel> _central_panel;
    std::vector<std::vector<session_cpp::Plane>> _bed_top_planes;
    std::array<double, 4> _rib_bottom = {}; // Per corner, the middle cutter level.

    /// Why the corners and the oculus make no floor, empty when they do.
    std::string invalid() const;

    /// The angle at oculus point k between the two oculus edges that meet there, and between its seam and the next quarter's oculus edge, in degrees.
    double oculus_corner_angle(size_t k) const;
    double oculus_seam_angle(size_t k) const;

    /// Why the oculus is too close to a bay edge, empty when it is not: every quarter's inner beam corners must lie inside the outer rib bands.
    std::string beam_corners_in_bands() const;

    /// A member's two faces: the plane and its copy moved by distance along the normal.
    static std::array<session_cpp::Plane, 2> pair(const session_cpp::Plane& plane, double distance);

    /// The steps of compute() for quarter q, in order.
    ConstructionPlanes compute_construction_planes(size_t q) const;
    ConstructionQuads compute_construction_quads(const ConstructionPlanes& cp) const;
    std::array<double, 2> compute_run_in(size_t q) const;
    void set_block_planes(size_t q);
    std::vector<std::array<session_cpp::Polyline, 3>> compute_boundary_parabolas(size_t q) const;
    CentralPanel compute_central_panel(size_t q) const;
    std::vector<session_cpp::Plane> compute_bed_top_planes(size_t q) const;

    /// The outer parabola over a rib quad: from -height at the run-in along the axis, controlled at the axis midpoint at -static_h, to the seam at -static_h.
    session_cpp::Polyline outer_parabola(const session_cpp::Polyline& quad, double run_in) const;

    /// The z where an outer rib's soffit meets its fan plane, and the run-in that lands it on a level, by the secant from the wedge.
    double fan_end(const session_cpp::Polyline& quad, double run_in, const session_cpp::Plane& fan, const session_cpp::Plane& seam) const;
    double run_in_to_level(const session_cpp::Polyline& quad, const session_cpp::Plane& fan, const session_cpp::Plane& seam, double level) const;

    /// Rule A: the root of the closure nearest the reference, scanned without crossing a rib face and refined by bisection; the closure for one sweep; which side of each rib face a sweep crosses; the bisection; the sweep at degrees from the reference.
    static session_cpp::Vector rib_sweep(const std::array<session_cpp::Polyline, 2>& shadows, const std::array<session_cpp::Vector, 2>& normals, double thickness, const session_cpp::Vector& reference);
    static double closure(const std::array<session_cpp::Polyline, 2>& shadows, const std::array<session_cpp::Vector, 2>& normals, double thickness, const session_cpp::Vector& r);
    static bool sweep_sides(const std::array<session_cpp::Vector, 2>& normals, const session_cpp::Vector& reference, double degrees, std::array<bool, 2>& sides);
    static double bisect(const std::array<session_cpp::Polyline, 2>& shadows, const std::array<session_cpp::Vector, 2>& normals, double thickness, const session_cpp::Vector& reference, double lo, double hi);
    static session_cpp::Vector turned(const session_cpp::Vector& reference, double degrees);

    /// The members' outlines: a rib from its trace, a rib face's loop, a t-section, one beside an outer rib, a bed row, an outer bed row, a stretched cutter quad.
    static Loops rib(const session_cpp::Polyline& trace, const session_cpp::Plane& face1, const session_cpp::Vector& sweep, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1, bool inner);
    static session_cpp::Polyline rib_loop(const std::vector<session_cpp::Point>& pts, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1, bool inner);
    static Loops tsection(const session_cpp::Polyline& soffit, const session_cpp::Polyline& layer, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1, const session_cpp::Xform& projection10, const session_cpp::Xform& projection11);
    static Loops outer_tsection(const std::array<session_cpp::Polyline, 3>& parabola, const std::array<session_cpp::Plane, 2>& faces, const session_cpp::Vector& outer, const session_cpp::Vector& sweep, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1);
    static std::vector<Loops> bed_row(const std::array<session_cpp::Polyline, 2>& lower, const std::array<session_cpp::Polyline, 2>& upper, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1);
    static std::vector<Loops> outer_bed_row(const std::array<session_cpp::Polyline, 3>& parabola, const session_cpp::Plane& side0, const session_cpp::Plane& side1, const session_cpp::Vector& normal, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1);
    static std::vector<session_cpp::Point> stretch(std::vector<session_cpp::Point> quad, bool top);

    /// Draws the construction into the session by quarter, under the names the Floor gives the members: quarter_q holds plan_q and a group per family with a group per member, holding its plan quad, its two face planes and for a rib its parabolas.
    void draw();
};

// ═══════════════════════════════════════════════════════════════════════════
// The tables FloorGuide computes per quarter
// ═══════════════════════════════════════════════════════════════════════════

/// The planes of a quarter in pairs, one pair per member: the first plane is the member's base face, the second the face it is offset to.
class ConstructionPlanes {
public:
    std::vector<std::array<session_cpp::Plane, 2>> outer_ribs; // Along the two bay edges, the band offset inwards by outer_ribs.
    std::vector<std::array<session_cpp::Plane, 2>> inner_beams; // Along the two seams and the oculus edge; the oculus one tilted by the oculus plane angle.
    std::vector<std::array<session_cpp::Plane, 2>> inner_ribs; // From the column head chamfer to the inner beam corners.
    std::vector<std::array<session_cpp::Plane, 2>> wedges; // The column head fan: side 0, the middle one tilted by wedge_plane_angle, side 1.
    std::vector<std::array<session_cpp::Plane, 2>> tsections; // Beside the ribs, tsections thick: outer rib 0, inner rib 0 outer and central face, inner rib 1 central and outer face, outer rib 1.

};

/// One plan quad per member at the floor datum, index i the footprint of member i of that family.
class ConstructionQuads {
public:
    std::vector<session_cpp::Polyline> outer_ribs; // Two.
    std::vector<session_cpp::Polyline> inner_beams; // Three: seam 0, oculus edge, seam 1.
    std::vector<session_cpp::Polyline> inner_ribs; // Two.
    std::vector<session_cpp::Polyline> wedges; // Three.
    std::vector<session_cpp::Polyline> tsections; // Six.
};

/// The central panel of one quarter by rule A: its ruling, the one sweep of both inner ribs, and the soffit, +t and +2t traces on the two inner ribs' central faces.
class CentralPanel {
public:
    session_cpp::Vector ruling; // u: the panel's horizontal ruling, along which rib 0's central trace projects onto rib 1's.
    session_cpp::Vector rib_sweep; // r: the horizontal direction both inner ribs are swept along from their outer to their central face.
    std::array<std::array<session_cpp::Polyline, 3>, 2> traces; // Per inner rib, its central face's soffit, +t and +2t, offset in the panel's own cross-section.
};

}
