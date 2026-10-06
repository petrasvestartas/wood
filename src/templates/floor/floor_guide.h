#pragma once
#include "wood_session.h"

// The geometry of the timber floor, in the order it is computed; floor.h builds the model from it.
//   FloorGuide        the corners and parameters, and compute(): every part below, drawn
//   its parts         BayEdge, Seam, OculusEdge, ColumnCorner, then per quarter QuarterGeometry
//                     (ConstructionPlanes, ConstructionQuads, CentralPanel)
//   Quarter           one quarter's members as face loops; ColumnCutters carve the column heads
//   ContactFaces      where two members touch, ScrewLines where screws join them (OculusScrew aims the ring's)

namespace wood_floor {

class BayEdge;
class Seam;
class OculusEdge;
class ColumnCorner;
class QuarterGeometry;
class Quarter;

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

/// The floor guide, a session ready to draw: the corners, the parameters as its fields, and the geometry every member is built from, computed by compute() on construction and again after a parameter changes, drawn into the session itself, grouped by quarter. A Floor builds the model from it.
class FloorGuide : public wood_session::WoodSession {
public:
    const std::array<session_cpp::Point, 4> corners; // Counter-clockwise at z 0.

    // the parameters, each with its default; after changing one, compute() again
    double oculus_radius = 1000.0; // Distance of every oculus corner from the centre along its seam: a square diamond on a rectangular bay.
    double column_head = 220.0; // Side of the square column shaft and of the head polygon at the corner.
    double column_head_chamfer = 120.0; // Where the chamfer vertices sit on the shaft faces; also the capitel width.
    double outer_ribs = 100.0; // Outer rib thickness.
    double inner_ribs = 60.0; // Inner rib thickness.
    double inner_beams = 60.0; // Seam and oculus beam thickness; also the ring beam width at the datum.
    double wedge = 240.0; // Side wedge block thickness; the middle block is middle_wedge_factor times it.
    double tsections = 27.0; // Flange plane offset and bed layer thickness.
    double height = 650.0; // Rib depth where the parabola starts, a wedge thickness past the column face.
    double rise = 453.0; // Parabola rise from there to the seam.
    double wedge_plane_angle = -10.0; // Degrees the chamfer fan plane leans about its top edge.
    double oculus_plane_angle = 5.0; // Degrees the oculus bearing plane leans about its top edge.
    double column_head_depth = 730.0; // Depth of the carved head and of the capitel.
    double bay_height = 3500.0; // Storey: the floor top above the slab, the column top.
    double middle_wedge_factor = 1.25; // The middle block in wedge thicknesses.
    bool seam_through_ribs = true; // Run the two seam beams of every seam on through the outer rib band to the bay's outer face: the outer ribs end on the beams, the rib screws go from the beam's seam face into the rib end, and no ties are made.

    // what compute() derives from them, in that order
    session_cpp::Point centre; // The vertex centroid, where the bimedians cross and bisect each other.
    std::array<session_cpp::Point, 4> oculus_corners; // Corner q on seam q, oculus_radius from the centre.
    std::vector<BayEdge> edges; // Edge q from corner q to corner q + 1.
    std::vector<Seam> seams; // Seam q from the midpoint of edge q to the centre.
    std::vector<OculusEdge> oculus_edges; // Edge q from oculus corner q to oculus corner q - 1.
    std::vector<ColumnCorner> columns; // Column q at corner q.
    std::vector<QuarterGeometry> geometry; // Quarter q at corner q.
    double soffit = 0.0; // The level of every inner and ring beam's soffit: the deepest end of a rib that ends on one, so every rib end meets its beam in full.

    /// The guide of the corners with the default parameters, computed.
    explicit FloorGuide(const std::array<session_cpp::Point, 4>& corners);

    /// The guide of the rectangle of half spans half_x and half_y about the origin, corner 0 at (-half_x, -half_y), with the default parameters, computed.
    static FloorGuide rectangle(double half_x, double half_y);

    /// Computes everything from the corners and the parameters, its drawing redrawn; throws naming the failure when the corners are not counter-clockwise and convex at z 0, an oculus corner leaves its seam, the rise leaves (0, height), an oculus corner lies in an outer rib band or the ring would leave a quarter's oculus beam uncovered.
    void compute();

