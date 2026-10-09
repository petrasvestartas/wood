#pragma once
#include "wood_session.h"
#include "reciprocal_boundary.h"

#include <optional>
#include <queue>

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// ReciprocalMove
// ═══════════════════════════════════════════════════════════════════════════

/// A reciprocal frame by translation: every other edge of each face a beam moved sideways past its neighbours, framed by straight boundary beams.
///
/// Fields: the mesh and the parameters; the beams are in the session, `beam_<i>` under `beams`, `frame_<k>` under `frame`.
class ReciprocalMove : public WoodSession {
public:
    /// Where a beam's up comes from.
    enum class BeamUp {
        OwningFace, // The normal of the face the half-edge was taken from.
        EdgeAverage, // The average of the normals of the faces sharing the edge.
    };

    const Mesh mesh; // The mesh whose edges become beams.
    const double shift; // mm every beam moves sideways; at least half the width for its ends to bear on their neighbours.
    const double width; // mm across every beam.
    const double height; // mm along every beam's up, twice the width when given as zero.
    const double cut_offset_factor; // Where an end stops from the crossing beam's axis, 1 flush with its side face.
    const std::map<std::pair<size_t, size_t>, Vector> frame_ups; // Up per naked edge key; the others are transported.
    const BeamUp beam_up; // Where every beam's up comes from.
    const wood_reciprocal::CornerJoint corner_joint; // How the frame beams meet at a corner.
    const std::map<std::pair<size_t, size_t>, int> through_priority; // Per naked edge key, the higher runs through a butt corner.

    /// The reciprocal frame on the mesh as the session named name.
    explicit ReciprocalMove(
        const Mesh& mesh,
        double shift = 50.0,
        double width = 100.0,
        double height = 0.0,
        double cut_offset_factor = 1.0,
        const std::map<std::pair<size_t, size_t>, Vector>& frame_ups = {},
        BeamUp beam_up = BeamUp::OwningFace,
        wood_reciprocal::CornerJoint corner_joint = wood_reciprocal::CornerJoint::Mitre,
        const std::map<std::pair<size_t, size_t>, int>& through_priority = {},
        const std::string& name = "reciprocal_move"
    );

    /// Not copied, as a session is not.
    ReciprocalMove(const ReciprocalMove&) = delete;

    /// Not assigned, as it is not copied.
    ReciprocalMove& operator=(const ReciprocalMove&) = delete;

    /// The interior beams, face by face.
    const std::vector<wood_reciprocal::ReciprocalBeam>& beams() const;

    /// The frame beams, in boundary-loop order.
    const std::vector<wood_reciprocal::ReciprocalBeam>& frame_beams() const;

private:
    /// One half-edge of a face as a beam axis.
    struct Axis {
        Line line; // The moved and trimmed axis.
        Vector dir; // The unit direction of the half-edge.
        Vector up; // The beam's up, from beam_up.
        bool beam; // Whether it becomes a beam: the colour taken, and an interior edge.
    };

    wood_reciprocal::MeshFaces _faces;
    std::vector<std::vector<Axis>> _axes;
    wood_reciprocal::BoundaryFrame _frame;
    std::vector<wood_reciprocal::ReciprocalBeam> _frame_beams;
    std::vector<wood_reciprocal::ReciprocalBeam> _beams;

    /// The faces of the mesh in vertex index order, with their Newell normals turned up.
    wood_reciprocal::MeshFaces compute_faces() const;

    /// The half-edge across the edge from local edge j of face i, none on the boundary.
    std::optional<std::pair<int, int>> opposite(int i, int j) const;

    /// Every half-edge's axis, the colour-0 ones moved sideways by shift and trimmed to their neighbours.
    std::vector<std::vector<Axis>> compute_axes() const;

    /// The side face of the beam on half-edge (ci, cj) that the beam on (i, j) stops at.
    Plane compute_cut_plane(
        int i,
        int j,
        const std::pair<int, int>& crossing,
        const Point& endpoint
    ) const;

    /// Every beam axis boxed and cut at both ends.
    std::vector<wood_reciprocal::ReciprocalBeam> compute_beams() const;
};

inline ReciprocalMove::ReciprocalMove(
    const Mesh& mesh,
    double shift,
    double width,
    double height,
    double cut_offset_factor,
    const std::map<std::pair<size_t, size_t>, Vector>& frame_ups,
    BeamUp beam_up,
    wood_reciprocal::CornerJoint corner_joint,
    const std::map<std::pair<size_t, size_t>, int>& through_priority,
    const std::string& name
)
    : WoodSession(name),
      mesh(mesh),
      shift(shift),
      width(width),
      height(height > 0.0 ? height : 2.0 * width),
      cut_offset_factor(cut_offset_factor),
      frame_ups(frame_ups),
      beam_up(beam_up),
      corner_joint(corner_joint),
      through_priority(through_priority) {

    // mesh: the input, its faces, upward normals and edge owners
    _faces = compute_faces();
    add_mesh(mesh, group_named("mesh"));

    // axes: every other half-edge of each face moved sideways by shift and trimmed to its neighbours
    _axes = compute_axes();
    const std::shared_ptr<TreeNode> axes = group_named("axes");

    for (const std::vector<Axis>& face_axes : _axes)
        for (const Axis& axis : face_axes)
            if (axis.beam)
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

    // beams: every axis boxed, each end cut flush at the side of the beam it bears on
    _beams = compute_beams();
    const std::shared_ptr<TreeNode> beams = group_named("beams");

    for (size_t i = 0; i < _beams.size(); i++)
        add(std::make_shared<Plate>(_beams[i].bottom, _beams[i].top, fmt::format("beam_{}", i)), beams);
}

