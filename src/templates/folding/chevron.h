#pragma once
#include "wood_session.h"
#include "json.h"

#include <fstream>

using namespace session_cpp;
using namespace wood_session;

namespace wood_chevron {

/// The 23 Annen NURBS surfaces of annen_surfaces.json, an empty list when the file is missing.
inline std::vector<NurbsSurface> annen_surfaces(const std::string& json_path) {

    std::ifstream file(json_path);

    if (!file)
        return {};

    nlohmann::json entries;
    file >> entries;
    std::vector<NurbsSurface> surfaces;

    for (const nlohmann::json& entry : entries) {
        const int count_u = entry["n_u"];
        const int count_v = entry["n_v"];
        std::array<std::vector<double>, 2> knots;
        const std::array<std::string, 2> multiplicity_keys = {"u_mults", "v_mults"};
        const std::array<std::string, 2> value_keys = {"u_nurbsknots", "v_nurbsknots"};

        // OpenNURBS knots: every value repeated by its multiplicity, without the first and the last
        for (size_t dir = 0; dir < 2; dir++) {
            const std::vector<int> multiplicities = entry[multiplicity_keys[dir]].get<std::vector<int>>();
            const std::vector<double> values = entry[value_keys[dir]].get<std::vector<double>>();

            for (size_t i = 0; i < values.size(); i++)
                for (int k = 0; k < multiplicities[i]; k++)
                    knots[dir].push_back(values[i]);

            knots[dir] = std::vector<double>(knots[dir].begin() + 1, knots[dir].end() - 1);
        }

        NurbsSurface surface;
        const int degree_u = entry["degree_u"];
        const int degree_v = entry["degree_v"];
        surface.create_raw(
            3,
            false,
            degree_u + 1,
            degree_v + 1,
            count_u,
            count_v
        );

        for (size_t dir = 0; dir < 2; dir++)
            for (size_t i = 0; i < knots[dir].size(); i++)
                surface.set_nurbsknot(dir, i, knots[dir][i]);

        const nlohmann::json& points = entry["points"];

        for (int i = 0; i < count_u; i++)
            for (int j = 0; j < count_v; j++)
                surface.set_cv(i, j, {points[i][j][0], points[i][j][1], points[i][j][2]});

        if (surface.is_valid())
            surfaces.push_back(surface);
    }

    return surfaces;
}

/// The quad faces of the chevron mesh, each face's vertices reversed, and the faces on every edge.
///
/// - `vertices`: per face its four vertex keys, reversed so the normals point down as in the Rhino original.
/// - `edge_faces`: per edge, by its sorted vertex pair, the faces on it.
struct ChevronFaces {
    std::vector<std::vector<size_t>> vertices; // Per face its four vertex keys, reversed.
    std::map<std::pair<size_t, size_t>, std::vector<int>> edge_faces; // Per sorted vertex pair the faces on it.
};

/// The faces joined in strips along v, each face with its two chevron edges.
///
/// - `chevron_edges`: per face its two edges against the next strip, {3, 0} on even strips, {0, 1} on odd ones.
/// - `flips`: per face whether its strip is even, which turns its rotation the other way.
/// - `order`: the faces strip by strip, the order of the plates.
struct ChevronStrips {
    std::vector<std::array<int, 2>> chevron_edges; // Per face its two chevron edges.
    std::vector<bool> flips; // Per face whether its strip is even.
    std::vector<int> order; // The faces strip by strip.
};

}

/// A chevron shell: a NURBS surface folded into chevron strips, four plates per face, a box between a top and a bottom plate.
///
/// Fields: the surface, the mesh divisions and the box sizes; every step's geometry and the plates are in the session.
class Chevron : public WoodSession {
public:
    const NurbsSurface surface; // The surface the chevrons are laid on.
    const int u_divisions; // Strips across u.
    const double v_division_dist; // The first row's height in v, in the surface's parameter units.
    const double shift; // Share of a row's height the middle of a chevron stands ahead of its sides.
    const double scale; // Growth of the row height per row, towards the middle.
    const double box_height; // mm between the box's top and bottom faces.
    const double top_plate_inlet; // mm the top and bottom plates stand in from the box faces.
    const double plate_thickness; // mm, every plate.
    const double edge_rotation; // Degrees the even chevron edges turn about their plane's y axis.
    const double edge_offset; // Share of the plate thickness the odd chevron edges move along their normal.
    const std::array<int, 4> ortho_edges; // Per edge index the axis a boundary edge plane snaps to: 0 none, 1 dominant, 2 X, 3 Y, 4 Z.