    /// Depth at every seam and at the oculus: height minus rise.
    double static_h() const;

    /// The midpoint of edge k, corner k to corner k + 1.
    session_cpp::Point midpoint(size_t k) const;

    /// The interior angle at corner k in degrees.
    double corner_angle(size_t k) const;

    /// The oculus corner angle at corner k in degrees, between the two oculus edges that meet there.
    double oculus_corner_angle(size_t k) const;

    /// The angle at oculus corner k between its seam and the next quarter's oculus edge, in degrees: the seam beam's end cut on that edge.
    double oculus_seam_angle(size_t k) const;

    /// A view of quarter q; it holds a reference and lives as long as the guide.
    Quarter quarter(size_t q) const;

    /// The oculus as face loops: four ring beams, each between its edge's tilted plane and ring inner plane from the previous beam's inner plane to the next beam's tilted plane (a pinwheel), four bottom wedges and the inner plate.
    std::vector<Loops> oculus() const;

private:
    /// Why the corners and oculus make no floor, empty when they do: the rise inside (0, height), the corners counter-clockwise and convex at z 0, every oculus corner between the centre and its edge midpoint, and the ring covering every quarter beam face, sin(oculus corner angle) >= sin(seam angle) at every oculus corner.
    std::string invalid() const;

    /// Why the oculus is too close to a bay edge, empty when it is not: every quarter's inner beam corners, where the seam beams' far faces meet the oculus beam's back face at the datum, must lie inside the outer rib bands, or the inner ribs end inside the outer ribs.
    std::string beam_corners_in_bands() const;

    /// The middle cutter level of quarter q's column: the deeper of its two outer ribs' bottom corners on their fan planes, on either face.
    double rib_bottom_level(size_t q) const;

    /// Draws the construction into the session by quarter, under the names the Floor gives the members: quarter_q holds plan_q (polygon_q, column_head_q, oculus_corner_q) and a group per family, with a group per member named as its element, holding its plan quad, its two face planes face_0 and face_1, and for a rib its parabolas: soffit, tsections_top and beds_top. Each family has its own colour.
    void draw();
};

// ═══════════════════════════════════════════════════════════════════════════
// The shared entities
// ═══════════════════════════════════════════════════════════════════════════

/// A bay edge: its line, midpoint and the two planes of the outer rib band on it, shared by the quarters on either side of the midpoint.
class BayEdge {
public:
    session_cpp::Line line; // Corner k to corner k + 1.
    session_cpp::Point midpoint; // Where the two quarters' outer ribs meet.
    std::array<session_cpp::Plane, 2> band; // The edge plane with its normal into the bay, and the same offset by outer_ribs.

    /// Bay edge k of the guide.
    BayEdge(const FloorGuide& guide, size_t k);
};

/// A seam: the line from an edge midpoint to the centre and its vertical plane, with the normal into the quarter on its beam-0 side.
class Seam {
public:
    size_t index = 0; // The quarter on the beam-0 side; quarter index + 1 is on the beam-2 side.
    session_cpp::Line line; // Midpoint to centre.
    session_cpp::Point oculus_corner; // Where the two seam beams end, on the line.
    session_cpp::Plane plane; // Through the line, origin at the midpoint of the half edge up to the oculus corner, normal into quarter index.
    double thickness = 0.0; // The seam beam thickness every quarter offsets by.

    /// Seam k of the guide, from the midpoint of edge k to the centre.
    Seam(const FloorGuide& guide, size_t k);

    /// The seam plane with its normal into that quarter.
    session_cpp::Plane plane_into(size_t quarter) const;

    /// The seam beam's two faces as that quarter reads them: the seam plane and its offset by thickness into the quarter.
    std::array<session_cpp::Plane, 2> faces_into(size_t quarter) const;
};

/// The oculus edge of one quarter: its line, the tilted bearing plane the quarter beam and the ring beam share, the vertical back face offset into the quarter, and the ring's inner plane.
class OculusEdge {
public:
    session_cpp::Line line; // Oculus corner q to oculus corner q - 1, the quarter polygon's third edge.
    session_cpp::Plane tilted; // The edge plane leaned by oculus_plane_angle about the edge.
    session_cpp::Plane back; // The edge plane offset by inner_beams into the quarter.
    session_cpp::Plane ring_inner; // The back face offset back by twice inner_beams: inner_beams inside the edge toward the centre.

