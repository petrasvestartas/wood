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

/// The sizes that do not change with the plan: thicknesses, offsets, depths, angles, the column and storey dimensions and compas_tf's middle wedge factor; the defaults are the example set of compas_tf example_model_1.
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
    double middle_wedge_factor = 1.25; // The middle block in wedge thicknesses, compas_tf floor_guide.py:314.

    /// Depth at every seam and at the oculus: height minus rise.
    double static_h() const;
};

/// How the oculus corners sit on the seams: the same distance on every seam (a square diamond on a rectangular bay), compas_tf's grid-aspect scaling kept for the parity gate, or four given distances.
enum class OculusRule { square_diamond, compas, explicit_distances };

/// How the central panel's +t and +2t layers are made; compas_tf's offsets in the outer rib's plane swept along the panel are the only mode until the model's cross-section layers arrive.
enum class CentralLayers { compas };

/// The plan: four bay corners counter-clockwise at the datum z 0 and the oculus; everything else is derived.
struct FloorPlan {
    std::array<session_cpp::Point, 4> corners; // Counter-clockwise; corner 0 is compas_tf's quarter 0.
    double oculus = 1000.0; // Distance of an oculus corner from the centre along its seam.
    OculusRule rule = OculusRule::square_diamond; // How the four corners sit on the seams.
    std::array<double, 4> oculus_distances = {}; // explicit_distances only.

    /// The rectangle of half spans half_x and half_y about the origin, corner 0 at (-half_x, -half_y).
    static FloorPlan rectangle(double half_x, double half_y, double oculus = 1000.0, OculusRule rule = OculusRule::square_diamond);

    /// Any four corners counter-clockwise at z 0.
    static FloorPlan quadrilateral(const std::array<session_cpp::Point, 4>& corners, double oculus = 1000.0, OculusRule rule = OculusRule::square_diamond);

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
    std::vector<session_cpp::Point> head; // compas_tf quarter_column_polygon in the frame: corner, two shaft corners, the two chamfer vertices.
    session_cpp::Vector chamfer_direction; // Unit head[3] - head[2].
    std::array<std::array<session_cpp::Plane, 2>, 3> wedge_fan; // Side 0, the tilted chamfer and side 1 with their far faces.
    std::array<session_cpp::Plane, 2> sides; // The head edges on the bay boundary, normal into the bay.
    std::array<double, 3> levels; // The cutter levels: the datum, the middle level and minus column_head_depth.
    session_cpp::Point axis_point; // The column axis at the datum, half a column head along both axes from the corner.
    session_cpp::Plane support_plane; // The support frame at the axis point on the slab.
    session_cpp::Line axis; // From the axis point up by bay_height.
    std::array<double, 2> column_offset; // Per bay edge, mm, signed: positive where the outer rib band overhangs the column's outer face, negative where the column stands outside the bay edge; 0 at a right corner.
    std::array<double, 3> wedge_seat; // The side 0, chamfer and side 1 seats left on the head beyond the rib bands, mm.
};

/// The private geometry of one quarter, computed once by the Floor: compas_tf's layout with the bands, seams, oculus edge and column fan read from the shared entities, every plane at the quarter's own points.
struct QuarterGeometry {
    std::vector<session_cpp::Point> polygon; // Corner, midpoint, oculus corner, oculus corner, midpoint.
    ConstructionPlanes planes; // The member planes.
    ConstructionQuads quads; // The plan quad of every member at z 0.
    std::vector<std::array<session_cpp::Polyline, 3>> parabolas; // Outer 0, outer 1, shadow 0, shadow 1, each with its +t and +2t offsets.
    std::vector<session_cpp::Plane> bed_top_planes; // Per bed panel, the plane fitted to its deepest quad, normal up.
    double block_level_bottom = 0.0; // Bottom level of the wedge block, kept for the parity dump.
    double block_level_top = 0.0; // Top level of the wedge block, kept for the parity dump.
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

/// The floor: a plan and sizes, every shared entity and every quarter's geometry computed once, quarter views on demand and the oculus; a plain value, since no view is stored.
struct Floor {
    FloorPlan plan; // The four corners and the oculus.
    FloorSizes sizes; // Everything that does not change with the plan.
    CentralLayers layers = CentralLayers::compas; // How the central panel's layers are made.
    session_cpp::Point centre; // The plan's centre.
    std::array<BayEdge, 4> edges; // Edge q from corner q to corner q + 1.
    std::array<Seam, 4> seams; // Seam q from the midpoint of edge q to the centre.
    std::array<session_cpp::Point, 4> oculus_corners; // Corner q on seam q.
    std::array<OculusEdge, 4> oculus_edges; // Edge q from oculus corner q to oculus corner q - 1.
    std::array<ColumnCorner, 4> columns; // Column q at corner q.
    std::array<QuarterGeometry, 4> geometry; // Quarter q at corner q.

    /// Computes everything from the plan and the sizes; throws when the plan is invalid.
    Floor(const FloorPlan& plan, const FloorSizes& sizes, CentralLayers layers = CentralLayers::compas);

