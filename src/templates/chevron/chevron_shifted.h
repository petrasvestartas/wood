#pragma once
#include "chevron.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_chevron {

/// A side of a cell's outline: its plane and the cell edge it stands on, -1 for a neighbour's cross plane.
struct ShiftedSide {
    Plane plane;
    int edge;
};

/// The cells of the shifted pattern: hexagons over three rows in staggered columns, a half-cell quad at one end of every column.
struct ShiftedCells {
    ChevronFaces faces; // Per cell its vertex keys into the point grid and the cells on every edge.
    std::vector<int>
        masks; // Per cell its kind: 20 and 2 even-column hexagons, -2 odd-column hexagons, 3 and -3 last quads, -1 first quads.
    std::vector<int> columns; // Per cell its column.
};

/// The plates of the shifted pattern, per cell its top and bottom plate and then its walls.
struct ShiftedPlates {
    std::vector<std::shared_ptr<Plate>> plates; // In the order they are added, plate_<i>.
    std::vector<std::vector<int>>
        edges; // Per plate the cell edge under every side of its first outline, -1 for none.
    std::vector<std::array<int, 2>> boxes; // Per cell its top and bottom plate.
    std::vector<std::array<int, 2>> walls; // Per wall its plate and its cell.
    std::vector<std::pair<size_t, size_t>> wall_edges; // Per wall the mesh edge it stands on.
};

}

/// The shifted chevron shell: hexagonal cells in staggered columns on a surface, a box of plates per cell, the 2018 Annen pattern.
class ChevronShifted : public WoodSession {
public:
    const NurbsSurface surface; // The surface the cells are laid on, its u running along the shell.
    const int iso_count; // Iso curves across the surface, the rows of the point grid.
    const int point_count; // Points on every iso curve, one more than the columns.
    const double
        shift; // Share of the row spacing the interior points move towards the next row in even columns and the previous one in odd columns.
    const double
        edge_rotation; // Degrees the merged long-side planes of the even columns turn about the surface normal, the sign alternating per column pair.
    const double plate_offset; // mm from the surface to the middle of the top and bottom plates.
    const double plate_thickness; // mm, every plate.
    const double wall_extend; // mm the walls run past the outer faces of the top and bottom plates.
    const bool
        average_planes; // Merge the two planes of every even cell's long sides into one turned plane.

    /// The shifted chevron shell on surface as the session named name.
    explicit ChevronShifted(
        const NurbsSurface& surface = Chevron::default_surface(),
        int iso_count = 54,
        int point_count = 9,
        double shift = 0.4,
        double edge_rotation = 0.05 * 180.0 / Tolerance::PI,
        double plate_offset = 300.0,
        double plate_thickness = 40.0,
        double wall_extend = 80.0,
        bool average_planes = true,
        const std::string& name = "chevron_shifted"
    );

    /// Not copied, as a session is not.
    ChevronShifted(const ChevronShifted&) = delete;

    /// Not assigned, as it is not copied.
    ChevronShifted& operator=(const ChevronShifted&) = delete;

    /// The cell mesh, a hexagon or quad face per cell.
    const Mesh& mesh() const;

    /// The cells: vertex keys, kinds and columns.
    const wood_chevron::ShiftedCells& cells() const;

    /// Per plate one insertion vector per face: the cell's direction on the top and bottom plates' tenon sides, zero elsewhere.
    const std::vector<std::vector<Vector>>& insertion_vectors() const;

    /// Per plate one joint type per face: 20, a tenon, on the top and bottom plates' sides that reach a wall, 0 elsewhere.
    const std::vector<std::vector<int>>& joints_per_face() const;

    /// Rows {t0, w, t1, w}: the top (or bottom) plates t0 and t1 of the two cells on a wall w both join it, so their tenons are aligned.
    const std::vector<std::array<int, 4>>& three_valence() const;

    /// The plate pairs that share a joint: every wall with the top and bottom plates of the cells on its edge.
    const std::vector<std::pair<int, int>>& adjacency() const;

private:
    using EdgeKey = std::pair<size_t, size_t>;

