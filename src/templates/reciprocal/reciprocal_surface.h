#pragma once
#include "session.h"
#include "nurbssurface.h"
#include "mesh.h"
#include "tolerance.h"
#include "line.h"
#include "intersection.h"
#include "xform.h"
#include "src/templates/folding/chevron.h"
#include "reciprocal_boundary.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace session_cpp;

/// Test surfaces for the reciprocal templates, the meshes on them and the frame ups they give.
namespace wood_reciprocal {

// ═══════════════════════════════════════════════════════════════════════════
// Surfaces
// ═══════════════════════════════════════════════════════════════════════════

/// The nx by ny quad grid over width by depth, lifted by height times two half sine waves.
inline Mesh sinusoidal_dome_mesh(
    int nx,
    int ny,
    double width,
    double depth,
    double height
) {

    std::vector<Point> points;

    for (int j = 0; j <= ny; j++)
        for (int i = 0; i <= nx; i++) {
            const double x = width * i / nx;
            const double y = depth * j / ny;
            const double z = height * std::sin(Tolerance::PI * x / width) * std::sin(Tolerance::PI * y / depth);
            points.push_back(Point(x, y, z));
        }

    std::vector<std::vector<size_t>> faces;

    for (int j = 0; j < ny; j++)
        for (int i = 0; i < nx; i++) {
            const size_t a = j * (nx + 1) + i;
            const size_t b = (j + 1) * (nx + 1) + i;
            faces.push_back({a, a + 1, b + 1, b});
        }

    return Mesh::from_vertices_and_faces(points, faces);
}

/// A hyperbolic paraboloid over width by depth, two opposite corners lifted by rise.
inline NurbsSurface hypar_surface(double width, double depth, double rise) {

    NurbsSurface surface;
    surface.create_raw(
        3,
        false,
        2,
        2,
        2,
        2
    );
    surface.set_nurbsknot(0, 0, 0.0);
    surface.set_nurbsknot(0, 1, width);
    surface.set_nurbsknot(1, 0, 0.0);
    surface.set_nurbsknot(1, 1, depth);
    surface.set_cv(0, 0, {0.0, 0.0, rise});
    surface.set_cv(1, 0, {width, 0.0, 0.0});
    surface.set_cv(0, 1, {0.0, depth, 0.0});
    surface.set_cv(1, 1, {width, depth, rise});
    return surface;
}

/// The height of the inner control points of a bicubic patch whose centre reaches rise.
inline double bicubic_inner_height(double rise) {
    return rise * 16.0 / 9.0;
}

/// A bicubic dome over a rectangle whose sides bow out by bulge, its centre at rise.
inline NurbsSurface pillow_dome_surface(
    double width,
    double depth,
    double rise,
    double bulge
) {

    NurbsSurface surface;
    surface.create_raw(
        3,
        false,
        4,
        4,
        4,
        4
    );
    const std::array<double, 4> us = {0.0, width / 3.0, 2.0 * width / 3.0, width};
    const std::array<double, 4> vs = {0.0, depth / 3.0, 2.0 * depth / 3.0, depth};
    const double inner = bicubic_inner_height(rise);

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            const bool edge_u = i == 0 || i == 3;
            const bool edge_v = j == 0 || j == 3;
            const double x = edge_u && !edge_v ? (i == 0 ? -bulge : width + bulge) : us[i];
            const double y = edge_v && !edge_u ? (j == 0 ? -bulge : depth + bulge) : vs[j];
            surface.set_cv(i, j, {x, y, edge_u || edge_v ? 0.0 : inner});
        }

    return surface;
}

/// The point on the circle of radius at angle, in z = 0.
inline Point circle_point(double radius, double angle) {
    return {radius * std::cos(angle), radius * std::sin(angle), 0.0};
}

/// The counter-clockwise unit tangent of a circle at angle.
inline Vector circle_tangent(double angle) {
    return {-std::sin(angle), std::cos(angle), 0.0};
}

