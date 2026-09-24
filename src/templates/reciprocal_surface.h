#pragma once
#include "session.h"
#include "nurbssurface.h"
#include "mesh.h"
#include "tolerance.h"
#include "line.h"
#include "intersection.h"
#include "xform.h"
#include "chevron.h"
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

/// Test surfaces for the reciprocal templates and the quad meshes on them: the grid counts follow the surface's own proportions so the cells stay near square whatever the parameter domain, and a disc gets a five-patch layout instead of one grid, whose corner cells would flatten to 180 degrees on a smooth boundary.
namespace wood_reciprocal {

/// The sinusoidal dome the reciprocal templates were written on: an nx by ny quad grid over width by depth, lifted by height times the product of two half sine waves.
inline Mesh sinusoidal_dome_mesh(int nx, int ny, double width, double depth, double height)
{

    std::vector<Point> points;
    points.reserve((nx + 1) * (ny + 1));
    for (int j = 0; j <= ny; j++)
        for (int i = 0; i <= nx; i++) {
            double x = width * i / nx;
            double y = depth * j / ny;
            double z = height * std::sin(Tolerance::PI * x / width) * std::sin(Tolerance::PI * y / depth);
            points.push_back(Point(x, y, z));
        }

    std::vector<std::vector<size_t>> faces;
    faces.reserve(nx * ny);
    for (int j = 0; j < ny; j++)
        for (int i = 0; i < nx; i++)
            faces.push_back({(size_t)(j * (nx + 1) + i), (size_t)(j * (nx + 1) + i + 1),
                             (size_t)((j + 1) * (nx + 1) + i + 1), (size_t)((j + 1) * (nx + 1) + i)});

    return Mesh::from_vertices_and_faces(points, faces);
}

/// A hyperbolic paraboloid: the bilinear surface over a width by depth rectangle with two opposite corners lifted by rise, so every iso-curve is a straight line and the domain is in model units.
inline NurbsSurface hypar_surface(double width, double depth, double rise)
{

    NurbsSurface surface;
    surface.create_raw(3, false, 2, 2, 2, 2);
    surface.set_nurbsknot(0, 0, 0.0);
    surface.set_nurbsknot(0, 1, width);
    surface.set_nurbsknot(1, 0, 0.0);
    surface.set_nurbsknot(1, 1, depth);
    surface.set_cv(0, 0, Point(0.0, 0.0, rise));
    surface.set_cv(1, 0, Point(width, 0.0, 0.0));
    surface.set_cv(0, 1, Point(0.0, depth, 0.0));
    surface.set_cv(1, 1, Point(width, depth, rise));

    return surface;
}

/// The height the four inner control points of a bicubic Bezier patch need for its centre to reach rise, the boundary control points staying at zero.
inline double bicubic_inner_height(double rise)
{
    return rise * 16.0 / 9.0;
}

/// A bicubic dome over a pillow: the rectangle's corners kept, the boundary control points between them pushed outwards by bulge so each side bows out in a cubic arc and the corners stay proper corners, the inner control points lifted so the centre reaches rise.
inline NurbsSurface pillow_dome_surface(double width, double depth, double rise, double bulge)
{

    NurbsSurface surface;
    surface.create_raw(3, false, 4, 4, 4, 4);
    const double us[4] = {0.0, width / 3.0, 2.0 * width / 3.0, width};
    const double vs[4] = {0.0, depth / 3.0, 2.0 * depth / 3.0, depth};
    const double inner = bicubic_inner_height(rise);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            bool edge_u = i == 0 || i == 3;
            bool edge_v = j == 0 || j == 3;
            double x = edge_u && !edge_v ? (i == 0 ? -bulge : width + bulge) : us[i];
            double y = edge_v && !edge_u ? (j == 0 ? -bulge : depth + bulge) : vs[j];
            double z = edge_u || edge_v ? 0.0 : inner;
            surface.set_cv(i, j, Point(x, y, z));
        }

    return surface;
}

/// The point on the circle of the given radius at the angle, in the plane z = 0.
inline Point circle_point(double radius, double angle)
{
    return Point(radius * std::cos(angle), radius * std::sin(angle), 0.0);
}

/// The counter-clockwise unit tangent of the circle at the angle.
inline Vector circle_tangent(double angle)
{
    return Vector(-std::sin(angle), std::cos(angle), 0.0);
}

/// A bicubic dome over a disc: the domain corners on the circle at the four diagonal directions, each boundary a cubic arc between two of them, so the plan boundary is a circle that stays smooth across the domain corners; the inner control points lifted so the centre reaches rise.
inline NurbsSurface disc_dome_surface(double radius, double rise)
{

    const double quarter = Tolerance::PI * 0.5;
    const double handle = radius * 4.0 / 3.0 * std::tan(quarter / 4.0);  // the cubic Bezier handle of a quarter arc
    const double a00 = -3.0 * quarter / 2.0;  // the angle of corner (0, 0); the others follow counter-clockwise
    const double a30 = a00 + quarter;
    const double a33 = a30 + quarter;
    const double a03 = a33 + quarter;

    std::vector<std::vector<Point>> cv(4, std::vector<Point>(4));
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
    surface.create_raw(3, false, 4, 4, 4, 4);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            bool is_inner = i > 0 && i < 3 && j > 0 && j < 3;
            surface.set_cv(i, j, Point(cv[i][j][0], cv[i][j][1], is_inner ? inner : 0.0));
        }

    return surface;
}