    NurbsSurface _across;
    std::vector<std::vector<Point>> _points;
    wood_chevron::ShiftedCells _cells;
    Mesh _mesh;
    std::map<EdgeKey, Plane> _edge_planes;
    std::vector<Plane> _face_planes;
    std::vector<std::vector<wood_chevron::ShiftedSide>> _sides;
    std::vector<std::vector<Vector>> _insertion_vectors;
    std::vector<std::vector<int>> _joints_per_face;
    std::vector<std::array<int, 4>> _three_valence;
    std::vector<std::pair<int, int>> _adjacency;

    /// The point grid: rows at equal arc length along the middle iso curve, the ends and then half a step in, each divided at equal arc length across, the interior points shifted.
    std::vector<std::vector<Point>> compute_points() const;

    /// The cells column by column, the odd columns from the far end, and the cells on every edge.
    wood_chevron::ShiftedCells compute_cells() const;

    /// The cells as one mesh.
    Mesh compute_mesh() const;

    /// A plane through every edge along the surface normal; on a naked edge along the curvature of the nearest long side curve.
    std::map<EdgeKey, Plane> compute_edge_planes() const;

    /// The edge planes with both halves of every even cell's long sides merged into one plane through their middle, turned by edge_rotation; not turned on a naked side.
    std::map<EdgeKey, Plane> compute_merged_edge_planes(std::map<EdgeKey, Plane> planes) const;

    /// A plane at the surface point nearest every cell's centre, x along the rows, z into the surface.
    std::vector<Plane> compute_face_planes() const;

    /// Per cell its edge planes in order, with the neighbour's cross plane inserted after edges 1 and 4 of a hexagon.
    std::vector<std::vector<wood_chevron::ShiftedSide>> compute_sides() const;

    /// Per cell a top and a bottom plate, stepped in by a wall where its own walls stand, then its walls between the outer box faces.
    wood_chevron::ShiftedPlates compute_plates() const;

    /// The insertion vectors, joint types, adjacency and three-valence groups of the plates, and an insertion line per cell.
    std::vector<Line> compute_joinery(const wood_chevron::ShiftedPlates& plates);

    /// The grid point of a vertex key.
    const Point& point(size_t key) const;

    /// The sorted vertex pair of a cell's edge.
    EdgeKey edge_key(int cell, int edge) const;

    /// Per side of a cell's outline whether the top and bottom plates step in by a wall (-1) or stay (0).
    static std::vector<int> step_flags(int mask);

    /// The walls of a cell as (side, outward) pairs in the order they are made.
    static std::vector<std::pair<int, bool>> wall_sides(int mask);

    /// The two cell edges whose outline segments give the insertion direction.
    static std::array<int, 2> insertion_edges(int mask);

    /// The cell edges whose walls the cell slides into: its direction goes to them.
    static std::vector<int> owned_edges(int mask);

    /// The plane turned about its y axis.
    static Plane turned_about_y(const Plane& plane, double angle);

    /// The plane moved by distance along its normal to the side nearer centre (inward) or farther from it.
    static Plane stepped(const Plane& plane, double distance, const Point& centre, bool inward);

    /// Both closed outlines without their coincident consecutive corners; returns the edge under every remaining side.
    static std::vector<int>
    compact(Polyline& first, Polyline& second, const std::vector<int>& edges);
};

