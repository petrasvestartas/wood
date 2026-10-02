#pragma once
#include "wood_session.h"

namespace wood_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Construction geometry
// ═══════════════════════════════════════════════════════════════════════════

/// The planes of a quarter in pairs, one pair per member: the first plane is the member's base face, the second the face it is offset to.
struct ConstructionPlanes {
    std::vector<std::array<session_cpp::Plane, 2>> outer_ribs; // Along the two grid edges, offset inwards by size_outer_ribs.
    std::vector<std::array<session_cpp::Plane, 2>> inner_beams; // Along the two seam lines and the oculus edge; the oculus one tilted by the oculus plane angle.
    std::vector<std::array<session_cpp::Plane, 2>> inner_ribs; // From the column head chamfer to the inner beam corners.
    std::vector<std::array<session_cpp::Plane, 2>> wedges; // Three around the column head, the middle one tilted by wedge_plane_angle, then the three inner beam faces.
    std::vector<std::array<session_cpp::Plane, 2>> t_sections; // Beside the outer and inner ribs, size_tsections thick.
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
// Floor guide
// ═══════════════════════════════════════════════════════════════════════════

/// The parametric source of one quarter of a timber floor bay, port of compas_tf FloorGuide, every member as an outline pair at the floor datum z 0; the quarter is framed by its bay corner and the two edge midpoints beside it, counter-clockwise about the bay centre at the origin.
struct FloorGuide {
    session_cpp::Point corner = session_cpp::Point(-3000.0, -3000.0, 0.0); // The bay corner the column stands at.
    session_cpp::Point midpoint_x = session_cpp::Point(0.0, -3000.0, 0.0); // Midpoint of the bay edge after the corner going counter-clockwise, the first outer rib's.
    session_cpp::Point midpoint_y = session_cpp::Point(-3000.0, 0.0, 0.0); // Midpoint of the bay edge before the corner, the second outer rib's.
    session_cpp::Point oculus_x = session_cpp::Point(0.0, -1000.0, 0.0); // The oculus corner on the line from the centre to midpoint_x.
    session_cpp::Point oculus_y = session_cpp::Point(-1000.0, 0.0, 0.0); // The oculus corner on the line from the centre to midpoint_y.
    double size_column_head = 250.0; // Side of the column head polygon at the corner.
    double size_column_head_chamfer = 100.0; // Chamfer of that polygon on the bay side.
    double size_outer_ribs = 100.0; // Outer rib thickness.
    double size_inner_ribs = 60.0; // Inner rib thickness.
    double size_inner_beams = 60.0; // Inner beam thickness.
    double size_wedge = 100.0; // Wedge thickness around the column head.
    double size_tsections = 27.0; // T-section and bed thickness.
    double height = 650.0; // Floor depth at the column.
    double rise = 453.0; // Rise of the rib parabola from the column to the edge midpoint.
    double size_oculus = 1000.0; // Half diagonal of the oculus; rectangle and trapezoid place oculus_x and oculus_y by it.
    double wedge_plane_angle = -10.0; // Degrees the middle wedge plane leans about its top edge.
    double bay_height = 3500.0; // Storey height, column plus support, the floor is lifted by.
    double column_head_depth = 730.0; // How far below the floor datum the column head cutters reach.
    double oculus_plane_angle = 5.0; // Degrees the oculus inner beam plane leans about its top edge.

    // ═══════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════

    /// The quarter of a bay with the four corners counter-clockwise about the origin, quadrant 0 to 3 picking the corner, with the sizes given; the oculus corner towards each edge midpoint lies size_oculus times that midpoint's distance over the geometric mean of its two neighbours' away from the centre. The members still sweep square to their ribs as compas_tf's guide does, so a corner off the right angle leaves the rib ends off the column's carved faces.
    static FloorGuide trapezoid(const std::array<session_cpp::Point, 4>& corners, int quadrant, const FloorGuide& sizes);

    /// The quarter of the rectangular bay of half sides gx and gy centred on the origin, quadrant 0 at (-gx, -gy) to 3 at (-gx, gy), with the sizes given.
    static FloorGuide rectangle(double gx, double gy, int quadrant, const FloorGuide& sizes);

    // ═══════════════════════════════════════════════════════════════════════
    // Plan
    // ═══════════════════════════════════════════════════════════════════════

    /// Depth of the floor at the edge midpoints: height minus rise.
    double static_h() const;

    /// Unit direction from the corner towards midpoint_x.
    session_cpp::Vector x_axis() const;

    /// Unit direction from the corner towards midpoint_y.
    session_cpp::Vector y_axis() const;

    /// Column base centre in plan for a column of that side: the corner inset by half of it along both edges.
    session_cpp::Point corner_point_column(double column_size = 200.0) const;

    /// The quarter outline: corner, midpoint_x, oculus_x, oculus_y, midpoint_y.
    std::vector<session_cpp::Point> quarter_polygon() const;

    /// The column head polygon at the corner the ribs start from, along the two edges.
    std::vector<session_cpp::Point> quarter_column_polygon() const;

    // ═══════════════════════════════════════════════════════════════════════
    // Construction planes
    // ═══════════════════════════════════════════════════════════════════════

    /// The member planes of the quarter.
    ConstructionPlanes construction_planes() const;

    /// The plan quad of every member at z 0.
    ConstructionQuads construction_quads() const;

