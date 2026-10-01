#pragma once
#include "wood_session.h"

namespace wood_floor {

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Construction geometry
// ═══════════════════════════════════════════════════════════════════════════

/// The planes of a quarter in pairs, one pair per member: the first plane is the member's base face, the second the face it is offset to.
struct ConstructionPlanes {
    std::vector<std::array<Plane, 2>> outer_ribs; // Along the two grid edges, offset inwards by size_outer_ribs.
    std::vector<std::array<Plane, 2>> inner_beams; // Along the two seam lines and the oculus edge; the oculus one tilted by the oculus plane angle.
    std::vector<std::array<Plane, 2>> inner_ribs; // From the column head chamfer to the inner beam corners.
    std::vector<std::array<Plane, 2>> wedges; // Three around the column head, the middle one tilted by wedge_plane_angle, then the three inner beam faces.
    std::vector<std::array<Plane, 2>> t_sections; // Beside the outer and inner ribs, size_tsections thick.
};

/// Four planes per member whose intersections with the floor datum give its plan quad.
struct QuadPlanes {
    std::vector<std::array<Plane, 4>> outer_ribs; // Two.
    std::vector<std::array<Plane, 4>> inner_beams; // Three.
    std::vector<std::array<Plane, 4>> inner_ribs; // Two.
    std::vector<std::array<Plane, 4>> wedges; // Six.
    std::vector<std::array<Plane, 4>> t_sections; // Six.
};

/// One plan quad per member at the floor datum, from its quad planes.
struct ConstructionQuads {
    std::vector<Polyline> outer_ribs; // Two.
    std::vector<Polyline> inner_beams; // Three.
    std::vector<Polyline> inner_ribs; // Two.
    std::vector<Polyline> wedges; // Six.
    std::vector<Polyline> t_sections; // Six.
};

/// The two closed outlines a member is lofted between, as the guide authors them; the elements built from them choose which is the bottom.
struct Outline {
    Polyline top; // First outline.
    Polyline bottom; // Second outline, vertex i facing vertex i of the first.
    int row = -1; // Bed row 0, 1 or 2, -1 for every other member.
};

// ═══════════════════════════════════════════════════════════════════════════
// Floor guide
// ═══════════════════════════════════════════════════════════════════════════

/// The parametric source of one quarter of a timber floor bay, port of compas_tf FloorGuide: a column head at the grid corner, ribs following parabolas to the edge midpoints and to the oculus, inner beams on the seams, t-sections, beds and the oculus, all as outline pairs at the floor datum z 0.
struct FloorGuide {
    double size_grid_x = 3000.0; // Half the bay in x: the column corner sits at -size_grid_x.
    double size_grid_y = 3000.0; // Half the bay in y.
    double size_column_head = 250.0; // Side of the column head polygon at the corner.
    double size_column_head_chamfer = 100.0; // Chamfer of that polygon on the bay side.
    double size_outer_ribs = 100.0; // Outer rib thickness.
    double size_inner_ribs = 60.0; // Inner rib thickness.
    double size_inner_beams = 60.0; // Inner beam thickness.
    double size_wedge = 100.0; // Wedge thickness around the column head.
    double size_tsections = 27.0; // T-section and bed thickness.
    double height = 650.0; // Floor depth at the column.
    double rise = 453.0; // Rise of the rib parabola from the column to the edge midpoint.
    double size_oculus = 1000.0; // Half diagonal of the oculus.
    double wedge_plane_angle = -10.0; // Degrees the middle wedge plane leans about its top edge.
    double bay_height = 3500.0; // Storey height, column plus support, the floor is lifted by.
    double column_head_lowest_height = -730.0; // Lowest level of the column head cutters.
    double oculus_plane_angle = 5.0; // Degrees the oculus inner beam plane leans about its top edge.

    // ═══════════════════════════════════════════════════════════════════════
    // Plan
    // ═══════════════════════════════════════════════════════════════════════

    /// Depth of the floor at the edge midpoints: height minus rise.
    double static_h() const;

    /// Column base centre in plan for a column of that side: the grid corner inset by half of it.
    Point corner_point_column(double column_size = 200.0) const;

    /// The four oculus corners on the axes, scaled by the grid aspect.
    std::vector<Point> oculus_points() const;

    /// The quarter outline: grid corner, edge midpoint, two oculus corners, edge midpoint.
    std::vector<Point> quarter_polygon() const;

    /// The column head polygon at the grid corner the ribs start from.
    std::vector<Point> quarter_column_polygon() const;

    // ═══════════════════════════════════════════════════════════════════════
    // Construction planes
    // ═══════════════════════════════════════════════════════════════════════

    /// The member planes of the quarter.
    ConstructionPlanes construction_planes() const;

    /// The four bounding planes of every member.
    QuadPlanes quad_planes() const;

    /// The plan quad of every member at z 0.
    ConstructionQuads construction_quads() const;