    /// Oculus edge q of the guide.
    OculusEdge(const FloorGuide& guide, size_t q);
};

/// A column corner: the square column's frame, the head polygon, the carved fan, the cutter levels, the support plane and the column axis.
class ColumnCorner {
public:
    session_cpp::Point corner; // The bay corner.
    session_cpp::Vector x_axis; // Along the edge after the corner at a right corner, symmetric about the bisector otherwise.
    session_cpp::Vector y_axis; // Along the edge before the corner, reversed.
    std::vector<session_cpp::Point> head; // The head polygon in the frame: corner, two shaft corners, the two chamfer vertices.
    std::array<std::array<session_cpp::Plane, 2>, 3> wedge_fan; // Side 0, the tilted chamfer and side 1 with their far faces, each block's far face over its ribs' run-ins.
    std::array<session_cpp::Plane, 2> sides; // The head edges on the bay boundary, normal into the bay.
    std::array<double, 3> levels; // The cutter levels: the datum, the middle level at the outer rib bottoms and minus column_head_depth.
    session_cpp::Plane support_plane; // The support frame on the slab under the column axis.

    /// Column corner k of the guide before its fan is leaned.
    ColumnCorner(const FloorGuide& guide, size_t k);

    /// Leans the wedge fan on the quarter's inner ribs: the tilted chamfer plane, and the two side planes through the head edges, each parallel to the chamfer plane's crease with an inner rib's central face.
    void lean_fan(const std::array<session_cpp::Plane, 2>& inner_rib_faces, const FloorGuide& guide);

    /// The support on the slab at z 0 under the column axis in the corner frame.
    std::shared_ptr<wood_session::Support> to_support() const;

    /// The column: the square shaft in the corner frame from the support's column foot to the floor, with its head a chamfer wider along both axes over the column head depth.
    std::shared_ptr<wood_session::Column> to_column(const FloorGuide& guide, const wood_session::Support& support) const;
};

// ═══════════════════════════════════════════════════════════════════════════
// The geometry of a quarter
// ═══════════════════════════════════════════════════════════════════════════

/// The planes of a quarter in pairs, one pair per member: the first plane is the member's base face, the second the face it is offset to.
class ConstructionPlanes {
public:
    std::vector<std::array<session_cpp::Plane, 2>> outer_ribs; // Along the two bay edges, the band offset inwards by outer_ribs.
    std::vector<std::array<session_cpp::Plane, 2>> inner_beams; // Along the two seams and the oculus edge; the oculus one tilted by the oculus plane angle.
    std::vector<std::array<session_cpp::Plane, 2>> inner_ribs; // From the column head chamfer to the inner beam corners.
    std::vector<std::array<session_cpp::Plane, 2>> wedges; // The column head fan: side 0, the middle one tilted by wedge_plane_angle, side 1.
    std::vector<std::array<session_cpp::Plane, 2>> tsections; // Beside the ribs, tsections thick: outer rib 0, inner rib 0 outer and central face, inner rib 1 central and outer face, outer rib 1.

    ConstructionPlanes() = default;

    /// The member planes of quarter q, every one at the quarter's own points; leans the column's fan on the inner ribs.
    ConstructionPlanes(const FloorGuide& guide, size_t q, const std::vector<session_cpp::Point>& polygon, ColumnCorner& column);

    /// A member's two faces: the plane and its copy moved by distance along the normal.
    static std::array<session_cpp::Plane, 2> pair(const session_cpp::Plane& plane, double distance);
};

/// One plan quad per member at the floor datum, index i the footprint of member i of that family.
class ConstructionQuads {
public:
    std::vector<session_cpp::Polyline> outer_ribs; // Two.
    std::vector<session_cpp::Polyline> inner_beams; // Three: seam 0, oculus edge, seam 1.
    std::vector<session_cpp::Polyline> inner_ribs; // Two.
    std::vector<session_cpp::Polyline> wedges; // Three.
    std::vector<session_cpp::Polyline> tsections; // Six.

    ConstructionQuads() = default;

