#pragma once
#include "wood_session.h"

namespace wood_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Construction geometry
// ═══════════════════════════════════════════════════════════════════════════

/// The planes of a quarter in pairs, one pair per member: the first plane is the member's base face, the second the face it is offset to.
struct ConstructionPlanes {
    std::vector<std::array<session_cpp::Plane, 2>> outer_ribs; // Along the two bay edges, the band offset inwards by outer_ribs.
    std::vector<std::array<session_cpp::Plane, 2>> inner_beams; // Along the two seams and the oculus edge; the oculus one tilted by the oculus plane angle.
    std::vector<std::array<session_cpp::Plane, 2>> inner_ribs; // From the column head chamfer to the inner beam corners.
    std::vector<std::array<session_cpp::Plane, 2>> wedges; // Three around the column head, the middle one tilted by wedge_plane_angle, then the three inner beam faces.
    std::vector<std::array<session_cpp::Plane, 2>> t_sections; // Beside the outer and inner ribs, tsections thick.
};

/// One plan quad per member at the floor datum.
struct ConstructionQuads {
    std::vector<session_cpp::Polyline> outer_ribs; // Two.
    std::vector<session_cpp::Polyline> inner_beams; // Three.
    std::vector<session_cpp::Polyline> inner_ribs; // Two.
    std::vector<session_cpp::Polyline> wedges; // Six.
    std::vector<session_cpp::Polyline> t_sections; // Six.
};

/// The two closed outlines a member is lofted between, as the guide authors them; the elements built from them choose which is the bottom.
struct Outline {
    session_cpp::Polyline top; // First outline.
    session_cpp::Polyline bottom; // Second outline, vertex i facing vertex i of the first.
};

// ═══════════════════════════════════════════════════════════════════════════
// Sizes and plan
// ═══════════════════════════════════════════════════════════════════════════

/// The sizes that do not change with the plan: thicknesses, offsets, depths, angles, the column and storey dimensions and the middle wedge factor.
struct FloorSizes {
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
    bool seam_through_ribs = false; // Run the two seam beams of every seam on through the outer rib band to the bay's outer face: the outer ribs end on the beams, the rib screws go from the beam's seam face into the rib end, and no ties are made.

    /// Depth at every seam and at the oculus: height minus rise.
    double static_h() const;
};

/// How the oculus corners sit on the seams: the same distance on every seam (a square diamond on a rectangular bay), or four given distances.
enum class OculusRule { square_diamond, explicit_distances };

/// The plan: four bay corners counter-clockwise at the datum z 0 and the oculus; everything else is derived.
struct FloorPlan {
    std::array<session_cpp::Point, 4> corners; // Counter-clockwise.
    double oculus = 1000.0; // Distance of an oculus corner from the centre along its seam.
    OculusRule rule = OculusRule::square_diamond; // How the four corners sit on the seams.
    std::array<double, 4> oculus_distances = {}; // explicit_distances only.

    /// The rectangle of half spans half_x and half_y about the origin, corner 0 at (-half_x, -half_y).
    static FloorPlan rectangle(double half_x, double half_y, double oculus = 1000.0);

    /// Any four corners counter-clockwise at z 0.
    static FloorPlan quadrilateral(const std::array<session_cpp::Point, 4>& corners, double oculus = 1000.0);

    /// The vertex centroid, where the bimedians cross and bisect each other.
    session_cpp::Point centre() const;

    /// The midpoint of edge k, corner k to corner k + 1.
    session_cpp::Point midpoint(size_t k) const;

    /// The interior angle at corner k in degrees.
    double corner_angle(size_t k) const;

    /// The oculus corner on seam k, between the midpoint of edge k and the centre, by the rule.
    std::array<session_cpp::Point, 4> oculus_corners() const;

    /// The oculus corner angle at corner k in degrees, between the two oculus edges that meet there.
    double oculus_corner_angle(size_t k) const;

    /// The angle at oculus corner k between its seam and the next quarter's oculus edge, in degrees: the seam beam's end cut on that edge.
    double oculus_seam_angle(size_t k) const;

    /// Counter-clockwise, convex, at z 0, every oculus corner between the centre and its edge midpoint, and the ring covering every quarter beam face, sin(oculus corner angle) >= sin(seam angle) at every oculus corner; why names the first failure.
    bool valid(std::string& why) const;
};