    /// The chevron shell on surface as the session named name.
    explicit Chevron(
        const NurbsSurface& surface = default_surface(),
        int u_divisions = 4,
        double v_division_dist = 900.0,
        double shift = 0.5,
        double scale = 0.05799,
        double box_height = 760.0,
        double top_plate_inlet = 80.0,
        double plate_thickness = 40.0,
        double edge_rotation = 1.0,
        double edge_offset = 0.5,
        const std::array<int, 4>& ortho_edges = {1, 1, 1, 1},
        const std::string& name = "chevron"
    );

    /// Not copied, as a session is not.
    Chevron(const Chevron&) = delete;

    /// Not assigned, as it is not copied.
    Chevron& operator=(const Chevron&) = delete;

    /// The chevron quad mesh.
    const Mesh& mesh() const;

    /// Per plate pair six insertion vectors as 18 numbers, the four sides along the corner bisectors.
    const std::vector<std::array<double, 18>>& insertion_vectors() const;

    /// Per plate pair six joint types, one per face: 0 none, 10 tenon, 20 mortise.
    const std::vector<std::array<int, 6>>& joints_per_face() const;

    /// Rows {s0, s1, e20, e31}: plate pairs s0 and e20 both join s1, so its joint is trimmed for both.
    const std::vector<std::array<int, 4>>& three_valence() const;

    /// The plate pairs that share a joint.
    const std::vector<std::pair<int, int>>& adjacency() const;

    /// A flat bicubic surface, 3000 by 5000, centred on the origin.
    static NurbsSurface default_surface();

private:
    Mesh _mesh;
    wood_chevron::ChevronFaces _faces;
    wood_chevron::ChevronStrips _strips;
    std::vector<Plane> _face_planes;
    std::vector<std::array<Plane, 4>> _edge_planes;
    std::vector<std::array<std::optional<Plane>, 4>> _bisectors;
    std::vector<std::array<double, 18>> _insertion_vectors;
    std::vector<std::array<int, 6>> _joints_per_face;
    std::vector<std::array<int, 4>> _three_valence;
    std::vector<std::pair<int, int>> _adjacency;

    /// Strips across u, rows from both v ends growing by scale towards the middle, each row a chevron.
    Mesh compute_mesh() const;

    /// Every face's reversed vertices and the faces on every edge.
    wood_chevron::ChevronFaces compute_faces() const;

    /// The faces grown into strips along v, alternate strips with the other two chevron edges.
    wood_chevron::ChevronStrips compute_strips() const;

    /// A plane through every face centre along its normal.
    std::vector<Plane> compute_face_planes() const;

    /// A plane on every face edge, through its middle, along the edge and the faces' mean normal.
    std::vector<std::array<Plane, 4>> compute_edge_planes() const;

    /// The edge planes with every inner chevron edge offset or rotated, copied flipped to its neighbour, the boundary ones snapped.
    std::vector<std::array<Plane, 4>> compute_turned_edge_planes(std::vector<std::array<Plane, 4>> edge_planes) const;

    /// The dihedral plane at every face corner, between the edge planes meeting there.
    std::vector<std::array<std::optional<Plane>, 4>> compute_bisectors() const;

    /// Four plates per face in strip order: top and bottom inset in the box, a side on each chevron edge.
    std::vector<std::shared_ptr<Plate>> compute_plates() const;

    /// The insertion vectors, joint types, three-valence groups and adjacency of the plate pairs, and an insertion line per face.
    std::vector<Line> compute_joinery();

    /// The three points of a chevron row at u: its sides at v_side, its middle at v_middle.
    std::array<Point, 3> chevron_row(const NurbsSurface& transposed, double u, double v_side, double v_middle) const;

    /// The faces on the edge between vertices a and b.
    const std::vector<int>& edge_faces(size_t a, size_t b) const;

    /// The face's normal from its reversed vertices: the cross product of the diagonals of a quad.
    Vector face_normal(int face) const;

    /// The face's two chevron edges in the order its side plates are made.
    std::array<int, 2> sorted_chevron_edges(int face) const;