/// A bicubic dome over the disc of radius, four cubic arcs as its boundary, its centre at rise.
inline NurbsSurface disc_dome_surface(double radius, double rise) {

    const double quarter = Tolerance::PI * 0.5;
    const double handle = radius * 4.0 / 3.0 * std::tan(quarter / 4.0); // the cubic handle of a quarter arc
    const double a00 = -3.0 * quarter / 2.0;
    const double a30 = a00 + quarter;
    const double a33 = a30 + quarter;
    const double a03 = a33 + quarter;

    std::array<std::array<Point, 4>, 4> cv;
    cv[0][0] = circle_point(radius, a00);
    cv[3][0] = circle_point(radius, a30);
    cv[3][3] = circle_point(radius, a33);
    cv[0][3] = circle_point(radius, a03);
    cv[1][0] = cv[0][0] + circle_tangent(a00) * handle;
    cv[2][0] = cv[3][0] - circle_tangent(a30) * handle;
    cv[3][1] = cv[3][0] + circle_tangent(a30) * handle;
    cv[3][2] = cv[3][3] - circle_tangent(a33) * handle;
    cv[2][3] = cv[3][3] + circle_tangent(a33) * handle;
    cv[1][3] = cv[0][3] - circle_tangent(a03) * handle;
    cv[0][2] = cv[0][3] + circle_tangent(a03) * handle;
    cv[0][1] = cv[0][0] - circle_tangent(a00) * handle;
    cv[1][1] = cv[1][0] + (cv[0][1] - cv[0][0]);
    cv[2][1] = cv[2][0] + (cv[3][1] - cv[3][0]);
    cv[1][2] = cv[1][3] + (cv[0][2] - cv[0][3]);
    cv[2][2] = cv[2][3] + (cv[3][2] - cv[3][3]);

    const double inner = bicubic_inner_height(rise);
    NurbsSurface surface;
    surface.create_raw(
        3,
        false,
        4,
        4,
        4,
        4
    );

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            const bool is_inner = i > 0 && i < 3 && j > 0 && j < 3;
            surface.set_cv(i, j, {cv[i][j][0], cv[i][j][1], is_inner ? inner : 0.0});
        }

    return surface;
}

/// A Scherk-like saddle on a plan bowed out by bulge, its two low sides level when flat_low_edges.
inline NurbsSurface scherk_surface(
    double width,
    double depth,
    double height,
    double bulge,
    bool flat_low_edges = true,
    double reach = 1.2
) {

    const int count = 6;
    const double scale = 1.0 / std::log(std::cos(reach)); // z = height at (u, v) = (0, reach)
    NurbsSurface surface;
    surface.create_raw(
        3,
        false,
        4,
        4,
        count,
        count
    );

    for (int i = 0; i < count; i++)
        for (int j = 0; j < count; j++) {
            const double s = (double)i / (count - 1);
            const double t = (double)j / (count - 1);
            const double u = -reach + 2.0 * reach * s;
            const double v = -reach + 2.0 * reach * t;
            const double bow_x = (i == 0 ? -1.0 : i == count - 1 ? 1.0 : 0.0) * bulge * std::sin(Tolerance::PI * t);
            const double bow_y = (j == 0 ? -1.0 : j == count - 1 ? 1.0 : 0.0) * bulge * std::sin(Tolerance::PI * s);
            const bool low_edge = flat_low_edges && (i == 0 || i == count - 1);
            const double z = low_edge ? -height : height * scale * std::log(std::cos(v) / std::cos(u));
            surface.set_cv(i, j, {width * s + bow_x, depth * t + bow_y, z});
        }

    return surface;
}

/// One of the serialized Annen arches from the data directory.
inline NurbsSurface annen_surface(const std::string& json_path, size_t index) {

    const std::vector<NurbsSurface> surfaces = wood_chevron::annen_surfaces(json_path);

    if (index >= surfaces.size())
        throw std::out_of_range(fmt::format("annen_surface: {} holds {} surfaces, index {} asked", json_path, surfaces.size(), index));

    return surfaces[index];
}