// ═══════════════════════════════════════════════════════════════════════════
// Shared entities
// ═══════════════════════════════════════════════════════════════════════════

/// A bay edge: its line, midpoint and the two planes of the outer rib band on it, shared by the quarters on either side of the midpoint, stored with one origin and re-origined per quarter.
struct BayEdge {
    session_cpp::Line line; // Corner k to corner k + 1.
    session_cpp::Point midpoint; // Where the two quarters' outer ribs meet.
    std::array<session_cpp::Plane, 2> band; // The edge plane with its normal into the bay, and the same offset by outer_ribs.
};

/// A seam: the line from an edge midpoint to the centre and its vertical plane, with the normal into the quarter on its beam-0 side.
struct Seam {
    size_t index = 0; // The quarter on the beam-0 side; quarter index + 1 is on the beam-2 side.
    session_cpp::Line line; // Midpoint to centre.
    session_cpp::Point oculus_corner; // Where the two seam beams end, on the line.
    session_cpp::Plane plane; // Through the line, origin at the midpoint of the half edge up to the oculus corner, normal into quarter index.
    double thickness = 0.0; // The seam beam thickness every quarter offsets by.

    /// The seam plane with its normal into that quarter.
    session_cpp::Plane plane_into(size_t quarter) const;

    /// The seam beam's two faces as that quarter reads them: the seam plane and its offset by thickness into the quarter.
    std::array<session_cpp::Plane, 2> faces_into(size_t quarter) const;
};

/// The oculus edge of one quarter: its line, the tilted bearing plane the quarter beam and the ring beam share, the vertical back face offset into the quarter, and the ring's inner plane.
struct OculusEdge {
    session_cpp::Line line; // Oculus corner q to oculus corner q - 1, the quarter polygon's third edge.
    session_cpp::Plane tilted; // The edge plane leaned by oculus_plane_angle about the edge.
    session_cpp::Plane back; // The edge plane offset by inner_beams into the quarter.
    session_cpp::Plane ring_inner; // The back face offset back by twice inner_beams: inner_beams inside the edge toward the centre.
};

/// A column corner: the fixed square column's frame, the head polygon, the carved fan, the cutter levels, the support plane and the column axis.
struct ColumnCorner {
    session_cpp::Point corner; // The bay corner.
    session_cpp::Vector x_axis; // Along the edge after the corner at a right corner, symmetric about the bisector otherwise.
    session_cpp::Vector y_axis; // Along the edge before the corner, reversed.
    std::vector<session_cpp::Point> head; // The head polygon in the frame: corner, two shaft corners, the two chamfer vertices.
    session_cpp::Vector chamfer_direction; // Unit head[3] - head[2].
    std::array<std::array<session_cpp::Plane, 2>, 3> wedge_fan; // Side 0, the tilted chamfer and side 1 with their far faces, each block's far face over its ribs' run-ins.
    std::array<session_cpp::Plane, 2> sides; // The head edges on the bay boundary, normal into the bay.
    std::array<double, 3> levels; // The cutter levels: the datum, the middle level at the outer rib bottoms and minus column_head_depth.
    session_cpp::Point axis_point; // The column axis at the datum, half a column head along both axes from the corner.
    session_cpp::Plane support_plane; // The support frame at the axis point on the slab.
    session_cpp::Line axis; // From the axis point up by bay_height.
    std::array<double, 2> column_offset; // Per bay edge, mm, signed: positive where the outer rib band overhangs the column's outer face, negative where the column stands outside the bay edge; 0 at a right corner.
    std::array<double, 3> wedge_seat; // The side 0, chamfer and side 1 seats left on the head beyond the rib bands, mm.
};

/// The central panel of one quarter by rule A: its ruling, the one sweep of both inner ribs, and the soffit, +t and +2t traces on the two inner ribs' central faces the panel's members read.
struct CentralPanel {
    session_cpp::Vector ruling; // u: the panel's horizontal ruling, along which rib 0's central trace projects onto rib 1's.
    session_cpp::Vector rib_sweep; // r: the horizontal direction both inner ribs are swept along from their outer to their central face.
    std::array<std::array<session_cpp::Polyline, 3>, 2> traces; // Per inner rib, its central face's soffit, +t and +2t, offset in the panel's own cross-section.
    std::array<double, 2> obliqueness = {0.0, 0.0}; // Degrees between the sweep and each inner rib's normal.
    double residual = 0.0; // How far rib 0's central trace projected along the ruling misses rib 1's, mm.
};