    /// The plan quad of every member at z 0, where its four planes meet the datum: corners 3-0, 0-1, 1-2 and 2-3.
    explicit ConstructionQuads(const ConstructionPlanes& cp);

private:
    /// The plan quads of a family of quad planes.
    static std::vector<session_cpp::Polyline> quads(const std::vector<std::array<session_cpp::Plane, 4>>& family);
};

/// The central panel of one quarter by rule A: its ruling, the one sweep of both inner ribs, and the soffit, +t and +2t traces on the two inner ribs' central faces.
class CentralPanel {
public:
    session_cpp::Vector ruling; // u: the panel's horizontal ruling, along which rib 0's central trace projects onto rib 1's.
    session_cpp::Vector rib_sweep; // r: the horizontal direction both inner ribs are swept along from their outer to their central face.
    std::array<std::array<session_cpp::Polyline, 3>, 2> traces; // Per inner rib, its central face's soffit, +t and +2t, offset in the panel's own cross-section.

    CentralPanel() = default;

    /// Rule A: the inner ribs' one sweep r so that rib 0's central trace projected along one ruling u lands on rib 1's, and the central traces by the layers.
    CentralPanel(const ConstructionPlanes& cp, const std::vector<std::array<session_cpp::Polyline, 3>>& parabolas, const FloorGuide& guide);

private:
    /// The shadows of the two inner ribs' outer faces, their normals, their thickness, and n0 - n1 the sweep is scanned around.
    std::array<session_cpp::Polyline, 2> shadows;
    std::array<session_cpp::Vector, 2> normals;
    double thickness = 0.0;
    session_cpp::Vector reference;

    /// The root of the closure nearest the reference, scanned in steps without crossing a rib face and refined by bisection; the reference itself when no root is bracketed.
    session_cpp::Vector solve_rib_sweep() const;

    /// The sine between the plan chords joining the two central traces at the start and at the vertex for the sweep at degrees from the reference, zero when one ruling joins both.
    double closure(double degrees) const;

    /// Whether the sweep at degrees from the reference crosses both rib faces, and on which side of each.
    bool sweep_sides(double degrees, std::array<bool, 2>& sides) const;

    /// The root of the closure between two scanned angles, by bisection.
    double bisect(double lo, double hi) const;

    /// The plan unit vector turned by degrees about z from the reference.
    session_cpp::Vector turned(double degrees) const;
};

/// The geometry of one quarter, computed once by the guide: polygon, planes, quads, run-ins, parabolas, the central panel and the bed planes.
class QuarterGeometry {
public:
    std::vector<session_cpp::Point> polygon; // Corner, midpoint, oculus corner, oculus corner, midpoint.
    ConstructionPlanes planes; // The member planes.
    ConstructionQuads quads; // The plan quad of every member at z 0.
    std::array<double, 2> run_in = {0.0, 0.0}; // Per outer rib, the straight run-in along its axis from the fan plane's datum trace to where its parabola starts, mm, solved so both outer ribs of a corner end at one level; the side column blocks are as thick, the middle one middle_wedge_factor times their mean.
    std::vector<std::array<session_cpp::Polyline, 3>> parabolas; // Outer 0, outer 1, shadow 0, shadow 1, each with its +t and +2t offsets.
    CentralPanel central_panel; // The central panel by rule A.
    std::vector<session_cpp::Plane> bed_top_planes; // Per bed panel, the plane fitted to its deepest quad, normal up.

    /// Quarter q's geometry in dependency order: polygon, planes, quads, run-ins, the blocks' far planes over them and the quads again, parabolas and shadows, the central panel, the bed planes; the column's fan is leaned and its blocks set.
    QuarterGeometry(const FloorGuide& guide, size_t q, ColumnCorner& column);

private:
    /// The outer parabola over a rib quad, a 7-point Bezier: from -height at the run-in along the axis past the fan plane's datum trace, controlled at the axis midpoint at -static_h, to the seam at -static_h.
    static session_cpp::Polyline outer_parabola(const session_cpp::Polyline& quad, double run_in, const FloorGuide& guide);

    /// The z where an outer rib's soffit meets its fan plane: the bottom of the rib's column end face.
    static double fan_end(const session_cpp::Polyline& quad, double run_in, const session_cpp::Plane& fan, const session_cpp::Plane& seam, const FloorGuide& guide);

    /// The run-in that lands an outer rib's end on the level, by the secant from the wedge; throws when it leaves the axis or does not converge.
    static double run_in_to_level(const session_cpp::Polyline& quad, const session_cpp::Plane& fan, const session_cpp::Plane& seam, double level, const FloorGuide& guide);

