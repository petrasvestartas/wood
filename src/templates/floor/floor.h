#pragma once
#include "wood_session.h"

// The timber floor, in the order it is built:
//   FloorGuide        the corners and parameters, and compute(): every part below, drawn
//   its parts         BayEdge, Seam, OculusEdge, ColumnCorner, then per quarter QuarterGeometry
//                     (ConstructionPlanes, ConstructionQuads, CentralPanel)
//   Outline, Quarter  a member's two loops and the element made from them; one quarter's outlines, made by Rib,
//                     TSection, BedRow and ColumnCutters
//   relationships     MemberRef, Relationship, Contacts and Screws (OculusScrew aims the ring screws)
//   Floor             the model: the members placed, the connectors and the screws added

namespace wood_floor {

class BayEdge;
class Seam;
class OculusEdge;
class ColumnCorner;
class QuarterGeometry;
class Quarter;
class Outline;
class Relationship;

// ═══════════════════════════════════════════════════════════════════════════
// Vocabulary
// ═══════════════════════════════════════════════════════════════════════════

/// The member families of the floor: the six of a quarter in the order they are added, then the ring beams, the columns and the supports.
enum class Family {
    outer_ribs, // Variable beams under the two outer parabolas.
    inner_ribs, // Variable beams under the two inner parabolas.
    inner_beams, // Variable beams between two slanted end faces.
    wedges, // The three wedge block plates at the column head.
    tsections, // T-section plates.
    beds, // Bed plates.
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

/// What two members share and the connector that belongs to it; the screw kinds are the assembly screws, pre-drilled lines both members read.
enum class Relation { support, column_plate, cross_lap, seam_tie, seam_wedge, oculus_wedge, block_dowels, screw_rib_beam, screw_beam_mitre, screw_rib_corner, screw_ring, screw_oculus };

/// The screw relation kinds in the order the relationships list them.
const std::array<Relation, 5> SCREW_RELATIONS = {Relation::screw_rib_beam, Relation::screw_beam_mitre, Relation::screw_rib_corner, Relation::screw_ring, Relation::screw_oculus};

/// The relation kinds of the connectors: the wedges, the column plates and their cross laps, the ties and the block dowels.
const std::vector<Relation> CONNECTOR_RELATIONS = {Relation::seam_wedge, Relation::oculus_wedge, Relation::column_plate, Relation::cross_lap, Relation::seam_tie, Relation::block_dowels};

/// The colour of every connector node and of every part and dowel node nested under it: the Block Research Group's primary blue.
const session_cpp::Color CONNECTOR_COLOR = session_cpp::Color(33.0f / 255.0f, 150.0f / 255.0f, 234.0f / 255.0f, 1.0f, "brg_blue");

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

    /// The oculus: four ring beams, each between its edge's tilted plane and ring inner plane from the previous beam's inner plane to the next beam's tilted plane (a pinwheel), four bottom wedges and the inner plate.
    std::vector<Outline> oculus() const;

    /// Every relationship of the floor in the order the connectors are named in: the contacts, then the screws.
    std::vector<Relationship> relationships() const;

    /// The relationships of one kind, in the same order.
    std::vector<Relationship> relationships(Relation kind) const;

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

/// The two closed loops a member is lofted between, as the guide authors them; the elements built from them choose which is the bottom.
class Outline {
public:
    session_cpp::Polyline top; // First loop.
    session_cpp::Polyline bottom; // Second loop, vertex i facing vertex i of the first.

    /// The member bounded by a ring of side planes between a bottom and a top plane: corner i of each loop where sides i and i + 1 meet its plane; flip swaps the two.
    static Outline loft(const std::vector<session_cpp::Plane>& sides, const session_cpp::Plane& bottom, const session_cpp::Plane& top, bool flip = false);

    /// The distance between the area centroids of the two loops.
    double thickness() const;

    /// The middle: the mean of the two loops' area centroids.
    session_cpp::Point body() const;

