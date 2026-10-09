#pragma once
#include "wood_session.h"
#include "reciprocal_boundary.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// ReciprocalRotation
// ═══════════════════════════════════════════════════════════════════════════

/// A reciprocal frame by rotation: every interior mesh edge a beam turned about its midpoint, framed by straight boundary beams.
///
/// Fields: the mesh and the parameters; the beams are in the session, `beam_<i>` under `beams`, `frame_<k>` under `frame`.
class ReciprocalRotation : public WoodSession {
public:
    const Mesh mesh; // The mesh whose interior edges become beams.
    const double angle; // Radians every beam turns about its midpoint.
    const double scale; // How much every beam is lengthened about its midpoint before the cuts.
    const double width; // mm across every beam.
    const double height; // mm along every beam's up, twice the width when given as zero.
    const double extend_factor; // Beam widths a beam may run past its ends to reach the beam it bears on.
    const double cut_offset_factor; // Where an end stops from the crossing beam's axis, 1 flush with its side face.
    const std::map<std::pair<size_t, size_t>, Vector> frame_ups; // Up per naked edge key; the others are transported.
    const wood_reciprocal::CornerJoint corner_joint; // How the frame beams meet at a corner.
    const std::map<std::pair<size_t, size_t>, int> through_priority; // Per naked edge key, the higher runs through a butt corner.

    /// The reciprocal frame on the mesh as the session named name.
    explicit ReciprocalRotation(
        const Mesh& mesh,
        double angle = 0.35,
        double scale = 1.4,
        double width = 100.0,
        double height = 0.0,
        double extend_factor = 5.0,
        double cut_offset_factor = 1.0,
        const std::map<std::pair<size_t, size_t>, Vector>& frame_ups = {},
        wood_reciprocal::CornerJoint corner_joint = wood_reciprocal::CornerJoint::Mitre,
        const std::map<std::pair<size_t, size_t>, int>& through_priority = {},
        const std::string& name = "reciprocal_rotation"
    );

    /// Not copied, as a session is not.
    ReciprocalRotation(const ReciprocalRotation&) = delete;

    /// Not assigned, as it is not copied.
    ReciprocalRotation& operator=(const ReciprocalRotation&) = delete;

    /// The interior beams, in the order of the mesh's edges.
    const std::vector<wood_reciprocal::ReciprocalBeam>& beams() const;

    /// The frame beams, in boundary-loop order.
    const std::vector<wood_reciprocal::ReciprocalBeam>& frame_beams() const;

private:
    /// One interior edge turned into a beam axis.
    struct Axis {
        std::array<size_t, 2> vertices; // The edge's start and end vertex keys.
        Line line; // The scaled and rotated axis.
        Vector up; // The average normal of the two faces of the edge.
    };

    wood_reciprocal::MeshFaces _faces;
    std::vector<Axis> _axes;
    wood_reciprocal::BoundaryFrame _frame;
    std::vector<wood_reciprocal::ReciprocalBeam> _frame_beams;
    std::vector<wood_reciprocal::ReciprocalBeam> _beams;

    /// Every interior edge scaled about its midpoint and turned about its up by angle.
    std::vector<Axis> compute_axes() const;

    /// The faces one end of axis i at vertex can stop at: the near sides of the other beams there, then the frame.
    std::vector<wood_reciprocal::CutFace> compute_end_cuts(
        size_t i,
        size_t vertex,
        const Point& axis_end,
        const std::vector<size_t>& at_vertex
    ) const;

    /// Every axis boxed and cut at both ends.
    std::vector<wood_reciprocal::ReciprocalBeam> compute_beams() const;
};

inline ReciprocalRotation::ReciprocalRotation(
    const Mesh& mesh,
    double angle,
    double scale,
    double width,
    double height,
    double extend_factor,
    double cut_offset_factor,
    const std::map<std::pair<size_t, size_t>, Vector>& frame_ups,
    wood_reciprocal::CornerJoint corner_joint,
    const std::map<std::pair<size_t, size_t>, int>& through_priority,
    const std::string& name
)
    : WoodSession(name),
      mesh(mesh),
      angle(angle),
      scale(scale),
      width(width),
      height(height > 0.0 ? height : 2.0 * width),
      extend_factor(extend_factor),
      cut_offset_factor(cut_offset_factor),
      frame_ups(frame_ups),
      corner_joint(corner_joint),
      through_priority(through_priority) {

    // mesh: the input, its faces, normals and edge owners
    _faces = wood_reciprocal::MeshFaces::of(mesh);
    add_mesh(mesh, group_named("mesh"));

    // axes: every interior edge scaled about its midpoint and turned about its up by angle
    _axes = compute_axes();
    const std::shared_ptr<TreeNode> axes = group_named("axes");

    for (const Axis& axis : _axes)
        add_line(axis.line, axes);

    // frame: a straight beam on every naked edge, mitred, or butted at the corners
    _frame = wood_reciprocal::boundary_frame(
        _faces.faces,
        _faces.owners,
        _faces.points,
        _faces.normals,
        this->width,
        this->height,
        frame_ups,
        corner_joint,
        through_priority
    );
    _frame_beams = wood_reciprocal::frame_beams(
        _frame,
        _faces.faces,
        _faces.points,
        this->width,
        this->height
    );
    const std::shared_ptr<TreeNode> frame = group_named("frame");

    for (size_t k = 0; k < _frame_beams.size(); k++)
        add(std::make_shared<Plate>(_frame_beams[k].bottom, _frame_beams[k].top, fmt::format("frame_{}", k)), frame);

    // beams: every axis boxed, each end stopped at the first side face it meets at its vertex
    _beams = compute_beams();
    const std::shared_ptr<TreeNode> beams = group_named("beams");

    for (size_t i = 0; i < _beams.size(); i++)
        add(std::make_shared<Plate>(_beams[i].bottom, _beams[i].top, fmt::format("beam_{}", i)), beams);
}