/// The plan point lifted onto the surface: where the vertical through it meets the surface, the closest surface point when it misses.
inline Point lifted_onto(const NurbsSurface& surface, const Point& plan, double z_low, double z_high)
{

    std::vector<Point> hits = surface.intersections_with_line(Line::from_points(Point(plan[0], plan[1], z_low), Point(plan[0], plan[1], z_high)));
    if (!hits.empty())
        return hits.front();

    return surface.closest_point(plan);
}

/// The one point of the disc layout rotated by quarter turns about the origin.
inline Point quarter_turned(const Point& p, int quarters)
{
    return p.transformed(Xform::rotation_z(quarters * Tolerance::PI * 0.5));
}

/// A quad mesh over the disc of the given radius with five patches: a central square of half side square_ratio times the radius and four patches between its sides and the circle, so every boundary vertex belongs to two faces and no face has two boundary edges; the cells are about target_edge long and the mesh is lifted onto the dome surface.
inline Mesh disc_quad_mesh(const NurbsSurface& dome, double radius, double target_edge, double square_ratio = 0.5)
{

    const double half = radius * square_ratio;
    const int n = std::max(2, (int)std::lround(2.0 * half / target_edge));
    const int m = std::max(1, (int)std::lround((radius - half) / target_edge));
    const double quarter = Tolerance::PI * 0.5;
    const double z_low = -radius, z_high = radius;

    std::vector<std::vector<Point>> polygons;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            double x0 = -half + 2.0 * half * i / n, x1 = -half + 2.0 * half * (i + 1) / n;
            double y0 = -half + 2.0 * half * j / n, y1 = -half + 2.0 * half * (j + 1) / n;
            polygons.push_back({Point(x0, y0, 0.0), Point(x1, y0, 0.0), Point(x1, y1, 0.0), Point(x0, y1, 0.0)});
        }

    for (int side = 0; side < 4; side++) {
        std::vector<std::vector<Point>> rows(m + 1, std::vector<Point>(n + 1));  // rows[j][i]: j from the square's top side out to the arc
        for (int i = 0; i <= n; i++) {
            Point on_square(-half + 2.0 * half * i / n, half, 0.0);
            double angle = 3.0 * quarter / 2.0 - quarter * i / n;  // the arc from the top-left to the top-right diagonal
            Point on_arc = circle_point(radius, angle);
            for (int j = 0; j <= m; j++)
                rows[j][i] = quarter_turned(on_square + (on_arc - on_square) * ((double)j / m), side);
        }

        for (int i = 0; i < n; i++)
            for (int j = 0; j < m; j++)
                polygons.push_back({rows[j][i], rows[j][i + 1], rows[j + 1][i + 1], rows[j + 1][i]});
    }

    for (std::vector<Point>& polygon : polygons)
        for (Point& point : polygon)
            point = lifted_onto(dome, point, z_low, z_high);

    return Mesh::from_polylines(polygons, 1e-6);
}

/// A Scherk-like saddle: a bicubic patch with six by six control points on z = height * ln(cos v / cos u), u and v over [-reach, reach], on a plan whose four sides bow outwards by bulge, so every boundary curve bends both in plan and in elevation. The two low sides dip to -height; with flat_low_edges their control rows are pinned there, so those two edges lie level on the ground, still bowed in plan.
inline NurbsSurface scherk_surface(double width, double depth, double height, double bulge, bool flat_low_edges = true, double reach = 1.2)
{

    const int count = 6;
    const double scale = 1.0 / std::log(std::cos(reach));  // ln(cos reach) is negative: z = height at (u, v) = (0, reach)
    NurbsSurface surface;
    surface.create_raw(3, false, 4, 4, count, count);
    for (int i = 0; i < count; i++)
        for (int j = 0; j < count; j++) {
            double s = (double)i / (count - 1);
            double t = (double)j / (count - 1);
            double u = -reach + 2.0 * reach * s;
            double v = -reach + 2.0 * reach * t;
            double bow_x = (i == 0 ? -1.0 : i == count - 1 ? 1.0 : 0.0) * bulge * std::sin(Tolerance::PI * t);  // the u sides bow out along x
            double bow_y = (j == 0 ? -1.0 : j == count - 1 ? 1.0 : 0.0) * bulge * std::sin(Tolerance::PI * s);  // the v sides bow out along y
            double z = height * scale * std::log(std::cos(v) / std::cos(u));
            if (flat_low_edges && (i == 0 || i == count - 1))
                z = -height;

            surface.set_cv(i, j, Point(width * s + bow_x, depth * t + bow_y, z));
        }

    return surface;
}

/// One up per naked edge of the mesh from the surface normal at the point of the surface closest to the edge's midpoint, turned to point up: the frame that follows the NURBS surface rather than the mesh's faces.
inline std::map<std::pair<size_t, size_t>, Vector> surface_normal_boundary_ups(const Mesh& mesh, const NurbsSurface& surface)
{

    std::map<std::pair<size_t, size_t>, Vector> ups;
    for (const auto& [u, v] : mesh.edges_on_boundary()) {
        Point mid = Point::mid_point(mesh.vertex_point(u).value(), mesh.vertex_point(v).value());
        std::pair<double, double> uv = surface.closest_parameters(mid);
        Vector normal = surface.normal_at(uv.first, uv.second);
        ups[wood_reciprocal::edge_key(u, v)] = normal[2] < 0.0 ? -normal : normal;
    }

    return ups;
}

/// The side of the surface's domain a parameter pair lies nearest to: 0 for u at its start, 1 for u at its end, 2 for v at its start, 3 for v at its end.
inline int nearest_domain_side(const NurbsSurface& surface, double u, double v)
{

    std::pair<double, double> du = surface.domain(0);
    std::pair<double, double> dv = surface.domain(1);
    double span_u = du.second - du.first, span_v = dv.second - dv.first;
    std::array<double, 4> distances = {(u - du.first) / span_u, (du.second - u) / span_u, (v - dv.first) / span_v, (dv.second - v) / span_v};
    return (int)(std::min_element(distances.begin(), distances.end()) - distances.begin());
}