    /// Per rib axis (outer 0, outer 1, inner 0, inner 1) the parabola and its two offsets by size_tsections.
    std::vector<std::array<session_cpp::Polyline, 3>> boundary_parabolas() const;

    /// Bottom level of the wedge block.
    double block_level_bottom() const;

    /// Top level of the wedge block, size_wedge above its bottom.
    double block_level_top() const;

    /// Per bed panel, matching the three wedges, the plane fitted to its deepest quad, normal up.
    std::vector<session_cpp::Plane> bed_top_planes() const;

    // ═══════════════════════════════════════════════════════════════════════
    // Members
    // ═══════════════════════════════════════════════════════════════════════

    /// The two outer ribs along the grid edges.
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

    /// The oculus of the bay the quarters ring, in quadrant order: one boundary beam per quarter on its oculus edge, one bottom wedge per corner and the inner plate, sized by the first quarter.
    static std::vector<Outline> oculus(const std::vector<FloorGuide>& quarters);

    /// The six plates that carve the column head, in the guide frame.
    std::vector<Outline> column_cutters() const;
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

/// The support of the quarter's column, port of compas_tf SupportElement, on the slab at z 0 under the column axis.
std::shared_ptr<wood_session::Support> to_support(const FloorGuide& guide);

/// The column of the quarter, port of compas_tf ColumnElement, from the support's column foot to the floor with its wider head over the column head depth.
std::shared_ptr<wood_session::Column> to_column(const FloorGuide& guide, const wood_session::Support& support);

/// The column cutters lifted to the floor, one solid difference cutter each aimed at the column.
std::vector<std::shared_ptr<wood_session::Joint>> to_column_cutters(const FloorGuide& guide, const wood_session::Column& column);

/// compas_tf's computed_thickness of a member outline: the distance between the area centroids of its two loops.
double outline_thickness(const Outline& outline);

// ═══════════════════════════════════════════════════════════════════════════
// Models
// ═══════════════════════════════════════════════════════════════════════════

/// A member in the scene with the thickness the connectors on it are sized by.
struct Member {
    std::shared_ptr<session_cpp::Element> element; // The placed element.
    double thickness = 0.0; // outline_thickness of the outline it was built from, measured in the guide frame.
};

/// The members of one quarter in the scene, by family.
struct Quarter {
    std::vector<Member> outer_ribs; // Two variable beams.
    std::vector<Member> inner_ribs; // Two variable beams.
    std::vector<Member> inner_beams; // Three variable beams.
    std::vector<Member> blocks; // Three wedge block plates.
    std::vector<Member> tsections; // Six plates.
    std::vector<std::vector<Member>> beds; // Three rows of plates.
};

/// A group named name under parent, at the root when parent is empty.
std::shared_ptr<session_cpp::TreeNode> add_group(wood_session::WoodSession& session, const std::string& name, const std::shared_ptr<session_cpp::TreeNode>& parent);

/// The column model of compas_tf example_model_2 at the guide's corner, every name ending in suffix; returns the column.
std::shared_ptr<wood_session::Column> add_column_model(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<session_cpp::TreeNode>& group, const std::string& suffix);

/// The quarter model of compas_tf example_model_4 in the guide's frame lifted to bay_height, grouped by family, every name ending in suffix.
Quarter add_quarter_model(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<session_cpp::TreeNode>& group, const std::string& suffix);

/// The oculus model of compas_tf example_model_5 ringed by the quarters, lifted to bay_height; returns its boundary beams.
std::vector<Member> add_oculus_model(wood_session::WoodSession& session, const std::vector<FloorGuide>& quarters, const std::shared_ptr<session_cpp::TreeNode>& group);

/// The wedges of compas_tf example_model_6: a wedge joint on every long-face contact among the ring beams, sized by the thicker member; returns the joints.
std::vector<std::shared_ptr<wood_session::JointBeam>> add_wedges(wood_session::WoodSession& session, const std::vector<Member>& ring, const std::shared_ptr<session_cpp::TreeNode>& group);

/// The column connectors of compas_tf example_model_8: a rectangle plate joint on the contact of every column with every outer rib; returns the joints.
std::vector<std::shared_ptr<wood_session::JointBeam>> add_rectangle_plates(wood_session::WoodSession& session, const std::vector<std::shared_ptr<wood_session::Column>>& columns, const std::vector<Member>& outer_ribs, const std::shared_ptr<session_cpp::TreeNode>& group);

/// The seam connectors of compas_tf example_model_8: a tie joint on every end-to-end contact of two outer ribs; returns the joints.
std::vector<std::shared_ptr<wood_session::JointBeam>> add_ties(wood_session::WoodSession& session, const std::vector<Member>& outer_ribs, const std::shared_ptr<session_cpp::TreeNode>& group);

/// The assembly dowels of every quarter: a dowels joint on every contact of a wedge block with a rib, found on the members as they were before any cut; returns the joints.
std::vector<std::shared_ptr<wood_session::JointBeam>> add_quarter_dowels(wood_session::WoodSession& session, const std::vector<Quarter>& quarters, const std::shared_ptr<session_cpp::TreeNode>& group, double radius = 4.0, double length = 30.0, double offset = 50.0);

/// The half laps of the column heads: a cross lap on every two rectangle plates that meet in one column; returns the joints.
std::vector<std::shared_ptr<wood_session::JointBeam>> add_cross_laps(wood_session::WoodSession& session, const std::vector<std::shared_ptr<wood_session::JointBeam>>& plates, const std::shared_ptr<session_cpp::TreeNode>& group);

}