    /// Per outer rib the run-in that lands its end on the corner's shared level, the shallower of the two ends at the wedge.
    std::array<double, 2> run_ins(const FloorGuide& guide) const;

    /// Sets the column blocks' far planes: each side block its fan plane offset by its own rib's run-in, the middle block by middle_wedge_factor times their mean.
    void set_block_planes(ColumnCorner& column, const FloorGuide& guide);

    /// Per rib axis (outer 0, outer 1, shadow 0, shadow 1) the parabola and its two offsets by tsections: the outer ones over their run-ins, the shadows projected onto the inner ribs' outer faces along the outer rib normals.
    std::vector<std::array<session_cpp::Polyline, 3>> boundary_parabolas(const FloorGuide& guide) const;

    /// Per bed panel the plane fitted to its deepest quad, normal up: its top layer on the panel's two side planes, each cut there by the panel planes.
    std::vector<session_cpp::Plane> fit_bed_top_planes() const;
};

// ═══════════════════════════════════════════════════════════════════════════
// The members of a quarter
// ═══════════════════════════════════════════════════════════════════════════

/// One quarter as a view of the guide: its members as face loops at the floor datum z 0, each family in member order.
class Quarter {
public:
    const FloorGuide& guide; // The guide the quarter belongs to.
    const size_t index; // Counter-clockwise from corner 0.

    /// Quarter q of the guide.
    Quarter(const FloorGuide& guide, size_t q);

    /// The quarter's geometry.
    const QuarterGeometry& geometry() const;

    /// The column corner the quarter starts from.
    const ColumnCorner& column() const;

    /// The two outer ribs along the bay edges: each its soffit trace trimmed by its end planes on its first face, and swept to its second (R4).
    std::vector<Loops> outer_ribs() const;

    /// The plane each outer rib ends on at its seam: the seam plane, or the seam beam's far face when the seam runs through the rib band.
    std::array<session_cpp::Plane, 2> rib_seam_ends() const;

    /// The two inner ribs from the column head towards the oculus, swept along the central panel's rib sweep.
    std::vector<Loops> inner_ribs() const;

    /// The three inner beams: seam 0, the oculus edge, seam 1.
    std::vector<Loops> inner_beams() const;

    /// The three column blocks between the ribs at the column head, standing on the beds.
    std::vector<Loops> wedges() const;

    /// The six t-sections beside the ribs.
    std::vector<Loops> tsections() const;

    /// The bed plates in three rows, each row trimmed alike so every plate stays a quad.
    std::vector<std::vector<Loops>> beds() const;

    /// The column's carved face on fan plane i (0 side 0, 1 the chamfer, 2 side 1) between the datum and the middle level: datum corners, then middle-level corners.
    std::vector<session_cpp::Point> column_face(size_t i) const;

    /// A member bounded by a ring of side planes between a bottom and a top plane: corner i of each loop where sides i and i + 1 meet its plane; flip swaps the two loops.
    static Loops loft(const std::vector<session_cpp::Plane>& sides, const session_cpp::Plane& bottom, const session_cpp::Plane& top, bool flip = false);

    /// The distance between the area centroids of a member's two loops.
    static double thickness(const Loops& loops);

    /// The middle of a member: the mean of its two loops' area centroids.
    static session_cpp::Point body(const Loops& loops);

    /// The lowest corner of a member on a plane it ends on, at most 0.
    static double end_level(const Loops& loops, const session_cpp::Plane& end);

private:
    /// A rib: its soffit trace trimmed by the two end planes on its first face, and on its second face the trace swept along the rib with its end corners cut on the end planes; an inner rib also ends its base on the second plane.
    static Loops rib(const session_cpp::Polyline& trace, const session_cpp::Plane& face1, const session_cpp::Vector& sweep, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1, bool inner);

    /// A rib face's loop: the trace closed up to z 0 over the end planes.
    static session_cpp::Polyline rib_loop(const std::vector<session_cpp::Point>& pts, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1, bool inner);

    /// A t-section: its soffit and +t traces trimmed on its first face and closed into one loop, the same projected onto its second face.
    static Loops tsection(const session_cpp::Polyline& soffit, const session_cpp::Polyline& layer, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1, const session_cpp::Xform& projection10, const session_cpp::Xform& projection11);