    /// The plane through the line where p0 and p1 meet, halfway between them; none when they are parallel or share an origin.
    static std::optional<Plane> dihedral_plane(const Plane& p0, const Plane& p1);

    /// The plane with its normal snapped to the world axis given by axis, 1 the dominant one.
    static Plane snapped_to_axis(const Plane& plane, int axis);

    /// The closed polygon where base meets sides one after the other.
    static Polyline polygon_from_planes(const Plane& base, const std::vector<Plane>& sides);

    /// The unit direction of the line where the face plane meets the bisector, zero without one.
    static Vector bisector_direction(const Plane& face_plane, const std::optional<Plane>& bisector);
};

inline Chevron::Chevron(
    const NurbsSurface& surface,
    int u_divisions,
    double v_division_dist,
    double shift,
    double scale,
    double box_height,
    double top_plate_inlet,
    double plate_thickness,
    double edge_rotation,
    double edge_offset,
    const std::array<int, 4>& ortho_edges,
    const std::string& name
)
    : WoodSession(name),
      surface(surface),
      u_divisions(std::max(u_divisions, 1)),
      v_division_dist(v_division_dist),
      shift(shift),
      scale(scale),
      box_height(std::max(box_height, 1.0)),
      top_plate_inlet(std::clamp(top_plate_inlet, 1.0, std::max(box_height, 1.0) * 0.333)),
      plate_thickness(std::clamp(plate_thickness, 1.0, std::max(box_height, 1.0) * 0.333)),
      edge_rotation(std::clamp(edge_rotation, -30.0, 30.0)),
      edge_offset(std::clamp(edge_offset, -2.0, 2.0)),
      ortho_edges(ortho_edges) {

    // surface: the NURBS surface the chevrons are laid on
    add_nurbssurface(surface, add_group("surface"));

    // mesh: strips across u, chevron rows growing from both ends to the middle
    _mesh = compute_mesh();
    add_mesh(_mesh, add_group("mesh"));

    // strips: the faces grown into strips along v, each face's two chevron edges against the next strip
    _faces = compute_faces();
    _strips = compute_strips();
    const std::shared_ptr<TreeNode> strips = add_group("strips");

    for (size_t face = 0; face < _faces.vertices.size(); face++) {
        const std::vector<size_t>& vertices = _faces.vertices[face];

        for (const int edge : _strips.chevron_edges[face]) {
            const std::optional<Point> start = _mesh.vertex_point(vertices[edge]);
            const std::optional<Point> end = _mesh.vertex_point(vertices[(edge + 1) % 4]);
            add_line(Line::from_points(*start, *end), strips);
        }
    }

    // edge_planes: a plane on every face edge, the inner chevron edges offset or rotated, the boundary ones snapped
    _face_planes = compute_face_planes();
    const std::vector<std::array<Plane, 4>> edge_planes = compute_edge_planes();
    _edge_planes = compute_turned_edge_planes(edge_planes);
    const std::shared_ptr<TreeNode> planes = add_group("edge_planes");

    for (const std::array<Plane, 4>& face_edges : _edge_planes)
        for (const Plane& plane : face_edges)
            add_plane(plane, planes);

    // bisectors: the dihedral plane at every face corner, where two side plates meet
    _bisectors = compute_bisectors();
    const std::shared_ptr<TreeNode> bisectors = add_group("bisectors");

    for (const std::array<std::optional<Plane>, 4>& corners : _bisectors)
        for (const std::optional<Plane>& bisector : corners)
            if (bisector)
                add_plane(*bisector, bisectors);

    // plates: per face a top and a bottom plate inset in the box and a side plate on each chevron edge, plate_<4 face + role>
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    for (const std::shared_ptr<Plate>& plate : compute_plates())
        add(plate, plates);

    // joinery: insertion vectors, joint types, three-valence groups and adjacency for the solver, an insertion line per face
    const std::shared_ptr<TreeNode> insertion = add_group("insertion");

    for (const Line& line : compute_joinery())
        add_line(line, insertion);
}

inline const Mesh& Chevron::mesh() const {
    return _mesh;
}

inline const std::vector<std::array<double, 18>>& Chevron::insertion_vectors() const {
    return _insertion_vectors;
}

inline const std::vector<std::array<int, 6>>& Chevron::joints_per_face() const {
    return _joints_per_face;
}