inline ChevronShifted::ChevronShifted(
    const NurbsSurface& surface,
    int iso_count,
    int point_count,
    double shift,
    double edge_rotation,
    double plate_offset,
    double plate_thickness,
    double wall_extend,
    bool average_planes,
    const std::string& name
)
    : WoodSession(name), surface(surface), iso_count(std::max(iso_count, 4)),
      point_count(std::max(point_count, 2)), shift(shift), edge_rotation(edge_rotation),
      plate_offset(plate_offset), plate_thickness(std::max(plate_thickness, 1.0)),
      wall_extend(wall_extend), average_planes(average_planes) {

    // surface: the NURBS surface the cells are laid on, turned so its u runs across the shell as the 2018 definition read it
    add_nurbssurface(surface, add_group("surface"));
    _across = surface;
    _across.transpose();

    // points: iso curves across the surface divided at equal arc length, the interior points shifted along the rows
    _points = compute_points();

    // cells: hexagons over three rows in staggered columns, a half-cell quad at one end of every column, as one mesh
    _cells = compute_cells();
    _mesh = compute_mesh();
    add_mesh(_mesh, add_group("mesh"));

    // edge_planes: through every edge along the surface normal, the even cells' long sides merged and turned
    const std::map<EdgeKey, Plane> edge_planes = compute_edge_planes();
    _edge_planes = compute_merged_edge_planes(edge_planes);
    const std::shared_ptr<TreeNode> planes = add_group("edge_planes");

    for (const std::pair<const EdgeKey, Plane>& plane : _edge_planes)
        add_plane(plane.second, planes);

    // sides: a plane at every cell's centre and its loop of side planes with the neighbours' cross planes inserted
    _face_planes = compute_face_planes();
    _sides = compute_sides();

    // plates: per cell a top and a bottom plate stepped in where its own walls stand, then its walls, plate_<i>
    const wood_chevron::ShiftedPlates plates = compute_plates();
    const std::shared_ptr<TreeNode> plates_group = add_group("plates");

    for (const std::shared_ptr<Plate>& plate : plates.plates)
        add(plate, plates_group);

    // joinery: insertion vectors, joint types, adjacency and three-valence groups for the solver, an insertion line per cell
    const std::shared_ptr<TreeNode> insertion = add_group("insertion");

    for (Line line : compute_joinery(plates)) {
        line.width = 2.5;
        line.linecolor = Color(206, 16, 66, 255);
        line.arrowhead = Arrowhead::END;
        add_line(line, insertion);
    }
    set_node_color(insertion, Color(206, 16, 66, 255), true);

    // joints: the wood solver reads the tables and cuts the tenons and mortises into the plates; a contact is at least a wall thickness square, so the slivers where a wall's slanted end crosses the middle of a merged side are none
    settings.clipper_area = plate_thickness * plate_thickness;
    WoodSession::adjacency = _adjacency;
    WoodSession::three_valence = {{0}};

    for (const std::array<int, 4>& group : _three_valence)
        WoodSession::three_valence.push_back(std::vector<int>(group.begin(), group.end()));

    for (size_t i = 0; i < plates.plates.size(); i++) {
        plates.plates[i]->insertion_vectors() = _insertion_vectors[i];
        plates.plates[i]->feature_types = _joints_per_face[i];
    }

    compute_features();
}

inline const Mesh& ChevronShifted::mesh() const {
    return _mesh;
}

inline const wood_chevron::ShiftedCells& ChevronShifted::cells() const {
    return _cells;
}

inline const std::vector<std::vector<Vector>>& ChevronShifted::insertion_vectors() const {
    return _insertion_vectors;
}

inline const std::vector<std::vector<int>>& ChevronShifted::joints_per_face() const {
    return _joints_per_face;
}

inline const std::vector<std::array<int, 4>>& ChevronShifted::three_valence() const {
    return _three_valence;
}

inline const std::vector<std::pair<int, int>>& ChevronShifted::adjacency() const {
    return _adjacency;
}