    /// The t-section beside an outer panel rib face: the outer parabola and its +t projected along the outer rib normal onto the face, the soffit continued to the far face along the sweep, the +t along the panel.
    static Loops outer_tsection(const std::array<session_cpp::Polyline, 3>& parabola, const std::array<session_cpp::Plane, 2>& faces, const session_cpp::Vector& outer, const session_cpp::Vector& sweep, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1);

    /// One bed row: the lower and upper layer on the panel's two side planes trimmed alike, one quad pair per segment.
    static std::vector<Loops> bed_row(const std::array<session_cpp::Polyline, 2>& lower, const std::array<session_cpp::Polyline, 2>& upper, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1);

    /// An outer bed row: the parabola's +t and +2t projected along the outer rib normal onto the panel's two side planes.
    static std::vector<Loops> outer_bed_row(const std::array<session_cpp::Polyline, 3>& parabola, const session_cpp::Plane& side0, const session_cpp::Plane& side1, const session_cpp::Vector& normal, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1);
};

/// The six plates that carve a column head at a quarter's corner, and the cuts they make in the column.
class ColumnCutters {
public:
    std::vector<Loops> plates; // The three fan faces down to the middle level, then three below it down to the head's depth.

    /// The cutters of the quarter's column.
    explicit ColumnCutters(const Quarter& quarter);

    /// The plates lifted to the floor as solid difference cuts of the column: features of the column, not elements of the scene.
    std::vector<wood_session::SolidCut> cuts(double bay_height) const;

private:
    /// The cutter quad stretched in its own plane: its long sides by the margin at both ends, then its short sides inwards, both for a top quad, only the first for a bottom one.
    static std::vector<session_cpp::Point> stretch(std::vector<session_cpp::Point> quad, bool top);
};

// ═══════════════════════════════════════════════════════════════════════════
// Contacts and screws
// ═══════════════════════════════════════════════════════════════════════════

/// Where two members of the floor touch, by the rules of the design: each contact a face interaction, named by its kind and place, its polygon read from the members' loops and lifted to the floor.
class ContactFaces {
public:
    /// The contacts of the guide's floor.
    explicit ContactFaces(const FloorGuide& guide);

    /// Inner beam 0 of quarter q and inner beam 2 of quarter q + 1 on the seam plane, where their end faces overlap.
    std::shared_ptr<wood_session::InteractionContactFace> seam_wedge(size_t q) const;

    /// Inner beam 1 of quarter q and ring beam q on the tilted plane: the beam's face on it.
    std::shared_ptr<wood_session::InteractionContactFace> oculus_wedge(size_t q) const;

    /// Column q and outer rib k: the rib's column end face on its fan plane where it meets the column's carved face.
    std::shared_ptr<wood_session::InteractionContactFace> column_plate(size_t q, size_t k) const;

    /// Outer rib 0 of quarter q and outer rib 1 of quarter q + 1 end to end on the seam plane: rib 0's seam end face.
    std::shared_ptr<wood_session::InteractionContactFace> seam_tie(size_t q) const;

    /// Column block k of quarter q on one of its two ribs: the block's face on that rib's plane.
    std::shared_ptr<wood_session::InteractionContactFace> block_dowels(size_t q, size_t k, size_t side) const;

private:
    const FloorGuide& guide;
    const session_cpp::Xform lift; // Up from the datum to the floor.

    /// A face interaction of a kind at a place, named <kind>_<place>, its polygon closed and lifted.
    std::shared_ptr<wood_session::InteractionContactFace> face(ContactKind kind, const std::string& place, wood_session::ContactType type, const session_cpp::Polyline& polygon) const;
};

/// Where the assembly screws go, as 200 mm lines in world coordinates, between members that butt.
class ScrewLines {
public:
    /// The screws of the guide's floor.
    explicit ScrewLines(const FloorGuide& guide);

    /// Outer rib k of quarter q into the seam beam it meets: along the beam from the rib's outer face, or, when the seam runs through the rib band, along the rib from the beam's seam face into the rib end, 20 mm below its top and above its bottom and either side of its axis.
    std::vector<session_cpp::Line> rib_beam(size_t q, size_t k) const;