/// The surface with every boundary control row projected onto the plane fitted through its boundary curve.
inline NurbsSurface flatten_surface_sides(const NurbsSurface& surface) {

    const std::pair<double, double> du = surface.domain(0);
    const std::pair<double, double> dv = surface.domain(1);
    std::array<std::optional<Plane>, 4> planes;

    // planes: one through every curved boundary curve, none for a straight one
    for (int side = 0; side < 4; side++) {
        std::vector<Point> points;

        for (int k = 0; k <= 64; k++) {
            const double t = (double)k / 64;
            const double u = side == 0 ? du.first : side == 1 ? du.second : du.first + (du.second - du.first) * t;
            const double v = side == 2 ? dv.first : side == 3 ? dv.second : dv.first + (dv.second - dv.first) * t;
            points.push_back(surface.point_at(u, v));
        }

        const Line chord = Line::from_points(points.front(), points.back());
        double sagitta = 0.0;

        for (const Point& point : points)
            sagitta = std::max(sagitta, point.distance(chord.closest_point(point, true).second));

        if (sagitta >= 1e-6 * points.front().distance(points.back()))
            planes[side] = Plane::from_points_pca(points);
    }

    // flat: every boundary control point onto its side's plane, a corner onto the line of two
    NurbsSurface flat = surface;
    const int nu = surface.cv_count(0);
    const int nv = surface.cv_count(1);

    for (int i = 0; i < nu; i++)
        for (int j = 0; j < nv; j++) {
            const std::array<bool, 4> on_side = {i == 0, i == nu - 1, j == 0, j == nv - 1};
            std::vector<Plane> onto;

            for (int side = 0; side < 4; side++)
                if (on_side[side] && planes[side])
                    onto.push_back(*planes[side]);

            if (onto.empty())
                continue;

            Point point = surface.get_cv(i, j);
            Line crease;

            if (onto.size() == 1)
                point = point - onto[0].z_axis() * (point - onto[0].origin()).dot(onto[0].z_axis());
            else if (Intersection::plane_plane(onto[0], onto[1], crease))
                point = crease.closest_point(point, false).second;

            flat.set_cv(i, j, point);
        }

    return flat;
}

// ═══════════════════════════════════════════════════════════════════════════
// Meshes
// ═══════════════════════════════════════════════════════════════════════════

/// The cumulative length along one mid iso-curve, to pick parameters at equal spacing on the surface.
struct ArcLengthMap {
    std::vector<double> parameters; // Sampled parameters, ascending.
    std::vector<double> lengths; // Length from the first sample to each sampled parameter.
    double total = 0.0; // The length of the whole iso-curve.

    /// The map of the mid iso-curve of the surface along dir.
    static ArcLengthMap of(const NurbsSurface& surface, int dir, int samples = 400) {

        const std::pair<double, double> domain = surface.domain(dir);
        const std::pair<double, double> other = surface.domain(1 - dir);
        const double constant = (other.first + other.second) * 0.5;
        ArcLengthMap map;
        Point previous;

        for (int k = 0; k <= samples; k++) {
            const double t = domain.first + (domain.second - domain.first) * k / samples;
            const Point current = dir == 0 ? surface.point_at(t, constant) : surface.point_at(constant, t);
            map.total += k == 0 ? 0.0 : previous.distance(current);
            map.parameters.push_back(t);
            map.lengths.push_back(map.total);
            previous = current;
        }

        return map;
    }

    /// The parameter at which the iso-curve has covered length.
    double parameter_at(double length) const {

        if (length <= 0.0)
            return parameters.front();

        if (length >= total)
            return parameters.back();

        const size_t k = std::upper_bound(lengths.begin(), lengths.end(), length) - lengths.begin();
        const double span = lengths[k] - lengths[k - 1];
        const double fraction = span > 0.0 ? (length - lengths[k - 1]) / span : 0.0;
        return parameters[k - 1] + (parameters[k] - parameters[k - 1]) * fraction;
    }
};

/// Whether faces on the surface must be reversed so their normals point up at the domain centre.
inline bool faces_reversed_on(const NurbsSurface& surface) {

    const std::pair<double, double> du = surface.domain(0);
    const std::pair<double, double> dv = surface.domain(1);
    const Vector normal = surface.normal_at((du.first + du.second) * 0.5, (dv.first + dv.second) * 0.5);
    return normal[2] < 0.0;
}

/// The quad grid on the surface with cells about target_edge long, spaced at equal arc length.
inline Mesh quad_mesh_from_surface(const NurbsSurface& surface, double target_edge) {

    const ArcLengthMap along_u = ArcLengthMap::of(surface, 0);
    const ArcLengthMap along_v = ArcLengthMap::of(surface, 1);
    const int nu = std::max(1, (int)std::lround(along_u.total / target_edge));
    const int nv = std::max(1, (int)std::lround(along_v.total / target_edge));
    std::vector<Point> points;

    for (int i = 0; i <= nu; i++)
        for (int j = 0; j <= nv; j++) {
            const double u = along_u.parameter_at(along_u.total * i / nu);
            const double v = along_v.parameter_at(along_v.total * j / nv);
            points.push_back(surface.point_at(u, v));
        }

    const bool reversed = faces_reversed_on(surface);
    std::vector<std::vector<size_t>> faces;

    for (int i = 0; i < nu; i++)
        for (int j = 0; j < nv; j++) {
            const size_t a = i * (nv + 1) + j;
            const size_t b = (i + 1) * (nv + 1) + j;
            std::vector<size_t> face = {a, b, b + 1, a + 1};

            if (reversed)
                std::reverse(face.begin(), face.end());

            faces.push_back(face);
        }

    return Mesh::from_vertices_and_faces(points, faces);
}