/// The private geometry of one quarter, computed once by the Floor: the bands, seams, oculus edge and column fan read from the shared entities, every plane at the quarter's own points.
struct QuarterGeometry {
    std::vector<session_cpp::Point> polygon; // Corner, midpoint, oculus corner, oculus corner, midpoint.
    ConstructionPlanes planes; // The member planes.
    ConstructionQuads quads; // The plan quad of every member at z 0.
    std::array<double, 2> run_in = {0.0, 0.0}; // Per outer rib, the straight run-in along its axis from the fan plane's datum trace to where its parabola starts, mm, solved so both outer ribs of a corner end at one level; the side column blocks are as thick, the middle one middle_wedge_factor times their mean.
    std::vector<std::array<session_cpp::Polyline, 3>> parabolas; // Outer 0, outer 1, shadow 0, shadow 1, each with its +t and +2t offsets.
    CentralPanel central_panel; // The central panel by rule A.
    std::vector<session_cpp::Plane> bed_top_planes; // Per bed panel, the plane fitted to its deepest quad, normal up.
};

// ═══════════════════════════════════════════════════════════════════════════
// Floor and quarter
// ═══════════════════════════════════════════════════════════════════════════

struct Floor;

/// One quarter as a view of the floor: the shared entities and its geometry read by reference, only members (outlines) built, every one at the floor datum z 0.
struct Quarter {
    const Floor& floor; // The floor the quarter belongs to.
    size_t index; // Counter-clockwise from corner 0.

    /// The quarter's geometry.
    const QuarterGeometry& geometry() const;

    /// The sizes of the floor.
    const FloorSizes& sizes() const;

    /// The column corner the quarter starts from.
    const ColumnCorner& column() const;

    /// The oculus edge the quarter ends on.
    const OculusEdge& oculus_edge() const;

    /// The seam on the quarter's beam-0 side (0) or beam-2 side (1).
    const Seam& seam(size_t side) const;

    /// The bay edge the quarter's outer rib 0 (0) or outer rib 1 (1) lies on.
    const BayEdge& edge(size_t side) const;

    /// The two outer ribs along the bay edges.
    std::vector<Outline> outer_ribs() const;

    /// The plane each outer rib ends on at its seam: the seam plane, or the seam beam's far face when the seam runs through the rib band.
    std::array<session_cpp::Plane, 2> rib_seam_ends() const;

    /// The two inner ribs from the column head towards the oculus.
    std::vector<Outline> inner_ribs() const;

    /// The three inner beams on the seams and the oculus edge.
    std::vector<Outline> inner_beams() const;

    /// The three wedge blocks between the ribs at the column head, standing on the beds.
    std::vector<Outline> wedges_inner_beams() const;

    /// The six t-sections beside the ribs.
    std::vector<Outline> tsections() const;

    /// The bed plates in three rows.
    std::vector<std::vector<Outline>> beds() const;

    /// The six plates that carve the column head at the quarter's corner.
    std::vector<Outline> column_cutters() const;
};