/// One up per naked edge, the same for every edge on the same boundary curve of the surface: the mesh edges are sorted onto the four sides of the domain by the surface parameters of their midpoints, and each side takes the average of its edges' owning face normals, turned to point up. Each side of the frame is then one constant section and only the four corners carry a mitre step.
inline std::map<std::pair<size_t, size_t>, Vector> side_average_boundary_ups(const Mesh& mesh, const NurbsSurface& surface)
{

    std::vector<size_t> fkeys = mesh.faces();
    std::vector<std::vector<size_t>> faces(fkeys.size());
    std::vector<Vector> face_normals(fkeys.size());
    for (size_t fi = 0; fi < fkeys.size(); fi++) {
        faces[fi] = mesh.face_vertices(fkeys[fi]).value();
        face_normals[fi] = mesh.face_normal(fkeys[fi]).value_or(Vector(0, 0, 1));
    }

    std::map<std::pair<size_t, size_t>, Vector> edge_normals = wood_reciprocal::owner_normal_ups(wood_reciprocal::edge_owners(faces), face_normals);
    std::array<Vector, 4> sums = {Vector(0, 0, 0), Vector(0, 0, 0), Vector(0, 0, 0), Vector(0, 0, 0)};
    std::map<std::pair<size_t, size_t>, int> side_of;
    for (const auto& [key, normal] : edge_normals) {
        Point mid = Point::mid_point(mesh.vertex_point(key.first).value(), mesh.vertex_point(key.second).value());
        std::pair<double, double> uv = surface.closest_parameters(mid);
        int side = nearest_domain_side(surface, uv.first, uv.second);
        side_of[key] = side;
        sums[side] += normal;
    }

    std::map<std::pair<size_t, size_t>, Vector> ups;
    for (const auto& [key, side] : side_of)
        if (!sums[side].is_zero())
            ups[key] = sums[side].normalized();

    return ups;
}

/// The boundary vertices of the mesh sorted onto the four sides of the surface's domain by the parameters of their closest surface point; a vertex within corner_fraction of two sides is a corner and belongs to both.
inline std::array<std::vector<size_t>, 4> boundary_vertices_by_side(const Mesh& mesh, const NurbsSurface& surface, double corner_fraction = 0.02)
{

    std::pair<double, double> du = surface.domain(0);
    std::pair<double, double> dv = surface.domain(1);
    double span_u = du.second - du.first, span_v = dv.second - dv.first;
    std::set<size_t> boundary;
    for (const auto& [u, v] : mesh.edges_on_boundary()) {
        boundary.insert(u);
        boundary.insert(v);
    }

    std::array<std::vector<size_t>, 4> sides;
    for (size_t vertex : boundary) {
        std::pair<double, double> uv = surface.closest_parameters(mesh.vertex_point(vertex).value());
        std::array<double, 4> distances = {(uv.first - du.first) / span_u, (du.second - uv.first) / span_u, (uv.second - dv.first) / span_v, (dv.second - uv.second) / span_v};
        int nearest = (int)(std::min_element(distances.begin(), distances.end()) - distances.begin());
        for (int side = 0; side < 4; side++)
            if (side == nearest || distances[side] <= corner_fraction)
                sides[side].push_back(vertex);
    }

    return sides;
}

/// One plane per side of the surface: the least-squares plane through the side's boundary vertices, its normal turned towards the side's average face normal; a straight side, whose fit is ambiguous, takes the plane through its line that contains that average normal.
inline std::array<Plane, 4> side_planes(const Mesh& mesh, const NurbsSurface& surface)
{

    std::array<std::vector<size_t>, 4> sides = boundary_vertices_by_side(mesh, surface);
    std::map<std::pair<size_t, size_t>, Vector> edge_normals = wood_reciprocal::owner_normal_ups(mesh);
    std::array<Plane, 4> planes;
    for (int side = 0; side < 4; side++) {
        std::vector<Point> points;
        for (size_t vertex : sides[side])
            points.push_back(mesh.vertex_point(vertex).value());

        Vector average(0, 0, 0);
        std::set<size_t> members(sides[side].begin(), sides[side].end());
        for (const auto& [key, normal] : edge_normals)
            if (members.count(key.first) && members.count(key.second))
                average += normal;

        if (average.is_zero())
            average = Vector(0, 0, 1);

        if (points.size() < 3) {
            planes[side] = Plane::from_point_normal(points.empty() ? Point(0, 0, 0) : points.front(), average);
            continue;
        }

        Point a = points.front(), b = points.front();
        for (const Point& p : points)
            for (const Point& q : points)
                if (p.distance(q) > a.distance(b)) {
                    a = p;
                    b = q;
                }

        Line chord = Line::from_points(a, b);
        double sagitta = 0.0;
        for (const Point& p : points)
            sagitta = std::max(sagitta, p.distance(chord.closest_point(p, true).second));

        Plane fitted = Plane::from_points_pca(points);
        Vector normal = fitted.z_axis();
        if (sagitta < 1e-6 * a.distance(b) || normal.is_zero())
            normal = wood_reciprocal::perpendicular_part(average, chord.to_direction());  // a straight side: the plane through its line that holds the average normal

        if (normal.dot(average) < 0.0)
            normal = -normal;

        planes[side] = Plane::from_point_normal(fitted.origin(), normal);
    }

    return planes;
}