inline const std::vector<std::array<int, 4>>& Chevron::three_valence() const {
    return _three_valence;
}

inline const std::vector<std::pair<int, int>>& Chevron::adjacency() const {
    return _adjacency;
}

inline NurbsSurface Chevron::default_surface() {

    const double half_u = 1500.0;
    const double half_v = 2500.0;
    std::vector<Point> points;

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            points.push_back({half_u * (2.0 * i / 3.0 - 1.0), half_v * (2.0 * j / 3.0 - 1.0), 0.0});

    NurbsSurface flat = NurbsSurface::create(
        false,
        false,
        3,
        3,
        4,
        4,
        points
    );
    flat.set_domain(0, -half_u, half_u);
    flat.set_domain(1, -half_v, half_v);
    return flat;
}

// ═══════════════════════════════════════════════════════════════════════════
// Mesh
// ═══════════════════════════════════════════════════════════════════════════

inline std::array<Point, 3> Chevron::chevron_row(const NurbsSurface& transposed, double u, double v_side, double v_middle) const {

    const double step_u = (transposed.domain(0).second - transposed.domain(0).first) / u_divisions;
    return {
        transposed.point_at(u, v_side),
        transposed.point_at(u + step_u * 0.5, v_middle),
        transposed.point_at(u + step_u, v_side),
    };
}

inline Mesh Chevron::compute_mesh() const {

    NurbsSurface transposed = surface;
    transposed.transpose();
    const std::pair<double, double> domain_u = transposed.domain(0);
    const std::pair<double, double> domain_v = transposed.domain(1);
    const double middle = (domain_v.first + domain_v.second) * 0.5;
    const double step_u = (domain_u.second - domain_u.first) / u_divisions;
    std::vector<std::vector<Point>> polygons;
    double u = domain_u.first;

    for (int strip = 0; strip < u_divisions; strip++) {
        double v = domain_v.first;
        double rest = (domain_v.second - domain_v.first) / 2.0;
        double step = v_division_dist;
        std::vector<double> steps;
        std::array<Point, 3> upper;
        bool running = true;

        for (int row = 0; row < 1000 && running; row++) {
            steps.push_back(step);

            // the first half: each row from the last row's chevron to the next, the first from a straight row
            const std::array<Point, 3> lower = row == 0 ? chevron_row(transposed, u, v, v) : upper;
            upper = chevron_row(
                transposed,
                u,
                v + step * (1.0 - shift / 2.0),
                v + step * (1.0 + shift / 2.0)
            );
            polygons.push_back({lower[0], upper[0], upper[1], lower[1]});
            polygons.push_back({lower[1], upper[1], upper[2], lower[2]});

            v += step;
            rest -= step;
            step += step * scale;

            if (v + step <= middle)
                continue;

            // the second half: the same rows mirrored from the middle, the chevron turned, the last row straight
            steps.push_back(rest);
            std::reverse(steps.begin(), steps.end());
            double mirrored = middle;
            std::array<Point, 3> mirrored_lower;

            for (size_t i = 0; i + 1 < steps.size(); i++) {
                mirrored += steps[i];
                const double next = steps[i + 1];

                if (i == 0) {
                    mirrored_lower = chevron_row(
                        transposed,
                        u,
                        mirrored - next * shift / 2.0,
                        mirrored + next * shift / 2.0
                    );
                    polygons.push_back({upper[0], mirrored_lower[0], mirrored_lower[1], upper[1]});
                    polygons.push_back({upper[1], mirrored_lower[1], mirrored_lower[2], upper[2]});
                } else {
                    mirrored_lower = upper;
                }

                if (i + 2 == steps.size()) {
                    upper = chevron_row(transposed, u, mirrored + next, mirrored + next);
                } else {
                    upper = chevron_row(
                        transposed,
                        u,
                        mirrored + next * (1.0 - shift / 2.0),
                        mirrored + next * (1.0 + shift / 2.0)
                    );
                }

                polygons.push_back({mirrored_lower[1], upper[1], upper[2], mirrored_lower[2]});
                polygons.push_back({mirrored_lower[0], upper[0], upper[1], mirrored_lower[1]});
            }

            running = false;
        }

        u += step_u;
    }

    std::vector<Polyline> loops;

    for (const std::vector<Point>& polygon : polygons)
        loops.push_back(Polyline(polygon));

    return Mesh::from_polylines(loops, 0.01);
}

