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

/// The parametric source of one quarter of a timber floor bay, port of compas_tf FloorGuide, every member as an outline pair at the floor datum z 0.
struct FloorGuide {
    double size_grid_x = 3000.0; // Half the bay in x: the column corner sits at -size_grid_x.
    double size_grid_y = 3000.0; // Half the bay in y.
    double size_oculus = 1000.0; // Half diagonal of the oculus.
    FloorSizes sizes; // Everything that does not change with the plan.

    // ═══════════════════════════════════════════════════════════════════════
    // Plan
    // ═══════════════════════════════════════════════════════════════════════

    /// Column base centre in plan for a column of that side: the grid corner inset by half of it.
    session_cpp::Point corner_point_column(double column_size = 200.0) const;

    /// The four oculus corners on the axes, scaled by the grid aspect.
    std::vector<session_cpp::Point> oculus_points() const;

    /// The quarter outline: grid corner, edge midpoint, two oculus corners, edge midpoint.
    std::vector<session_cpp::Point> quarter_polygon() const;

    /// The column head polygon at the grid corner the ribs start from.
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

    /// The oculus: four boundary beams, four bottom wedges and the inner plate.
    std::vector<Outline> oculus() const;

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

/// The column model of compas_tf example_model_2 moved by placement, every name ending in suffix; returns the column.
std::shared_ptr<wood_session::Column> add_column_model(wood_session::WoodSession& session, const FloorGuide& guide, const session_cpp::Xform& placement, const std::shared_ptr<session_cpp::TreeNode>& group, const std::string& suffix);

/// The quarter model of compas_tf example_model_4 lifted to bay_height and moved by placement, grouped by family, every name ending in suffix.
Quarter add_quarter_model(wood_session::WoodSession& session, const FloorGuide& guide, const session_cpp::Xform& placement, const std::shared_ptr<session_cpp::TreeNode>& group, const std::string& suffix);

/// The oculus model of compas_tf example_model_5 lifted to bay_height; returns its four boundary beams.
std::vector<Member> add_oculus_model(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<session_cpp::TreeNode>& group);

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