/// A quad mesh over the disc of radius: a central square and four patches out to the circle, lifted onto the dome.
inline Mesh disc_quad_mesh(
    const NurbsSurface& dome,
    double radius,
    double target_edge,
    double square_ratio = 0.5
) {

    const double half = radius * square_ratio;
    const int n = std::max(2, (int)std::lround(2.0 * half / target_edge));
    const int m = std::max(1, (int)std::lround((radius - half) / target_edge));
    const double quarter = Tolerance::PI * 0.5;
    std::vector<std::vector<Point>> polygons;

    // square: the central n by n grid
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            const double x0 = -half + 2.0 * half * i / n;
            const double x1 = -half + 2.0 * half * (i + 1) / n;
            const double y0 = -half + 2.0 * half * j / n;
            const double y1 = -half + 2.0 * half * (j + 1) / n;
            polygons.push_back({{x0, y0, 0.0}, {x1, y0, 0.0}, {x1, y1, 0.0}, {x0, y1, 0.0}});
        }

    // patches: from every side of the square out to its quarter arc
    for (int side = 0; side < 4; side++) {
        const Xform turn = Xform::rotation_z(side * quarter);
        std::vector<std::vector<Point>> rows(m + 1, std::vector<Point>(n + 1));

        for (int i = 0; i <= n; i++) {
            const Point on_square(-half + 2.0 * half * i / n, half, 0.0);
            const Point on_arc = circle_point(radius, 3.0 * quarter / 2.0 - quarter * i / n);

            for (int j = 0; j <= m; j++) {
                const Point between = on_square + (on_arc - on_square) * ((double)j / m);
                rows[j][i] = between.transformed(turn);
            }
        }

        for (int i = 0; i < n; i++)
            for (int j = 0; j < m; j++)
                polygons.push_back({rows[j][i], rows[j][i + 1], rows[j + 1][i + 1], rows[j + 1][i]});
    }

    // lifted: the vertical through every point onto the dome, its closest point when it misses
    for (std::vector<Point>& polygon : polygons)
        for (Point& point : polygon) {
            const Line vertical = Line::from_points({point[0], point[1], -radius}, {point[0], point[1], radius});
            const std::vector<Point> hits = dome.intersections_with_line(vertical);
            point = hits.empty() ? dome.closest_point(point) : hits.front();
        }

    return Mesh::from_polylines(polygons, 1e-6);
}

/// A convex polygon clipped to the side of an axis-aligned line at limit, above it when keep_above.
inline std::vector<std::pair<double, double>> clip_polygon(
    const std::vector<std::pair<double, double>>& polygon,
    int axis,
    double limit,
    bool keep_above
) {

    std::vector<std::pair<double, double>> kept;
    const double sign = keep_above ? 1.0 : -1.0;

    for (size_t k = 0; k < polygon.size(); k++) {
        const std::pair<double, double>& a = polygon[k];
        const std::pair<double, double>& b = polygon[(k + 1) % polygon.size()];
        const double va = sign * ((axis == 0 ? a.first : a.second) - limit);
        const double vb = sign * ((axis == 0 ? b.first : b.second) - limit);
        const bool a_in = va >= -1e-9;
        const bool b_in = vb >= -1e-9;

        if (a_in)
            kept.push_back(a);

        if (a_in != b_in) {
            const double t = va / (va - vb);
            kept.emplace_back(a.first + (b.first - a.first) * t, a.second + (b.second - a.second) * t);
        }
    }

    return kept;
}