/// One up per naked edge from the plane fitted through its side of the surface: either the plane's normal, a rib lying flat in the plane, or the in-plane direction perpendicular to the edge, a rib standing in the plane; both close every mitre of a planar side exactly, and each side takes the one closer to its average face normal, so the frame stands like the interior beams wherever it can.
inline std::map<std::pair<size_t, size_t>, Vector> side_plane_boundary_ups(const Mesh& mesh, const NurbsSurface& surface)
{

    std::array<Plane, 4> planes = side_planes(mesh, surface);
    std::map<std::pair<size_t, size_t>, Vector> edge_normals = wood_reciprocal::owner_normal_ups(mesh);
    std::array<std::vector<std::pair<std::pair<size_t, size_t>, Vector>>, 4> edges_by_side;  // per side: the naked edge and its unit direction
    std::array<Vector, 4> averages = {Vector(0, 0, 0), Vector(0, 0, 0), Vector(0, 0, 0), Vector(0, 0, 0)};
    for (const auto& [u, v] : mesh.edges_on_boundary()) {
        Point a = mesh.vertex_point(u).value(), b = mesh.vertex_point(v).value();
        if ((b - a).is_zero())
            continue;

        std::pair<double, double> uv = surface.closest_parameters(Point::mid_point(a, b));
        int side = nearest_domain_side(surface, uv.first, uv.second);
        edges_by_side[side].emplace_back(wood_reciprocal::edge_key(u, v), (b - a).normalized());
        averages[side] += edge_normals[wood_reciprocal::edge_key(u, v)];
    }

    std::map<std::pair<size_t, size_t>, Vector> ups;
    for (int side = 0; side < 4; side++) {
        if (edges_by_side[side].empty())
            continue;

        Vector average = averages[side].is_zero() ? planes[side].z_axis() : averages[side].normalized();
        Vector flat = planes[side].z_axis().dot(average) < 0.0 ? -planes[side].z_axis() : planes[side].z_axis();
        double flat_score = std::abs(flat.dot(average));
        double standing_score = 0.0;
        for (const auto& [key, dir] : edges_by_side[side])
            standing_score += std::abs(planes[side].z_axis().cross(dir).dot(average));

        standing_score /= (double)edges_by_side[side].size();
        for (const auto& [key, dir] : edges_by_side[side]) {
            if (flat_score >= standing_score) {
                ups[key] = flat;
                continue;
            }

            Vector standing = planes[side].z_axis().cross(dir).normalized();
            ups[key] = standing.dot(average) < 0.0 ? -standing : standing;
        }
    }

    return ups;
}

/// The mesh with every boundary vertex moved onto the plane fitted through its side, a corner vertex onto the line where its two side planes meet: each side of the boundary becomes a curve in one plane, the interior stays on the surface.
inline Mesh flatten_boundary_sides(const Mesh& mesh, const NurbsSurface& surface)
{

    std::array<std::vector<size_t>, 4> sides = boundary_vertices_by_side(mesh, surface);
    std::array<Plane, 4> planes = side_planes(mesh, surface);
    std::map<size_t, std::vector<int>> sides_of;
    for (int side = 0; side < 4; side++)
        for (size_t vertex : sides[side])
            sides_of[vertex].push_back(side);

    std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data = mesh.to_vertices_and_faces();
    std::vector<size_t> keys = mesh.vertices();
    for (size_t k = 0; k < keys.size(); k++) {
        std::map<size_t, std::vector<int>>::const_iterator found = sides_of.find(keys[k]);
        if (found == sides_of.end())
            continue;

        Point& point = data.first[k];
        if (found->second.size() == 1) {
            const Plane& plane = planes[found->second[0]];
            point = point - plane.z_axis() * (point - plane.origin()).dot(plane.z_axis());
            continue;
        }

        Line crease;
        if (Intersection::plane_plane(planes[found->second[0]], planes[found->second[1]], crease))
            point = crease.closest_point(point, false).second;
    }

    return Mesh::from_vertices_and_faces(data.first, data.second);
}

/// The plane through one boundary curve of the surface, fitted through samples along it; none for a straight boundary, which is planar already.
inline std::optional<Plane> boundary_curve_plane(const NurbsSurface& surface, int side, int samples = 64)
{

    std::pair<double, double> du = surface.domain(0);
    std::pair<double, double> dv = surface.domain(1);
    std::vector<Point> points;
    for (int k = 0; k <= samples; k++) {
        double t = (double)k / samples;
        double u = side == 0 ? du.first : side == 1 ? du.second : du.first + (du.second - du.first) * t;
        double v = side == 2 ? dv.first : side == 3 ? dv.second : dv.first + (dv.second - dv.first) * t;
        points.push_back(surface.point_at(u, v));
    }

    Line chord = Line::from_points(points.front(), points.back());
    double sagitta = 0.0;
    for (const Point& point : points)
        sagitta = std::max(sagitta, point.distance(chord.closest_point(point, true).second));

    if (sagitta < 1e-6 * points.front().distance(points.back()))
        return std::nullopt;

    return Plane::from_points_pca(points);
}