    /// The lowest corner on a plane the member ends on, at most 0.
    double end_level(const session_cpp::Plane& end) const;

    /// A rib as a variable beam: one section per parabola point up to the top edge, its far corners read from the second loop, so the end sections lie in the end planes.
    std::shared_ptr<wood_session::BeamVariable> to_rib(const std::string& name) const;

    /// A four-corner member as a variable beam between the end sections over corners start and end, start[i] and end[i] on one long edge.
    std::shared_ptr<wood_session::BeamVariable> to_beam(const std::array<size_t, 2>& start, const std::array<size_t, 2>& end, const std::string& name) const;

    /// The member as a plate, bottom then top.
    std::shared_ptr<wood_session::Plate> to_plate(const std::string& name) const;
};

/// One quarter as a view of the guide: only member outlines built, every one at the floor datum z 0.
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

    /// The two outer ribs along the bay edges.
    std::vector<Outline> outer_ribs() const;

    /// The plane each outer rib ends on at its seam: the seam plane, or the seam beam's far face when the seam runs through the rib band.
    std::array<session_cpp::Plane, 2> rib_seam_ends() const;

    /// The two inner ribs from the column head towards the oculus.
    std::vector<Outline> inner_ribs() const;

    /// The three inner beams on the seams and the oculus edge.
    std::vector<Outline> inner_beams() const;

    /// The three wedge blocks between the ribs at the column head, standing on the beds.
    std::vector<Outline> wedges() const;

    /// The six t-sections beside the ribs.
    std::vector<Outline> tsections() const;

    /// The bed plates in three rows.
    std::vector<std::vector<Outline>> beds() const;

    /// The column's carved face on fan plane i (0 side 0, 1 the chamfer, 2 side 1) between the datum and the middle level: datum corners, then middle-level corners.
    std::vector<session_cpp::Point> column_face(size_t i) const;
};

/// A rib's outline: its soffit trace trimmed by the two end planes on its first face, and on its second face the trace swept along the rib with its end corners cut on the end planes (R4).
class Rib {
public:
    /// The rib of a soffit trace, its second face, the sweep between its faces and its two end planes; an inner rib also ends its base on the second plane.
    Rib(const session_cpp::Polyline& trace, const session_cpp::Plane& face1, const session_cpp::Vector& sweep, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1, bool inner);

    /// The two face loops.
    Outline outline() const;

private:
    std::vector<session_cpp::Point> near; // The trimmed soffit trace on the first face, from the first end plane.
    std::vector<session_cpp::Point> far; // The same swept onto the second face, its ends on the end planes.
    session_cpp::Plane cut_plane0; // The end plane at the column.
    session_cpp::Plane cut_plane1; // The end plane at the seam or the oculus beam.
    bool inner = false; // An inner rib, its base ending on the second plane too.

    /// A face's loop: the trace closed up to z 0 over the end planes.
    session_cpp::Polyline loop(const std::vector<session_cpp::Point>& pts) const;
};

/// A t-section's outline: its soffit and +t traces trimmed on its first face and closed into one loop, the same projected onto its second face.
class TSection {
public:
    Outline outline;

    /// The t-section of a soffit and a layer trace, trimmed by the two end planes, carried to the second face by the two projections.
    TSection(const session_cpp::Polyline& soffit, const session_cpp::Polyline& layer, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1, const session_cpp::Xform& projection10, const session_cpp::Xform& projection11);

    /// The t-section beside an outer panel rib face: the outer parabola and its +t projected along the outer rib normal onto the face, the soffit continued to the far face along the sweep, the +t along the panel.
    static TSection beside_outer(const std::array<session_cpp::Polyline, 3>& parabola, const std::array<session_cpp::Plane, 2>& faces, const session_cpp::Vector& outer, const session_cpp::Vector& sweep, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1);
};

/// One row of bed plates: the lower and upper layer on the panel's two side planes trimmed alike, one quad pair per segment, so on a skewed bay too.
class BedRow {
public:
    std::vector<Outline> plates;