/// Staggered pointy-top hexagons of edge about target_edge filling width by depth, clipped to it.
inline std::vector<std::vector<std::pair<double, double>>> clipped_hex_lattice(double width, double depth, double target_edge) {

    const int rows = std::max(1, (int)std::lround(depth / (1.5 * target_edge)));
    const int columns = std::max(1, (int)std::lround(width / (std::sqrt(3.0) * target_edge)));
    const double edge = depth / (1.5 * rows);
    const double pitch = width / columns;
    std::vector<std::vector<std::pair<double, double>>> cells;

    for (int row = 0; row <= rows; row++)
        for (int column = -1; column <= columns; column++) {
            const double cx = column * pitch + (row % 2 == 1 ? pitch * 0.5 : 0.0);
            const double cy = row * 1.5 * edge;
            std::vector<std::pair<double, double>> polygon;

            for (int k = 0; k < 6; k++) {
                const double angle = Tolerance::PI * (0.5 + k / 3.0);
                polygon.emplace_back(cx + pitch * 0.5 * std::cos(angle) / std::cos(Tolerance::PI / 6.0), cy + edge * std::sin(angle));
            }

            polygon = clip_polygon(polygon, 0, 0.0, true);
            polygon = clip_polygon(polygon, 0, width, false);
            polygon = clip_polygon(polygon, 1, 0.0, true);
            polygon = clip_polygon(polygon, 1, depth, false);

            // cleaned: repeated corners dropped, slivers left out
            std::vector<std::pair<double, double>> cleaned;

            for (const std::pair<double, double>& q : polygon)
                if (cleaned.empty() || std::hypot(q.first - cleaned.back().first, q.second - cleaned.back().second) > 1e-6)
                    cleaned.push_back(q);

            if (cleaned.size() > 1 && std::hypot(cleaned.front().first - cleaned.back().first, cleaned.front().second - cleaned.back().second) <= 1e-6)
                cleaned.pop_back();

            double area = 0.0;

            for (size_t k = 0; k < cleaned.size(); k++) {
                const std::pair<double, double>& a = cleaned[k];
                const std::pair<double, double>& b = cleaned[(k + 1) % cleaned.size()];
                area += a.first * b.second - b.first * a.second;
            }

            if (cleaned.size() >= 3 && std::abs(area) > 1e-6 * width * depth)
                cells.push_back(cleaned);
        }

    return cells;
}

/// The plan cells welded within a thousandth: their plan points at z = 0 and faces, reversed when asked.
inline std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> welded_cells(const std::vector<std::vector<std::pair<double, double>>>& cells, bool reversed) {

    std::map<std::pair<long long, long long>, size_t> index_of;
    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;

    for (const std::vector<std::pair<double, double>>& cell : cells) {
        std::vector<size_t> face;

        for (const std::pair<double, double>& corner : cell) {
            const std::pair<long long, long long> key{std::llround(corner.first * 1000.0), std::llround(corner.second * 1000.0)};
            std::map<std::pair<long long, long long>, size_t>::iterator found = index_of.find(key);

            if (found == index_of.end()) {
                found = index_of.emplace(key, points.size()).first;
                points.push_back({corner.first, corner.second, 0.0});
            }

            if (face.empty() || face.back() != found->second)
                face.push_back(found->second);
        }

        if (face.size() > 1 && face.front() == face.back())
            face.pop_back();

        if (face.size() < 3)
            continue;

        if (reversed)
            std::reverse(face.begin(), face.end());

        faces.push_back(face);
    }

    return {points, faces};
}

/// A hexagonal mesh on the surface: the clipped lattice in its arc-length plane, lifted onto it.
inline Mesh hex_mesh_from_surface(const NurbsSurface& surface, double target_edge) {

    const ArcLengthMap along_u = ArcLengthMap::of(surface, 0);
    const ArcLengthMap along_v = ArcLengthMap::of(surface, 1);
    const std::vector<std::vector<std::pair<double, double>>> cells = clipped_hex_lattice(along_u.total, along_v.total, target_edge);
    std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> welded = welded_cells(cells, faces_reversed_on(surface));

    for (Point& point : welded.first)
        point = surface.point_at(along_u.parameter_at(point[0]), along_v.parameter_at(point[1]));

    return Mesh::from_vertices_and_faces(welded.first, welded.second);
}

/// The sinusoidal dome as a hexagonal mesh: the clipped lattice over its plan, lifted by its height function.
inline Mesh sinusoidal_dome_hex_mesh(
    double width,
    double depth,
    double height,
    double target_edge
) {

    const std::vector<std::vector<std::pair<double, double>>> cells = clipped_hex_lattice(width, depth, target_edge);
    std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> welded = welded_cells(cells, false);

    for (Point& point : welded.first)
        point[2] = height * std::sin(Tolerance::PI * point[0] / width) * std::sin(Tolerance::PI * point[1] / depth);

    return Mesh::from_vertices_and_faces(welded.first, welded.second);
}