/// The surface with each boundary control row projected onto the plane fitted through its boundary curve, the corner control points onto the line where two such planes meet: every boundary curve then lies in one plane, the surface inside follows, and a mesh sampled from it matches it.
inline NurbsSurface flatten_surface_sides(const NurbsSurface& surface)
{

    std::array<std::optional<Plane>, 4> planes;
    for (int side = 0; side < 4; side++)
        planes[side] = boundary_curve_plane(surface, side);

    NurbsSurface flat = surface;
    int nu = surface.cv_count(0), nv = surface.cv_count(1);
    for (int i = 0; i < nu; i++)
        for (int j = 0; j < nv; j++) {
            std::vector<int> sides;
            if (i == 0)
                sides.push_back(0);
            if (i == nu - 1)
                sides.push_back(1);
            if (j == 0)
                sides.push_back(2);
            if (j == nv - 1)
                sides.push_back(3);
            std::vector<Plane> onto;
            for (int side : sides)
                if (planes[side])
                    onto.push_back(*planes[side]);

            if (onto.empty())
                continue;

            Point point = surface.get_cv(i, j);
            if (onto.size() == 1) {
                point = point - onto[0].z_axis() * (point - onto[0].origin()).dot(onto[0].z_axis());
            } else {
                Line crease;
                if (Intersection::plane_plane(onto[0], onto[1], crease))
                    point = crease.closest_point(point, false).second;
            }

            flat.set_cv(i, j, point);
        }

    return flat;
}

/// One up per naked edge with one tilt per side: in the plane fitted through the side, every beam's up is the plane normal turned towards the shell by the same angle, the mean angle between the surface normal and that plane along the side, or, when side_vectors are given, one per side in the order u start, u end, v start, v end, the angle of that vector instead. Consecutive sections are exact mirror images across each mitre, so every joint on the side closes; only the corners carry a step.
inline std::map<std::pair<size_t, size_t>, Vector> side_tilt_boundary_ups(const Mesh& mesh, const NurbsSurface& surface,
                                                                         const std::optional<std::array<Vector, 4>>& side_vectors = std::nullopt)
{

    std::array<Plane, 4> planes = side_planes(mesh, surface);
    std::map<std::pair<size_t, size_t>, Vector> edge_normals = wood_reciprocal::owner_normal_ups(mesh);
    struct SideEdge { std::pair<size_t, size_t> key; Vector dir; Vector normal; };  // a naked edge, its unit direction and the surface normal at its midpoint
    std::array<std::vector<SideEdge>, 4> edges_by_side;
    std::array<Vector, 4> averages = {Vector(0, 0, 0), Vector(0, 0, 0), Vector(0, 0, 0), Vector(0, 0, 0)};
    for (const auto& [u, v] : mesh.edges_on_boundary()) {
        Point a = mesh.vertex_point(u).value(), b = mesh.vertex_point(v).value();
        if ((b - a).is_zero())
            continue;

        Point mid = Point::mid_point(a, b);
        std::pair<double, double> uv = surface.closest_parameters(mid);
        int side = nearest_domain_side(surface, uv.first, uv.second);
        Vector normal = side_vectors ? (*side_vectors)[side] : surface.normal_at(uv.first, uv.second);  // a given vector stands in for the surface normal on its side
        const Vector& face = edge_normals[wood_reciprocal::edge_key(u, v)];
        if (normal.dot(face) < 0.0)
            normal = -normal;

        edges_by_side[side].push_back({wood_reciprocal::edge_key(u, v), (b - a).normalized(), normal});
        averages[side] += face;
    }

    std::map<std::pair<size_t, size_t>, Vector> ups;
    for (int side = 0; side < 4; side++) {
        if (edges_by_side[side].empty())
            continue;

        Vector average = averages[side].is_zero() ? planes[side].z_axis() : averages[side].normalized();
        Vector n = planes[side].z_axis().dot(average) < 0.0 ? -planes[side].z_axis() : planes[side].z_axis();

        double sin_sum = 0.0, cos_sum = 0.0;
        for (const SideEdge& edge : edges_by_side[side]) {
            Vector across = n.cross(edge.dir).normalized();  // in the plane, perpendicular to the edge; the winding makes it consistent along the side
            sin_sum += edge.normal.dot(across);
            cos_sum += edge.normal.dot(n);
        }

        double tilt = std::atan2(sin_sum, cos_sum);
        for (const SideEdge& edge : edges_by_side[side]) {
            Vector across = n.cross(edge.dir).normalized();
            ups[edge.key] = (n * std::cos(tilt) + across * std::sin(tilt)).normalized();
        }
    }

    return ups;
}

/// The through priority per naked edge for the frame's butt corners: 1 on the two opposite boundary curves that run through at their ends, the u ends of the domain when u_sides_through and the v ends otherwise, 0 on the other two, which are cut against them; opposite sides always share a role, a b a b around the loop.
inline std::map<std::pair<size_t, size_t>, int> through_side_priority(const Mesh& mesh, const NurbsSurface& surface, bool u_sides_through = true)
{

    std::map<std::pair<size_t, size_t>, int> priority;
    for (const auto& [u, v] : mesh.edges_on_boundary()) {
        Point mid = Point::mid_point(mesh.vertex_point(u).value(), mesh.vertex_point(v).value());
        std::pair<double, double> uv = surface.closest_parameters(mid);
        int side = nearest_domain_side(surface, uv.first, uv.second);
        bool u_side = side == 0 || side == 1;
        priority[wood_reciprocal::edge_key(u, v)] = u_side == u_sides_through ? 1 : 0;
    }

    return priority;
}

/// The same up for every naked edge of the mesh, for a frame that stays, say, vertical whatever the surface does.
inline std::map<std::pair<size_t, size_t>, Vector> constant_boundary_ups(const Mesh& mesh, const Vector& up)
{

    std::map<std::pair<size_t, size_t>, Vector> ups;
    for (const auto& [u, v] : mesh.edges_on_boundary())
        ups[wood_reciprocal::edge_key(u, v)] = up;

    return ups;
}

/// True when the first dual vertex turns before the second about the normal.
inline bool angle_less(const std::pair<double, std::pair<std::string, Point>>& p, const std::pair<double, std::pair<std::string, Point>>& q)
{
    return p.first < q.first;
}