// ═══════════════════════════════════════════════════════════════════════════
// Points and cells
// ═══════════════════════════════════════════════════════════════════════════
inline std::vector<std::vector<Point>> ChevronShifted::compute_points() const {

    const std::pair<double, double> domain_u = _across.domain(0);
    const int steps = iso_count - 2;
    const NurbsCurve middle = _across.iso_curve(1, 0.5 * (domain_u.first + domain_u.second));
    const std::vector<double> stations = middle.divide_by_count(2 * steps + 1, true).second;
    std::vector<double> rows = {stations.front()};

    for (int i = 1; i <= steps; i++)
        rows.push_back(stations[2 * i - 1]);

    rows.push_back(stations.back());

    std::vector<std::vector<std::pair<double, double>>> parameters;
    std::vector<std::vector<Point>> points;

    for (const double v : rows) {
        const std::vector<double> columns =
            _across.iso_curve(0, v).divide_by_count(point_count, true).second;
        std::vector<std::pair<double, double>> row;
        std::vector<Point> row_points;

        for (const double u : columns) {
            row.emplace_back(u, v);
            row_points.push_back(_across.point_at(u, v));
        }

        parameters.push_back(row);
        points.push_back(row_points);
    }

    for (size_t i = 1; i + 1 < rows.size(); i++)
        for (int j = 0; j < point_count; j++) {
            const std::pair<double, double>& here = parameters[i][j];
            const std::pair<double, double>& there = parameters[j % 2 == 0 ? i + 1 : i - 1][j];
            points[i][j] = _across.point_at(
                here.first + (there.first - here.first) * shift,
                here.second + (there.second - here.second) * shift
            );
        }

    return points;
}

inline wood_chevron::ShiftedCells ChevronShifted::compute_cells() const {

    const int columns = point_count - 1;
    std::vector<std::vector<size_t>> vertices;
    std::vector<int> masks;
    std::vector<int> cell_columns;
    std::vector<std::vector<int>> sequence(columns);

    const auto key = [&](int i, int j) {
        return static_cast<size_t>(i * point_count + j);
    };
    const auto add_cell = [&](std::vector<size_t> keys, int mask, int j) {
        sequence[j].push_back(static_cast<int>(vertices.size()));
        vertices.push_back(std::move(keys));
        masks.push_back(mask);
        cell_columns.push_back(j);
    };
    const auto hexagon = [&](int i, int j, int mask) {
        add_cell(
            {key(i, j),
             key(i, j + 1),
             key(i + 1, j + 1),
             key(i + 2, j + 1),
             key(i + 2, j),
             key(i + 1, j)},
            mask,
            j
        );
    };
    const auto quad = [&](int i, int j, int mask) {
        add_cell({key(i, j), key(i, j + 1), key(i + 1, j + 1), key(i + 1, j)}, mask, j);
    };

    for (int i = 0; i < iso_count - 2; i++)
        for (int j = 0; j < columns; j++) {
            if (i % 2 == 0 && j % 2 == 0) {
                hexagon(i, j, j % 4 == 2 ? 2 : 20);

                if (i == iso_count - 4)
                    quad(i + 2, j, 3);
            }

            if (i % 2 == 1 && j % 2 == 1) {
                if (i == 1)
                    quad(0, j, -1);

                hexagon(i, j, -2);

                if (i == iso_count - 4)
                    quad(i + 2, j, -3);
            }
        }

    wood_chevron::ShiftedCells cells;

    for (int j = 0; j < columns; j++) {
        std::vector<int> order = sequence[j];

        if (j % 2 == 1)
            std::reverse(order.begin(), order.end());

        for (const int cell : order) {
            cells.faces.vertices.push_back(vertices[cell]);
            cells.masks.push_back(masks[cell]);
            cells.columns.push_back(cell_columns[cell]);
        }
    }

    for (size_t cell = 0; cell < cells.faces.vertices.size(); cell++) {
        const std::vector<size_t>& keys = cells.faces.vertices[cell];

        for (size_t k = 0; k < keys.size(); k++) {
            const size_t a = keys[k];
            const size_t b = keys[(k + 1) % keys.size()];
            cells.faces.edge_faces[{std::min(a, b), std::max(a, b)}].push_back(
                static_cast<int>(cell)
            );
        }
    }

    return cells;
}

inline Mesh ChevronShifted::compute_mesh() const {

    std::vector<std::vector<Point>> polygons;

    for (const std::vector<size_t>& keys : _cells.faces.vertices) {
        std::vector<Point> polygon;

        for (const size_t key : keys)
            polygon.push_back(point(key));

        polygons.push_back(polygon);
    }

    return Mesh::from_polylines(polygons);
}

inline const Point& ChevronShifted::point(size_t key) const {
    return _points[key / point_count][key % point_count];
}