/// True when the first dual corner turns before the second about the normal.
inline bool angle_less(const std::pair<double, std::pair<std::string, Point>>& p, const std::pair<double, std::pair<std::string, Point>>& q) {
    return p.first < q.first;
}

/// The dual of the triangulated quads: one cell per vertex through its triangle centroids, half cells along a straight side.
inline Mesh dual_hex_mesh(const Mesh& quads) {

    // triangles: every quad split along its first diagonal
    const std::vector<size_t> keys = quads.faces();
    std::vector<std::vector<size_t>> triangles;

    for (size_t key : keys) {
        const std::vector<size_t> face = quads.face_vertices(key).value();

        for (size_t k = 1; k + 1 < face.size(); k++)
            triangles.push_back({face[0], face[k], face[k + 1]});
    }

    std::map<size_t, std::vector<size_t>> fans; // vertex -> the triangles around it

    for (size_t t = 0; t < triangles.size(); t++)
        for (size_t vertex : triangles[t])
            fans[vertex].push_back(t);

    std::map<size_t, std::vector<size_t>> naked_neighbours; // boundary vertex -> the far ends of its naked edges
    const std::vector<std::pair<size_t, size_t>> naked = quads.edges_on_boundary();

    for (const std::pair<size_t, size_t>& edge : naked) {
        naked_neighbours[edge.first].push_back(edge.second);
        naked_neighbours[edge.second].push_back(edge.first);
    }

    // cells: the corners round every vertex sorted by angle about its normal
    std::vector<Point> points;
    std::map<std::string, size_t> index_of; // "t<i>", "e<u>_<v>", "v<k>" -> dual vertex
    std::vector<std::vector<size_t>> cells;

    for (const std::pair<const size_t, std::vector<size_t>>& fan : fans) {
        const size_t vertex = fan.first;
        const Point centre = quads.vertex_point(vertex).value();
        Vector normal = quads.vertex_normal(vertex).value_or(Vector(0, 0, 1));

        if (normal[2] < 0.0)
            normal = -normal;

        Vector x_axis;
        x_axis.perpendicular_to(normal);
        x_axis = x_axis.normalized();
        const Vector y_axis = normal.cross(x_axis);
        std::vector<std::pair<double, std::pair<std::string, Point>>> around;

        for (size_t t : fan.second) {
            const Point a = quads.vertex_point(triangles[t][0]).value();
            const Point b = quads.vertex_point(triangles[t][1]).value();
            const Point c = quads.vertex_point(triangles[t][2]).value();
            const Point centroid((a[0] + b[0] + c[0]) / 3.0, (a[1] + b[1] + c[1]) / 3.0, (a[2] + b[2] + c[2]) / 3.0);
            const Vector d = centroid - centre;
            around.push_back({std::atan2(d.dot(y_axis), d.dot(x_axis)), {fmt::format("t{}", t), centroid}});
        }

        const std::map<size_t, std::vector<size_t>>::const_iterator boundary = naked_neighbours.find(vertex);
        const bool on_boundary = boundary != naked_neighbours.end();

        if (on_boundary) {
            for (size_t other : boundary->second) {
                const Point mid = Point::mid_point(centre, quads.vertex_point(other).value());
                const Vector d = mid - centre;
                const std::string name = fmt::format("e{}_{}", std::min(vertex, other), std::max(vertex, other));
                around.push_back({std::atan2(d.dot(y_axis), d.dot(x_axis)), {name, mid}});
            }
        }

        std::sort(around.begin(), around.end(), angle_less);

        // a boundary cell opens at its widest gap and closes through the vertex only at a real corner
        if (on_boundary) {
            size_t widest = 0;
            double widest_gap = -1.0;

            for (size_t k = 0; k < around.size(); k++) {
                const double next = k + 1 < around.size() ? around[k + 1].first : around[0].first + 2.0 * Tolerance::PI;

                if (next - around[k].first > widest_gap) {
                    widest_gap = next - around[k].first;
                    widest = k;
                }
            }

            std::rotate(around.begin(), around.begin() + (widest + 1) % around.size(), around.end());
            const std::vector<size_t>& others = boundary->second;
            bool straight = false;

            if (others.size() == 2) {
                const Vector d0 = (quads.vertex_point(others[0]).value() - centre).normalized();
                const Vector d1 = (quads.vertex_point(others[1]).value() - centre).normalized();
                straight = d0.dot(d1) < -std::cos(5.0 * Tolerance::TO_RADIANS);
            }

            if (!straight)
                around.push_back({0.0, {fmt::format("v{}", vertex), centre}});
        }

        std::vector<size_t> cell;

        for (const std::pair<double, std::pair<std::string, Point>>& corner : around) {
            std::map<std::string, size_t>::iterator found = index_of.find(corner.second.first);

            if (found == index_of.end()) {
                found = index_of.emplace(corner.second.first, points.size()).first;
                points.push_back(corner.second.second);
            }

            cell.push_back(found->second);
        }

        if (cell.size() >= 3)
            cells.push_back(cell);
    }

    return Mesh::from_vertices_and_faces(points, cells);
}