/// The dual of the triangulated mesh: every quad split along the diagonal from its first to its third vertex, then one cell per vertex through the centroids of the triangles around it, so an interior grid vertex gives a hexagon. A boundary vertex's cell is closed through the midpoints of its two naked edges, and through the vertex itself only at a real corner, so the cells along a straight side are half hexagons with no needless vertex on the boundary. Cells are wound counter-clockwise about the vertex normal.
inline Mesh dual_hex_mesh(const Mesh& quads)
{

    std::vector<size_t> fkeys = quads.faces();
    std::vector<std::vector<size_t>> triangles;
    for (size_t fk : fkeys) {
        std::vector<size_t> face = quads.face_vertices(fk).value();
        for (size_t k = 1; k + 1 < face.size(); k++)
            triangles.push_back({face[0], face[k], face[k + 1]});
    }

    std::map<size_t, std::vector<size_t>> fans;  // vertex → the triangles around it
    for (size_t t = 0; t < triangles.size(); t++)
        for (size_t vk : triangles[t])
            fans[vk].push_back(t);

    std::map<size_t, std::vector<size_t>> naked_neighbours;  // boundary vertex → the far vertices of its naked edges
    for (const auto& [u, v] : quads.edges_on_boundary()) {
        naked_neighbours[u].push_back(v);
        naked_neighbours[v].push_back(u);
    }

    std::vector<Point> points;
    std::map<std::string, size_t> index_of;  // "t<i>", "e<u>_<v>", "v<k>" → dual vertex index
    std::vector<std::vector<size_t>> cells;
    for (const auto& [vk, fan] : fans) {
        Point centre = quads.vertex_point(vk).value();
        Vector normal = quads.vertex_normal(vk).value_or(Vector(0, 0, 1));
        if (normal[2] < 0.0)
            normal = -normal;

        Vector x_axis;
        x_axis.perpendicular_to(normal);
        x_axis = x_axis.normalized();
        Vector y_axis = normal.cross(x_axis);
        std::vector<std::pair<double, std::pair<std::string, Point>>> around;  // angle about the normal, dual vertex key and position
        for (size_t t : fan) {
            const std::vector<size_t>& tri = triangles[t];
            Point a = quads.vertex_point(tri[0]).value(), b = quads.vertex_point(tri[1]).value(), c = quads.vertex_point(tri[2]).value();
            Point centroid((a[0] + b[0] + c[0]) / 3.0, (a[1] + b[1] + c[1]) / 3.0, (a[2] + b[2] + c[2]) / 3.0);
            Vector d = centroid - centre;
            around.push_back({std::atan2(d.dot(y_axis), d.dot(x_axis)), {"t" + std::to_string(t), centroid}});
        }

        std::map<size_t, std::vector<size_t>>::const_iterator naked = naked_neighbours.find(vk);
        bool on_boundary = naked != naked_neighbours.end();
        if (on_boundary)
            for (size_t other : naked->second) {
                Point mid = Point::mid_point(centre, quads.vertex_point(other).value());
                Vector d = mid - centre;
                around.push_back({std::atan2(d.dot(y_axis), d.dot(x_axis)), {"e" + std::to_string(std::min(vk, other)) + "_" + std::to_string(std::max(vk, other)), mid}});
            }

        std::sort(around.begin(), around.end(), angle_less);
        if (on_boundary) {
            size_t widest = 0;
            double widest_gap = -1.0;
            for (size_t k = 0; k < around.size(); k++) {
                double gap = (k + 1 < around.size() ? around[k + 1].first : around[0].first + 2.0 * Tolerance::PI) - around[k].first;
                if (gap > widest_gap) {
                    widest_gap = gap;
                    widest = k;
                }
            }

            std::rotate(around.begin(), around.begin() + (widest + 1) % around.size(), around.end());
            const std::vector<size_t>& others = naked->second;
            bool straight = false;
            if (others.size() == 2) {
                Vector d0 = (quads.vertex_point(others[0]).value() - centre).normalized();
                Vector d1 = (quads.vertex_point(others[1]).value() - centre).normalized();
                straight = d0.dot(d1) < -std::cos(5.0 * Tolerance::TO_RADIANS);
            }

            if (!straight)
                around.push_back({0.0, {"v" + std::to_string(vk), centre}});
        }

        std::vector<size_t> cell;
        for (const auto& [angle, entry] : around) {
            std::map<std::string, size_t>::iterator found = index_of.find(entry.first);
            if (found == index_of.end()) {
                found = index_of.emplace(entry.first, points.size()).first;
                points.push_back(entry.second);
            }

            cell.push_back(found->second);
        }

        if (cell.size() >= 3)
            cells.push_back(cell);
    }

    return Mesh::from_vertices_and_faces(points, cells);
}

/// One of the serialized Annen arches, loaded from the data directory.
inline NurbsSurface annen_surface(const std::string& json_path, size_t index)
{

    std::vector<NurbsSurface> surfaces = wood_chevron::annen_surfaces(json_path);
    if (index >= surfaces.size())
        throw std::out_of_range(fmt::format("annen_surface: {} holds {} surfaces, index {} asked", json_path, surfaces.size(), index));

    return surfaces[index];
}

/// The length of the iso-curve at parameter constant in the other direction, as a polyline of `samples` segments over the domain of dir.
inline double iso_curve_length(const NurbsSurface& surface, int dir, double constant, int samples)
{

    std::pair<double, double> domain = surface.domain(dir);
    double length = 0.0;
    Point previous = dir == 0 ? surface.point_at(domain.first, constant) : surface.point_at(constant, domain.first);
    for (int k = 1; k <= samples; k++) {
        double t = domain.first + (domain.second - domain.first) * k / samples;
        Point current = dir == 0 ? surface.point_at(t, constant) : surface.point_at(constant, t);
        length += previous.distance(current);
        previous = current;
    }

    return length;
}