inline const std::vector<wood_reciprocal::ReciprocalBeam>& ReciprocalMove::beams() const {
    return _beams;
}

inline const std::vector<wood_reciprocal::ReciprocalBeam>& ReciprocalMove::frame_beams() const {
    return _frame_beams;
}

// ═══════════════════════════════════════════════════════════════════════════
// Steps
// ═══════════════════════════════════════════════════════════════════════════

inline wood_reciprocal::MeshFaces ReciprocalMove::compute_faces() const {

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data = mesh.to_vertices_and_faces();
    wood_reciprocal::MeshFaces faces;
    faces.faces = data.second;

    for (size_t v = 0; v < data.first.size(); v++)
        faces.points.emplace(v, data.first[v]);

    for (const std::vector<size_t>& face : faces.faces) {
        const Point origin(0, 0, 0);
        Vector normal(0, 0, 0);

        // Newell: the sum of the corners' cross products about the origin
        for (size_t k = 0; k < face.size(); k++) {
            const Vector a = data.first[face[k]] - origin;
            const Vector b = data.first[face[(k + 1) % face.size()]] - origin;
            normal += a.cross(b);
        }

        normal = normal.is_zero() ? Vector(0, 0, 1) : normal.normalized();
        faces.normals.push_back(normal[2] < 0.0 ? -normal : normal);
    }

    faces.owners = wood_reciprocal::edge_owners(faces.faces);
    return faces;
}

inline std::optional<std::pair<int, int>> ReciprocalMove::opposite(int i, int j) const {

    const std::vector<size_t>& face = _faces.faces[i];
    const std::pair<size_t, size_t> key = wood_reciprocal::edge_key(face[j], face[(j + 1) % face.size()]);
    const std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>>::const_iterator found = _faces.owners.find(key);

    if (found == _faces.owners.end())
        return std::nullopt;

    for (const std::pair<int, int>& owner : found->second)
        if (owner.first != i)
            return owner;

    return std::nullopt;
}