inline const std::vector<wood_reciprocal::ReciprocalBeam>& ReciprocalRotation::beams() const {
    return _beams;
}

inline const std::vector<wood_reciprocal::ReciprocalBeam>& ReciprocalRotation::frame_beams() const {
    return _frame_beams;
}

// ═══════════════════════════════════════════════════════════════════════════
// Steps
// ═══════════════════════════════════════════════════════════════════════════

inline std::vector<ReciprocalRotation::Axis> ReciprocalRotation::compute_axes() const {

    std::vector<Axis> axes;
    const std::vector<std::pair<size_t, size_t>> edges = mesh.edges();

    for (const std::pair<size_t, size_t>& edge : edges) {
        const std::vector<std::pair<int, int>>& owners = _faces.owners.at(wood_reciprocal::edge_key(edge.first, edge.second));
        const Point& start = _faces.points.at(edge.first);
        const Point& end = _faces.points.at(edge.second);
        const Vector along = end - start;

        if (owners.size() < 2 || along.is_zero())
            continue;

        Vector up(0, 0, 0);

        for (const std::pair<int, int>& owner : owners)
            up += _faces.normals[owner.first];

        up = up.is_zero() ? Vector(0, 0, 1) : up.normalized();
        const Point mid = Point::mid_point(start, end);
        const Vector dir = along.normalized().transformed(Xform::rotation(up, angle));
        const double half = along.magnitude() * 0.5 * scale;
        axes.push_back(Axis{{edge.first, edge.second}, Line::from_points(mid - dir * half, mid + dir * half), up});
    }

    return axes;
}

inline std::vector<wood_reciprocal::CutFace> ReciprocalRotation::compute_end_cuts(
    size_t i,
    size_t vertex,
    const Point& axis_end,
    const std::vector<size_t>& at_vertex
) const {

    const Point mid = _axes[i].line.center();
    const Vector dir = _axes[i].line.to_direction();
    const Vector outward = axis_end - mid;
    const Plane beyond_vertex = Plane::from_point_normal(_faces.points.at(vertex), outward.is_zero() ? dir : outward.normalized());
    std::vector<wood_reciprocal::CutFace> cuts;

    // the side face of every other beam at the vertex that looks at this one, within its length and past the vertex
    for (size_t j : at_vertex) {
        if (j == i)
            continue;

        const Axis& other = _axes[j];
        const Vector other_dir = other.line.to_direction();
        Vector side = other_dir.cross(other.up);

        if (side.is_zero())
            continue;

        side = side.normalized();
        const Point other_mid = other.line.center();

        if ((mid - other_mid).dot(side) < 0.0)
            side = -side;

        const std::vector<Plane> bounds = {
            Plane::from_point_normal(other.line.start() - other_dir * width, other_dir),
            Plane::from_point_normal(other.line.end() + other_dir * width, -other_dir),
            beyond_vertex,
        };
        const Plane face = Plane::from_point_normal(other_mid + side * (width * 0.5 * cut_offset_factor), side);
        cuts.push_back(wood_reciprocal::CutFace{face, bounds});
    }

    // the inner faces of the frame at a boundary vertex
    const std::vector<wood_reciprocal::CutFace> frame = wood_reciprocal::frame_cuts(_frame, vertex, dir);
    cuts.insert(cuts.end(), frame.begin(), frame.end());

    if (cuts.empty())
        return wood_reciprocal::unbounded(Plane::from_point_normal(axis_end, dir));

    return cuts;
}

inline std::vector<wood_reciprocal::ReciprocalBeam> ReciprocalRotation::compute_beams() const {

    std::vector<wood_reciprocal::ReciprocalBeam> beams;
    const double extend = width * extend_factor;
    std::map<size_t, std::vector<size_t>> axes_at; // vertex key -> the axes of its edges

    for (size_t i = 0; i < _axes.size(); i++) {
        axes_at[_axes[i].vertices[0]].push_back(i);
        axes_at[_axes[i].vertices[1]].push_back(i);
    }

    for (size_t i = 0; i < _axes.size(); i++) {
        const Line& line = _axes[i].line;
        const Vector dir = line.to_direction();
        const size_t start = _axes[i].vertices[0];
        const size_t end = _axes[i].vertices[1];
        const std::vector<wood_reciprocal::CutFace> cuts_start = compute_end_cuts(
            i,
            start,
            line.start(),
            axes_at[start]
        );
        const std::vector<wood_reciprocal::CutFace> cuts_end = compute_end_cuts(
            i,
            end,
            line.end(),
            axes_at[end]
        );
        const wood_reciprocal::ReciprocalBeam beam = wood_reciprocal::cut_beam(
            line.start() - dir * extend,
            line.end() + dir * extend,
            dir,
            _axes[i].up,
            width,
            height,
            cuts_start,
            cuts_end
        );
        beams.push_back(beam);
    }

    return beams;
}
