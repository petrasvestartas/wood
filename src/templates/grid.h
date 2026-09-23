#pragma once
#include "wood_session.h"

namespace wood_grid {

// ═══════════════════════════════════════════════════════════════════════════
// Structs
// ═══════════════════════════════════════════════════════════════════════════

/// A building grid: nodes and members in a graph, floor, roof and wall loops over its nodes, levels by elevation; every meaning is a double attribute.
struct Grid {
    session_cpp::Graph graph; // Nodes "0", "1", ... carrying x, y, z; edges are member lines and face sides.
    std::vector<std::vector<std::string>> faces; // Node loops of floors, roofs and walls; floors counter-clockwise seen from above.
    std::vector<std::map<std::string, double>> facedata; // Per-face values, parallel to faces.
    std::map<std::string, double> default_face_attributes; // Values every face falls back to.
    std::vector<double> levels; // Distinct floor elevations, ascending; storey k spans levels[k] to levels[k + 1].
    double tolerance = 1.0; // Weld distance and elevation gap, in model units.

    /// Grid from member lines and surface loops: ends and corners welded within tolerance, every line split at the nodes on it and flagged "line", surfaces as faces, levels computed.
    static Grid from_lines(const std::vector<session_cpp::Line>& lines, const std::vector<session_cpp::Polyline>& surfaces, double tolerance = 1.0);

    /// Grid from a planar plan over storeys of heights: a floor per plan face and level from its "bottom" to its "top", columns under them, walls under plan edges flagged "wall".
    static Grid from_plan(const session_cpp::Mesh& plan, const std::vector<double>& heights, double tolerance = 1.0);

    /// Key of the node within tolerance of point, a new node carrying x, y, z when none is.
    std::string add_vertex(const session_cpp::Point& point);

    /// Index of a new face over loop; its missing sides become graph edges.
    size_t add_face(const std::vector<std::string>& loop);

    /// Position of a node, from its x, y, z.
    session_cpp::Point vertex_point(const std::string& key) const;

    /// Line of an edge, first key to second.
    session_cpp::Line edge_line(const std::tuple<std::string, std::string>& edge) const;

    /// Loop points of a face, open.
    std::vector<session_cpp::Point> face_points(size_t face) const;

    /// Faces with the edge as a side.
    std::vector<size_t> edge_faces(const std::tuple<std::string, std::string>& edge) const;

    /// Faces through the node.
    std::vector<size_t> vertex_faces(const std::string& key) const;

    /// Merge attrs into the default face attributes.
    void update_default_face_attributes(const std::vector<std::pair<std::string, double>>& attrs);

    /// Attribute of a face, falling back to the default; nullopt when neither exists.
    std::optional<double> face_attribute(size_t face, const std::string& name) const;

    /// Store an attribute on a face.
    void set_face_attribute(size_t face, const std::string& name, double value);

    /// Faces whose attributes match every (name, value) condition.
    std::vector<size_t> faces_where(const std::vector<std::pair<std::string, double>>& conditions) const;

    /// Store an attribute on a node that has no value for it yet, stored or default.
    void fill_vertex_attribute(const std::string& key, const std::string& name, double value);

    /// Store an attribute on an edge that has no value for it yet, stored or default.
    void fill_edge_attribute(const std::tuple<std::string, std::string>& edge, const std::string& name, double value);