/// The lengths of the two mid iso-curves, u then v: the surface's proportions in model units, whatever its parameter domain.
inline std::pair<double, double> mid_iso_curve_lengths(const NurbsSurface& surface, int samples = 200)
{

    std::pair<double, double> du = surface.domain(0);
    std::pair<double, double> dv = surface.domain(1);
    double mid_u = (du.first + du.second) * 0.5;
    double mid_v = (dv.first + dv.second) * 0.5;

    return {iso_curve_length(surface, 0, mid_v, samples), iso_curve_length(surface, 1, mid_u, samples)};
}

/// The cumulative length along one mid iso-curve of a surface, so that parameters can be picked at equal spacing on the surface instead of in the parameter, which stretches cells wherever the parametrisation speeds up.
struct ArcLengthMap {
    std::vector<double> parameters;  // sampled parameters along the direction, ascending
    std::vector<double> lengths;     // chord length from the first sample to each sampled parameter
    double total = 0.0;              // the length of the whole iso-curve

    /// The map of the mid iso-curve of the surface along dir, sampled with `samples` segments.
    static ArcLengthMap of(const NurbsSurface& surface, int dir, int samples = 400)
    {

        std::pair<double, double> domain = surface.domain(dir);
        std::pair<double, double> other = surface.domain(1 - dir);
        double constant = (other.first + other.second) * 0.5;
        ArcLengthMap map;
        Point previous;
        for (int k = 0; k <= samples; k++) {
            double t = domain.first + (domain.second - domain.first) * k / samples;
            Point current = dir == 0 ? surface.point_at(t, constant) : surface.point_at(constant, t);
            map.total += k == 0 ? 0.0 : previous.distance(current);
            map.parameters.push_back(t);
            map.lengths.push_back(map.total);
            previous = current;
        }

        return map;
    }

    /// The parameter at which the iso-curve has covered the given length, interpolated in the table.
    double parameter_at(double length) const
    {

        if (length <= 0.0)
            return parameters.front();

        if (length >= total)
            return parameters.back();

        size_t k = std::upper_bound(lengths.begin(), lengths.end(), length) - lengths.begin();
        double span = lengths[k] - lengths[k - 1];
        double fraction = span > 0.0 ? (length - lengths[k - 1]) / span : 0.0;
        return parameters[k - 1] + (parameters[k] - parameters[k - 1]) * fraction;
    }
};

/// Whether the faces of a mesh on the surface must be reversed so their normals agree with the surface normal at the domain centre turned to point up.
inline bool faces_reversed_on(const NurbsSurface& surface)
{

    std::pair<double, double> du = surface.domain(0);
    std::pair<double, double> dv = surface.domain(1);
    Vector centre_normal = surface.normal_at((du.first + du.second) * 0.5, (dv.first + dv.second) * 0.5);
    return centre_normal[2] < 0.0;
}

/// The nu by nv quad grid on the surface, the rows and columns at equal arc length along the mid iso-curves, faces wound so their normals agree with the surface normal at the domain centre turned to point up.
inline Mesh quad_mesh_from_surface(const NurbsSurface& surface, int nu, int nv)
{

    ArcLengthMap along_u = ArcLengthMap::of(surface, 0);
    ArcLengthMap along_v = ArcLengthMap::of(surface, 1);
    std::vector<Point> points;
    points.reserve((nu + 1) * (nv + 1));
    for (int i = 0; i <= nu; i++)
        for (int j = 0; j <= nv; j++)
            points.push_back(surface.point_at(along_u.parameter_at(along_u.total * i / nu), along_v.parameter_at(along_v.total * j / nv)));

    bool reversed = faces_reversed_on(surface);
    std::vector<std::vector<size_t>> faces;
    faces.reserve(nu * nv);
    for (int i = 0; i < nu; i++)
        for (int j = 0; j < nv; j++) {
            std::vector<size_t> face = {(size_t)(i * (nv + 1) + j), (size_t)((i + 1) * (nv + 1) + j),
                                        (size_t)((i + 1) * (nv + 1) + j + 1), (size_t)(i * (nv + 1) + j + 1)};
            if (reversed)
                std::reverse(face.begin(), face.end());

            faces.push_back(face);
        }

    return Mesh::from_vertices_and_faces(points, faces);
}

/// The quad grid on the surface whose cells are about target_edge long both ways: the counts are the mid iso-curve lengths over target_edge, rounded, never below one, so a long arch gets many rows and few columns.
inline Mesh quad_mesh_from_surface(const NurbsSurface& surface, double target_edge)
{

    std::pair<double, double> lengths = mid_iso_curve_lengths(surface);
    int nu = std::max(1, (int)std::lround(lengths.first / target_edge));
    int nv = std::max(1, (int)std::lround(lengths.second / target_edge));

    return quad_mesh_from_surface(surface, nu, nv);
}

/// A convex polygon clipped to one side of an axis-aligned line: the part with coordinate axis at or above limit when keep_above, at or below it otherwise.
inline std::vector<std::pair<double, double>> clip_polygon(const std::vector<std::pair<double, double>>& polygon, int axis, double limit, bool keep_above)
{

    std::vector<std::pair<double, double>> kept;
    for (size_t k = 0; k < polygon.size(); k++) {
        const std::pair<double, double>& a = polygon[k];
        const std::pair<double, double>& b = polygon[(k + 1) % polygon.size()];
        double va = (axis == 0 ? a.first : a.second) - limit, vb = (axis == 0 ? b.first : b.second) - limit;
        if (!keep_above) {
            va = -va;
            vb = -vb;
        }

        bool a_in = va >= -1e-9, b_in = vb >= -1e-9;
        if (a_in)
            kept.push_back(a);

        if (a_in != b_in) {
            double t = va / (va - vb);
            kept.emplace_back(a.first + (b.first - a.first) * t, a.second + (b.second - a.second) * t);
        }
    }

    return kept;
}

