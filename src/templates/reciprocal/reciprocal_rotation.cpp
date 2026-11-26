#include "src/templates/reciprocal/reciprocal_rotation.h"

namespace wood_reciprocal {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
ReciprocalRotation::ReciprocalRotation(const ReciprocalRotationGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // mesh: the input, its faces, normals and edge owners
    _faces = MeshFaces::of(guide.mesh);
    add_mesh(guide.mesh, group_named("mesh"));

    // axes: every interior edge scaled about its midpoint and turned about its up by angle
    _axes = compute_axes();
    const std::shared_ptr<TreeNode> axes = group_named("axes");

    for (const Axis& axis : _axes)
        add_line(axis.line, axes);

    // frame: a straight beam on every naked edge, mitered, or butted at the corners
    _frame = boundary_frame(
        _faces.faces,
        _faces.owners,
        _faces.points,
        _faces.normals,
        guide.width,
        guide.beam_height(),
        guide.frame_ups,
        guide.butt,
        guide.through_priority
    );
    _frame_beams = wood_reciprocal::frame_beams(_frame, _faces.faces, _faces.points, guide.width, guide.beam_height());
    const std::shared_ptr<TreeNode> frame = group_named("frame");

    for (size_t k = 0; k < _frame_beams.size(); k++)
        add(std::make_shared<FacePlanarElement>(
                _frame_beams[k].bottom, _frame_beams[k].top, fmt::format("frame_{}", k)
            ),
            frame);

    // beams: every axis boxed, each end stopped at the first side face it meets at its vertex
    _beams = compute_beams();
    const std::shared_ptr<TreeNode> beams = group_named("beams");

    for (size_t i = 0; i < _beams.size(); i++)
        add(std::make_shared<FacePlanarElement>(_beams[i].bottom, _beams[i].top, fmt::format("beam_{}", i)),
            beams);

    // assembly: the frame beam by beam one module, then the beams from the frame to the middle, each slid in past the ones that bear on it
    std::vector<AssemblyModule> whole = {AssemblyModule::module("frame", AssemblyModule::parts_of(get_elements_numbered<FacePlanarElement>("frame")))};
    const std::vector<AssemblyModule> bars = AssemblyModule::parts_of(get_elements_numbered<FacePlanarElement>("beam"));
    whole.insert(whole.end(), bars.begin(), bars.end());
    assemblies = {AssemblyModule::raise(name, whole)};
}

const std::vector<ReciprocalBeam>& ReciprocalRotation::beams() const {
    return _beams;
}

const std::vector<ReciprocalBeam>& ReciprocalRotation::frame_beams() const {
    return _frame_beams;
}

// ═══════════════════════════════════════════════════════════════════════════
// Steps
// ═══════════════════════════════════════════════════════════════════════════
std::vector<ReciprocalRotation::Axis> ReciprocalRotation::compute_axes() const {

    std::vector<Axis> axes;
    const std::vector<std::pair<size_t, size_t>> edges = guide.mesh.edges();

    for (const std::pair<size_t, size_t>& edge : edges) {
        const std::vector<std::pair<int, int>>& owners =
            _faces.owners.at(edge_key(edge.first, edge.second));
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
        const Vector dir = along.normalized().transformed(Xform::rotation(up, guide.angle));
        const double half = along.magnitude() * 0.5 * guide.scale;
        axes.push_back(Axis{
            {edge.first, edge.second}, Line::from_points(mid - dir * half, mid + dir * half), up
        });
    }

    return axes;
}

std::vector<CutFace> ReciprocalRotation::compute_end_cuts(
    size_t i, size_t vertex, const Point& axis_end, const std::vector<size_t>& at_vertex
) const {

    const Point mid = _axes[i].line.center();
    const Vector dir = _axes[i].line.to_direction();
    const Vector outward = axis_end - mid;
    const Plane beyond_vertex = Plane::from_point_normal(
        _faces.points.at(vertex), outward.is_zero() ? dir : outward.normalized()
    );
    std::vector<CutFace> cuts;

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
            Plane::from_point_normal(other.line.start() - other_dir * guide.width, other_dir),
            Plane::from_point_normal(other.line.end() + other_dir * guide.width, -other_dir),
            beyond_vertex,
        };
        const Plane face =
            Plane::from_point_normal(other_mid + side * (guide.width * 0.5 * guide.cut_offset_factor), side);
        cuts.push_back(CutFace{face, bounds});
    }

    const std::vector<CutFace> frame =
        frame_cuts(_frame, vertex, dir);
    cuts.insert(cuts.end(), frame.begin(), frame.end());

    if (cuts.empty())
        return unbounded(Plane::from_point_normal(axis_end, dir));

    return cuts;
}

std::vector<ReciprocalBeam> ReciprocalRotation::compute_beams() const {

    std::vector<ReciprocalBeam> beams;
    const double extend = guide.width * guide.extend_factor;
    std::map<size_t, std::vector<size_t>> axes_at; // vertex key -> the axes of its edges.

    for (size_t i = 0; i < _axes.size(); i++) {
        axes_at[_axes[i].vertices[0]].push_back(i);
        axes_at[_axes[i].vertices[1]].push_back(i);
    }

    for (size_t i = 0; i < _axes.size(); i++) {
        const Line& line = _axes[i].line;
        const Vector dir = line.to_direction();
        const size_t start = _axes[i].vertices[0];
        const size_t end = _axes[i].vertices[1];
        const std::vector<CutFace> cuts_start =
            compute_end_cuts(i, start, line.start(), axes_at[start]);
        const std::vector<CutFace> cuts_end =
            compute_end_cuts(i, end, line.end(), axes_at[end]);
        const ReciprocalBeam beam = cut_beam(
            line.start() - dir * extend,
            line.end() + dir * extend,
            dir,
            _axes[i].up,
            guide.width,
            guide.beam_height(),
            cuts_start,
            cuts_end
        );
        beams.push_back(beam);
    }

    return beams;
}

}