    /// Store an attribute on a face that has no value for it yet, stored or default.
    void fill_face_attribute(size_t face, const std::string& name, double value);
};

/// Member sizes every element of a grid shares, in model units.
struct Dimensions {
    double column = 200.0; // Column width across flats, also the head bottom.
    double head = 300.0; // Head height, column top to beam underside.
    double reach = 200.0; // Head top half-width; open beam ends and outer deck edges run this far past the node.
    double beam = 200.0; // Girder and beam section side.
    double purlin = 200.0; // Purlin section side, top flush with the beams.
    double deck = 200.0; // Deck thickness; nodes are the deck top.
    double wall = 100.0; // Wall thickness, centred on the grid line.
};

/// One row of a bay table, what a bay design tool sizes members from.
struct Bay {
    double area = 0.0; // Plan area.
    double girder = 0.0; // Longest girder side, the girder span.
    double purlin = 0.0; // Longest purlin, centre to centre, the purlin span.
    double deck = 0.0; // Distance between the members the deck spans onto.
    int purlins = 0; // Purlin count.
    double length = 0.0; // Summed purlin lengths.
};

// ═══════════════════════════════════════════════════════════════════════════
// Grid accessors
// ═══════════════════════════════════════════════════════════════════════════

inline std::string Grid::add_vertex(const session_cpp::Point& point) {

    for (const session_cpp::Vertex& vertex : graph.get_vertices())
        if (vertex_point(vertex.name).distance(point) <= tolerance)
            return vertex.name;

    const std::string key = graph.add_node(std::to_string(graph.number_of_vertices()));
    graph.set_vertex_attribute(key, "x", point[0]);
    graph.set_vertex_attribute(key, "y", point[1]);
    graph.set_vertex_attribute(key, "z", point[2]);

    return key;
}

inline size_t Grid::add_face(const std::vector<std::string>& loop) {

    for (size_t i = 0; i < loop.size(); i++)
        if (!graph.has_edge({loop[i], loop[(i + 1) % loop.size()]}))
            graph.add_edge(loop[i], loop[(i + 1) % loop.size()]);

    faces.push_back(loop);
    facedata.emplace_back();

    return faces.size() - 1;
}

inline session_cpp::Point Grid::vertex_point(const std::string& key) const {
    return session_cpp::Point(*graph.vertex_attribute(key, "x"), *graph.vertex_attribute(key, "y"), *graph.vertex_attribute(key, "z"));
}

inline session_cpp::Line Grid::edge_line(const std::tuple<std::string, std::string>& edge) const {
    return session_cpp::Line::from_points(vertex_point(std::get<0>(edge)), vertex_point(std::get<1>(edge)));
}

inline std::vector<session_cpp::Point> Grid::face_points(size_t face) const {

    std::vector<session_cpp::Point> points;
    for (const std::string& key : faces[face])
        points.push_back(vertex_point(key));

    return points;
}

inline std::vector<size_t> Grid::edge_faces(const std::tuple<std::string, std::string>& edge) const {

    std::vector<size_t> result;
    for (size_t face = 0; face < faces.size(); face++)
        for (size_t i = 0; i < faces[face].size(); i++)
            if (std::minmax(faces[face][i], faces[face][(i + 1) % faces[face].size()]) == std::minmax(std::get<0>(edge), std::get<1>(edge)))
                result.push_back(face);

    return result;
}

inline std::vector<size_t> Grid::vertex_faces(const std::string& key) const {

    std::vector<size_t> result;
    for (size_t face = 0; face < faces.size(); face++)
        if (std::find(faces[face].begin(), faces[face].end(), key) != faces[face].end())
            result.push_back(face);

    return result;
}

inline void Grid::update_default_face_attributes(const std::vector<std::pair<std::string, double>>& attrs) {
    for (const std::pair<std::string, double>& attr : attrs)
        default_face_attributes[attr.first] = attr.second;
}

inline std::optional<double> Grid::face_attribute(size_t face, const std::string& name) const {

    const auto value = facedata[face].find(name);
    if (value != facedata[face].end())
        return value->second;

    const auto fallback = default_face_attributes.find(name);
    if (fallback != default_face_attributes.end())
        return fallback->second;

    return std::nullopt;
}

inline void Grid::set_face_attribute(size_t face, const std::string& name, double value) {
    facedata[face][name] = value;
}

inline std::vector<size_t> Grid::faces_where(const std::vector<std::pair<std::string, double>>& conditions) const {

    std::vector<size_t> result;
    for (size_t face = 0; face < faces.size(); face++)
        if (std::all_of(conditions.begin(), conditions.end(), [&](const std::pair<std::string, double>& condition) { return face_attribute(face, condition.first) == condition.second; }))
            result.push_back(face);

    return result;
}

inline void Grid::fill_vertex_attribute(const std::string& key, const std::string& name, double value) {
    if (!graph.vertex_attribute(key, name))
        graph.set_vertex_attribute(key, name, value);
}

inline void Grid::fill_edge_attribute(const std::tuple<std::string, std::string>& edge, const std::string& name, double value) {
    if (!graph.edge_attribute(edge, name))
        graph.set_edge_attribute(edge, name, value);
}

inline void Grid::fill_face_attribute(size_t face, const std::string& name, double value) {
    if (!face_attribute(face, name))
        set_face_attribute(face, name, value);
}

// ═══════════════════════════════════════════════════════════════════════════
// Normals and levels
// ═══════════════════════════════════════════════════════════════════════════

/// Newell normal of a loop, unnormalised: twice the area along the side it faces.
inline session_cpp::Vector compute_normal(const std::vector<session_cpp::Point>& points) {

    session_cpp::Vector normal(0.0, 0.0, 0.0);
    for (size_t i = 0; i < points.size(); i++) {
        const session_cpp::Point& a = points[i];
        const session_cpp::Point& b = points[(i + 1) % points.size()];
        normal += session_cpp::Vector((a[1] - b[1]) * (a[2] + b[2]), (a[2] - b[2]) * (a[0] + b[0]), (a[0] - b[0]) * (a[1] + b[1]));
    }

    return normal;
}

/// Angle in degrees between the segment from a to b and the horizontal plane.
inline double compute_tilt(const session_cpp::Point& a, const session_cpp::Point& b) {
    return std::atan2(std::abs(b[2] - a[2]), std::hypot(b[0] - a[0], b[1] - a[1])) * session_cpp::Tolerance::TO_DEGREES;
}

/// Unit plan direction from a to b.
inline session_cpp::Vector compute_direction(const session_cpp::Point& a, const session_cpp::Point& b) {
    return session_cpp::Vector(b[0] - a[0], b[1] - a[1], 0.0).normalized();
}

/// Unit plan direction of the span side of a floor, the way its girders run.
inline session_cpp::Vector compute_span(const Grid& grid, size_t face) {

    const std::vector<session_cpp::Point> points = grid.face_points(face);
    const size_t side = static_cast<size_t>(*grid.face_attribute(face, "span"));

    return compute_direction(points[side], points[(side + 1) % points.size()]);
}

/// Clusters node elevations into levels, the nodes of a face within 45 degrees of horizontal at its lowest; level on nodes, storey on nodes, edges and faces: the storey an element caps or stands in.
inline void compute_levels(Grid& grid) {

    std::map<std::string, double> elevations;
    for (const session_cpp::Vertex& vertex : grid.graph.get_vertices())
        elevations[vertex.name] = grid.vertex_point(vertex.name)[2];

    for (size_t face = 0; face < grid.faces.size(); face++) {
        const std::vector<session_cpp::Point> points = grid.face_points(face);
        const session_cpp::Vector normal = compute_normal(points);
        if (std::abs(normal[2]) < normal.magnitude() * std::sqrt(0.5))
            continue;

        double low = points[0][2];
        for (const session_cpp::Point& point : points)
            low = std::min(low, point[2]);
        for (const std::string& key : grid.faces[face])
            elevations[key] = std::min(elevations[key], low);
    }

    std::vector<double> heights;
    for (const std::pair<const std::string, double>& elevation : elevations)
        heights.push_back(elevation.second);
    std::sort(heights.begin(), heights.end());

    grid.levels.clear();
    for (const double z : heights)
        if (grid.levels.empty() || z - grid.levels.back() > grid.tolerance)
            grid.levels.push_back(z);

    const auto level = [&](const std::string& key) {
        return static_cast<double>(std::upper_bound(grid.levels.begin(), grid.levels.end(), elevations.at(key) + grid.tolerance) - grid.levels.begin() - 1);
    };

    for (const session_cpp::Vertex& vertex : grid.graph.get_vertices()) {
        grid.graph.set_vertex_attribute(vertex.name, "level", level(vertex.name));
        grid.graph.set_vertex_attribute(vertex.name, "storey", std::max(0.0, level(vertex.name) - 1.0));
    }

    for (const std::tuple<std::string, std::string>& edge : grid.graph.get_edges())
        grid.graph.set_edge_attribute(edge, "storey", std::max(0.0, std::max(level(std::get<0>(edge)), level(std::get<1>(edge))) - 1.0));

    for (size_t face = 0; face < grid.faces.size(); face++) {
        double top = 0.0;
        for (const std::string& key : grid.faces[face])
            top = std::max(top, level(key));
        grid.set_face_attribute(face, "storey", std::max(0.0, top - 1.0));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Generators
// ═══════════════════════════════════════════════════════════════════════════

inline Grid Grid::from_lines(const std::vector<session_cpp::Line>& lines, const std::vector<session_cpp::Polyline>& surfaces, double tolerance) {

    Grid grid;
    grid.tolerance = tolerance;

    for (const session_cpp::Line& line : lines) {
        grid.add_vertex(line.start());
        grid.add_vertex(line.end());
    }

    std::vector<std::vector<std::string>> loops;
    for (const session_cpp::Polyline& surface : surfaces) {
        loops.emplace_back();
        for (const session_cpp::Point& point : surface.get_points()) {
            const std::string key = grid.add_vertex(point);
            if (loops.back().empty() || loops.back().back() != key)
                loops.back().push_back(key);
        }
    }

    for (const session_cpp::Line& line : lines) {
        std::vector<std::pair<double, std::string>> stops;
        for (const session_cpp::Vertex& vertex : grid.graph.get_vertices()) {
            const std::pair<double, session_cpp::Point> closest = line.closest_point(grid.vertex_point(vertex.name));
            if (closest.second.distance(grid.vertex_point(vertex.name)) <= tolerance)
                stops.emplace_back(closest.first, vertex.name);
        }
        std::sort(stops.begin(), stops.end());

        for (size_t i = 0; i + 1 < stops.size(); i++) {
            if (!grid.graph.has_edge({stops[i].second, stops[i + 1].second}))
                grid.graph.add_edge(stops[i].second, stops[i + 1].second);
            grid.graph.set_edge_attribute({stops[i].second, stops[i + 1].second}, "line", 1.0);
        }
    }

    for (std::vector<std::string>& loop : loops) {
        if (loop.size() > 1 && loop.back() == loop.front())
            loop.pop_back();
        if (loop.size() < 3)
            continue;

        const size_t face = grid.add_face(loop);
        const session_cpp::Vector normal = compute_normal(grid.face_points(face));
        if (normal[2] < -normal.magnitude() * std::sqrt(0.5))
            std::reverse(grid.faces[face].begin(), grid.faces[face].end());
    }

    compute_levels(grid);

    return grid;
}

inline Grid Grid::from_plan(const session_cpp::Mesh& plan, const std::vector<double>& heights, double tolerance) {

    Grid grid;
    grid.tolerance = tolerance;

    std::vector<double> elevations = {0.0};
    for (const double height : heights)
        elevations.push_back(elevations.back() + height);

    std::map<std::pair<size_t, size_t>, std::string> keys;
    const auto node = [&](size_t vertex, size_t level) {
        if (!keys.contains({vertex, level})) {
            const session_cpp::Point point = *plan.vertex_point(vertex);
            keys[{vertex, level}] = grid.add_vertex(session_cpp::Point(point[0], point[1], elevations[level]));
        }
        return keys.at({vertex, level});
    };

    for (const size_t face : plan.faces()) {
        std::vector<size_t> loop = *plan.face_vertices(face);
        std::map<std::string, double> data = plan.facedata.contains(face) ? plan.facedata.at(face) : std::map<std::string, double>();

        std::vector<session_cpp::Point> points;
        for (const size_t vertex : loop)
            points.push_back(*plan.vertex_point(vertex));
        if (compute_normal(points)[2] < 0.0) {
            std::reverse(loop.begin(), loop.end());
            if (data.contains("span") && data["span"] >= 0.0)
                data["span"] = std::fmod(2.0 * loop.size() - 2.0 - data["span"], static_cast<double>(loop.size()));
        }

        const size_t bottom = static_cast<size_t>(plan.face_attribute(face, "bottom").value_or(1.0));
        const size_t top = static_cast<size_t>(plan.face_attribute(face, "top").value_or(static_cast<double>(heights.size())));

        for (size_t level = bottom; level <= top; level++) {
            std::vector<std::string> ring;
            for (const size_t vertex : loop)
                ring.push_back(node(vertex, level));

            const size_t index = grid.add_face(ring);
            grid.facedata[index] = data;
            for (size_t i = 0; i < ring.size(); i++)
                grid.graph.set_edge_attribute({ring[i], ring[(i + 1) % ring.size()]}, "line", 1.0);
        }

        for (size_t level = 0; level < top; level++)
            for (const size_t vertex : loop) {
                if (plan.vertex_attribute(vertex, "column").value_or(1.0) == 0.0)
                    continue;

                const std::tuple<std::string, std::string> edge(node(vertex, level), node(vertex, level + 1));
                if (!grid.graph.has_edge(edge))
                    grid.graph.add_edge(std::get<0>(edge), std::get<1>(edge));
                grid.graph.set_edge_attribute(edge, "line", 1.0);
            }
    }

    for (const std::pair<size_t, size_t>& edge : plan.edges()) {
        if (plan.edge_attribute(edge, "wall").value_or(0.0) != 1.0)
            continue;

        for (size_t level = 0; level < heights.size(); level++)
            if (keys.contains({edge.first, level + 1}) && keys.contains({edge.second, level + 1}) && grid.graph.has_edge({keys.at({edge.first, level + 1}), keys.at({edge.second, level + 1})}))
                grid.add_face({node(edge.first, level), node(edge.second, level), node(edge.second, level + 1), node(edge.first, level + 1)});
    }

    for (const std::pair<const std::string, double>& attribute : plan.default_face_attributes)
        grid.default_face_attributes[attribute.first] = attribute.second;

    compute_levels(grid);

    return grid;
}

// ═══════════════════════════════════════════════════════════════════════════
// Plans
// ═══════════════════════════════════════════════════════════════════════════

/// Bays of widths xs along x and depths ys along y from the origin, the y axis leaning skew degrees towards x; skew 0 is orthogonal; side 0 of every bay runs along x.
inline session_cpp::Mesh create_orthogonal(const std::vector<double>& xs, const std::vector<double>& ys, double skew = 0.0) {

    std::vector<double> x = {0.0};
    for (const double width : xs)
        x.push_back(x.back() + width);

    std::vector<double> y = {0.0};
    for (const double depth : ys)
        y.push_back(y.back() + depth);

    const double angle = skew * session_cpp::Tolerance::TO_RADIANS;
    const auto point = [&](size_t i, size_t j) { return session_cpp::Point(x[i] + y[j] * std::sin(angle), y[j] * std::cos(angle), 0.0); };

    std::vector<std::vector<session_cpp::Point>> polygons;
    for (size_t j = 0; j < ys.size(); j++)
        for (size_t i = 0; i < xs.size(); i++)
            polygons.push_back({point(i, j), point(i + 1, j), point(i + 1, j + 1), point(i, j + 1)});

    return session_cpp::Mesh::from_polylines(polygons, 0.001);
}

/// Rings at radii cut into sectors over sweep degrees, rings as chords; side 0 of every bay is a ray; a first radius of 0 gives a centre node and triangles.
inline session_cpp::Mesh create_radial(const std::vector<double>& radii, int sectors, double sweep = 360.0) {

    const auto point = [&](size_t i, int j) {
        const double angle = sweep * j / sectors * session_cpp::Tolerance::TO_RADIANS;
        return session_cpp::Point(radii[i] * std::cos(angle), radii[i] * std::sin(angle), 0.0);
    };

    std::vector<std::vector<session_cpp::Point>> polygons;
    for (size_t i = 0; i + 1 < radii.size(); i++)
        for (int j = 0; j < sectors; j++)
            polygons.push_back({point(i, j), point(i + 1, j), point(i + 1, j + 1), point(i, j + 1)});

    return session_cpp::Mesh::from_polylines(polygons, 0.001);
}

/// Equilateral triangles of side, nx by ny rhombi each split in two.
inline session_cpp::Mesh create_triangular(double side, int nx, int ny) {

    const auto point = [&](int i, int j) { return session_cpp::Point(side * (i + 0.5 * j), side * std::sqrt(3.0) / 2.0 * j, 0.0); };

    std::vector<std::vector<session_cpp::Point>> polygons;
    for (int j = 0; j < ny; j++)
        for (int i = 0; i < nx; i++) {
            polygons.push_back({point(i, j), point(i + 1, j), point(i, j + 1)});
            polygons.push_back({point(i + 1, j), point(i + 1, j + 1), point(i, j + 1)});
        }

    return session_cpp::Mesh::from_polylines(polygons, 0.001);
}

/// Pointy-top hexagons of side in nx columns and ny rows, odd rows shifted half a cell.
inline session_cpp::Mesh create_hexagonal(double side, int nx, int ny) {

    std::vector<std::vector<session_cpp::Point>> polygons;
    for (int j = 0; j < ny; j++)
        for (int i = 0; i < nx; i++) {
            const session_cpp::Point centre(std::sqrt(3.0) * side * (i + 0.5 * (j % 2)), 1.5 * side * j, 0.0);
            std::vector<session_cpp::Point> corners;
            for (int k = 0; k < 6; k++)
                corners.push_back(centre + session_cpp::Vector(std::cos((30.0 + 60.0 * k) * session_cpp::Tolerance::TO_RADIANS), std::sin((30.0 + 60.0 * k) * session_cpp::Tolerance::TO_RADIANS), 0.0) * side);
            polygons.push_back(corners);
        }

    return session_cpp::Mesh::from_polylines(polygons, 0.001);
}

// ═══════════════════════════════════════════════════════════════════════════
// Rules
// ═══════════════════════════════════════════════════════════════════════════

/// Lowest and highest (p - p0) . direction over a loop.
inline std::pair<double, double> compute_extent(const std::vector<session_cpp::Point>& points, const session_cpp::Vector& direction) {

    std::pair<double, double> extent(0.0, 0.0);
    for (const session_cpp::Point& point : points) {
        extent.first = std::min(extent.first, (point - points[0]).dot(direction));
        extent.second = std::max(extent.second, (point - points[0]).dot(direction));
    }

    return extent;
}

/// Faces into floor and wall by tilt within angle degrees; roof on a floor no column rises from; structural system 1 on floors without one.
inline void compute_faces(Grid& grid, double angle = 10.0) {

    for (size_t face = 0; face < grid.faces.size(); face++) {
        const session_cpp::Vector normal = compute_normal(grid.face_points(face));
        if (std::acos(std::abs(normal[2]) / normal.magnitude()) * session_cpp::Tolerance::TO_DEGREES >= 90.0 - angle) {
            grid.fill_face_attribute(face, "wall", 1.0);
            continue;
        }

        grid.fill_face_attribute(face, "floor", 1.0);
        grid.fill_face_attribute(face, "structural_system", 1.0);

        bool roof = true;
        for (const std::string& key : grid.faces[face])
            for (const std::string& other : grid.graph.neighbors(key))
                if (grid.graph.edge_attribute({key, other}, "line") == 1.0 && grid.vertex_point(other)[2] > grid.vertex_point(key)[2] && compute_tilt(grid.vertex_point(key), grid.vertex_point(other)) >= 90.0 - angle)
                    roof = false;

        if (roof)
            grid.fill_face_attribute(face, "roof", 1.0);
    }
}

/// Span side per floor, the loop side its girders run parallel to: the shortest, or the longest, first within tolerance; -1 under structural system 0.
inline void compute_spans(Grid& grid, bool longest = false) {

    for (const size_t face : grid.faces_where({{"floor", 1.0}})) {
        if (grid.face_attribute(face, "structural_system") == 0.0) {
            grid.fill_face_attribute(face, "span", -1.0);
            continue;
        }

        const std::vector<session_cpp::Point> points = grid.face_points(face);
        std::vector<double> lengths;
        for (size_t i = 0; i < points.size(); i++)
            lengths.push_back(std::hypot(points[(i + 1) % points.size()][0] - points[i][0], points[(i + 1) % points.size()][1] - points[i][1]));

        const double target = longest ? *std::max_element(lengths.begin(), lengths.end()) : *std::min_element(lengths.begin(), lengths.end());
        size_t side = 0;
        while (std::abs(lengths[side] - target) > grid.tolerance)
            side++;

        grid.fill_face_attribute(face, "span", static_cast<double>(side));
    }
}

/// Line edges into column, beam and brace by tilt within angle degrees; on horizontal lines is_boundary, girder along a floor's span, purlin on cross lines of system 2, beam 0 on cross lines the deck spans over.
inline void compute_members(Grid& grid, double angle = 10.0) {

    for (const std::tuple<std::string, std::string>& edge : grid.graph.edges_where({{"line", 1.0}})) {
        const session_cpp::Line line = grid.edge_line(edge);
        const double tilt = compute_tilt(line.start(), line.end());
        if (tilt >= 90.0 - angle) {
            grid.fill_edge_attribute(edge, "column", 1.0);
            continue;
        }

        if (tilt > angle) {
            grid.fill_edge_attribute(edge, "brace", 1.0);
            continue;
        }

        std::vector<size_t> floors;
        for (const size_t face : grid.edge_faces(edge))
            if (grid.face_attribute(face, "floor") == 1.0)
                floors.push_back(face);

        if (floors.size() <= 1) {
            grid.fill_edge_attribute(edge, "is_boundary", 1.0);
            grid.fill_vertex_attribute(std::get<0>(edge), "is_boundary", 1.0);
            grid.fill_vertex_attribute(std::get<1>(edge), "is_boundary", 1.0);
        }

        bool purlin = false;
        for (const size_t face : floors) {
            const double system = grid.face_attribute(face, "structural_system").value_or(1.0);
            purlin = purlin || system == 2.0;
            if (grid.face_attribute(face, "span").value_or(-1.0) < 0.0 || system == 0.0)
                continue;

            if (std::abs(compute_direction(line.start(), line.end()).dot(compute_span(grid, face))) >= std::cos(angle * session_cpp::Tolerance::TO_RADIANS))
                grid.fill_edge_attribute(edge, "girder", 1.0);
        }

        const bool carried = grid.graph.edge_attribute(edge, "girder") == 1.0 || grid.graph.edge_attribute(edge, "is_boundary") == 1.0;
        if (!carried && purlin)
            grid.fill_edge_attribute(edge, "purlin", 1.0);
        grid.fill_edge_attribute(edge, "beam", carried || purlin ? 1.0 : 0.0);
    }
}

/// Purlin count per floor of structural system 2, the fewest that keep every gap along its span within spacing; 0 on other floors.
inline void compute_purlins(Grid& grid, double spacing) {

    for (const size_t face : grid.faces_where({{"floor", 1.0}})) {
        if (grid.face_attribute(face, "structural_system") != 2.0 || grid.face_attribute(face, "span").value_or(-1.0) < 0.0 || spacing <= 0.0) {
            grid.fill_face_attribute(face, "purlins", 0.0);
            continue;
        }

        const std::pair<double, double> extent = compute_extent(grid.face_points(face), compute_span(grid, face));
        grid.fill_face_attribute(face, "purlins", std::max(0.0, std::ceil((extent.second - extent.first) / spacing - 1e-9) - 1.0));
    }
}

/// Support at the foot of every column stack, head where a column arrives under a beam or a floor.
inline void compute_supports(Grid& grid) {

    for (const session_cpp::Vertex& vertex : grid.graph.get_vertices()) {
        const double z = grid.vertex_point(vertex.name)[2];
        bool up = false;
        bool down = false;
        bool carries = false;
        for (const std::string& other : grid.graph.neighbors(vertex.name)) {
            if (grid.graph.edge_attribute({vertex.name, other}, "column") == 1.0) {
                up = up || grid.vertex_point(other)[2] > z;
                down = down || grid.vertex_point(other)[2] < z;
            }
            carries = carries || grid.graph.edge_attribute({vertex.name, other}, "beam") == 1.0;
        }

        for (const size_t face : grid.vertex_faces(vertex.name))
            carries = carries || grid.face_attribute(face, "floor") == 1.0;

        if (up && !down)
            grid.fill_vertex_attribute(vertex.name, "support", 1.0);
        if (down && carries)
            grid.fill_vertex_attribute(vertex.name, "head", 1.0);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// Section side of the member on an edge: purlin size on a purlin line, else beam size.
inline double compute_width(const Grid& grid, const std::tuple<std::string, std::string>& edge, const Dimensions& dimensions) {
    return grid.graph.edge_attribute(edge, "purlin") == 1.0 ? dimensions.purlin : dimensions.beam;
}

/// Unit up of the members on faces: the floor normals among them summed, z when there is none; members drop along it under the deck.
inline session_cpp::Vector compute_up(const Grid& grid, const std::vector<size_t>& faces) {

    session_cpp::Vector up(0.0, 0.0, 0.0);
    for (const size_t face : faces)
        if (grid.face_attribute(face, "floor") == 1.0)
            up += compute_normal(grid.face_points(face)).normalized();

    return up.magnitude() > 0.0 ? up.normalized() : session_cpp::Vector(0.0, 0.0, 1.0);
}

/// Point moved vertically onto plane.
inline session_cpp::Point compute_lift(const session_cpp::Plane& plane, const session_cpp::Point& point) {
    return point - session_cpp::Vector(0.0, 0.0, (point - plane.origin()).dot(plane.z_axis()) / plane.z_axis()[2]);
}

/// Unit plan directions of the horizontal edges at a node and their opposites, counter-clockwise, closer than 1 degree merged; one line adds its perpendicular, none gives x and y.
inline std::vector<session_cpp::Vector> compute_directions(const Grid& grid, const std::string& node) {

    const session_cpp::Point origin = grid.vertex_point(node);
    std::vector<double> angles;
    for (const std::string& other : grid.graph.neighbors(node)) {
        const session_cpp::Point point = grid.vertex_point(other);
        if (grid.graph.edge_attribute({node, other}, "column") == 1.0 || grid.graph.edge_attribute({node, other}, "brace") == 1.0 || std::hypot(point[0] - origin[0], point[1] - origin[1]) <= grid.tolerance)
            continue;

        angles.push_back(std::atan2(point[1] - origin[1], point[0] - origin[0]));
        angles.push_back(std::atan2(origin[1] - point[1], origin[0] - point[0]));
    }

    if (angles.empty())
        angles = {-session_cpp::Tolerance::HALF_PI, 0.0, session_cpp::Tolerance::HALF_PI, session_cpp::Tolerance::PI};
    std::sort(angles.begin(), angles.end());

    std::vector<double> merged;
    for (const double angle : angles)
        if (merged.empty() || angle - merged.back() > session_cpp::Tolerance::TO_RADIANS)
            merged.push_back(angle);
    if (merged.size() > 1 && merged.front() + session_cpp::Tolerance::TWO_PI - merged.back() <= session_cpp::Tolerance::TO_RADIANS)
        merged.pop_back();

    if (merged.size() == 2) {
        merged.push_back(merged[0] + session_cpp::Tolerance::HALF_PI);
        merged.push_back(merged[0] - session_cpp::Tolerance::HALF_PI);
        std::sort(merged.begin(), merged.end());
    }

    std::vector<session_cpp::Vector> directions;
    for (const double angle : merged)
        directions.emplace_back(std::cos(angle), std::sin(angle), 0.0);

    return directions;
}

/// Closed polygon about centre whose edge j is perpendicular to directions[j] at distance.
inline session_cpp::Polyline compute_polygon(const std::vector<session_cpp::Vector>& directions, const session_cpp::Point& centre, double distance) {

    std::vector<session_cpp::Point> points;
    for (size_t j = 0; j < directions.size(); j++) {
        const session_cpp::Vector& a = directions[j];
        const session_cpp::Vector& b = directions[(j + 1) % directions.size()];
        points.push_back(centre + (a + b) * (distance / (1.0 + a.dot(b))));
    }
    points.push_back(points.front());

    return session_cpp::Polyline(points);
}

/// Loop with side i moved out by distances[i] in the plane of normal, the loop counter-clockwise about normal.
inline std::vector<session_cpp::Point> compute_offset(const std::vector<session_cpp::Point>& points, const session_cpp::Vector& normal, const std::vector<double>& distances) {

    const size_t count = points.size();
    std::vector<session_cpp::Vector> outward;
    for (size_t i = 0; i < count; i++)
        outward.push_back((points[(i + 1) % count] - points[i]).cross(normal).normalized());

    std::vector<session_cpp::Point> result;
    for (size_t i = 0; i < count; i++) {
        const size_t before = (i + count - 1) % count;
        const double cosine = outward[before].dot(outward[i]);
        if (1.0 - cosine * cosine < 1e-9) {
            result.push_back(points[i] + outward[i] * std::max(distances[before], distances[i]));
            continue;
        }

        const double alpha = (distances[before] - cosine * distances[i]) / (1.0 - cosine * cosine);
        const double beta = (distances[i] - cosine * distances[before]) / (1.0 - cosine * cosine);
        result.push_back(points[i] + outward[before] * alpha + outward[i] * beta);
    }

    return result;
}

/// Other beam neighbour at node within 45 degrees of straight on from other, the straightest.
inline std::optional<std::string> compute_continuation(const Grid& grid, const std::string& node, const std::string& other) {

    const session_cpp::Point origin = grid.vertex_point(node);
    const session_cpp::Vector ahead = compute_direction(grid.vertex_point(other), origin);
    std::optional<std::string> straight;
    double best = std::cos(45.0 * session_cpp::Tolerance::TO_RADIANS);
    for (const std::string& next : grid.graph.neighbors(node)) {
        if (next == other || grid.graph.edge_attribute({node, next}, "beam") != 1.0)
            continue;

        const double dot = compute_direction(origin, grid.vertex_point(next)).dot(ahead);
        if (dot > best) {
            best = dot;
            straight = next;
        }
    }

    return straight;
}

/// Beam neighbour whose beam runs through node: a girder first, then one with a continuation, then the smallest angle from x.
inline std::optional<std::string> compute_through(const Grid& grid, const std::string& node) {

    const session_cpp::Point origin = grid.vertex_point(node);
    std::optional<std::string> through;
    std::tuple<double, double, double> best;
    for (const std::string& other : grid.graph.neighbors(node)) {
        if (grid.graph.edge_attribute({node, other}, "beam") != 1.0)
            continue;

        const session_cpp::Vector direction = compute_direction(origin, grid.vertex_point(other));
        const double angle = std::atan2(direction[1], direction[0]);
        const std::tuple<double, double, double> score(
            grid.graph.edge_attribute({node, other}, "girder") == 1.0 ? 0.0 : 1.0,
            compute_continuation(grid, node, other) ? 0.0 : 1.0,
            angle < 0.0 ? angle + session_cpp::Tolerance::TWO_PI : angle
        );

        if (!through || score < best) {
            best = score;
            through = other;
        }
    }

    return through;
}

/// Plane of the head top at node: under the deck and the deepest beam arriving there, parallel to the floors through the node.
inline session_cpp::Plane compute_head_top(const Grid& grid, const std::string& node, const Dimensions& dimensions) {

    double depth = 0.0;
    for (const std::string& other : grid.graph.neighbors(node))
        if (grid.graph.edge_attribute({node, other}, "beam") == 1.0)
            depth = std::max(depth, compute_width(grid, {node, other}, dimensions));

    const session_cpp::Vector up = compute_up(grid, grid.vertex_faces(node));

    return session_cpp::Plane::from_point_normal(grid.vertex_point(node) - up * (dimensions.deck + depth), up);
}

/// End point and cut planes of the beam from node towards other at its node end, its axis dropped along its up: through and straight on ends meet on the bisector or run reach past, the others butt on the through beam's side.
inline std::pair<session_cpp::Point, std::vector<session_cpp::Plane>> compute_cuts(const Grid& grid, const std::string& node, const std::string& other, const Dimensions& dimensions) {

    const session_cpp::Point origin = grid.vertex_point(node);
    const auto toward = [&](const std::string& key) { return (grid.vertex_point(key) - origin).normalized(); };
    const session_cpp::Vector up = compute_up(grid, grid.edge_faces({node, other}));
    const session_cpp::Point axis = origin - up * (dimensions.deck + compute_width(grid, {node, other}, dimensions) / 2.0);
    const session_cpp::Vector along = toward(other);
    const std::string through = *compute_through(grid, node);
    const std::optional<std::string> straight = compute_continuation(grid, node, through);

    if (other == through || other == straight) {
        const std::optional<std::string> partner = other == through ? straight : std::optional<std::string>(through);
        if (!partner)
            return {axis - along * dimensions.reach, {}};

        return {axis - along * dimensions.reach, {session_cpp::Plane::from_point_normal(axis, (along - toward(*partner)).normalized())}};
    }

    session_cpp::Vector side = up.cross(toward(through));
    if (side.dot(along) < 0.0)
        side = -side;
    std::vector<session_cpp::Plane> planes = {session_cpp::Plane::from_point_normal(axis + side * (compute_width(grid, {node, through}, dimensions) / 2.0), side)};

    std::vector<std::pair<double, std::string>> beams;
    for (const std::string& next : grid.graph.neighbors(node))
        if (grid.graph.edge_attribute({node, next}, "beam") == 1.0) {
            const session_cpp::Vector direction = compute_direction(origin, grid.vertex_point(next));
            beams.emplace_back(std::atan2(direction[1], direction[0]), next);
        }
    std::sort(beams.begin(), beams.end());

    const size_t index = std::find_if(beams.begin(), beams.end(), [&](const std::pair<double, std::string>& beam) { return beam.second == other; }) - beams.begin();
    for (const size_t neighbour : {(index + 1) % beams.size(), (index + beams.size() - 1) % beams.size()}) {
        const std::string& key = beams[neighbour].second;
        if (key == other || key == through || key == straight)
            continue;

        planes.push_back(session_cpp::Plane::from_point_normal(axis, (along - toward(key)).normalized()));
    }

    return {axis, planes};
}

/// Purlin centre lines of a floor at its level: stations evenly along its span side, each from side to side across it.
inline std::vector<session_cpp::Line> compute_stations(const Grid& grid, size_t face) {

    const int count = static_cast<int>(grid.face_attribute(face, "purlins").value_or(0.0));
    if (count <= 0 || grid.face_attribute(face, "span").value_or(-1.0) < 0.0)
        return {};

    const std::vector<session_cpp::Point> points = grid.face_points(face);
    const size_t size = points.size();
    const session_cpp::Vector along = compute_span(grid, face);
    const session_cpp::Vector across = session_cpp::Vector(0.0, 0.0, 1.0).cross(along);
    const std::pair<double, double> extent = compute_extent(points, along);

    std::vector<session_cpp::Line> lines;
    for (int j = 1; j <= count; j++) {
        const double station = extent.first + (extent.second - extent.first) * j / (count + 1);
        std::vector<std::pair<double, session_cpp::Point>> crossings;
        for (size_t i = 0; i < size; i++) {
            const double da = (points[i] - points[0]).dot(along) - station;
            const double db = (points[(i + 1) % size] - points[0]).dot(along) - station;
            if ((da >= 0.0) == (db >= 0.0))
                continue;

            const session_cpp::Point crossing = points[i] + (points[(i + 1) % size] - points[i]) * (da / (da - db));
            crossings.emplace_back((crossing - points[0]).dot(across), crossing);
        }

        std::sort(crossings.begin(), crossings.end(), [](const std::pair<double, session_cpp::Point>& a, const std::pair<double, session_cpp::Point>& b) { return a.first < b.first; });
        for (size_t k = 0; k + 1 < crossings.size(); k += 2)
            lines.push_back(session_cpp::Line::from_points(crossings[k].second, crossings[k + 1].second));
    }

    return lines;
}

/// Bay table row of a floor: area, girder, purlin and deck spans, purlin count and length.
inline Bay compute_bay(const Grid& grid, size_t face) {

    const std::vector<session_cpp::Point> points = grid.face_points(face);
    const std::vector<std::string>& loop = grid.faces[face];

    Bay bay;
    bay.area = compute_normal(points).magnitude() / 2.0;
    bay.purlins = static_cast<int>(grid.face_attribute(face, "purlins").value_or(0.0));
    for (size_t i = 0; i < loop.size(); i++) {
        const double length = points[i].distance(points[(i + 1) % loop.size()]);
        bay.deck = std::max(bay.deck, length);
        if (grid.graph.edge_attribute({loop[i], loop[(i + 1) % loop.size()]}, "girder") == 1.0)
            bay.girder = std::max(bay.girder, length);
    }

    for (const session_cpp::Line& line : compute_stations(grid, face)) {
        bay.purlin = std::max(bay.purlin, line.length());
        bay.length += line.length();
    }

    if (grid.face_attribute(face, "span").value_or(-1.0) < 0.0)
        return bay;

    const bool purlins = grid.face_attribute(face, "structural_system") == 2.0;
    const session_cpp::Vector along = compute_span(grid, face);
    const std::pair<double, double> extent = compute_extent(points, purlins ? along : session_cpp::Vector(0.0, 0.0, 1.0).cross(along));
    bay.deck = (extent.second - extent.first) / (purlins ? bay.purlins + 1 : 1);

    return bay;
}

// ═══════════════════════════════════════════════════════════════════════════
// Builders
// ═══════════════════════════════════════════════════════════════════════════

/// Column on a column edge: lower node up to the head bottom, the upper node without a head; section the upper node's direction polygon at column / 2.
inline std::shared_ptr<wood_session::Column> to_column(const Grid& grid, const std::tuple<std::string, std::string>& edge, const Dimensions& dimensions) {

    const bool rising = grid.vertex_point(std::get<0>(edge))[2] < grid.vertex_point(std::get<1>(edge))[2];
    const std::string high = rising ? std::get<1>(edge) : std::get<0>(edge);
    const session_cpp::Point start = grid.vertex_point(rising ? std::get<0>(edge) : std::get<1>(edge));
    const session_cpp::Point end = grid.vertex_point(high);
    const double top = grid.graph.vertex_attribute(high, "head") == 1.0 ? compute_lift(compute_head_top(grid, high, dimensions), end)[2] - dimensions.head : end[2];

    return std::make_shared<wood_session::Column>(
        session_cpp::Line::from_points(start, start + (end - start) * ((top - start[2]) / (end[2] - start[2]))),
        compute_polygon(compute_directions(grid, high), start, dimensions.column / 2.0),
        "column"
    );
}

/// Head block on a head node: the direction polygon at column / 2 on the column top, at reach lifted onto the head top.
inline std::shared_ptr<wood_session::Block> to_head(const Grid& grid, const std::string& node, const Dimensions& dimensions) {

    const session_cpp::Point end = grid.vertex_point(node);
    session_cpp::Point start = end;
    for (const std::string& other : grid.graph.neighbors(node))
        if (grid.graph.edge_attribute({node, other}, "column") == 1.0 && grid.vertex_point(other)[2] < end[2])
            start = grid.vertex_point(other);

    const session_cpp::Plane plane = compute_head_top(grid, node, dimensions);
    const double top = compute_lift(plane, end)[2];
    const std::vector<session_cpp::Vector> directions = compute_directions(grid, node);
    const auto at = [&](double z) { return start + (end - start) * ((z - start[2]) / (end[2] - start[2])); };

    std::vector<session_cpp::Point> ring;
    for (const session_cpp::Point& point : compute_polygon(directions, at(top), dimensions.reach).get_points())
        ring.push_back(compute_lift(plane, point));

    return std::make_shared<wood_session::Block>(std::vector<session_cpp::Polyline>{
        compute_polygon(directions, at(top - dimensions.head), dimensions.column / 2.0),
        session_cpp::Polyline(ring)
    }, "head");
}

/// Beam on a beam edge, named girder, purlin or beam by its flags, its section square to its up, its ends from compute_cuts at both nodes.
inline std::shared_ptr<wood_session::Beam> to_beam(const Grid& grid, const std::tuple<std::string, std::string>& edge, const Dimensions& dimensions) {

    const std::pair<session_cpp::Point, std::vector<session_cpp::Plane>> start = compute_cuts(grid, std::get<0>(edge), std::get<1>(edge), dimensions);
    const std::pair<session_cpp::Point, std::vector<session_cpp::Plane>> end = compute_cuts(grid, std::get<1>(edge), std::get<0>(edge), dimensions);
    const std::string name = grid.graph.edge_attribute(edge, "girder") == 1.0 ? "girder" : grid.graph.edge_attribute(edge, "purlin") == 1.0 ? "purlin" : "beam";

    std::shared_ptr<wood_session::Beam> beam = std::make_shared<wood_session::Beam>(
        session_cpp::Polyline({start.first, end.first}),
        std::vector<double>{compute_width(grid, edge, dimensions) / 2.0},
        std::vector<session_cpp::Vector>{compute_up(grid, grid.edge_faces(edge))},
        -1,
        name
    );
    beam->cuts = start.second;
    beam->cuts.insert(beam->cuts.end(), end.second.begin(), end.second.end());

    return beam;
}

/// Purlins of a floor on its stations, dropped along its normal, top flush with the beams, cut by the side planes of the members they end on.
inline std::vector<std::shared_ptr<wood_session::Beam>> to_purlins(const Grid& grid, size_t face, const Dimensions& dimensions) {

    const std::vector<session_cpp::Point> points = grid.face_points(face);
    const std::vector<std::string>& loop = grid.faces[face];
    const session_cpp::Vector normal = compute_normal(points).normalized();
    const session_cpp::Vector drop = normal * (dimensions.deck + dimensions.purlin / 2.0);

    std::vector<std::shared_ptr<wood_session::Beam>> purlins;
    for (const session_cpp::Line& line : compute_stations(grid, face)) {
        std::vector<session_cpp::Plane> cuts;
        for (const session_cpp::Point& end : {line.start(), line.end()})
            for (size_t i = 0; i < loop.size(); i++) {
                const session_cpp::Line side = session_cpp::Line::from_points(points[i], points[(i + 1) % loop.size()]);
                if (side.closest_point(end).second.distance(end) > grid.tolerance)
                    continue;

                const session_cpp::Vector inward = normal.cross(side.to_vector()).normalized();
                cuts.push_back(session_cpp::Plane::from_point_normal(end + inward * (compute_width(grid, {loop[i], loop[(i + 1) % loop.size()]}, dimensions) / 2.0), inward));
            }

        purlins.push_back(std::make_shared<wood_session::Beam>(
            session_cpp::Polyline({line.start() - drop, line.end() - drop}),
            std::vector<double>{dimensions.purlin / 2.0},
            std::vector<session_cpp::Vector>{normal},
            -1,
            "purlin"
        ));
        purlins.back()->cuts = cuts;
    }

    return purlins;
}

/// Deck plate of a floor, its top on the nodes: sides no other floor shares pushed out by reach, shared sides on the centre line, a mitre to the next deck at a reentrant corner.
inline std::shared_ptr<wood_session::Plate> to_deck(const Grid& grid, size_t face, const Dimensions& dimensions) {

    const std::vector<session_cpp::Point> points = grid.face_points(face);
    const std::vector<std::string>& loop = grid.faces[face];
    const size_t count = loop.size();
    const session_cpp::Vector normal = compute_normal(points).normalized();

    std::vector<std::vector<size_t>> sides;
    std::vector<double> distances;
    for (size_t i = 0; i < count; i++) {
        sides.push_back(grid.edge_faces({loop[i], loop[(i + 1) % count]}));
        const bool shared = std::any_of(sides[i].begin(), sides[i].end(), [&](size_t other) { return other != face && grid.face_attribute(other, "floor") == 1.0; });
        distances.push_back(shared ? 0.0 : dimensions.reach);
    }

    const std::vector<session_cpp::Point> offset = compute_offset(points, normal, distances);
    std::vector<session_cpp::Point> outline;
    for (size_t i = 0; i < count; i++) {
        const size_t before = (i + count - 1) % count;
        const std::vector<size_t> around = grid.vertex_faces(loop[i]);
        const bool reentrant = distances[before] != distances[i] && std::any_of(around.begin(), around.end(), [&](size_t other) {
            return grid.face_attribute(other, "floor") == 1.0 && std::count(sides[before].begin(), sides[before].end(), other) + std::count(sides[i].begin(), sides[i].end(), other) == 0;
        });

        if (!reentrant) {
            outline.push_back(offset[i]);
            continue;
        }

        const session_cpp::Vector in = (points[i] - points[before]).cross(normal).normalized();
        const session_cpp::Vector out = (points[(i + 1) % count] - points[i]).cross(normal).normalized();
        const session_cpp::Vector pushed = distances[i] > 0.0 ? out : in;
        const session_cpp::Vector held = distances[i] > 0.0 ? in : out;
        const session_cpp::Point mitre = points[i] + (pushed - held) * (dimensions.reach / (1.0 - pushed.dot(held)));
        outline.push_back(distances[i] > 0.0 ? points[i] : mitre);
        outline.push_back(distances[i] > 0.0 ? mitre : points[i]);
    }

    const session_cpp::Polyline top = session_cpp::Polyline(outline).closed();

    return std::make_shared<wood_session::Plate>(top.translated(normal * -dimensions.deck), top, "deck");
}

/// Wall plate of a quad wall face: between the column faces, chamfered along the head faces, from the lower node up to the beam underside, thickness centred on the face.
inline std::shared_ptr<wood_session::Plate> to_wall(const Grid& grid, size_t face, const Dimensions& dimensions) {

    const std::vector<std::string>& loop = grid.faces[face];
    size_t first = 0;
    for (size_t i = 1; i < loop.size(); i++)
        if (grid.vertex_point(loop[i])[2] + grid.vertex_point(loop[(i + 1) % 4])[2] < grid.vertex_point(loop[first])[2] + grid.vertex_point(loop[(first + 1) % 4])[2])
            first = i;

    const session_cpp::Point start = grid.vertex_point(loop[first]);
    const session_cpp::Point end = grid.vertex_point(loop[(first + 1) % 4]);
    const session_cpp::Vector along = compute_direction(start, end);
    const session_cpp::Vector normal = session_cpp::Vector(0.0, 0.0, 1.0).cross(along);
    const auto rise = [&](const session_cpp::Point& base, const std::string& above, double inward) {
        const session_cpp::Plane plane = compute_head_top(grid, above, dimensions);
        const double top = compute_lift(plane, grid.vertex_point(above))[2];
        const auto at = [&](double offset, double z) { return session_cpp::Point(base[0] + along[0] * offset * inward, base[1] + along[1] * offset * inward, z); };
        if (grid.graph.vertex_attribute(above, "head") != 1.0)
            return std::vector<session_cpp::Point>{at(dimensions.column / 2.0, base[2]), compute_lift(plane, at(dimensions.column / 2.0, top))};
        return std::vector<session_cpp::Point>{at(dimensions.column / 2.0, base[2]), at(dimensions.column / 2.0, top - dimensions.head), compute_lift(plane, at(dimensions.reach, top))};
    };

    std::vector<session_cpp::Point> outline = rise(end, loop[(first + 2) % 4], -1.0);
    const std::vector<session_cpp::Point> back = rise(start, loop[(first + 3) % 4], 1.0);
    outline.insert(outline.end(), back.rbegin(), back.rend());
    const session_cpp::Polyline middle = session_cpp::Polyline(outline).closed();

    return std::make_shared<wood_session::Plate>(middle.translated(normal * (-dimensions.wall / 2.0)), middle.translated(normal * (dimensions.wall / 2.0)), "wall");
}

} // namespace wood_grid