    /// A view of quarter q; it holds a reference and lives as long as the floor.
    Quarter quarter(size_t q) const;

    /// The oculus: four ring beams, each between its edge's tilted plane and ring inner plane from the previous beam's inner plane to the next beam's tilted plane (compas_tf's pinwheel), four bottom wedges and the inner plate.
    std::vector<Outline> oculus() const;
};

// ═══════════════════════════════════════════════════════════════════════════
// Elements
// ═══════════════════════════════════════════════════════════════════════════

/// A rib outline as a variable beam: one section per parabola point up to the top edge, through the rib thickness.
std::shared_ptr<wood_session::BeamVariable> to_rib(const Outline& outline, const std::string& name);

/// A four-corner member outline as a variable beam between the end sections over corners start and end, start[i] and end[i] on one long edge.
std::shared_ptr<wood_session::BeamVariable> to_beam(const Outline& outline, const std::array<size_t, 2>& start, const std::array<size_t, 2>& end, const std::string& name);

/// A member outline as a plate, bottom then top.
std::shared_ptr<wood_session::Plate> to_plate(const Outline& outline, const std::string& name);

/// The support of the column at corner 0, port of compas_tf SupportElement, on the slab at z 0 under the column axis.
std::shared_ptr<wood_session::Support> to_support(const Floor& floor);

/// The column at corner 0, port of compas_tf ColumnElement, from the support's column foot to the floor with its wider head over the column head depth.
std::shared_ptr<wood_session::Column> to_column(const Floor& floor, const wood_session::Support& support);

/// The quarter's column cutters lifted to the floor, one solid difference cutter each aimed at the column.
std::vector<std::shared_ptr<wood_session::Joint>> to_column_cutters(const Quarter& quarter, const wood_session::Column& column);

/// compas_tf's computed_thickness of a member outline: the distance between the area centroids of its two loops.
double outline_thickness(const Outline& outline);

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
};

/// A group named name under parent, at the root when parent is empty.
std::shared_ptr<session_cpp::TreeNode> add_group(wood_session::WoodSession& session, const std::string& name, const std::shared_ptr<session_cpp::TreeNode>& parent);

/// The column model of compas_tf example_model_2 at corner 0 moved by placement, every name ending in suffix; returns the column.
std::shared_ptr<wood_session::Column> add_column_model(wood_session::WoodSession& session, const Floor& floor, const session_cpp::Xform& placement, const std::shared_ptr<session_cpp::TreeNode>& group, const std::string& suffix);

/// The quarter model of compas_tf example_model_4 built in place and lifted to bay_height, grouped by family, every name ending in the quarter's index.
QuarterMembers add_quarter_model(wood_session::WoodSession& session, const Quarter& quarter, const std::shared_ptr<session_cpp::TreeNode>& group);

/// The oculus model of compas_tf example_model_5 lifted to bay_height; returns its four boundary beams.
std::vector<Member> add_oculus_model(wood_session::WoodSession& session, const Floor& floor, const std::shared_ptr<session_cpp::TreeNode>& group);

/// The wedges of compas_tf example_model_6: a wedge joint on every long-face contact among the ring beams, sized by the thicker member; returns the joints.
std::vector<std::shared_ptr<wood_session::JointBeam>> add_wedges(wood_session::WoodSession& session, const std::vector<Member>& ring, const std::shared_ptr<session_cpp::TreeNode>& group);

/// The column connectors of compas_tf example_model_8: a rectangle plate joint on the contact of every column with every outer rib; returns the joints.
std::vector<std::shared_ptr<wood_session::JointBeam>> add_rectangle_plates(wood_session::WoodSession& session, const std::vector<std::shared_ptr<wood_session::Column>>& columns, const std::vector<Member>& outer_ribs, const std::shared_ptr<session_cpp::TreeNode>& group);

/// The seam connectors of compas_tf example_model_8: a tie joint on every end-to-end contact of two outer ribs; returns the joints.
std::vector<std::shared_ptr<wood_session::JointBeam>> add_ties(wood_session::WoodSession& session, const std::vector<Member>& outer_ribs, const std::shared_ptr<session_cpp::TreeNode>& group);

/// The assembly dowels of every quarter: a dowels joint on every contact of a wedge block with a rib, found on the members as they were before any cut; returns the joints.
std::vector<std::shared_ptr<wood_session::JointBeam>> add_quarter_dowels(wood_session::WoodSession& session, const std::vector<QuarterMembers>& quarters, const std::shared_ptr<session_cpp::TreeNode>& group, double radius = 4.0, double length = 30.0, double offset = 50.0);

/// The half laps of the column heads: a cross lap on every two rectangle plates that meet in one column; returns the joints.
std::vector<std::shared_ptr<wood_session::JointBeam>> add_cross_laps(wood_session::WoodSession& session, const std::vector<std::shared_ptr<wood_session::JointBeam>>& plates, const std::shared_ptr<session_cpp::TreeNode>& group);

}