inline ChevronShifted::EdgeKey ChevronShifted::edge_key(int cell, int edge) const {

    const std::vector<size_t>& keys = _cells.faces.vertices[cell];
    const size_t a = keys[edge];
    const size_t b = keys[(edge + 1) % keys.size()];
    return {std::min(a, b), std::max(a, b)};
}

// ═══════════════════════════════════════════════════════════════════════════
// Planes
// ═══════════════════════════════════════════════════════════════════════════
inline std::map<ChevronShifted::EdgeKey, Plane> ChevronShifted::compute_edge_planes() const {

    const std::pair<double, double> domain_u = _across.domain(0);
    const std::array<NurbsCurve, 2> sides = {
        _across.iso_curve(1, domain_u.first), _across.iso_curve(1, domain_u.second)
    };
    std::map<EdgeKey, Plane> planes;

    for (const std::pair<const EdgeKey, std::vector<int>>& edge : _cells.faces.edge_faces) {
        const Point& start = point(edge.first.first);
        const Point& end = point(edge.first.second);
        const Point middle = Point::mid_point(start, end);
        const std::pair<double, double> uv = _across.closest_parameters(middle);
        Vector y = _across.normal_at(uv.first, uv.second);

        if (edge.second.size() == 1) {
            double nearest = std::numeric_limits<double>::infinity();
            Vector curvature(0.0, 0.0, 0.0);

            for (const NurbsCurve& side : sides) {
                const double t = side.closest_parameter(middle);
                const double distance = Point::distance(side.point_at(t), middle);

                if (distance < nearest) {
                    nearest = distance;
                    curvature = side.evaluate(t, 2)[2];
                }
            }

            if (curvature.magnitude() > Tolerance::ZERO_TOLERANCE)
                y = -curvature;
        }

        planes.emplace(edge.first, Plane(middle, end - start, y));
    }

    return planes;
}

inline std::map<ChevronShifted::EdgeKey, Plane>
ChevronShifted::compute_merged_edge_planes(std::map<EdgeKey, Plane> planes) const {

    if (!average_planes)
        return planes;

    const double angle = edge_rotation * Tolerance::PI / 180.0;

    for (size_t cell = 0; cell < _cells.masks.size(); cell++) {
        const int mask = _cells.masks[cell];

        if (mask != 2 && mask != 20)
            continue;

        const double turn = mask == 2 ? -angle : angle;

        for (const std::array<int, 2> pair : {std::array<int, 2>{1, 2}, std::array<int, 2>{4, 5}}) {
            const EdgeKey first = edge_key(static_cast<int>(cell), pair[0]);
            const EdgeKey second = edge_key(static_cast<int>(cell), pair[1]);
            const Plane& a = planes.at(first);
            const Plane& b = planes.at(second);
            const Point middle = Point::mid_point(a.origin(), b.origin());
            Plane merged = Plane(middle, a.x_axis(), a.y_axis());

            if (_cells.faces.edge_faces.at(first).size() != 1) {
                const std::pair<double, double> uv = _across.closest_parameters(middle);
                merged = turned_about_y(
                    Plane(middle, a.x_axis() + b.x_axis(), _across.normal_at(uv.first, uv.second)),
                    turn
                );
            }

            planes.at(first) = merged;
            planes.at(second) = merged;
        }
    }

    return planes;
}

inline Plane ChevronShifted::turned_about_y(const Plane& plane, double angle) {

    const Vector x =
        (plane.x_axis() * std::cos(angle) - plane.z_axis() * std::sin(angle)).normalized();
    const Vector z =
        (plane.x_axis() * std::sin(angle) + plane.z_axis() * std::cos(angle)).normalized();
    return Plane::from_frame(plane.origin(), x, plane.y_axis(), z);
}