    /// The row between the lower and the upper layers on the two side planes, trimmed by the two end planes.
    BedRow(const std::array<session_cpp::Polyline, 2>& lower, const std::array<session_cpp::Polyline, 2>& upper, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1);

    /// An outer row: the parabola's +t and +2t projected along the outer rib normal onto the panel's two side planes.
    static BedRow outer(const std::array<session_cpp::Polyline, 3>& parabola, const session_cpp::Plane& side0, const session_cpp::Plane& side1, const session_cpp::Vector& normal, const session_cpp::Plane& cut_plane0, const session_cpp::Plane& cut_plane1);
};

/// The six plates that carve a column head at a quarter's corner, and the cuts they make in the column.
class ColumnCutters {
public:
    std::vector<Outline> plates;

    /// The cutters of the quarter's column: the three fan faces down to the middle level, and three below it down to the head's depth.
    explicit ColumnCutters(const Quarter& quarter);

    /// The plates lifted to the floor as solid difference cuts of the column: features of the column, not elements of the scene.
    std::vector<wood_session::SolidCut> cuts(double bay_height) const;

private:
    /// The cutter quad stretched in its own plane: its long sides by the margin at both ends, then its short sides inwards, both for a top quad, only the first for a bottom one.
    static std::vector<session_cpp::Point> stretch(std::vector<session_cpp::Point> quad, bool top);
};

// ═══════════════════════════════════════════════════════════════════════════
// Relationships
// ═══════════════════════════════════════════════════════════════════════════

/// A member of the floor by quarter (-1 for the ring, the columns and the supports), family and index; row for a bed plate.
class MemberRef {
public:
    int quarter = -1; // The quarter the member belongs to, -1 for the ring, a column or a support.
    Family family = Family::outer_ribs; // Its family.
    size_t index = 0; // Its index in the family, or in the bed row.
    int row = -1; // The bed row, -1 for every other family.

    /// Member index of a family of quarter q.
    static MemberRef of_quarter(size_t quarter, Family family, size_t index);

    /// A ring beam, a column or a support, which belong to no quarter.
    static MemberRef shared(Family family, size_t index);

    /// The scene name of the member, as the model names it.
    std::string name() const;
};

/// One relationship: the two members by the rule of the design, the shared plane, the contact polygon read from their outlines in world coordinates, and the seam or corner it belongs to.
class Relationship {
public:
    Relation kind = Relation::support; // What the two members share.
    MemberRef a; // The first member by the rule.
    MemberRef b; // The second member by the rule.
    session_cpp::Plane plane; // The plane they meet on, lifted to the floor.
    session_cpp::Polyline contact; // The contact polygon on that plane, closed; empty for a support or a cross lap.
    wood_session::ContactType type = wood_session::ContactType::unknown; // The contact class the kernel's search reports for the pair, unknown where the design does not fix it.
    size_t seam_or_corner = 0; // The seam or the corner the relationship belongs to.
    std::vector<session_cpp::Line> screws; // A screw relationship's screw axes, head to tip, in world coordinates; empty for every other kind.
    std::vector<MemberRef> through; // The members a screw relationship's screws pass besides a and b: the seam beam end an inner rib screw crosses at the beam corner.
    std::optional<session_cpp::Plane> end; // The plane a seam wedge runs on to when the seam beams run through the rib band: the bay's outer face.

    /// The name of a relation kind, as text() writes it.
    static std::string kind_name(Relation kind);

    /// The contact area in mm2.
    double area() const;

    /// The relationship as text: its kind and its two members.
    std::string text() const;
};

/// The contacts the connectors stand on: wedges, column plates and their cross laps, ties, block dowels and supports.
class Contacts {
public:
    /// The contacts of the guide's floor.
    explicit Contacts(const FloorGuide& guide);

    /// The seam wedges, the oculus wedges, the column plates, the cross laps, the ties (none when the seam runs through the ribs), the block dowels and the supports, each kind in quarter order.
    std::vector<Relationship> rows() const;

private:
    const FloorGuide& guide;
    const session_cpp::Xform lift; // Up from the datum to the floor.