/// The relations the design relies on, measured per quarter (and per corner, which is the quarter's); ok() when the structural ones hold.
struct FloorReport {
    std::array<double, 4> seam_plane_gap = {}; // How far quarter q + 1's seam beam face leaves quarter q's seam plane, mm; 0 by construction.
    std::array<double, 4> oculus_corner_gap = {}; // How far quarter q + 1's polygon misses quarter q's oculus corner, mm; 0 by construction.
    std::array<double, 4> ruling_off_chamfer_deg = {}; // The central ruling against the column chamfer, signed about z.
    std::array<double, 4> ruling_off_oculus_edge_deg = {}; // The central ruling against the oculus edge, signed about z.
    std::array<std::array<double, 2>, 4> rib_sweep_obliqueness_deg = {}; // The inner rib sweep against each inner rib's normal.
    std::array<std::array<double, 2>, 4> rib_shear_mm = {}; // How far each inner rib's central face is sheared against its outer face along the rib.
    std::array<double, 4> closure_residual_mm = {}; // Rule A: rib 0's central trace projected along the ruling against rib 1's.
    std::array<double, 4> end_face_planarity_mm = {}; // The farthest rib end face corner from its end plane, over the four ribs.
    std::array<double, 4> bed_flange_coincidence_mm = {}; // The farthest bed underside corner from the top of the flange beside it, over the three rows and both sides.
    std::array<std::array<double, 2>, 4> rib_bottom_clearance_mm = {}; // Each outer rib's bottom at its fan plane above the middle cutter level; negative where it runs below the carved face.
    std::array<double, 4> rib_level_spread_mm = {}; // Per corner, the highest less the lowest of the eight rib face bottoms at the column head: both faces of the two outer and the two inner ribs.
    std::array<std::array<double, 3>, 4> wedge_seat_mm = {}; // The side 0, chamfer and side 1 seats on the head.
    std::array<std::array<double, 2>, 4> column_offset_mm = {}; // Signed, per corner and bay edge (R8).
    double ring_overlap_mm2 = 0.0; // The ring beams' mutual overlap in plan, 0 required.
    double ring_uncovered_mm2 = 0.0; // The quarter oculus beam faces' area outside their ring beams' faces, 0 required (R7).

    /// Whether every structural relation holds within the tolerance: the seam and oculus identities, the closure, the end faces, the beds on the flanges and the ring.
    bool ok(double tolerance = 1e-6) const;

    /// The report as text, one line per relation.
    std::string str() const;
};

/// The floor: a plan and sizes, every shared entity and every quarter's geometry computed once, quarter views on demand and the oculus; a plain value, since no view is stored.
struct Floor {
    FloorPlan plan; // The four corners and the oculus.
    FloorSizes sizes; // Everything that does not change with the plan.
    session_cpp::Point centre; // The plan's centre.
    std::array<BayEdge, 4> edges; // Edge q from corner q to corner q + 1.
    std::array<Seam, 4> seams; // Seam q from the midpoint of edge q to the centre.
    std::array<session_cpp::Point, 4> oculus_corners; // Corner q on seam q.
    std::array<OculusEdge, 4> oculus_edges; // Edge q from oculus corner q to oculus corner q - 1.
    std::array<ColumnCorner, 4> columns; // Column q at corner q.
    std::array<QuarterGeometry, 4> geometry; // Quarter q at corner q.
    double soffit = 0.0; // The level of every inner and ring beam's soffit: the deepest end of a rib that ends on one, so every rib end meets its beam in full.

    /// Computes everything from the plan and the sizes; throws when the plan is invalid.
    Floor(const FloorPlan& plan, const FloorSizes& sizes);

    /// A view of quarter q; it holds a reference and lives as long as the floor.
    Quarter quarter(size_t q) const;

    /// The oculus: four ring beams, each between its edge's tilted plane and ring inner plane from the previous beam's inner plane to the next beam's tilted plane (a pinwheel), four bottom wedges and the inner plate.
    std::vector<Outline> oculus() const;

    /// Measures the relations the design relies on, for every quarter and the ring.
    FloorReport check() const;
};

// ═══════════════════════════════════════════════════════════════════════════
// Elements
// ═══════════════════════════════════════════════════════════════════════════

/// A rib outline as a variable beam: one section per parabola point up to the top edge, its far corners read from the second outline, so the end sections lie in the end planes.
std::shared_ptr<wood_session::BeamVariable> to_rib(const Outline& outline, const std::string& name);

/// A four-corner member outline as a variable beam between the end sections over corners start and end, start[i] and end[i] on one long edge.
std::shared_ptr<wood_session::BeamVariable> to_beam(const Outline& outline, const std::array<size_t, 2>& start, const std::array<size_t, 2>& end, const std::string& name);

/// A member outline as a plate, bottom then top.
std::shared_ptr<wood_session::Plate> to_plate(const Outline& outline, const std::string& name);

/// The support of a column corner on the slab at z 0 under the column axis in the corner frame.
std::shared_ptr<wood_session::Support> to_support(const ColumnCorner& corner);