// ═══════════════════════════════════════════════════════════════════════════
// Strips
// ═══════════════════════════════════════════════════════════════════════════

inline wood_chevron::ChevronFaces Chevron::compute_faces() const {

    wood_chevron::ChevronFaces faces;

    for (const std::pair<const size_t, std::vector<size_t>>& face : _mesh.face) {
        std::vector<size_t> vertices = _mesh.face_vertices(face.first).value_or(std::vector<size_t>{});
        std::reverse(vertices.begin(), vertices.end());
        faces.vertices.push_back(vertices);
    }

    for (size_t i = 0; i < faces.vertices.size(); i++) {
        const std::vector<size_t>& vertices = faces.vertices[i];

        for (size_t j = 0; j < vertices.size(); j++) {
            const size_t a = vertices[j];
            const size_t b = vertices[(j + 1) % vertices.size()];
            faces.edge_faces[{std::min(a, b), std::max(a, b)}].push_back(static_cast<int>(i));
        }
    }

    return faces;
}

inline wood_chevron::ChevronStrips Chevron::compute_strips() const {

    const std::vector<std::vector<size_t>>& vertices = _faces.vertices;
    const size_t count = vertices.size();
    wood_chevron::ChevronStrips strips;
    strips.chevron_edges.assign(count, {0, 0});
    strips.flips.assign(count, false);
    std::vector<bool> taken(count, false);

    for (int strip_index = 0; ; strip_index++) {
        const std::vector<bool>::iterator free = std::find(taken.begin(), taken.end(), false);

        if (free == taken.end())
            break;

        const bool flip = strip_index % 2 == 0;
        const std::array<int, 2> edges = flip ? std::array<int, 2>{3, 0} : std::array<int, 2>{0, 1};
        std::vector<int> strip = {static_cast<int>(free - taken.begin())};
        taken[strip[0]] = true;

        // grow along v: a face joins its strip through its edge 1-2 or 3-0
        for (size_t k = 0; k < strip.size(); k++) {
            const std::vector<size_t>& f = vertices[strip[k]];

            for (size_t other = 0; other < count; other++) {
                if (taken[other])
                    continue;

                const std::vector<size_t>& g = vertices[other];
                const bool shared = (f[1] == g[2] && f[2] == g[1]) ||
                                    (f[3] == g[0] && f[0] == g[3]) ||
                                    (f[1] == g[0] && f[2] == g[3]) ||
                                    (f[3] == g[2] && f[0] == g[1]);

                if (!shared)
                    continue;

                taken[other] = true;
                strip.push_back(static_cast<int>(other));
            }
        }

        for (const int face : strip) {
            strips.chevron_edges[face] = edges;
            strips.flips[face] = flip;
            strips.order.push_back(face);
        }
    }

    return strips;
}

// ═══════════════════════════════════════════════════════════════════════════
// Planes
// ═══════════════════════════════════════════════════════════════════════════

inline const std::vector<int>& Chevron::edge_faces(size_t a, size_t b) const {
    return _faces.edge_faces.at({std::min(a, b), std::max(a, b)});
}

inline Vector Chevron::face_normal(int face) const {

    const std::vector<size_t>& vertices = _faces.vertices[face];
    const Point a = _mesh.vertex_point(vertices[0]).value();
    const Point b = _mesh.vertex_point(vertices[1]).value();
    const Point c = _mesh.vertex_point(vertices[2]).value();
    const Point d = _mesh.vertex_point(vertices[3]).value();
    return (c - a).cross(d - b).normalized();
}

inline std::vector<Plane> Chevron::compute_face_planes() const {

    std::vector<Plane> planes;

    for (size_t face = 0; face < _faces.vertices.size(); face++) {
        const Vector normal = face_normal(static_cast<int>(face));
        std::vector<Point> corners;

        for (const size_t vertex : _faces.vertices[face])
            corners.push_back(_mesh.vertex_point(vertex).value());

        const Vector reference = std::abs(normal[0]) < 0.9 ? Vector(1.0, 0.0, 0.0) : Vector(0.0, 1.0, 0.0);
        const Vector x = reference.cross(normal).normalized();
        planes.push_back(Plane::from_frame(
            Point::centroid(corners),
            x,
            normal.cross(x),
            normal
        ));
    }

    return planes;
}