// ═══════════════════════════════════════════════════════════════════════════
// Frame ups
// ═══════════════════════════════════════════════════════════════════════════

/// The side of the domain a parameter pair lies nearest to: u start, u end, v start, v end.
inline int nearest_domain_side(const NurbsSurface& surface, double u, double v) {

    const std::pair<double, double> du = surface.domain(0);
    const std::pair<double, double> dv = surface.domain(1);
    const double span_u = du.second - du.first;
    const double span_v = dv.second - dv.first;
    const std::array<double, 4> distances = {(u - du.first) / span_u, (du.second - u) / span_u, (v - dv.first) / span_v, (dv.second - v) / span_v};
    return (int)(std::min_element(distances.begin(), distances.end()) - distances.begin());
}

/// One plane per side of the surface, fitted through the side's boundary vertices, turned to its average face normal.
inline std::array<Plane, 4> side_planes(const Mesh& mesh, const NurbsSurface& surface) {

    const std::pair<double, double> du = surface.domain(0);
    const std::pair<double, double> dv = surface.domain(1);
    const double span_u = du.second - du.first;
    const double span_v = dv.second - dv.first;
    const std::map<std::pair<size_t, size_t>, Vector> edge_normals = owner_normal_ups(mesh);
    const std::vector<std::pair<size_t, size_t>> naked = mesh.edges_on_boundary();

    // sides: every boundary vertex on its nearest side, a corner within 2 percent on both
    std::set<size_t> boundary;

    for (const std::pair<size_t, size_t>& edge : naked) {
        boundary.insert(edge.first);
        boundary.insert(edge.second);
    }

    std::array<std::vector<size_t>, 4> sides;

    for (size_t vertex : boundary) {
        const std::pair<double, double> uv = surface.closest_parameters(mesh.vertex_point(vertex).value());
        const std::array<double, 4> distances = {(uv.first - du.first) / span_u, (du.second - uv.first) / span_u, (uv.second - dv.first) / span_v, (dv.second - uv.second) / span_v};
        const int nearest = (int)(std::min_element(distances.begin(), distances.end()) - distances.begin());

        for (int side = 0; side < 4; side++)
            if (side == nearest || distances[side] <= 0.02)
                sides[side].push_back(vertex);
    }

    // planes: the fit, or the plane through a straight side's line holding its average normal
    std::array<Plane, 4> planes;

    for (int side = 0; side < 4; side++) {
        std::vector<Point> points;
        const std::set<size_t> members(sides[side].begin(), sides[side].end());
        Vector average(0, 0, 0);

        for (size_t vertex : sides[side])
            points.push_back(mesh.vertex_point(vertex).value());

        for (const std::pair<const std::pair<size_t, size_t>, Vector>& normal : edge_normals)
            if (members.count(normal.first.first) && members.count(normal.first.second))
                average += normal.second;

        if (average.is_zero())
            average = Vector(0, 0, 1);

        if (points.size() < 3) {
            planes[side] = Plane::from_point_normal(points.empty() ? Point(0, 0, 0) : points.front(), average);
            continue;
        }

        Point a = points.front();
        Point b = points.front();

        for (const Point& p : points)
            for (const Point& q : points)
                if (p.distance(q) > a.distance(b)) {
                    a = p;
                    b = q;
                }

        const Line chord = Line::from_points(a, b);
        double sagitta = 0.0;

        for (const Point& p : points)
            sagitta = std::max(sagitta, p.distance(chord.closest_point(p, true).second));

        const Plane fitted = Plane::from_points_pca(points);
        Vector normal = fitted.z_axis();

        if (sagitta < 1e-6 * a.distance(b) || normal.is_zero())
            normal = perpendicular_part(average, chord.to_direction());

        if (normal.dot(average) < 0.0)
            normal = -normal;

        planes[side] = Plane::from_point_normal(fitted.origin(), normal);
    }

    return planes;
}