/// The column of a corner: the square shaft in the corner frame from the support's column foot to the floor, with its head a chamfer wider along both axes over the column head depth.
std::shared_ptr<wood_session::Column> to_column(const ColumnCorner& corner, const FloorSizes& sizes, const wood_session::Support& support);

/// The quarter's column cutters lifted to the floor as solid difference cuts of its column: features of the column, not elements of the scene.
std::vector<wood_session::SolidCut> column_cuts(const Quarter& quarter);

/// The thickness of a member outline: the distance between the area centroids of its two loops.
double outline_thickness(const Outline& outline);

// ═══════════════════════════════════════════════════════════════════════════
// Relationships
// ═══════════════════════════════════════════════════════════════════════════

/// The member families of the floor: the six of a quarter in the order they are added, then the ring beams, the columns and the supports.
enum class Family {
    outer_ribs, // Variable beams under the two outer parabolas.
    inner_ribs, // Variable beams under the two inner parabolas.
    inner_beams, // Variable beams between two slanted end faces.
    wedges_inner_beams, // Wedge block plates.
    tsections, // T-section plates.
    beds, // Bed plates.
    ring, // The four ring beams of the oculus.
    column, // The four columns.
    support, // The four supports.
};

/// What two members share and the connector that belongs to it; the screw kinds are the assembly screws, pre-drilled lines both members read.
enum class Relation { support, column_plate, cross_lap, seam_tie, seam_wedge, oculus_wedge, block_dowels, screw_rib_beam, screw_beam_mitre, screw_rib_corner, screw_ring, screw_oculus };

/// Where a relationship's connector lives in the scene tree: inside one quarter, in the oculus, at a column, or on the seam between two quarters.
enum class Place { quarter, oculus, column, seam };

/// The screw relation kinds in the order relationships() lists them.
const std::array<Relation, 5> SCREW_RELATIONS = {Relation::screw_rib_beam, Relation::screw_beam_mitre, Relation::screw_rib_corner, Relation::screw_ring, Relation::screw_oculus};

/// A member of the floor by quarter (-1 for the ring, the columns and the supports), family and index; row for a bed plate.
struct MemberRef {
    int quarter = -1; // The quarter the member belongs to, -1 for the ring, a column or a support.
    Family family = Family::outer_ribs; // Its family.
    size_t index = 0; // Its index in the family, or in the bed row.
    int row = -1; // The bed row, -1 for every other family.

    /// The scene name of the member, as the models name it.
    std::string name() const;
};

/// One relationship: the two members by the rule of the design, the shared plane, the contact polygon read from their outlines in world coordinates, and the seam or corner it belongs to.
struct Relationship {
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

    /// The contact area in mm2.
    double area() const;

    /// The relationship as text: its kind and its two members.
    std::string text() const;

    /// Where its connector lives, by its kind: the block dowels and the quarter screws in quarter seam_or_corner, the oculus wedges and the ring and oculus screws in the oculus, the column plates, cross laps and supports at column seam_or_corner, the seam wedges and ties on seam seam_or_corner.
    Place place() const;
};

// ═══════════════════════════════════════════════════════════════════════════
// Models
// ═══════════════════════════════════════════════════════════════════════════

/// A member in the scene with the thickness the connectors on it are sized by.
struct Member {
    std::shared_ptr<session_cpp::Element> element; // The placed element.
    double thickness = 0.0; // outline_thickness of the outline it was built from.
};

/// The members of one quarter in the scene, by family.
struct QuarterMembers {
    std::vector<Member> outer_ribs; // Two variable beams.
    std::vector<Member> inner_ribs; // Two variable beams.
    std::vector<Member> inner_beams; // Three variable beams.
    std::vector<Member> blocks; // Three wedge block plates.
    std::vector<Member> tsections; // Six plates.
    std::vector<std::vector<Member>> beds; // Three rows of plates.
    std::shared_ptr<session_cpp::TreeNode> group; // Its quarter_model_q group, which holds its connectors_q.
};

/// A column model in the scene: the support and the column, carved by its six head cuts.
struct ColumnModel {
    std::shared_ptr<wood_session::Support> support; // On the slab.
    std::shared_ptr<wood_session::Column> column; // Carved by the head cuts, solid cuts of its own drawn as its cut features.
    std::shared_ptr<session_cpp::TreeNode> group; // Its column_model_q group, which holds its connectors_column_q.
};