inline std::vector<std::vector<ReciprocalMove::Axis>> ReciprocalMove::compute_axes() const {

    const std::vector<std::vector<size_t>>& faces = _faces.faces;
    const int nf = (int)faces.size();

    // half-edge ids, and every interior half-edge linked to its opposite and its interior face neighbours
    std::vector<std::vector<int>> ids(nf);
    int count = 0;

    for (int i = 0; i < nf; i++)
        for (size_t j = 0; j < faces[i].size(); j++)
            ids[i].push_back(count++);

    std::vector<std::vector<int>> links(count);

    for (int i = 0; i < nf; i++) {
        const int n = (int)faces[i].size();

        for (int j = 0; j < n; j++) {
            const std::optional<std::pair<int, int>> across = opposite(i, j);

            if (!across)
                continue;

            links[ids[i][j]].push_back(ids[across->first][across->second]);

            if (opposite(i, (j + 1) % n))
                links[ids[i][j]].push_back(ids[i][(j + 1) % n]);

            if (opposite(i, (j - 1 + n) % n))
                links[ids[i][j]].push_back(ids[i][(j - 1 + n) % n]);
        }
    }

    // colours: the half-edges two-coloured breadth first, colour 0 the beams
    std::vector<int> colours(count, -1);

    for (int seed = 0; seed < count; seed++) {
        if (colours[seed] != -1)
            continue;

        colours[seed] = 0;
        std::queue<int> queue;
        queue.push(seed);

        while (!queue.empty()) {
            const int current = queue.front();
            queue.pop();

            for (int next : links[current]) {
                if (colours[next] != -1)
                    continue;

                colours[next] = 1 - colours[current];
                queue.push(next);
            }
        }
    }

    // lines: every half-edge with its unit direction and up
    std::vector<std::vector<Axis>> axes(nf);

    for (int i = 0; i < nf; i++) {
        const int n = (int)faces[i].size();

        for (int j = 0; j < n; j++) {
            const Point& from = _faces.points.at(faces[i][j]);
            const Point& to = _faces.points.at(faces[i][(j + 1) % n]);
            const Vector edge = to - from;
            Vector up = _faces.normals[i];

            if (beam_up == BeamUp::EdgeAverage) {
                Vector sum(0, 0, 0);
                const std::pair<size_t, size_t> key = wood_reciprocal::edge_key(faces[i][j], faces[i][(j + 1) % n]);

                for (const std::pair<int, int>& owner : _faces.owners.at(key))
                    sum += _faces.normals[owner.first];

                if (!sum.is_zero())
                    up = sum.normalized();
            }

            const bool beam = colours[ids[i][j]] == 0 && opposite(i, j).has_value();
            axes[i].push_back(Axis{Line::from_points(from, to), edge.is_zero() ? edge : edge.normalized(), up, beam});
        }
    }

    // moved: every colour-0 half-edge shifted along the bisector of its two neighbours
    for (int i = 0; i < nf; i++) {
        const int n = (int)faces[i].size();

        for (int j = 0; j < n; j++) {
            if (colours[ids[i][j]] != 0)
                continue;

            const Vector bisector = (axes[i][(j + 1) % n].dir - axes[i][(j - 1 + n) % n].dir) * 0.5;
            axes[i][j].line += bisector * shift;
        }
    }

    // trimmed: every colour-0 line from its closest point to the previous neighbour to the next
    for (int i = 0; i < nf; i++) {
        const int n = (int)faces[i].size();

        for (int j = 0; j < n; j++) {
            if (colours[ids[i][j]] != 0)
                continue;

            const Point start = axes[i][j].line.start();
            const Line ray = Line::from_points(start, start + axes[i][j].dir);
            std::array<Point, 2> ends = {axes[i][j].line.start(), axes[i][j].line.end()};

            for (int side = 0; side < 2; side++) {
                const int neighbour = side == 0 ? (j - 1 + n) % n : (j + 1) % n;
                const std::pair<int, int> next = opposite(i, neighbour).value_or(std::make_pair(i, neighbour));
                const Axis& other = axes[next.first][next.second];
                const Point other_start = other.line.start();
                const Line other_ray = Line::from_points(other_start, other_start + other.dir);
                double t = 0.0;
                double s = 0.0;

                if (Intersection::line_line_parameters(ray, other_ray, t, s, 0.0, false))
                    ends[side] = ray.point_at(t);
                else
                    ends[side] = start;
            }

            axes[i][j].line = Line::from_points(ends[0], ends[1]);
        }
    }

    return axes;
}

inline Plane ReciprocalMove::compute_cut_plane(
    int i,
    int j,
    const std::pair<int, int>& crossing,
    const Point& endpoint
) const {

    constexpr double FLAT_CAP_ALIGNMENT = 0.15; // cos below which the side face runs away along the beam
    const Axis& own = _axes[i][j];
    const Axis& other = _axes[crossing.first][crossing.second];
    Vector normal = other.dir.cross(other.up);
    normal = normal.is_zero() ? other.dir.cross(_axes[i][0].up) : normal.normalized();

    if (std::abs(normal.dot(own.dir)) < FLAT_CAP_ALIGNMENT)
        return Plane::from_point_normal(endpoint, own.dir);

    const Point mid = other.line.center();

    if ((own.line.center() - mid).dot(normal) < 0.0)
        normal = -normal;

    return Plane::from_point_normal(mid + normal * (width * 0.5 * cut_offset_factor), normal);
}

inline std::vector<wood_reciprocal::ReciprocalBeam> ReciprocalMove::compute_beams() const {

    std::vector<wood_reciprocal::ReciprocalBeam> beams;

    for (int i = 0; i < (int)_axes.size(); i++) {
        const int n = (int)_axes[i].size();

        for (int j = 0; j < n; j++) {
            const Axis& axis = _axes[i][j];

            if (!axis.beam)
                continue;

            // each end: the side of the beam across the neighbouring edge, or the frame on the boundary
            std::array<std::vector<wood_reciprocal::CutFace>, 2> cuts;
            const std::array<Point, 2> ends = {axis.line.start(), axis.line.end()};
            const std::array<int, 2> neighbours = {(j - 1 + n) % n, (j + 1) % n};
            const std::array<size_t, 2> vertices = {_faces.faces[i][j], _faces.faces[i][(j + 1) % n]};

            for (int side = 0; side < 2; side++) {
                const std::optional<std::pair<int, int>> crossing = opposite(i, neighbours[side]);

                if (crossing) {
                    cuts[side] = wood_reciprocal::unbounded(compute_cut_plane(i, j, *crossing, ends[side]));
                    continue;
                }

                cuts[side] = wood_reciprocal::frame_cuts(_frame, vertices[side], axis.dir);

                if (cuts[side].empty())
                    cuts[side] = wood_reciprocal::unbounded(Plane::from_point_normal(ends[side], axis.dir));
            }

            const wood_reciprocal::ReciprocalBeam beam = wood_reciprocal::cut_beam(
                ends[0],
                ends[1],
                axis.dir,
                axis.up,
                width,
                height,
                cuts[0],
                cuts[1]
            );
            beams.push_back(beam);
        }
    }

    return beams;
}