    /// Seam beam 0 (k 0) or 2 (k 1) of quarter q into the oculus beam ending on it, along the oculus beam from the seam plane.
    std::vector<session_cpp::Line> beam_mitre(size_t q, size_t k) const;

    /// The oculus beam of quarter q into inner rib k ending on its back face, along the rib through the beam corner; throws when the bay is too narrow for them.
    std::vector<session_cpp::Line> rib_corner(size_t q, size_t k) const;

    /// Whether the inner rib screws of quarter q at end k pass the seam beam's end at the beam corner.
    bool passes_seam_beam(size_t q, size_t k, const std::vector<session_cpp::Line>& screws) const;

    /// Ring beam q into ring beam q + 1, along ring beam q + 1 from ring beam q's tilted face.
    std::vector<session_cpp::Line> ring(size_t q) const;

    /// Ring beam q into the oculus beam of quarter q at its end k, aimed by OculusScrew.
    std::vector<session_cpp::Line> oculus(size_t q, size_t k) const;

private:
    const FloorGuide& guide;
    const std::vector<Loops> rings; // The oculus loops, the four ring beams first.
    const session_cpp::Xform lift; // Up from the datum to the floor.

    /// The lines lifted to the floor.
    std::vector<session_cpp::Line> lifted(const std::vector<session_cpp::Line>& lines) const;

    /// A screw at level z through a side member into the member butting on it, along the butting member's axis: the head where that axis leaves the side member's far face, the tip on towards the butting member's body.
    static session_cpp::Line along_axis(const std::array<session_cpp::Plane, 2>& butting, const session_cpp::Plane& far_face, const session_cpp::Point& butting_body, double z);

    /// A screw at level z along a rib ending on a seam beam that runs through the rib band, its axis offset across the rib, from the beam's seam face through the beam into the rib end.
    static session_cpp::Line from_seam_face(const std::array<session_cpp::Plane, 2>& rib, const std::array<session_cpp::Plane, 2>& beam, double z, double offset);

    /// The axis of a member between two faces at level z: the line midway between their traces.
    static session_cpp::Line axis(const std::array<session_cpp::Plane, 2>& faces, double z);

    /// The level of a screw in a corner's level set: down from the datum in sevenths of the depth.
    double corner_level(double levels) const;
};

/// The screws from a ring beam through the oculus wedge's contact into a quarter's oculus beam towards one of its corners, each the 200 mm line with the largest clearance, found on a coarse grid of head offsets and angles and refined around its best.
class OculusScrew {
public:
    /// The screws of ring beam q into quarter q's oculus beam at its end k.
    OculusScrew(const FloorGuide& guide, const std::vector<Loops>& rings, size_t q, size_t k);

    /// The screw at level z.
    session_cpp::Line at(double z) const;

private:
    /// A head offset along the ring's inner face from the corner and an angle off square to the contact, in degrees, with its clearance.
    class Aim {
    public:
        double offset = 0.0;
        double angle = 0.0;
        double clearance = -1e300;
    };

    std::array<session_cpp::Plane, 2> beam; // The oculus beam: the tilted face it shares with the ring, its back face.
    session_cpp::Plane beam_end; // The seam beam's inner face the oculus beam ends on at this corner.
    session_cpp::Point beam_body; // A point inside the oculus beam.
    session_cpp::Plane inner; // The ring beam's inner face, where the heads sit.
    session_cpp::Plane ring_end; // The ring beam's end plane at this corner.
    session_cpp::Point ring_body; // A point inside the ring beam.
    session_cpp::Point wedge_start; // The contact's top edge end at this corner moved along the edge by the wedge's margin.
    session_cpp::Vector along; // Along the contact's top edge, away from this corner.
    double band = 0.0; // Half the beam thickness: within it of the contact plane the wedge's pocket lies.

    /// The best aim on a grid of offsets and angles around a centre, each within its range.
    Aim best_aim(const session_cpp::Point& start, const session_cpp::Vector& across, const Aim& centre, double offset_span, double angle_span, double offset_step, double angle_step) const;

    /// How far a screw from head along u keeps inside: the head and the contact crossing inside the ring's end, the part within the pocket band short of the wedge, the tip inside the oculus beam's back face and the seam beam end.
    double clearance(const session_cpp::Point& head, const session_cpp::Vector& u) const;
};

}