    /// Inner beam 0 of q and inner beam 2 of q + 1 on the seam plane, the contact where their end faces on it overlap; run on to the bay's outer face when the beams run through the rib band.
    Relationship seam_wedge(size_t q) const;

    /// Inner beam 1 of q and ring beam q on the tilted plane, the contact beam 1's loop on it.
    Relationship oculus_wedge(size_t q) const;

    /// Column q and outer rib k: the rib's column end face on the fan side plane where it meets the column's carved face, down to the middle cutter level.
    Relationship column_plate(size_t q, size_t k) const;

    /// The two column plates of corner q, on outer ribs 0 and 1, crossing inside the column.
    Relationship cross_lap(size_t q) const;

    /// Outer rib 0 of q and outer rib 1 of q + 1 meeting end to end on the seam plane, the contact rib 0's seam end face.
    Relationship seam_tie(size_t q) const;

    /// Block k of quarter q on one of its two ribs: the block's face on that rib's plane, corners 3 and 0 of its loops on the first rib plane, 1 and 2 on the second.
    Relationship block_dowels(size_t q, size_t k, size_t side, const MemberRef& rib) const;

    /// The support of corner q under its column.
    Relationship support(size_t q) const;
};

/// The assembly screws: per quarter the outer ribs into the seam beams, the mitres and the inner rib ends, then the ring corners and the ring into the quarters' oculus beams.
class Screws {
public:
    /// The screws of the guide's floor.
    explicit Screws(const FloorGuide& guide);

    /// Every screw relationship with its screw axes, in that order.
    std::vector<Relationship> rows() const;

private:
    const FloorGuide& guide;
    const std::vector<Outline> rings; // The oculus outlines, the four ring beams first.
    const session_cpp::Xform lift; // Up from the datum to the floor.

    /// Outer rib k of quarter q into the seam beam it meets: along the beam from the rib's outer face, or, when the seam runs through the rib band, along the rib from the beam's seam face into the rib end, 20 mm below its top and above its bottom and either side of its axis.
    Relationship rib_beam(size_t q, size_t k) const;

    /// Seam beam 0 (k 0) or 2 (k 1) of quarter q into the oculus beam ending on it, along the oculus beam from the seam plane.
    Relationship beam_mitre(size_t q, size_t k) const;

    /// The oculus beam of quarter q into inner rib k ending on its back face, along the rib through the beam corner, so they also pass the seam beam's end where the corner needs it.
    Relationship rib_corner(size_t q, size_t k) const;

    /// Ring beam q into ring beam q + 1, along ring beam q + 1 from ring beam q's tilted face.
    Relationship ring(size_t q) const;

    /// Ring beam q into the oculus beam of quarter q at its end k, aimed by OculusScrew.
    Relationship oculus(size_t q, size_t k) const;

    /// A screw relationship: the two members, the face the second ends on, its end face there and the screws, lifted to the floor.
    Relationship row(Relation kind, const MemberRef& a, const MemberRef& b, const session_cpp::Plane& plane, const std::vector<session_cpp::Point>& contact, const std::vector<session_cpp::Line>& screws, size_t corner) const;

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
    OculusScrew(const FloorGuide& guide, const std::vector<Outline>& rings, size_t q, size_t k);

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

// ═══════════════════════════════════════════════════════════════════════════
// Floor
// ═══════════════════════════════════════════════════════════════════════════

/// A member in the scene with the thickness the connectors on it are sized by.
class Member {
public:
    std::shared_ptr<session_cpp::Element> element; // The element, placed once added.
    double thickness = 0.0; // The thickness of the outline it was built from.