inline std::vector<Plane> ChevronShifted::compute_face_planes() const {

    std::vector<Plane> planes;

    for (const std::vector<size_t>& keys : _cells.faces.vertices) {
        Vector weighted(0.0, 0.0, 0.0);
        double perimeter = 0.0;

        for (size_t k = 0; k < keys.size(); k++) {
            const Point& a = point(keys[k]);
            const Point& b = point(keys[(k + 1) % keys.size()]);
            const double length = Point::distance(a, b);
            weighted =
                weighted + (Vector(a[0], a[1], a[2]) + Vector(b[0], b[1], b[2])) * (0.5 * length);
            perimeter += length;
        }

        const Vector centre = weighted * (1.0 / perimeter);
        const std::pair<double, double> uv =
            _across.closest_parameters(Point(centre[0], centre[1], centre[2]));
        const std::vector<Vector> derivatives = _across.evaluate(uv.first, uv.second, 1);
        const Vector normal = _across.normal_at(uv.first, uv.second);
        const Vector x = derivatives[1].normalized();
        planes.push_back(
            Plane::from_frame(
                _across.point_at(uv.first, uv.second), x, x.cross(normal).normalized(), -normal
            )
        );
    }

    return planes;
}

inline std::vector<std::vector<wood_chevron::ShiftedSide>> ChevronShifted::compute_sides() const {

    std::vector<std::vector<wood_chevron::ShiftedSide>> sides;

    for (size_t cell = 0; cell < _cells.masks.size(); cell++) {
        const std::vector<size_t>& keys = _cells.faces.vertices[static_cast<int>(cell)];
        const int count = static_cast<int>(keys.size());
        std::vector<wood_chevron::ShiftedSide> loop;

        for (int j = 0; j < count; j++) {
            const EdgeKey key = edge_key(static_cast<int>(cell), j);
            loop.push_back({_edge_planes.at(key), j});

            if (count != 6 || (j != 1 && j != 4))
                continue;

            const std::vector<int>& on_edge = _cells.faces.edge_faces.at(key);

            if (on_edge.size() != 1) {
                // the neighbour's edge before the shared one: its cross plane through this cell's middle vertex
                const int neighbour =
                    on_edge[0] != static_cast<int>(cell) ? on_edge[0] : on_edge[1];
                const int neighbour_count =
                    static_cast<int>(_cells.faces.vertices[neighbour].size());
                int local = 0;

                while (edge_key(neighbour, local) != key)
                    local++;

                const int previous = (local + neighbour_count - 1) % neighbour_count;
                loop.push_back({_edge_planes.at(edge_key(neighbour, previous)), -1});
            } else {
                const Plane& plane = _edge_planes.at(key);
                const Plane& next =
                    _edge_planes.at(edge_key(static_cast<int>(cell), (j + 1) % count));
                loop.push_back(
                    {Plane(
                         Point::mid_point(plane.origin(), next.origin()),
                         plane.z_axis(),
                         plane.y_axis()
                     ),
                     -1}
                );
            }
        }

        sides.push_back(loop);
    }

    return sides;
}

// ═══════════════════════════════════════════════════════════════════════════
// Plates
// ═══════════════════════════════════════════════════════════════════════════
inline std::vector<int> ChevronShifted::step_flags(int mask) {

    if (mask == 2 || mask == 20 || mask == -2)
        return {-1, 0, 0, 0, 0, -1, 0, -1};

    if (mask == -1)
        return {0, 0, 0, 0};

    return {-1, 0, 0, -1};
}

inline std::vector<std::pair<int, bool>> ChevronShifted::wall_sides(int mask) {

    if (mask == 2 || mask == 20)
        return {{0, false}, {1, true}, {3, true}, {5, false}, {7, false}};

    if (mask == 3)
        return {{2, true}, {0, false}, {1, true}, {3, false}};

    if (mask == -1)
        return {{2, true}};

    if (mask == -2)
        return {{4, true}};

    return {};
}

inline Plane
ChevronShifted::stepped(const Plane& plane, double distance, const Point& centre, bool inward) {

    const Plane in = plane.translate_by_normal(-distance);
    const Plane out = plane.translate_by_normal(distance);
    const double to_in = Point::distance(centre, in.origin());
    const double to_out = Point::distance(centre, out.origin());

    if (inward)
        return to_in > to_out ? out : in;

    return to_in < to_out ? out : in;
}

