#include "src/templates/reciprocal/reciprocal_move.h"

#include <queue>

namespace wood_reciprocal {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
ReciprocalMove::ReciprocalMove(const ReciprocalMoveGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // mesh: the input, its faces, upward normals and edge owners
    _faces = compute_faces();
    add_mesh(guide.mesh, group_named("mesh"));

    // axes: every other half-edge of each face moved sideways by shift and trimmed to its neighbours
    _axes = compute_axes();
    const std::shared_ptr<TreeNode> axes = group_named("axes");

    for (const std::vector<Axis>& face_axes : _axes)
        for (const Axis& axis : face_axes)
            if (axis.beam)
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

    // beams: every axis boxed, each end cut flush at the side of the beam it bears on
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

const std::vector<ReciprocalBeam>& ReciprocalMove::beams() const {
    return _beams;
}

const std::vector<ReciprocalBeam>& ReciprocalMove::frame_beams() const {
    return _frame_beams;
}

// ═══════════════════════════════════════════════════════════════════════════
// Steps
// ═══════════════════════════════════════════════════════════════════════════
MeshFaces ReciprocalMove::compute_faces() const {

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data =
        guide.mesh.to_vertices_and_faces();
    MeshFaces faces;
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

    faces.owners = edge_owners(faces.faces);
    return faces;
}

std::optional<std::pair<int, int>> ReciprocalMove::opposite(int i, int j) const {

    const std::vector<size_t>& face = _faces.faces[i];
    const std::pair<size_t, size_t> key =
        edge_key(face[j], face[(j + 1) % face.size()]);
    const std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>>::const_iterator
        found = _faces.owners.find(key);

    if (found == _faces.owners.end())
        return std::nullopt;

    for (const std::pair<int, int>& owner : found->second)
        if (owner.first != i)
            return owner;

    return std::nullopt;
}

std::vector<std::vector<ReciprocalMove::Axis>> ReciprocalMove::compute_axes() const {

    const std::vector<std::vector<size_t>>& faces = _faces.faces;
    const int nf = (int)faces.size();

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

            if (guide.edge_up) {
                Vector sum(0, 0, 0);
                const std::pair<size_t, size_t> key =
                    edge_key(faces[i][j], faces[i][(j + 1) % n]);

                for (const std::pair<int, int>& owner : _faces.owners.at(key))
                    sum += _faces.normals[owner.first];

                if (!sum.is_zero())
                    up = sum.normalized();
            }

            const bool beam = colours[ids[i][j]] == 0 && opposite(i, j).has_value();
            axes[i].push_back(Axis{
                Line::from_points(from, to), edge.is_zero() ? edge : edge.normalized(), up, beam
            });
        }
    }

    // moved: every colour-0 half-edge shifted along the bisector of its two neighbours
    for (int i = 0; i < nf; i++) {
        const int n = (int)faces[i].size();

        for (int j = 0; j < n; j++) {
            if (colours[ids[i][j]] != 0)
                continue;

            const Vector bisector = (axes[i][(j + 1) % n].dir - axes[i][(j - 1 + n) % n].dir) * 0.5;
            axes[i][j].line += bisector * guide.shift;
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
                const std::pair<int, int> next =
                    opposite(i, neighbour).value_or(std::make_pair(i, neighbour));
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

Plane ReciprocalMove::compute_cut_plane(
    int i, int j, const std::pair<int, int>& crossing, const Point& endpoint
) const {

    constexpr double FLAT_CAP_ALIGNMENT =
        0.15; // cos below which the side face runs away along the beam.
    const Axis& own = _axes[i][j];
    const Axis& other = _axes[crossing.first][crossing.second];
    Vector normal = other.dir.cross(other.up);
    normal = normal.is_zero() ? other.dir.cross(_axes[i][0].up) : normal.normalized();

    if (std::abs(normal.dot(own.dir)) < FLAT_CAP_ALIGNMENT)
        return Plane::from_point_normal(endpoint, own.dir);

    const Point mid = other.line.center();

    if ((own.line.center() - mid).dot(normal) < 0.0)
        normal = -normal;

    return Plane::from_point_normal(mid + normal * (guide.width * 0.5 * guide.cut_offset_factor), normal);
}

std::vector<ReciprocalBeam> ReciprocalMove::compute_beams() const {

    std::vector<ReciprocalBeam> beams;

    for (int i = 0; i < (int)_axes.size(); i++) {
        const int n = (int)_axes[i].size();

        for (int j = 0; j < n; j++) {
            const Axis& axis = _axes[i][j];

            if (!axis.beam)
                continue;

            // each end: the side of the beam across the neighbouring edge, or the frame on the boundary
            std::array<std::vector<CutFace>, 2> cuts;
            const std::array<Point, 2> ends = {axis.line.start(), axis.line.end()};
            const std::array<int, 2> neighbours = {(j - 1 + n) % n, (j + 1) % n};
            const std::array<size_t, 2> vertices = {
                _faces.faces[i][j], _faces.faces[i][(j + 1) % n]
            };

            for (int side = 0; side < 2; side++) {
                const std::optional<std::pair<int, int>> crossing = opposite(i, neighbours[side]);

                if (crossing) {
                    cuts[side] =
                        unbounded(compute_cut_plane(i, j, *crossing, ends[side]));
                    continue;
                }

                cuts[side] = frame_cuts(_frame, vertices[side], axis.dir);

                if (cuts[side].empty())
                    cuts[side] =
                        unbounded(Plane::from_point_normal(ends[side], axis.dir));
            }

            const ReciprocalBeam beam = cut_beam(
                ends[0], ends[1], axis.dir, axis.up, guide.width, guide.beam_height(), cuts[0], cuts[1]
            );
            beams.push_back(beam);
        }
    }

    return beams;
}

}