inline std::vector<std::array<Plane, 4>> Chevron::compute_edge_planes() const {

    std::vector<std::array<Plane, 4>> planes(_faces.vertices.size());

    for (size_t face = 0; face < _faces.vertices.size(); face++) {
        const std::vector<size_t>& vertices = _faces.vertices[face];

        for (size_t j = 0; j < 4; j++) {
            const Point start = _mesh.vertex_point(vertices[j]).value();
            const Point end = _mesh.vertex_point(vertices[(j + 1) % 4]).value();
            Vector mean(0.0, 0.0, 0.0);

            for (const int other : edge_faces(vertices[j], vertices[(j + 1) % 4]))
                mean = mean + face_normal(other);

            // x along the edge and y along the mean normal, each unit, not made square to each other
            const Vector x = (start - end).normalized();
            const Vector y = mean.normalized();
            planes[face][j] = Plane::from_frame(
                Point::mid_point(start, end),
                x,
                y,
                x.cross(y).normalized()
            );
        }
    }

    return planes;
}

inline std::vector<std::array<Plane, 4>> Chevron::compute_turned_edge_planes(std::vector<std::array<Plane, 4>> planes) const {

    const double angle = edge_rotation * Tolerance::PI / 180.0;

    for (size_t face = 0; face < _faces.vertices.size(); face++) {
        const std::vector<size_t>& vertices = _faces.vertices[face];
        const std::array<int, 2>& chevron = _strips.chevron_edges[face];

        for (int j = 0; j < 4; j++) {
            const size_t a = vertices[j];
            const size_t b = vertices[(j + 1) % 4];
            const std::vector<int>& faces = edge_faces(a, b);

            // a boundary edge plane stands upright along a world axis
            if (faces.size() != 2) {
                if (ortho_edges[j] != 0)
                    planes[face][j] = snapped_to_axis(planes[face][j], ortho_edges[j]);

                continue;
            }

            if (j != chevron[0] && j != chevron[1])
                continue;

            // an odd chevron edge moves along its normal, an even one turns about its y axis, each strip the other way
            Plane& plane = planes[face][j];

            if (j % 2 == 1) {
                plane = plane.translate_by_normal(plate_thickness * edge_offset);
            } else {
                const double turn = _strips.flips[face] ? angle : -angle;
                const Vector x = (plane.x_axis() * std::cos(turn) - plane.z_axis() * std::sin(turn)).normalized();
                const Vector z = (plane.x_axis() * std::sin(turn) + plane.z_axis() * std::cos(turn)).normalized();
                plane = Plane::from_frame(
                    plane.origin(),
                    x,
                    z.cross(x),
                    z
                );
            }

            // the neighbour across the edge gets the same plane seen from its side
            const int neighbour = faces[0] != static_cast<int>(face) ? faces[0] : faces[1];
            const std::vector<size_t>& other = _faces.vertices[neighbour];

            for (size_t k = 0; k < 4; k++) {
                const size_t c = other[k];
                const size_t d = other[(k + 1) % 4];

                if (std::min(c, d) != std::min(a, b) || std::max(c, d) != std::max(a, b))
                    continue;

                planes[neighbour][k] = Plane::from_frame(
                    plane.origin(),
                    -plane.x_axis(),
                    plane.y_axis(),
                    -plane.z_axis()
                );
                break;
            }
        }
    }

    return planes;
}

inline std::vector<std::array<std::optional<Plane>, 4>> Chevron::compute_bisectors() const {

    std::vector<std::array<std::optional<Plane>, 4>> bisectors(_edge_planes.size());

    for (size_t face = 0; face < _edge_planes.size(); face++)
        for (size_t j = 0; j < 4; j++)
            bisectors[face][j] = dihedral_plane(_edge_planes[face][(j + 1) % 4], _edge_planes[face][j]);

    return bisectors;
}