/// Staggered rows of pointy-top hexagons filling the width by depth rectangle and clipped to it: the row centre lines land on the bottom and top edges and the column centre lines on the left and right edges, so the boundary cells are half hexagons and the corners quarter hexagons, every cell convex with a straight outer edge. The cell count follows target_edge, the hexagons stretched slightly to fit whole rows and columns.
inline std::vector<std::vector<std::pair<double, double>>> clipped_hex_lattice(double width, double depth, double target_edge)
{

    const int rows = std::max(1, (int)std::lround(depth / (1.5 * target_edge)));
    const int columns = std::max(1, (int)std::lround(width / (std::sqrt(3.0) * target_edge)));
    const double edge = depth / (1.5 * rows);  // the vertical half-size of a hexagon, its edge when unstretched
    const double pitch = width / columns;      // centre to centre along a row
    std::vector<std::vector<std::pair<double, double>>> cells;
    for (int row = 0; row <= rows; row++)
        for (int column = -1; column <= columns; column++) {
            double cx = column * pitch + (row % 2 == 1 ? pitch * 0.5 : 0.0);
            double cy = row * 1.5 * edge;
            std::vector<std::pair<double, double>> polygon;
            for (int k = 0; k < 6; k++) {
                double angle = Tolerance::PI * (0.5 + k / 3.0);
                polygon.emplace_back(cx + pitch * 0.5 * std::cos(angle) / std::cos(Tolerance::PI / 6.0), cy + edge * std::sin(angle));
            }

            polygon = clip_polygon(polygon, 0, 0.0, true);
            polygon = clip_polygon(polygon, 0, width, false);
            polygon = clip_polygon(polygon, 1, 0.0, true);
            polygon = clip_polygon(polygon, 1, depth, false);
            std::vector<std::pair<double, double>> cleaned;
            for (const std::pair<double, double>& q : polygon)
                if (cleaned.empty() || std::hypot(q.first - cleaned.back().first, q.second - cleaned.back().second) > 1e-6)
                    cleaned.push_back(q);

            if (cleaned.size() > 1 && std::hypot(cleaned.front().first - cleaned.back().first, cleaned.front().second - cleaned.back().second) <= 1e-6)
                cleaned.pop_back();

            double area = 0.0;
            for (size_t k = 0; k < cleaned.size(); k++)
                area += cleaned[k].first * cleaned[(k + 1) % cleaned.size()].second - cleaned[(k + 1) % cleaned.size()].first * cleaned[k].second;

            if (cleaned.size() >= 3 && std::abs(area) > 1e-6 * width * depth)
                cells.push_back(cleaned);
        }

    return cells;
}

/// The plane cells lifted through lift into a welded mesh: cells sharing a plane position within a thousandth share the vertex.
template <typename Lift>
inline Mesh lifted_cells_mesh(const std::vector<std::vector<std::pair<double, double>>>& cells, bool reversed, const Lift& lift)
{

    std::map<std::pair<long long, long long>, size_t> index_of;
    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;
    for (const std::vector<std::pair<double, double>>& cell : cells) {
        std::vector<size_t> face;
        for (const auto& [x, y] : cell) {
            std::pair<long long, long long> key{std::llround(x * 1000.0), std::llround(y * 1000.0)};
            std::map<std::pair<long long, long long>, size_t>::iterator found = index_of.find(key);
            if (found == index_of.end()) {
                found = index_of.emplace(key, points.size()).first;
                points.push_back(lift(x, y));
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

    return Mesh::from_vertices_and_faces(points, faces);
}

/// Lifts a point of the arc-length plane of a surface onto the surface.
struct SurfaceLift {
    const NurbsSurface& surface;  // the surface the plane maps onto
    ArcLengthMap along_u;         // arc length along the mid u iso-curve
    ArcLengthMap along_v;         // arc length along the mid v iso-curve

    Point operator()(double x, double y) const
    {
        return surface.point_at(along_u.parameter_at(x), along_v.parameter_at(y));
    }
};

/// A hexagonal mesh on the surface: staggered pointy-top hexagons of edge about target_edge in the arc-length plane of the two mid iso-curves, clipped to the domain so the boundary is straight in that plane, half cells along the sides and quarter cells at the corners, all lifted onto the surface.
inline Mesh hex_mesh_from_surface(const NurbsSurface& surface, double target_edge)
{

    SurfaceLift lift{surface, ArcLengthMap::of(surface, 0), ArcLengthMap::of(surface, 1)};
    return lifted_cells_mesh(clipped_hex_lattice(lift.along_u.total, lift.along_v.total, target_edge), faces_reversed_on(surface), lift);
}

/// Lifts a plan point onto the sinusoidal dome.
struct SinusoidalDomeLift {
    double width, depth, height;  // the dome's plan and rise

    Point operator()(double x, double y) const
    {
        return Point(x, y, height * std::sin(Tolerance::PI * x / width) * std::sin(Tolerance::PI * y / depth));
    }
};

/// The sinusoidal dome as a hexagonal mesh: the clipped lattice over its plan, lifted by the dome's height function.
inline Mesh sinusoidal_dome_hex_mesh(double width, double depth, double height, double target_edge)
{
    return lifted_cells_mesh(clipped_hex_lattice(width, depth, target_edge), false, SinusoidalDomeLift{width, depth, height});
}

}  // namespace wood_reciprocal