/// The placed members of the whole floor by quarter and family, the ring beams and the column models.
struct FloorMembers {
    std::array<QuarterMembers, 4> quarters; // Quarter q at corner q.
    std::vector<Member> ring; // The four ring beams.
    std::vector<ColumnModel> columns; // Column q at corner q, empty until the columns are added.
    std::shared_ptr<session_cpp::TreeNode> group; // The group the floor was added under, null at the tree root; it holds the seams group.
    std::shared_ptr<session_cpp::TreeNode> oculus; // The oculus group, which holds connectors_oculus.

    /// The scene element a reference names; null for a reference that is not in the scene.
    std::shared_ptr<session_cpp::Element> get(const MemberRef& ref) const;

    /// The thickness the connectors on that member are sized by, 0 for a column or a support.
    double thickness(const MemberRef& ref) const;

    /// The two scene elements of a relationship; throws naming it when one is not in the scene.
    std::array<std::shared_ptr<session_cpp::Element>, 2> pair(const Relationship& row) const;
};

/// The member as it was before any cut, for the contact search: a copy without its plane and solid cuts, so a pocket or a hole on the cut model neither splits nor loses a contact.
std::shared_ptr<session_cpp::Element> uncut(const session_cpp::Element& member);

/// A searched contact that does not agree with the constructed one.
struct ContactMismatch {
    std::string relation; // The relationship, kind and members.
    std::string what; // How the search disagrees: missing, another type, the plane, the top edge or the area.
};

/// The constructed contacts checked against the kernel's contact search.
struct ContactCheck {
    size_t count = 0; // Contacts checked.
    std::vector<ContactMismatch> mismatches; // Every contact the search disagrees with.

    /// Whether the search agrees with every contact.
    bool ok() const;

    /// The check as text: how many agree, then every mismatch.
    std::string str() const;
};

/// A group named name under parent, at the root when parent is empty.
std::shared_ptr<session_cpp::TreeNode> add_group(wood_session::WoodSession& session, const std::string& name, const std::shared_ptr<session_cpp::TreeNode>& parent);

/// The column model built in place at a corner: support, column, support joint and the column carved by the quarter's six head cuts, every name ending in the corner index.
ColumnModel add_column_model(wood_session::WoodSession& session, const Floor& floor, size_t corner, const std::shared_ptr<session_cpp::TreeNode>& group);

/// The quarter model built in place and lifted to bay_height, grouped by family, every name ending in the quarter's index.
QuarterMembers add_quarter_model(wood_session::WoodSession& session, const Quarter& quarter, const std::shared_ptr<session_cpp::TreeNode>& group);

/// The oculus model lifted to bay_height; returns its four boundary beams.
std::vector<Member> add_oculus_model(wood_session::WoodSession& session, const Floor& floor, const std::shared_ptr<session_cpp::TreeNode>& group);

/// The floor under group: the four quarters under quarters_model and the oculus; the columns are added apart.
FloorMembers add_floor(wood_session::WoodSession& session, const Floor& floor, const std::shared_ptr<session_cpp::TreeNode>& group);

/// The columns under group: the four column models, each in its own column_model_q group, filled into the members.
void add_columns(wood_session::WoodSession& session, const Floor& floor, const std::shared_ptr<session_cpp::TreeNode>& group, FloorMembers& members);

/// Every relationship of the floor in the order the connectors are named in: the seam wedges, the oculus wedges, the column plates, the cross laps, the ties (none when the seam runs through the ribs), the block dowels and the supports, each kind in quarter order; then the screws, per quarter and kind, then the ring's.
std::vector<Relationship> relationships(const Floor& floor);

/// The relationships of one kind, in the same order.
std::vector<Relationship> relationships(const Floor& floor, Relation kind);

/// The colour of every connector node and of every part and dowel node nested under it.
const session_cpp::Color CONNECTOR_COLOR = session_cpp::Color(0.3f, 0.3f, 0.3f, 1.0f, "dark_grey");