    /// The outlines of one family as members: ribs and inner beams as variable beams, every other family as plates.
    static std::vector<Member> of_family(Family family, const std::vector<Outline>& outlines);
};

/// The members of one quarter in the scene, by family.
class QuarterMembers {
public:
    std::vector<Member> outer_ribs; // Two variable beams.
    std::vector<Member> inner_ribs; // Two variable beams.
    std::vector<Member> inner_beams; // Three variable beams.
    std::vector<Member> wedges; // Three wedge block plates.
    std::vector<Member> tsections; // Six plates.
    std::vector<std::vector<Member>> beds; // Three rows of plates.

    /// The member a quarter reference names, null when the family or index is not in the quarter.
    const Member* get(const MemberRef& ref) const;
};

/// A column in the scene: the support and the column, carved by its six head cuts.
class ColumnModel {
public:
    std::shared_ptr<wood_session::Support> support; // On the slab.
    std::shared_ptr<wood_session::Column> column; // Carved by the head cuts.
};

/// The floor model, a session built step by step from a guide, grouped by quarter: quarter_0 to quarter_3 each with its members, its column, its part of the oculus ring and its connectors and screws, and the oculus with the central plate.
class Floor : public wood_session::WoodSession {
public:
    const FloorGuide guide; // The geometry the model is built from.
    std::array<QuarterMembers, 4> quarters; // The placed members of quarter q.
    std::vector<Member> ring; // The four ring beams.
    std::vector<ColumnModel> columns; // Column q at corner q, empty until the columns are added.
    std::vector<std::shared_ptr<wood_session::JointBeam>> connectors; // Every connector added: wedges, plates, cross laps, ties and dowels.
    std::vector<std::shared_ptr<wood_session::JointBeam>> screws; // Every screw connector added.

    /// An empty model of the guide, the session named name.
    explicit Floor(const FloorGuide& guide, const std::string& name = "floor");

    /// Not copied: the members name this session's own objects.
    Floor(const Floor&) = delete;

    /// Not assigned, as it is not copied.
    Floor& operator=(const Floor&) = delete;

    /// Adds the quarters, the oculus and the columns: every member of the floor.
    void add_members();

    /// Adds the four quarters, each lifted to bay_height and grouped by family.
    void add_quarters();

    /// Adds the oculus lifted to bay_height: ring beam q and bottom wedge q in oculus_q of quarter q, the central plate in oculus.
    void add_oculus();

    /// Adds the column at every corner.
    void add_columns();

    /// Adds the column at one corner: its support, the column, the support joint and the column's six head cuts.
    void add_column(size_t corner);

    /// Adds one connector per relationship of the kinds asked for, named `<prefix>_<n>` within its kind, numbered on from those already in the session, under connectors_q of its quarter, and returns them. All are built before any is added, so a member missing from the scene throws with nothing added; cross laps need the column plates in the same call.
    std::vector<std::shared_ptr<wood_session::JointBeam>> add_connectors(const std::vector<Relation>& kinds = CONNECTOR_RELATIONS);

    /// Adds the assembly screws, after every other connector so nothing before them changes, and returns them.
    std::vector<std::shared_ptr<wood_session::JointBeam>> add_screws();

    /// The scene element a reference names; null for a reference that is not in the scene.
    std::shared_ptr<session_cpp::Element> get(const MemberRef& ref) const;

private:
    /// The thickness the connectors on a member are sized by, 0 for a column or a support.
    double thickness(const MemberRef& ref) const;

    /// The connector of one contact or screw relationship through its JointBeam factory.
    std::shared_ptr<wood_session::JointBeam> connector_of(const Relationship& row) const;

    /// The name prefix of a connector of that kind.
    static std::string connector_prefix(Relation kind);

    /// Places the members under a new group <prefix><suffix>, lifted to the floor and each named <prefix>_<i><suffix>.
    void add_family(const std::vector<Member>& family, const std::string& prefix, const std::string& suffix, const std::shared_ptr<session_cpp::TreeNode>& group);

    /// Lifts an element to the floor, names it and adds it under the group.
    void add_placed(const std::shared_ptr<session_cpp::Element>& element, const std::string& name, const std::shared_ptr<session_cpp::TreeNode>& group);
};

}