inline std::vector<int>
ChevronShifted::compact(Polyline& first, Polyline& second, const std::vector<int>& edges) {

    const std::vector<Point> corners = first.get_points();
    const std::vector<Point> others = second.get_points();
    const size_t count = edges.size();
    std::vector<size_t> kept;

    for (size_t k = 0; k < count; k++)
        if (Point::distance(corners[k], corners[(k + count - 1) % count]) > 0.01)
            kept.push_back(k);

    std::vector<Point> points;
    std::vector<Point> other_points;
    std::vector<int> sides;

    for (size_t s = 0; s < kept.size(); s++) {
        points.push_back(corners[kept[s]]);
        other_points.push_back(others[kept[s]]);
        sides.push_back(edges[kept[(s + 1) % kept.size()]]);
    }

    points.push_back(points.front());
    other_points.push_back(other_points.front());
    first = Polyline(points);
    second = Polyline(other_points);
    return sides;
}

inline wood_chevron::ShiftedPlates ChevronShifted::compute_plates() const {

    wood_chevron::ShiftedPlates plates;
    const double half = plate_thickness * 0.5;

    for (size_t cell = 0; cell < _cells.masks.size(); cell++) {
        const int mask = _cells.masks[cell];
        const Plane& face = _face_planes[cell];
        const std::vector<wood_chevron::ShiftedSide>& sides = _sides[cell];
        const std::vector<int> flags = step_flags(mask);
        std::array<int, 2> box = {0, 0};

        // top and bottom plate: the face plane moved by the offset, its outline stepped in by a wall on the flagged sides
        for (const double sign : {1.0, -1.0}) {
            const Plane near = face.translate_by_normal(sign * plate_offset - half);
            const Plane far = face.translate_by_normal(sign * plate_offset + half);
            std::vector<Plane> loop;
            std::vector<int> edges;

            for (size_t k = 0; k < sides.size(); k++) {
                loop.push_back(
                    flags[k] == 0
                        ? sides[k].plane
                        : stepped(sides[k].plane, plate_thickness, near.origin(), flags[k] < 0)
                );
                edges.push_back(sides[k].edge);
            }

            Polyline first = Polyline::from_planes(loop, near);
            Polyline second = Polyline::from_planes(loop, far);
            const std::vector<int> side_edges = compact(first, second, edges);
            box[sign > 0.0 ? 0 : 1] = static_cast<int>(plates.plates.size());
            plates.plates.push_back(
                std::make_shared<Plate>(
                    first, second, fmt::format("plate_{}", plates.plates.size())
                )
            );
            plates.edges.push_back(side_edges);
        }

        plates.boxes.push_back(box);

        // walls: each between the outer box faces and the two neighbouring sides, a wall thick inward or outward from its side plane
        const Plane top = face.translate_by_normal(plate_offset + half + wall_extend);
        const Plane bottom = face.translate_by_normal(-plate_offset - half - wall_extend);

        for (const std::pair<int, bool>& wall : wall_sides(mask)) {
            const size_t k = wall.first;
            const std::vector<Plane> loop = {
                top,
                sides[(k + 1) % sides.size()].plane,
                bottom,
                sides[(k + sides.size() - 1) % sides.size()].plane,
            };
            const Plane moved =
                stepped(sides[k].plane, plate_thickness, top.origin(), !wall.second);
            plates.walls.push_back(
                {static_cast<int>(plates.plates.size()), static_cast<int>(cell)}
            );
            plates.wall_edges.push_back(edge_key(static_cast<int>(cell), sides[k].edge));
            plates.plates.push_back(
                std::make_shared<Plate>(
                    Polyline::from_planes(loop, sides[k].plane),
                    Polyline::from_planes(loop, moved),
                    fmt::format("plate_{}", plates.plates.size())
                )
            );
            plates.edges.push_back({-1, -1, -1, -1});
        }
    }

    return plates;
}

// ═══════════════════════════════════════════════════════════════════════════
// Joinery
// ═══════════════════════════════════════════════════════════════════════════
inline std::array<int, 2> ChevronShifted::insertion_edges(int mask) {

    if (mask == 2 || mask == 20)
        return {5, 0};

    if (mask == -2)
        return {3, 4};

    if (mask == 3)
        return {3, 0};

    return {2, 3};
}