/// One connector per relationship of the kinds asked for, through the JointBeam factories on the constructed contacts, named within its kind as the examples name them and added under its connector_group, its node and every node nested under it in CONNECTOR_COLOR; cross laps need the column plates in the same call.
std::vector<std::shared_ptr<wood_session::JointBeam>> add_connectors(wood_session::WoodSession& session, const Floor& floor, const FloorMembers& members, const std::vector<Relation>& kinds = {Relation::seam_wedge, Relation::oculus_wedge, Relation::column_plate, Relation::cross_lap, Relation::seam_tie, Relation::block_dowels});

/// The kernel's contact search on uncut copies of the members against every constructed contact of the kinds asked for: the plane normal, the top edge and the area must agree within the tolerance (mm and radians).
ContactCheck verify_contacts(wood_session::WoodSession& session, const Floor& floor, const FloorMembers& members, double tolerance = 1e-6, const std::vector<Relation>& kinds = {Relation::seam_wedge, Relation::oculus_wedge, Relation::column_plate, Relation::seam_tie, Relation::block_dowels});

/// The searched contact of two members as they were before any cut, of the expected type; throws naming the relation when there is none.
std::shared_ptr<wood_session::InteractionContactFace> require_contact(wood_session::WoodSession& session, const std::shared_ptr<session_cpp::Element>& a, const std::shared_ptr<session_cpp::Element>& b, wood_session::ContactType expected, const std::string& relation);

// ═══════════════════════════════════════════════════════════════════════════
// Screws
// ═══════════════════════════════════════════════════════════════════════════

/// The name of a relation kind, as Relationship::text() writes it.
std::string relation_name(Relation kind);

/// How the screws of a floor sit: counts per kind, the closest approaches and how much of every screw its two members hold.
struct ScrewCheck {
    std::map<Relation, size_t> counts; // Screws per relation kind.
    double screw_screw_mm = 1e300; // The smallest distance between the axes of two screws.
    double screw_bore_mm = 1e300; // The smallest clearance between a screw and a dowel bore of another connector, run on by its overshoot: axis distance less both radii.
    double screw_pocket_mm = 1e300; // The smallest distance from a screw's surface to a pocket or a part of another connector; negative where it cuts into one.
    double embedded_min_mm = 1e300; // The shortest length of a screw inside the members it names together.
    double member_min_mm = 1e300; // The shortest length of a screw inside one of the two members of its joint.
    std::vector<std::string> misfits; // Every screw that breaks a rule: closer than 8 mm to another, into a bore or a pocket, or not held over its length by the members it names, each of the joint's two holding some.

    /// The check as text: the counts and the distances, then every misfit.
    std::string str() const;
};

/// Measures the screw connectors add_connectors made for the screw kinds, in relationships() order, against each other, every other connector's bores, pockets and parts, and their two members' solids before any cut.
ScrewCheck check_screws(const wood_session::WoodSession& session, const Floor& floor, const std::vector<std::shared_ptr<wood_session::JointBeam>>& screws);

// ═══════════════════════════════════════════════════════════════════════════
// BReps
// ═══════════════════════════════════════════════════════════════════════════

/// How the cut members and the connector parts come out as BReps: exact where a dowel bores them, and whether every dowel stretch through a member or a part found its bore.
struct BrepCheck {
    size_t exact = 0; // Cut members with at least one exact bore.
    size_t bores = 0; // Exact bores in the cut members.
    size_t connectors = 0; // Connectors with a part or a dowel.
    size_t part_bores = 0; // Exact bores in the connector parts.
    size_t stretches = 0; // Dowel stretches through members and parts: the bores the dowels ask for.
    double ms = 0.0; // The time the BReps took.
    std::vector<std::string> faceted; // Every cut member without an exact bore.

    /// The check as text: the counts, the bores found against the bores asked for, then every faceted member.
    std::string str() const;
};

/// The exact bores of a BRep: its rational surfaces, cylinders.
size_t count_bores(const session_cpp::BRep& brep);

/// Builds the BRep of every cut member and connector part and counts their exact bores against the dowel stretches.
BrepCheck check_breps(const wood_session::WoodSession& session);

/// Writes every cut member, connector part and dowel as its BRep instead of its mesh, the bores exact cylinders.
void compute_breps(wood_session::WoodSession& session);

}