inline std::optional<Plane> Chevron::dihedral_plane(const Plane& p0, const Plane& p1) {

    const std::optional<Line> seam = Intersection::plane_plane(p1, p0);

    if (!seam || p0.z_axis().dot(p1.z_axis()) > 0.99 || (p0.origin() - p1.origin()).magnitude() < 0.001)
        return std::nullopt;

    // the two normals' closest point is the centre the bisector halves the angle around
    const Line axis0 = Line::from_points(p0.origin(), p0.origin() + p0.z_axis());
    const Line axis1 = Line::from_points(p1.origin(), p1.origin() + p1.z_axis());
    double t0 = 0.0;
    double t1 = 0.0;
    Point centre = p0.origin();

    const bool crossed = Intersection::line_line_parameters(
        axis0,
        axis1,
        t0,
        t1,
        0.0,
        false,
        false
    );

    if (crossed)
        centre = axis0.point_at(t0);

    const Vector half = (p0.origin() - centre).normalized() + (p1.origin() - centre).normalized();

    if (half.magnitude() < 1e-12)
        return std::nullopt;

    const Vector y = half.normalized();
    const Vector x = seam->to_vector().normalized();
    return Plane::from_frame(
        seam->start(),
        x,
        y,
        x.cross(y).normalized()
    );
}

inline Plane Chevron::snapped_to_axis(const Plane& plane, int axis) {

    const Vector normal = plane.z_axis();
    int index = 0;

    if (axis >= 2 && axis <= 4) {
        index = axis - 2;
    } else {
        for (int i = 1; i < 3; i++)
            if (std::abs(normal[i]) > std::abs(normal[index]))
                index = i;
    }

    Vector z(0.0, 0.0, 0.0);
    z[index] = normal[index] >= 0.0 ? 1.0 : -1.0;
    const Vector reference = index != 0 ? Vector(1.0, 0.0, 0.0) : Vector(0.0, 1.0, 0.0);
    const Vector x = reference.cross(z).normalized();
    return Plane::from_frame(
        plane.origin(),
        x,
        z.cross(x),
        z
    );
}

// ═══════════════════════════════════════════════════════════════════════════
// Plates
// ═══════════════════════════════════════════════════════════════════════════

inline Polyline Chevron::polygon_from_planes(const Plane& base, const std::vector<Plane>& sides) {

    std::vector<Point> points;

    for (size_t i = 0; i < sides.size(); i++) {
        Point corner = base.origin();
        Intersection::plane_plane_plane(base, sides[i], sides[(i + 1) % sides.size()], corner);
        points.push_back(corner);
    }

    points.push_back(points.front());
    return Polyline(points);
}

inline std::array<int, 2> Chevron::sorted_chevron_edges(int face) const {

    std::array<int, 2> edges = _strips.chevron_edges[face];

    if (edges[0] > edges[1])
        std::swap(edges[0], edges[1]);

    if (edges[0] == 0 && edges[1] == 3)
        std::swap(edges[0], edges[1]);

    return edges;
}

inline std::vector<std::shared_ptr<Plate>> Chevron::compute_plates() const {

    const double half = box_height * 0.5;
    const double t = plate_thickness;
    std::vector<Polyline> outlines;

    for (const int face : _strips.order) {
        const Plane& face_plane = _face_planes[face];
        const std::array<Plane, 4>& edges = _edge_planes[face];
        const std::array<std::optional<Plane>, 4>& bisectors = _bisectors[face];
        const std::array<int, 2>& chevron = _strips.chevron_edges[face];

        // top and bottom: the face plane moved into the box, cut by the edge planes, the chevron ones a plate further out
        std::vector<Plane> sides(edges.begin(), edges.end());

        for (const int j : chevron)
            sides[j] = sides[j].translate_by_normal(t);

        const std::array<double, 4> levels = {
            half - top_plate_inlet - t * 0.5,
            half - top_plate_inlet + t * 0.5,
            -half + top_plate_inlet - t * 0.5,
            -half + top_plate_inlet + t * 0.5,
        };

        for (const double level : levels)
            outlines.push_back(polygon_from_planes(face_plane.translate_by_normal(level), sides));

        // sides: each chevron edge plane cut by the box faces, its neighbour edge and the corner bisector
        const std::array<int, 2> sorted = sorted_chevron_edges(face);

        for (size_t k = 0; k < 2; k++) {
            const int edge = sorted[k];
            const int previous = (edge + 3) % 4;
            const int next = (edge + 1) % 4;
            const Plane top = face_plane.translate_by_normal(half);
            const Plane bottom = face_plane.translate_by_normal(-half);
            const Plane before = k == 0 ? edges[previous] : bisectors[previous].value_or(edges[previous]);
            const Plane after = k == 0 ? bisectors[edge].value_or(edges[next]) : edges[next];
            const std::vector<Plane> box = {top, before, bottom, after};
            outlines.push_back(polygon_from_planes(edges[edge], box));
            outlines.push_back(polygon_from_planes(edges[edge].translate_by_normal(t), box));
        }
    }

    std::vector<std::shared_ptr<Plate>> plates;

    for (size_t i = 0; i + 1 < outlines.size(); i += 2) {
        const std::string name = fmt::format("plate_{}", i / 2);
        plates.push_back(std::make_shared<Plate>(outlines[i], outlines[i + 1], name));
    }

    return plates;
}