    /// Per rib axis (outer 0, outer 1, inner 0, inner 1) the parabola and its two offsets by size_tsections; the inner ones are the outer projected onto the inner rib planes.
    std::vector<std::array<Polyline, 3>> boundary_parabolas() const;

    /// Bottom level of the wedge block.
    double block_level_bottom() const;

    /// Top level of the wedge block, size_wedge above its bottom.
    double block_level_top() const;

    /// Per bed panel, matching the three wedges, the plane fitted to its deepest quad, normal up.
    std::vector<Plane> bed_top_planes() const;

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

    /// The bed plates in three rows, row set on each.
    std::vector<Outline> beds() const;

    /// The oculus: four boundary beams, four bottom wedges and the inner plate.
    std::vector<Outline> oculus() const;

    /// The six plates that carve the column head, in the guide frame.
    std::vector<Outline> column_cutters() const;
};

// ═══════════════════════════════════════════════════════════════════════════
// Elements
// ═══════════════════════════════════════════════════════════════════════════

/// A rib outline as a variable beam: one section per parabola point, from the parabola up to the top edge at z 0, through the rib thickness; the axis runs along the middle of the top face from the column end to the far end.
std::shared_ptr<wood_session::BeamVariable> to_rib(const Outline& outline, const std::string& name);

/// A four-corner member outline as a variable beam between two end sections: corners start[0], start[1] at one end and end[0], end[1] at the other, start[i] and end[i] on one long edge; the axis joins the section centroids.
std::shared_ptr<wood_session::BeamVariable> to_beam(const Outline& outline, const std::array<size_t, 2>& start, const std::array<size_t, 2>& end, const std::string& name);

/// A member outline as a plate, bottom then top.
std::shared_ptr<wood_session::Plate> to_plate(const Outline& outline, const std::string& name);

/// The support of the quarter's column, port of compas_tf SupportElement: on the slab at z 0 under the column axis.
std::shared_ptr<wood_session::Support> to_support(const FloorGuide& guide);

/// The column of the quarter, port of compas_tf ColumnElement in example_model_2: a size_column_head square from the support's column foot to the floor at bay_height, its outer corner on the grid corner, and over the lowest column head level a head size_column_head_chamfer wider on the two bay sides.
std::shared_ptr<wood_session::Column> to_column(const FloorGuide& guide, const wood_session::Support& support);

/// The column cutters lifted to the floor, one solid difference cutter each aimed at the column.
std::vector<std::shared_ptr<wood_session::Joint>> to_column_cutters(const FloorGuide& guide, const wood_session::Column& column);

// ═══════════════════════════════════════════════════════════════════════════
// Models
// ═══════════════════════════════════════════════════════════════════════════

/// A group named name under parent, at the root when parent is empty.
std::shared_ptr<TreeNode> add_group(wood_session::WoodSession& session, const std::string& name, const std::shared_ptr<TreeNode>& parent);

/// The column model of compas_tf example_model_2 moved by placement: the support, the column on it, the support joint and the six head cutters, every name ending in suffix; returns the column.
std::shared_ptr<wood_session::Column> add_column_model(wood_session::WoodSession& session, const FloorGuide& guide, const Xform& placement, const std::shared_ptr<TreeNode>& group, const std::string& suffix);

/// The quarter model of compas_tf example_model_4 lifted to bay_height and moved by placement: groups beds (one per row), tsections, outer_ribs, inner_ribs, wedges_inner_beams and inner_beams, ribs and inner beams as variable beams, the rest plates, every name ending in suffix.
void add_quarter_model(wood_session::WoodSession& session, const FloorGuide& guide, const Xform& placement, const std::shared_ptr<TreeNode>& group, const std::string& suffix);

/// The oculus model of compas_tf example_model_5 lifted to bay_height: the four boundary beams as variable beams, the four bottom wedges and the inner plate as plates.
void add_oculus_model(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<TreeNode>& group);

/// The column connectors of compas_tf example_model_8: a rectangle plate joint on the contact of every column with every outer rib, its dowels as long as the rib is thick by compas_tf's measure; returns the joints.
std::vector<std::shared_ptr<wood_session::JointBeam>> add_rectangle_plates(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<TreeNode>& group);

/// The seam connectors of compas_tf example_model_8: a tie joint on every end-to-end contact of two outer ribs of neighbouring quarters; returns the joints.
std::vector<std::shared_ptr<wood_session::JointBeam>> add_ties(wood_session::WoodSession& session, const std::shared_ptr<TreeNode>& group);

/// The wedges of compas_tf example_model_6: a wedge joint on every long-face contact among the inner beams and the four oculus boundary beams, shortened at both ends by 1.5 and pocketed 2/3 of the thicker member, its thickness the distance between its two outline centroids as compas_tf measures it; returns the joints.
std::vector<std::shared_ptr<wood_session::JointBeam>> add_wedges(wood_session::WoodSession& session, const FloorGuide& guide, const std::shared_ptr<TreeNode>& group);

}