/// One up per naked edge, one tilt per side of the surface out of its plane, or from side_vectors when given.
inline std::map<std::pair<size_t, size_t>, Vector> side_tilt_boundary_ups(
    const Mesh& mesh,
    const NurbsSurface& surface,
    const std::optional<std::array<Vector, 4>>& side_vectors = std::nullopt
) {

    const std::array<Plane, 4> planes = side_planes(mesh, surface);
    const std::map<std::pair<size_t, size_t>, Vector> edge_normals = owner_normal_ups(mesh);
    const std::vector<std::pair<size_t, size_t>> naked = mesh.edges_on_boundary();
    std::array<std::vector<std::pair<size_t, size_t>>, 4> keys;
    std::array<std::vector<Vector>, 4> directions;
    std::array<std::vector<Vector>, 4> normals;
    std::array<Vector, 4> averages = {Vector(0, 0, 0), Vector(0, 0, 0), Vector(0, 0, 0), Vector(0, 0, 0)};

    // sides: every naked edge on its side with its direction and the surface normal, or the given vector
    for (const std::pair<size_t, size_t>& edge : naked) {
        const Point a = mesh.vertex_point(edge.first).value();
        const Point b = mesh.vertex_point(edge.second).value();

        if ((b - a).is_zero())
            continue;

        const std::pair<double, double> uv = surface.closest_parameters(Point::mid_point(a, b));
        const int side = nearest_domain_side(surface, uv.first, uv.second);
        const std::pair<size_t, size_t> key = edge_key(edge.first, edge.second);
        const Vector& face = edge_normals.at(key);
        Vector normal = side_vectors ? (*side_vectors)[side] : surface.normal_at(uv.first, uv.second);

        if (normal.dot(face) < 0.0)
            normal = -normal;

        keys[side].push_back(key);
        directions[side].push_back((b - a).normalized());
        normals[side].push_back(normal);
        averages[side] += face;
    }

    // tilt: the mean angle of the normals out of the side's plane, the same for every edge on it
    std::map<std::pair<size_t, size_t>, Vector> ups;

    for (int side = 0; side < 4; side++) {
        if (keys[side].empty())
            continue;

        const Vector& side_normal = planes[side].z_axis();
        const Vector average = averages[side].is_zero() ? side_normal : averages[side].normalized();
        const Vector n = side_normal.dot(average) < 0.0 ? -side_normal : side_normal;
        double sin_sum = 0.0;
        double cos_sum = 0.0;

        for (size_t k = 0; k < keys[side].size(); k++) {
            const Vector across = n.cross(directions[side][k]).normalized();
            sin_sum += normals[side][k].dot(across);
            cos_sum += normals[side][k].dot(n);
        }

        const double tilt = std::atan2(sin_sum, cos_sum);

        for (size_t k = 0; k < keys[side].size(); k++) {
            const Vector across = n.cross(directions[side][k]).normalized();
            ups[keys[side][k]] = (n * std::cos(tilt) + across * std::sin(tilt)).normalized();
        }
    }

    return ups;
}

/// The through priority per naked edge: 1 on the u sides of the domain when u_sides_through, 0 on the others.
inline std::map<std::pair<size_t, size_t>, int> through_side_priority(const Mesh& mesh, const NurbsSurface& surface, bool u_sides_through = true) {

    std::map<std::pair<size_t, size_t>, int> priority;
    const std::vector<std::pair<size_t, size_t>> naked = mesh.edges_on_boundary();

    for (const std::pair<size_t, size_t>& edge : naked) {
        const Point mid = Point::mid_point(mesh.vertex_point(edge.first).value(), mesh.vertex_point(edge.second).value());
        const std::pair<double, double> uv = surface.closest_parameters(mid);
        const int side = nearest_domain_side(surface, uv.first, uv.second);
        const bool u_side = side == 0 || side == 1;
        priority[edge_key(edge.first, edge.second)] = u_side == u_sides_through ? 1 : 0;
    }

    return priority;
}

}