// ═══════════════════════════════════════════════════════════════════════════
// Joinery
// ═══════════════════════════════════════════════════════════════════════════

inline Vector Chevron::bisector_direction(const Plane& face_plane, const std::optional<Plane>& bisector) {

    if (!bisector)
        return Vector(0.0, 0.0, 0.0);

    const std::optional<Line> seam = Intersection::plane_plane(*bisector, face_plane);
    return seam ? seam->to_vector().normalized() : Vector(0.0, 0.0, 0.0);
}

inline std::vector<Line> Chevron::compute_joinery() {

    const size_t count = _strips.order.size();
    std::vector<int> place(_faces.vertices.size(), -1);
    std::vector<Line> lines;
    _insertion_vectors.assign(count * 4, {});
    _joints_per_face.assign(count * 4, {0, 0, 0, 0, 0, 0});

    for (size_t c = 0; c < count; c++)
        place[_strips.order[c]] = static_cast<int>(c);

    for (size_t c = 0; c < count; c++) {
        const int face = _strips.order[c];
        const int pair = static_cast<int>(c) * 4;
        const std::array<int, 2> sorted = sorted_chevron_edges(face);
        const Vector first = bisector_direction(_face_planes[face], _bisectors[face][sorted[0]]);
        const Vector second = bisector_direction(_face_planes[face], _bisectors[face][(sorted[1] + 1) % 4]);

        // the top and bottom plates go in along the corner bisectors, the sides one way and the two before them the other
        std::array<double, 18> vectors = {};

        for (int side = 2; side < 6; side++)
            for (int k = 0; k < 3; k++)
                vectors[side * 3 + k] = second[k];

        for (const int offset : {2, 3}) {
            const int side = 2 + (sorted[1] + offset) % 4;

            for (int k = 0; k < 3; k++)
                vectors[side * 3 + k] = first[k];
        }

        _insertion_vectors[pair + 0] = vectors;
        _insertion_vectors[pair + 1] = vectors;

        // the top and bottom plates take mortises, the side plates tenons
        _joints_per_face[pair + 0] = {0, 0, 20, 20, 20, 20};
        _joints_per_face[pair + 1] = {0, 0, 20, 20, 20, 20};
        _joints_per_face[pair + 2] = {0, 0, 10, 10, 10, 10};
        _joints_per_face[pair + 3] = {0, 0, 10, 10, 10, 10};

        for (const int role_a : {0, 1})
            for (const int role_b : {2, 3})
                _adjacency.emplace_back(pair + role_a, pair + role_b);

        _adjacency.emplace_back(pair + 2, pair + 3);

        // a side plate also holds the top and bottom plates of the face across its chevron edge
        for (int k = 0; k < 2; k++) {
            const std::vector<size_t>& vertices = _faces.vertices[face];
            const std::vector<int>& faces = edge_faces(vertices[sorted[k]], vertices[(sorted[k] + 1) % 4]);

            if (faces.size() != 2)
                continue;

            const int neighbour = faces[0] != face ? faces[0] : faces[1];
            const int across = place[neighbour] * 4;
            _adjacency.emplace_back(across + 0, pair + 2 + k);
            _adjacency.emplace_back(across + 1, pair + 2 + k);
            _three_valence.push_back({pair + 0, pair + 2 + k, across + 0, pair + 2 + k});
            _three_valence.push_back({pair + 1, pair + 2 + k, across + 1, pair + 2 + k});
        }

        const std::shared_ptr<Plate> top = get_element_by_name<Plate>(fmt::format("plate_{}", pair));
        const Point centre = top->polylines[1].center();
        lines.push_back(Line::from_points(centre, centre + second * 300.0));
    }

    return lines;
}