inline std::vector<int> ChevronShifted::owned_edges(int mask) {

    if (mask == 2 || mask == 20)
        return {5, 0, 4};

    if (mask == -2)
        return {3, 4, 5};

    if (mask == 3)
        return {3, 0};

    return {2, 3};
}

inline std::vector<Line>
ChevronShifted::compute_joinery(const wood_chevron::ShiftedPlates& plates) {

    _insertion_vectors.assign(plates.plates.size(), {});
    _joints_per_face.assign(plates.plates.size(), {});
    std::map<EdgeKey, Vector> directions;
    std::vector<Line> lines;

    // per cell its direction, the bisector of two of its own sides on its outline, on the edges whose walls it slides into
    for (size_t cell = 0; cell < _cells.masks.size(); cell++) {
        const int mask = _cells.masks[cell];
        const std::vector<wood_chevron::ShiftedSide>& sides = _sides[cell];
        std::vector<Plane> loop;

        for (const wood_chevron::ShiftedSide& side : sides)
            loop.push_back(side.plane);

        const std::vector<Point> corners =
            Polyline::from_planes(loop, _face_planes[cell]).get_points();
        std::array<Vector, 2> segments;

        for (size_t which = 0; which < 2; which++) {
            size_t k = 0;

            while (sides[k].edge != insertion_edges(mask)[which])
                k++;

            segments[which] =
                (corners[k] - corners[(k + loop.size() - 1) % loop.size()]).normalized();
        }

        const Vector direction = segments[0] - segments[1];

        for (const int edge : owned_edges(mask))
            directions[edge_key(static_cast<int>(cell), edge)] = direction;

        const Point centre = _face_planes[cell].translate_by_normal(plate_offset).origin();
        lines.push_back(Line::from_points(centre, centre + direction * 300.0));
    }

    // top and bottom plates: a tenon on every side whose edge has a direction, along it
    for (size_t cell = 0; cell < _cells.masks.size(); cell++)
        for (const int plate : plates.boxes[cell]) {
            const std::vector<int>& edges = plates.edges[plate];
            std::vector<Vector> vectors(2 + edges.size(), Vector(0.0, 0.0, 0.0));
            std::vector<int> types(2 + edges.size(), 0);

            for (size_t k = 0; k < edges.size(); k++) {
                if (edges[k] < 0)
                    continue;

                const std::map<EdgeKey, Vector>::const_iterator found =
                    directions.find(edge_key(static_cast<int>(cell), edges[k]));

                if (found == directions.end())
                    continue;

                vectors[2 + k] = found->second;
                types[2 + k] = 20;
            }

            _insertion_vectors[plate] = vectors;
            _joints_per_face[plate] = types;
        }

    // walls: no joint type of their own, every wall first in its pairs with the top and bottom plates of the cells on its edge, its own cell first, so the solver meets the wall's face before a plate's kink side meets the wall's end
    for (size_t w = 0; w < plates.walls.size(); w++) {
        const int wall = plates.walls[w][0];
        _insertion_vectors[wall] = std::vector<Vector>(6, Vector(0.0, 0.0, 0.0));
        _joints_per_face[wall] = std::vector<int>(6, 0);
        std::vector<int> cells = _cells.faces.edge_faces.at(plates.wall_edges[w]);
        std::rotate(
            cells.begin(), std::find(cells.begin(), cells.end(), plates.walls[w][1]), cells.end()
        );

        for (const int cell : cells) {
            _adjacency.emplace_back(wall, plates.boxes[cell][0]);
            _adjacency.emplace_back(wall, plates.boxes[cell][1]);
        }

        if (cells.size() == 2) {
            _three_valence.push_back(
                {plates.boxes[cells[0]][0], wall, plates.boxes[cells[1]][0], wall}
            );
            _three_valence.push_back(
                {plates.boxes[cells[0]][1], wall, plates.boxes[cells[1]][1], wall}
            );
        }
    }

    return lines;
}
