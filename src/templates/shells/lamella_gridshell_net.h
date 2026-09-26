#pragma once
#include "lamella_gridshell.h"

namespace wood_gridshell {

// ═══════════════════════════════════════════════════════════════════════════
// Mesh carrier
// ═══════════════════════════════════════════════════════════════════════════

/// The nearest point of the mesh to a query, as a triangle and barycentric weights.
struct Foot {
    size_t triangle = 0; // Triangle index.
    Vector weights; // Barycentric weights of its three corners.
};

/// A triangle or quad mesh read as a smooth carrier: vertex normals and shape operators blended inside each triangle, a point lifted off its facet by half of Phong tessellation, triangles bucketed in a grid for the closest point.
struct Carrier {
    std::vector<Point> points; // Vertex positions.
    std::vector<std::array<size_t, 3>> triangles; // Faces fanned into triangles.
    std::vector<Vector> normals; // Unit vertex normals, Max's weights, exact on a sphere.
    std::vector<std::array<Vector, 3>> tensors; // Shape operator per vertex, its three rows, summed over the faces around weighted by twice their area.
    std::vector<bool> band; // True for a vertex of a triangle touching the boundary.
    double cell = 1.0; // Side of the grid cells the triangles are bucketed in, the mean edge.
    std::unordered_map<long long, std::vector<size_t>> buckets; // Triangles per grid cell their box touches.

    /// The carrier of mesh, each face fanned from its first corner.
    explicit Carrier(const Mesh& mesh) {

        std::map<size_t, size_t> index;
        for (const size_t vertex : mesh.vertices()) {
            index[vertex] = points.size();
            points.push_back(*mesh.vertex_point(vertex));
        }

        for (const size_t face : mesh.faces()) {
            const std::vector<size_t> corners = *mesh.face_vertices(face);
            for (size_t k = 1; k + 1 < corners.size(); k++)
                triangles.push_back({index[corners[0]], index[corners[k]], index[corners[k + 1]]});
        }

        compute_normals();
        compute_tensors();
        compute_band();
        compute_buckets();
    }

    /// Vertex normals with Max's weights.
    void compute_normals() {

        normals.assign(points.size(), Vector(0.0, 0.0, 0.0));
        for (const std::array<size_t, 3>& t : triangles)
            for (int k = 0; k < 3; k++) {
                const Vector a = points[t[(k + 1) % 3]] - points[t[k]];
                const Vector b = points[t[(k + 2) % 3]] - points[t[k]];
                normals[t[k]] = normals[t[k]] + a.cross(b) / (a.dot(a) * b.dot(b));
            }

        for (Vector& normal : normals)
            normal = normal.normalized();
    }

    /// Vertex shape operators, the face tensors around summed by area.
    void compute_tensors() {

        tensors.assign(points.size(), {Vector(0.0, 0.0, 0.0), Vector(0.0, 0.0, 0.0), Vector(0.0, 0.0, 0.0)});
        for (const std::array<size_t, 3>& t : triangles) {
            const std::array<Vector, 3> tensor = compute_face_tensor(t);
            const double area = (points[t[1]] - points[t[0]]).cross(points[t[2]] - points[t[0]]).magnitude();
            for (const size_t corner : t)
                for (int row = 0; row < 3; row++)
                    tensors[corner][row] = tensors[corner][row] + tensor[row] * area;
        }
    }

    /// The vertices of every triangle that touches the boundary.
    void compute_band() {

        std::set<std::pair<size_t, size_t>> edges;
        for (const std::array<size_t, 3>& t : triangles)
            for (int k = 0; k < 3; k++) {
                const std::pair<size_t, size_t> edge(std::min(t[k], t[(k + 1) % 3]), std::max(t[k], t[(k + 1) % 3]));
                if (!edges.erase(edge))
                    edges.insert(edge);
            }

        std::vector<bool> open(points.size(), false);
        for (const std::pair<size_t, size_t>& edge : edges) {
            open[edge.first] = true;
            open[edge.second] = true;
        }

        band.assign(points.size(), false);
        for (const std::array<size_t, 3>& t : triangles)
            if (open[t[0]] || open[t[1]] || open[t[2]])
                for (const size_t corner : t)
                    band[corner] = true;
    }

    /// Every triangle into the grid cells its box touches.
    void compute_buckets() {

        double length = 0.0;
        for (const std::array<size_t, 3>& t : triangles)
            length += (points[t[1]] - points[t[0]]).magnitude();

        cell = length / static_cast<double>(triangles.size());
        for (size_t i = 0; i < triangles.size(); i++) {
            Point low = points[triangles[i][0]];
            Point high = low;
            for (const size_t corner : triangles[i])
                for (int axis = 0; axis < 3; axis++) {
                    low[axis] = std::min(low[axis], points[corner][axis]);
                    high[axis] = std::max(high[axis], points[corner][axis]);
                }

            for (long x = std::floor(low[0] / cell); x <= std::floor(high[0] / cell); x++)
                for (long y = std::floor(low[1] / cell); y <= std::floor(high[1] / cell); y++)
                    for (long z = std::floor(low[2] / cell); z <= std::floor(high[2] / cell); z++)
                        buckets[compute_key(x, y, z)].push_back(i);
        }
    }

    /// The shape operator of one triangle from the turn of the vertex normals along its edges, least squares in the face plane, as three rows in xyz.
    std::array<Vector, 3> compute_face_tensor(const std::array<size_t, 3>& t) const {

        const Vector x = (points[t[1]] - points[t[0]]).normalized();
        const Vector y = (points[t[1]] - points[t[0]]).cross(points[t[2]] - points[t[0]]).normalized().cross(x);
        double a[3][4] = {{0.0}};
        for (int k = 0; k < 3; k++) {
            const Vector edge = points[t[(k + 1) % 3]] - points[t[k]];
            const Vector turn = normals[t[(k + 1) % 3]] - normals[t[k]];
            const double rows[2][4] = {{edge.dot(x), edge.dot(y), 0.0, turn.dot(x)}, {0.0, edge.dot(x), edge.dot(y), turn.dot(y)}};
            for (const double* row : rows)
                for (int i = 0; i < 3; i++)
                    for (int j = 0; j < 4; j++)
                        a[i][j] += row[i] * row[j];
        }

        for (int i = 0; i < 3; i++)
            for (int k = i + 1; k < 3; k++) {
                const double factor = a[k][i] / a[i][i];
                for (int j = i; j < 4; j++)
                    a[k][j] -= factor * a[i][j];
            }

        const double s22 = a[2][3] / a[2][2];
        const double s12 = (a[1][3] - a[1][2] * s22) / a[1][1];
        const double s11 = (a[0][3] - a[0][1] * s12 - a[0][2] * s22) / a[0][0];
        std::array<Vector, 3> tensor;
        for (int row = 0; row < 3; row++)
            tensor[row] = x * (s11 * x[row] + s12 * y[row]) + y * (s12 * x[row] + s22 * y[row]);

        return tensor;
    }

    /// The key of grid cell (x, y, z).
    static long long compute_key(long x, long y, long z) {
        return ((x + 1048576LL) << 42) | ((y + 1048576LL) << 21) | (z + 1048576LL);
    }

    /// Barycentric weights of the point of triangle abc nearest p (Ericson, Real-Time Collision Detection 5.1.5).
    static Vector compute_weights(const Point& a, const Point& b, const Point& c, const Point& p) {

        const Vector ab = b - a;
        const Vector ac = c - a;
        const Vector ap = p - a;
        const double d1 = ab.dot(ap);
        const double d2 = ac.dot(ap);
        if (d1 <= 0.0 && d2 <= 0.0)
            return Vector(1.0, 0.0, 0.0);

        const double d3 = ab.dot(p - b);
        const double d4 = ac.dot(p - b);
        if (d3 >= 0.0 && d4 <= d3)
            return Vector(0.0, 1.0, 0.0);

        const double vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0)
            return Vector(1.0 - d1 / (d1 - d3), d1 / (d1 - d3), 0.0);

        const double d5 = ab.dot(p - c);
        const double d6 = ac.dot(p - c);
        if (d6 >= 0.0 && d5 <= d6)
            return Vector(0.0, 0.0, 1.0);

        const double vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0)
            return Vector(1.0 - d2 / (d2 - d6), 0.0, d2 / (d2 - d6));

        const double va = d3 * d6 - d5 * d4;
        if (va <= 0.0 && d4 - d3 >= 0.0 && d5 - d6 >= 0.0) {
            const double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return Vector(0.0, 1.0 - w, w);
        }

        const double v = vb / (va + vb + vc);
        const double w = vc / (va + vb + vc);

        return Vector(1.0 - v - w, v, w);
    }

    /// The foot on triangle i if it is nearer than nearest, which it then becomes.
    void compute_nearer(const Point& point, size_t i, Foot& best, double& nearest) const {

        const std::array<size_t, 3>& t = triangles[i];
        const Vector weights = compute_weights(points[t[0]], points[t[1]], points[t[2]], point);
        const double distance = (points[t[0]] + (points[t[1]] - points[t[0]]) * weights[1] + (points[t[2]] - points[t[0]]) * weights[2] - point).magnitude();
        if (distance < nearest) {
            nearest = distance;
            best = Foot{i, weights};
        }
    }

    /// The nearest point of the mesh to point: the triangles in the 27 grid cells around it, every triangle when none of those lies within a cell.
    Foot compute_foot(const Point& point) const {

        Foot best{0, Vector(1.0, 0.0, 0.0)};
        double nearest = 1e300;
        const long x = std::floor(point[0] / cell);
        const long y = std::floor(point[1] / cell);
        const long z = std::floor(point[2] / cell);
        for (long dx = -1; dx <= 1; dx++)
            for (long dy = -1; dy <= 1; dy++)
                for (long dz = -1; dz <= 1; dz++) {
                    const std::unordered_map<long long, std::vector<size_t>>::const_iterator bucket = buckets.find(compute_key(x + dx, y + dy, z + dz));
                    if (bucket != buckets.end())
                        for (const size_t i : bucket->second)
                            compute_nearer(point, i, best, nearest);
                }

        if (nearest > cell)
            for (size_t i = 0; i < triangles.size(); i++)
                compute_nearer(point, i, best, nearest);

        return best;
    }

    /// The point a foot stands on, lifted off its flat triangle by half of Phong tessellation.
    Point compute_point(const Foot& foot) const {

        const std::array<size_t, 3>& t = triangles[foot.triangle];
        const Point flat = points[t[0]] + (points[t[1]] - points[t[0]]) * foot.weights[1] + (points[t[2]] - points[t[0]]) * foot.weights[2];
        Vector lift(0.0, 0.0, 0.0);
        for (int k = 0; k < 3; k++)
            lift = lift - normals[t[k]] * ((flat - points[t[k]]).dot(normals[t[k]]) * foot.weights[k]);

        return flat + lift * 0.5;
    }

    /// The unit normal at a foot, the vertex normals blended.
    Vector compute_normal(const Foot& foot) const {

        const std::array<size_t, 3>& t = triangles[foot.triangle];
        Vector normal(0.0, 0.0, 0.0);
        for (int k = 0; k < 3; k++)
            normal = normal + normals[t[k]] * foot.weights[k];

        return normal.normalized();
    }

    /// True where the foot's triangle touches the boundary.
    bool is_edge(const Foot& foot) const {
        const std::array<size_t, 3>& t = triangles[foot.triangle];
        return band[t[0]] || band[t[1]] || band[t[2]];
    }

    /// The two unit asymptotic directions at a foot, where the blended second fundamental form l a^2 + 2 m a b + n b^2 vanishes on an orthonormal tangent basis; none where the Gaussian curvature is positive.
    std::vector<Vector> compute_directions(const Foot& foot) const {

        const Vector normal = compute_normal(foot);
        const Vector x = (std::abs(normal[0]) < 0.9 ? Vector(1.0, 0.0, 0.0) : Vector(0.0, 1.0, 0.0)).cross(normal).normalized();
        const Vector y = normal.cross(x);
        Vector along_x(0.0, 0.0, 0.0);
        Vector along_y(0.0, 0.0, 0.0);
        for (int k = 0; k < 3; k++) {
            const std::array<Vector, 3>& tensor = tensors[triangles[foot.triangle][k]];
            along_x = along_x + Vector(tensor[0].dot(x), tensor[1].dot(x), tensor[2].dot(x)) * foot.weights[k];
            along_y = along_y + Vector(tensor[0].dot(y), tensor[1].dot(y), tensor[2].dot(y)) * foot.weights[k];
        }

        const double l = x.dot(along_x);
        const double m = x.dot(along_y);
        const double n = y.dot(along_y);
        const double disc = m * m - l * n;
        if (disc < 0.0)
            return {};

        std::vector<Vector> directions;
        for (const double sign : {1.0, -1.0}) {
            const Vector d = std::abs(l) >= std::abs(n) ? Vector((-m + sign * std::sqrt(disc)) / l, 1.0, 0.0) : Vector(1.0, (-m + sign * std::sqrt(disc)) / n, 0.0);
            directions.push_back((x * d[0] + y * d[1]).normalized());
        }

        return directions;
    }

    /// Largest share of mean curvature over the vertices off the boundary band, |k1 + k2| / sqrt(2 (k1^2 + k2^2)) from the vertex shape operators: 0 on a minimal surface, 1 on a sphere.
    double compute_mean_curvature() const {

        double worst = 0.0;
        for (size_t i = 0; i < points.size(); i++) {
            const double trace = tensors[i][0][0] + tensors[i][1][1] + tensors[i][2][2];
            const double norm = std::sqrt(tensors[i][0].dot(tensors[i][0]) + tensors[i][1].dot(tensors[i][1]) + tensors[i][2].dot(tensors[i][2]));
            if (!band[i] && norm > 0.0)
                worst = std::max(worst, std::abs(trace) / (std::sqrt(2.0) * norm));
        }

        return worst;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// Tracing on the mesh
// ═══════════════════════════════════════════════════════════════════════════

/// The step of length step along whichever asymptotic direction at point lies closer to last, sign matched, as Bowerbird's Choose; zero where there is none.
inline Vector compute_mesh_direction(const Carrier& carrier, const Point& point, const Vector& last, double step) {

    const std::vector<Vector> directions = carrier.compute_directions(carrier.compute_foot(point));
    if (directions.size() < 2)
        return Vector(0.0, 0.0, 0.0);

    const Vector s = last.normalized();
    const double e1 = directions[0].dot(s);
    const double e2 = directions[1].dot(s);
    const Vector d = std::abs(e1) > std::abs(e2) ? directions[0] * (e1 > 0.0 ? step : -step) : directions[1] * (e2 > 0.0 ? step : -step);

    return d;
}

/// Runge-Kutta steps of step over the mesh from point along direction, every point put back on the mesh, appended until a triangle touching the boundary, a standstill or count points; returns the last step.
inline Vector compute_mesh_path(const Carrier& carrier, Point point, Vector direction, double step, size_t count, std::vector<Point>& points) {

    while (points.size() < count) {
        const Vector d0 = compute_mesh_direction(carrier, point, direction, step);
        const Vector d1 = compute_mesh_direction(carrier, carrier.compute_point(carrier.compute_foot(point + d0 * 0.5)), direction, step);
        const Vector d2 = compute_mesh_direction(carrier, carrier.compute_point(carrier.compute_foot(point + d1 * 0.5)), direction, step);
        const Vector d3 = compute_mesh_direction(carrier, carrier.compute_point(carrier.compute_foot(point + d2)), direction, step);
        const Vector delta = (d0 + d1 * 2.0 + d2 * 2.0 + d3) * (1.0 / 6.0);
        if (delta.dot(delta) == 0.0)
            break;

        const Foot foot = carrier.compute_foot(point + delta);
        if (carrier.is_edge(foot))
            break;

        const Point next = carrier.compute_point(foot);
        const Vector moved = next - point;
        if (moved.dot(moved) < 1e-20)
            break;

        direction = moved;
        point = next;
        points.push_back(point);
    }

    return direction;
}

/// An asymptotic curve of the mesh through seed, both ways along the asymptotic direction there closer to reference, a point every step until the boundary band; empty where the seed lies in the band or has no direction.
inline std::vector<Point> compute_mesh_trace(const Carrier& carrier, const Point& seed, const Vector& reference, double step, size_t count = 100000) {

    const Foot foot = carrier.compute_foot(seed);
    const std::vector<Vector> directions = carrier.compute_directions(foot);
    if (carrier.is_edge(foot) || directions.size() < 2)
        return {};

    const Vector direction = std::abs(directions[0].dot(reference)) >= std::abs(directions[1].dot(reference)) ? directions[0] : directions[1];
    std::vector<Point> points{carrier.compute_point(foot)};
    compute_mesh_path(carrier, points.front(), direction * -1.0, step, count, points);
    std::reverse(points.begin(), points.end());
    compute_mesh_path(carrier, points.back(), direction, step, count, points);

    return points;
}

/// The ends of a seed line across a family on the mesh: through the mesh point nearest centre, perpendicular in the tangent plane to the family's first or second asymptotic direction there, half the mesh's diameter each way; seeds along it are put on the mesh by their feet.
inline std::pair<Point, Point> compute_mesh_seed_line(const Carrier& carrier, const Point& centre, bool first) {

    const Foot foot = carrier.compute_foot(centre);
    const std::vector<Vector> directions = carrier.compute_directions(foot);
    Point low = carrier.points.front();
    Point high = low;
    for (const Point& p : carrier.points)
        for (int axis = 0; axis < 3; axis++) {
            low[axis] = std::min(low[axis], p[axis]);
            high[axis] = std::max(high[axis], p[axis]);
        }

    const double reach = (high - low).magnitude() / 2.0;
    const Vector along = directions.size() < 2 ? Vector(1.0, 0.0, 0.0) : (first ? directions[0] : directions[1]);
    const Vector across = carrier.compute_normal(foot).cross(along).normalized() * reach;
    const Point origin = carrier.compute_point(foot);

    return {origin - across, origin + across};
}

/// count points evenly spaced strictly inside the segment a-b, each put on the mesh; those landing in the boundary band or farther than the mesh's cell from the segment are dropped.
inline std::vector<Point> compute_mesh_seeds(const Carrier& carrier, const Point& a, const Point& b, int count) {

    std::vector<Point> seeds;
    for (int i = 0; i < count; i++) {
        const Point p = a + (b - a) * ((i + 1.0) / (count + 1.0));
        const Foot foot = carrier.compute_foot(p);
        const Point q = carrier.compute_point(foot);
        if (!carrier.is_edge(foot) && (q - p).magnitude() < carrier.cell)
            seeds.push_back(q);
    }

    return seeds;
}

/// A rotational A-net's curves from one traced asymptotic curve of a surface of revolution about axis through centre: count copies turned by equal angles as the first family, and as many of its mirror image in the plane through the axis and the curve's first point as the second (Schling Sec. 3.3, initialisation).
inline std::pair<std::vector<std::vector<Point>>, std::vector<std::vector<Point>>> compute_rotational(const std::vector<Point>& curve, const Point& centre, const Vector& axis, int count) {

    const Vector z = axis.normalized();
    const Vector radial = curve.front() - centre;
    const Vector x = (radial - z * radial.dot(z)).normalized();
    const Vector y = z.cross(x);
    std::vector<std::vector<Point>> first;
    std::vector<std::vector<Point>> second;
    for (int k = 0; k < count; k++) {
        const double angle = 2.0 * Tolerance::PI * k / count;
        std::vector<Point> turned;
        std::vector<Point> mirrored;
        for (const Point& p : curve) {
            const Vector r = p - centre;
            const double a = r.dot(x);
            const double b = r.dot(y);
            const double c = r.dot(z);
            turned.push_back(centre + x * (a * std::cos(angle) - b * std::sin(angle)) + y * (a * std::sin(angle) + b * std::cos(angle)) + z * c);
            mirrored.push_back(centre + x * (a * std::cos(angle) + b * std::sin(angle)) + y * (a * std::sin(angle) - b * std::cos(angle)) + z * c);
        }

        first.push_back(turned);
        second.push_back(mirrored);
    }

    return {first, second};
}

// ═══════════════════════════════════════════════════════════════════════════
// Net
// ═══════════════════════════════════════════════════════════════════════════

/// A discrete net of two curve families: a vertex at every crossing with one normal, the u-lines the first family and the v-lines the second, each an ordered list of vertices.
struct Net {
    std::vector<Point> points; // Vertex positions.
    std::vector<Vector> normals; // One unit normal per vertex, shared by both lines through it.
    std::vector<std::vector<size_t>> lines; // Every line as its vertices in order, the first family first.
    size_t firsts = 0; // Number of lines in the first family.
    std::vector<std::array<long, 4>> stars; // Per vertex the previous and next vertex on its first-family line and on its second, -1 where none.
    std::vector<std::pair<size_t, size_t>> pairs; // Per vertex the lines through it, first-family then second, as positions in lines; both -1 for a vertex added by refinement.
    std::vector<bool> lamellas; // Per line, true for a traced curve that carries boards, false for a line added by refinement.
};

/// The stars of the net from its lines: previous and next along the first-family line of every vertex, then along its second.
inline void compute_stars(Net& net) {

    net.stars.assign(net.points.size(), {-1, -1, -1, -1});
    for (size_t l = 0; l < net.lines.size(); l++) {
        const size_t offset = l < net.firsts ? 0 : 2;
        for (size_t k = 0; k < net.lines[l].size(); k++) {
            net.stars[net.lines[l][k]][offset] = k == 0 ? -1 : static_cast<long>(net.lines[l][k - 1]);
            net.stars[net.lines[l][k]][offset + 1] = k + 1 == net.lines[l].size() ? -1 : static_cast<long>(net.lines[l][k + 1]);
        }
    }
}

/// The closest points of two segments, as (fraction on a-b, fraction on c-d, distance).
inline std::array<double, 3> compute_closest(const Point& a, const Point& b, const Point& c, const Point& d) {

    const Vector u = b - a;
    const Vector v = d - c;
    const Vector w = a - c;
    const double uu = u.dot(u);
    const double uv = u.dot(v);
    const double vv = v.dot(v);
    const double uw = u.dot(w);
    const double vw = v.dot(w);
    const double det = uu * vv - uv * uv;
    double s = det > 1e-12 * uu * vv ? std::clamp((uv * vw - vv * uw) / det, 0.0, 1.0) : 0.0;
    double t = std::clamp((uv * s + vw) / vv, 0.0, 1.0);
    s = std::clamp((uv * t - uw) / uu, 0.0, 1.0);

    return {s, t, (a + u * s - (c + v * t)).magnitude()};
}

/// A crossing of two polylines: where two segments pass within tolerance, as (position along a, position along b, point), one per pair of polylines within a tolerance's reach.
inline std::vector<std::tuple<double, double, Point>> compute_polyline_crossings(const std::vector<Point>& a, const std::vector<Point>& b, double tolerance) {

    std::vector<std::tuple<double, double, Point>> crossings;
    for (size_t i = 0; i + 1 < a.size(); i++)
        for (size_t j = 0; j + 1 < b.size(); j++) {
            const double reach = (a[i + 1] - a[i]).magnitude() + (b[j + 1] - b[j]).magnitude() + tolerance;
            if ((a[i] - b[j]).magnitude() > reach)
                continue;

            const std::array<double, 3> closest = compute_closest(a[i], a[i + 1], b[j], b[j + 1]);
            if (closest[2] > tolerance)
                continue;

            const double ta = i + closest[0];
            const double tb = j + closest[1];
            bool fresh = true;
            for (const std::tuple<double, double, Point>& known : crossings)
                fresh = fresh && std::abs(std::get<0>(known) - ta) > 1.5;

            if (fresh)
                crossings.emplace_back(ta, tb, (a[i] + (a[i + 1] - a[i]) * closest[0] + (b[j] + (b[j + 1] - b[j]) * closest[1] - Point(0.0, 0.0, 0.0))) * 0.5);
        }

    return crossings;
}

/// The net of two families of polylines on a carrier: a vertex where a first-family polyline crosses a second-family one, its normal the carrier's there, lines ordered along their polylines.
inline Net compute_net(const Carrier& carrier, const std::vector<std::vector<Point>>& firsts, const std::vector<std::vector<Point>>& seconds, double tolerance) {

    Net net;
    net.firsts = firsts.size();
    net.lines.resize(firsts.size() + seconds.size());
    std::vector<std::vector<std::pair<double, size_t>>> along(net.lines.size());
    for (size_t i = 0; i < firsts.size(); i++)
        for (size_t j = 0; j < seconds.size(); j++)
            for (const std::tuple<double, double, Point>& crossing : compute_polyline_crossings(firsts[i], seconds[j], tolerance)) {
                const size_t vertex = net.points.size();
                net.points.push_back(std::get<2>(crossing));
                net.normals.push_back(carrier.compute_normal(carrier.compute_foot(net.points.back())));
                net.pairs.emplace_back(i, firsts.size() + j);
                along[i].emplace_back(std::get<0>(crossing), vertex);
                along[firsts.size() + j].emplace_back(std::get<1>(crossing), vertex);
            }

    for (size_t l = 0; l < net.lines.size(); l++) {
        std::sort(along[l].begin(), along[l].end());
        for (const std::pair<double, size_t>& entry : along[l])
            net.lines[l].push_back(entry.second);
    }

    net.lamellas.assign(net.lines.size(), true);
    compute_stars(net);

    return net;
}

/// The vertex of the refined net between two vertices of the coarse one, made once per unordered pair: their midpoint with the mean normal, no line yet.
inline size_t compute_midpoint(const Net& net, Net& refined, std::map<std::pair<size_t, size_t>, size_t>& made, size_t a, size_t b) {

    const std::pair<size_t, size_t> key(std::min(a, b), std::max(a, b));
    const std::map<std::pair<size_t, size_t>, size_t>::iterator found = made.find(key);
    if (found != made.end())
        return found->second;

    made[key] = refined.points.size();
    refined.points.push_back(net.points[a] + (net.points[b] - net.points[a]) * 0.5);
    refined.normals.push_back((net.normals[a] + net.normals[b]).normalized());
    refined.pairs.emplace_back(static_cast<size_t>(-1), static_cast<size_t>(-1));

    return made[key];
}

/// The mid-line of the refined net after coarse line l: through the midpoints of the edges leaving each of its vertices along the other family and the centres of the quads anchored there, cut into pieces where a quad is missing.
inline void compute_midline(const Net& net, Net& refined, std::map<std::pair<size_t, size_t>, size_t>& made, const std::vector<long>& centres, size_t l) {

    const size_t across = l < net.firsts ? 2 : 0;
    std::vector<size_t> piece;
    for (const size_t v : net.lines[l]) {
        const long other = net.stars[v][across + 1];
        if (other < 0 || centres[v] < 0) {
            if (other >= 0)
                piece.push_back(compute_midpoint(net, refined, made, v, static_cast<size_t>(other)));

            if (piece.size() >= 2)
                refined.lines.push_back(piece);

            piece.clear();
            continue;
        }

        piece.push_back(compute_midpoint(net, refined, made, v, static_cast<size_t>(other)));
        piece.push_back(static_cast<size_t>(centres[v]));
    }

    if (piece.size() >= 2)
        refined.lines.push_back(piece);
}

/// The net refined once by bilinear subdivision (Schling Sec. 3.4): every quad v, next(v) on both lines, and their common corner gets a centre, every edge a midpoint; each coarse line keeps its vertices with the midpoints between, a mid-line runs after it through the other family's midpoints and the centres, so the result is a quad net of twice the resolution whose coarse lines alone are lamellas.
inline Net compute_refined(const Net& net) {

    Net refined;
    refined.points = net.points;
    refined.normals = net.normals;
    refined.pairs = net.pairs;
    std::vector<long> centres(net.points.size(), -1);
    for (size_t v = 0; v < net.points.size(); v++) {
        const long a = net.stars[v][1];
        const long b = net.stars[v][3];
        if (a < 0 || b < 0 || net.stars[static_cast<size_t>(a)][3] < 0 || net.stars[static_cast<size_t>(a)][3] != net.stars[static_cast<size_t>(b)][1])
            continue;

        const size_t d = static_cast<size_t>(net.stars[static_cast<size_t>(a)][3]);
        centres[v] = static_cast<long>(refined.points.size());
        refined.points.push_back(Point::centroid({net.points[v], net.points[static_cast<size_t>(a)], net.points[static_cast<size_t>(b)], net.points[d]}));
        refined.normals.push_back((net.normals[v] + net.normals[static_cast<size_t>(a)] + net.normals[static_cast<size_t>(b)] + net.normals[d]).normalized());
        refined.pairs.emplace_back(static_cast<size_t>(-1), static_cast<size_t>(-1));
    }

    std::map<std::pair<size_t, size_t>, size_t> made;
    std::vector<size_t> index(net.lines.size(), 0);
    for (size_t family = 0; family < 2; family++) {
        const size_t begin = family == 0 ? 0 : net.firsts;
        const size_t end = family == 0 ? net.firsts : net.lines.size();
        for (size_t l = begin; l < end; l++) {
            std::vector<size_t> line;
            for (size_t k = 0; k < net.lines[l].size(); k++) {
                line.push_back(net.lines[l][k]);
                if (k + 1 < net.lines[l].size())
                    line.push_back(compute_midpoint(net, refined, made, net.lines[l][k], net.lines[l][k + 1]));
            }

            index[l] = refined.lines.size();
            refined.lines.push_back(line);
            refined.lamellas.push_back(true);
            compute_midline(net, refined, made, centres, l);
            refined.lamellas.resize(refined.lines.size(), false);
        }

        if (family == 0)
            refined.firsts = refined.lines.size();
    }

    for (size_t v = 0; v < net.points.size(); v++)
        refined.pairs[v] = {index[net.pairs[v].first], index[net.pairs[v].second]};

    compute_stars(refined);

    return refined;
}

/// The largest |n . e| / |e| over every edge of the net at both its ends: how far the net is from an A-net.
inline double compute_residual(const Net& net) {

    double worst = 0.0;
    for (size_t v = 0; v < net.points.size(); v++)
        for (const long other : net.stars[v]) {
            if (other < 0)
                continue;

            const Vector e = net.points[static_cast<size_t>(other)] - net.points[v];
            worst = std::max(worst, std::abs(net.normals[v].dot(e)) / e.magnitude());
        }

    return worst;
}

// ═══════════════════════════════════════════════════════════════════════════
// Guided projection
// ═══════════════════════════════════════════════════════════════════════════

/// One row of the sparse Jacobian with its residual: the linearised constraint sum entries . delta = -value.
struct Row {
    std::vector<std::pair<size_t, double>> entries; // Column and coefficient.
    double value = 0.0; // The constraint's current value.
};

/// The weights of one round of guided projection (Schling Eq. 14).
struct Weights {
    double fairness = 5e-4; // omega1 on the second differences along every line.
    double closeness = 0.0; // omega2 on the change of every vertex.
    double proximity = 0.1; // omega3 on the tangent-plane distance to the carrier.
    double damping = 1e-3; // epsilon on every variable.
};

/// The rows of the hard constraints of an A-net on x (6 per vertex: position, normal): n . (v_i - v) = 0 for every star edge, |n|^2 - 1 = 0 (Schling Eqs. 6-7).
inline void compute_hard_rows(const Net& net, const std::vector<double>& x, std::vector<Row>& rows) {

    for (size_t v = 0; v < net.points.size(); v++) {
        const Vector n(x[6 * v + 3], x[6 * v + 4], x[6 * v + 5]);
        const Vector p(x[6 * v], x[6 * v + 1], x[6 * v + 2]);
        for (const long other : net.stars[v]) {
            if (other < 0)
                continue;

            const size_t o = static_cast<size_t>(other);
            const Vector q(x[6 * o], x[6 * o + 1], x[6 * o + 2]);
            Row row;
            row.value = n.dot(q - p);
            for (int k = 0; k < 3; k++) {
                row.entries.emplace_back(6 * v + 3 + k, (q - p)[k]);
                row.entries.emplace_back(6 * o + k, n[k]);
                row.entries.emplace_back(6 * v + k, -n[k]);
            }

            rows.push_back(row);
        }

        Row unit;
        unit.value = n.dot(n) - 1.0;
        for (int k = 0; k < 3; k++)
            unit.entries.emplace_back(6 * v + 3 + k, 2.0 * n[k]);

        rows.push_back(unit);
    }
}

/// The soft rows: fairness on the second differences along every line, proximity to the carrier's tangent plane at the foot of every vertex, closeness to the last position; each scaled by the root of its weight.
inline void compute_soft_rows(const Net& net, const std::vector<double>& x, const Carrier& carrier, const Point& origin, double scale, const Weights& weights, std::vector<Row>& rows) {

    const double fair = std::sqrt(weights.fairness);
    for (const std::vector<size_t>& line : net.lines)
        for (size_t k = 1; k + 1 < line.size(); k++)
            for (int c = 0; c < 3; c++) {
                Row row;
                row.value = fair * (x[6 * line[k - 1] + c] - 2.0 * x[6 * line[k] + c] + x[6 * line[k + 1] + c]);
                row.entries = {{6 * line[k - 1] + c, fair}, {6 * line[k] + c, -2.0 * fair}, {6 * line[k + 1] + c, fair}};
                rows.push_back(row);
            }

    const double near = std::sqrt(weights.proximity);
    for (size_t v = 0; v < net.points.size() && weights.proximity > 0.0; v++) {
        const Point p(x[6 * v], x[6 * v + 1], x[6 * v + 2]);
        const Foot foot = carrier.compute_foot(origin + (p - Point(0.0, 0.0, 0.0)) * scale);
        const Point q = Point(0.0, 0.0, 0.0) + (carrier.compute_point(foot) - origin) * (1.0 / scale);
        const Vector m = carrier.compute_normal(foot);
        Row row;
        row.value = near * (p - q).dot(m);
        for (int c = 0; c < 3; c++)
            row.entries.emplace_back(6 * v + c, near * m[c]);

        rows.push_back(row);
    }

    const double close = std::sqrt(weights.closeness);
    for (size_t v = 0; v < net.points.size() && weights.closeness > 0.0; v++)
        for (int c = 0; c < 3; c++) {
            Row row;
            row.entries.emplace_back(6 * v + c, close);
            rows.push_back(row);
        }
}

/// y = (J^T J + damping I) x for the rows.
inline std::vector<double> compute_apply(const std::vector<Row>& rows, const std::vector<double>& x, double damping) {

    std::vector<double> y(x.size(), 0.0);
    for (size_t i = 0; i < x.size(); i++)
        y[i] = damping * x[i];

    for (const Row& row : rows) {
        double dot = 0.0;
        for (const std::pair<size_t, double>& entry : row.entries)
            dot += entry.second * x[entry.first];

        for (const std::pair<size_t, double>& entry : row.entries)
            y[entry.first] += entry.second * dot;
    }

    return y;
}

/// The Gauss-Newton step of the rows, (J^T J + damping I) delta = -J^T r, by conjugate gradients.
inline std::vector<double> compute_step(const std::vector<Row>& rows, size_t n, double damping) {

    std::vector<double> b(n, 0.0);
    for (const Row& row : rows)
        for (const std::pair<size_t, double>& entry : row.entries)
            b[entry.first] -= entry.second * row.value;

    std::vector<double> x(n, 0.0);
    std::vector<double> r = b;
    std::vector<double> p = r;
    double rr = 0.0;
    for (const double value : r)
        rr += value * value;

    const double start = rr;
    for (int k = 0; k < 2000 && rr > start * 1e-28 && rr > 0.0; k++) {
        const std::vector<double> ap = compute_apply(rows, p, damping);
        double pap = 0.0;
        for (size_t i = 0; i < n; i++)
            pap += p[i] * ap[i];

        const double alpha = rr / pap;
        double next = 0.0;
        for (size_t i = 0; i < n; i++) {
            x[i] += alpha * p[i];
            r[i] -= alpha * ap[i];
            next += r[i] * r[i];
        }

        for (size_t i = 0; i < n; i++)
            p[i] = r[i] + (next / rr) * p[i];

        rr = next;
    }

    return x;
}

/// The sum of squares of the hard constraints of the net at x, the residual of Schling's Table 2.
inline double compute_hard_residual(const Net& net, const std::vector<double>& x) {

    std::vector<Row> rows;
    compute_hard_rows(net, x, rows);
    double sum = 0.0;
    for (const Row& row : rows)
        sum += row.value * row.value;

    return sum;
}

/// One round of guided projection on the net at x with the weights: the Gauss-Newton step of the hard and soft rows, damped.
inline void compute_round(const Net& net, std::vector<double>& x, const Carrier& carrier, const Point& origin, double scale, const Weights& weights) {

    std::vector<Row> rows;
    compute_hard_rows(net, x, rows);
    compute_soft_rows(net, x, carrier, origin, scale, weights, rows);
    const std::vector<double> delta = compute_step(rows, x.size(), weights.damping);
    for (size_t i = 0; i < x.size(); i++)
        x[i] += delta[i];
}

/// The net optimised to an A-net by guided projection (Schling Sec. 3.3): vertices and normals as one vector on a copy scaled to a unit bounding-box diameter, rounds with fairness 5e-3, closeness 0.01 and proximity 0.1 until the hard residual drops below 1e-6 or rounds run out, then five with fairness 5e-4 and closeness 0, then five with every soft weight 0; returns the rounds taken and the final hard residual.
inline std::pair<int, double> compute_optimised(Net& net, const Carrier& carrier, int rounds) {

    Point low = net.points.front();
    Point high = low;
    for (const Point& p : net.points)
        for (int axis = 0; axis < 3; axis++) {
            low[axis] = std::min(low[axis], p[axis]);
            high[axis] = std::max(high[axis], p[axis]);
        }

    const double scale = (high - low).magnitude();
    const Point origin = low;
    std::vector<double> x;
    for (size_t v = 0; v < net.points.size(); v++) {
        for (int c = 0; c < 3; c++)
            x.push_back((net.points[v][c] - origin[c]) / scale);

        for (int c = 0; c < 3; c++)
            x.push_back(net.normals[v][c]);
    }

    int taken = 0;
    Weights weights{5e-3, 0.01, 0.1, 1e-3};
    for (; taken < rounds && compute_hard_residual(net, x) > 1e-6; taken++)
        compute_round(net, x, carrier, origin, scale, weights);

    for (const Weights& phase : {Weights{5e-4, 0.0, 0.1, 1e-3}, Weights{0.0, 0.0, 0.0, 1e-3}})
        for (int k = 0; k < 5; k++, taken++)
            compute_round(net, x, carrier, origin, scale, phase);

    for (size_t v = 0; v < net.points.size(); v++) {
        net.points[v] = Point(origin[0] + x[6 * v] * scale, origin[1] + x[6 * v + 1] * scale, origin[2] + x[6 * v + 2] * scale);
        net.normals[v] = Vector(x[6 * v + 3], x[6 * v + 4], x[6 * v + 5]).normalized();
    }

    return {taken, compute_hard_residual(net, x)};
}

// ═══════════════════════════════════════════════════════════════════════════
// Elements on the net
// ═══════════════════════════════════════════════════════════════════════════

/// The Darboux data at parameter t of the cubic through a line's vertices: the normal the vertex normals blended along the span, exact at a vertex, the tangent the curve's made normal to it, the geodesic torsion from the turn of that normal over a small step.
inline Darboux compute_line_darboux(const NurbsCurve& curve, const std::vector<double>& parameters, const std::vector<Vector>& normals, double t) {

    const std::vector<Vector> c = curve.evaluate(t, 2);
    size_t k = 0;
    while (k + 2 < parameters.size() && parameters[k + 1] <= t)
        k++;

    const double w = std::clamp((t - parameters[k]) / (parameters[k + 1] - parameters[k]), 0.0, 1.0);
    const double h = (parameters[k + 1] - parameters[k]) * 1e-3;
    Darboux d;
    d.x = Point(c[0][0], c[0][1], c[0][2]);
    d.n = (normals[k] * (1.0 - w) + normals[k + 1] * w).normalized();
    d.t = (c[1] - d.n * c[1].dot(d.n)).normalized();
    d.u = d.n.cross(d.t);
    const double speed2 = c[1].dot(c[1]);
    const Vector bend = (c[2] * speed2 - c[1] * c[1].dot(c[2])) * (1.0 / (speed2 * speed2));
    d.kn = bend.dot(d.n);
    d.kg = bend.dot(d.u);
    const Vector turn = (normals[k] * (1.0 - std::min(w + h, 1.0)) + normals[k + 1] * std::min(w + h, 1.0)).normalized() - (normals[k] * (1.0 - std::max(w - h, 0.0)) + normals[k + 1] * std::max(w - h, 0.0)).normalized();
    d.tg = -turn.dot(d.u) / ((std::min(w + h, 1.0) - std::max(w - h, 0.0)) * (parameters[k + 1] - parameters[k]) * std::sqrt(speed2));

    return d;
}

/// The stations of one line of the net and the ruling at each: its vertices and samples every sample mm of the cubic through them, the ruling the strip's blended to the normal near the nodes (the vertices where a stud stands), as on a surface; the frames at the vertices, the same as their stations, into corners.
inline void compute_line_stations(const Net& net, const std::vector<size_t>& line, const Path& path, const Lamella& lamella, std::vector<Plane>& stations, std::vector<Vector>& rulings, std::vector<Plane>& corners, double& lean, size_t& capped) {

    std::vector<Point> points;
    std::vector<Vector> normals;
    std::vector<Point> joints;
    for (const size_t v : line) {
        points.push_back(net.points[v]);
        normals.push_back(net.normals[v]);
        if (net.pairs[v].first != static_cast<size_t>(-1))
            joints.push_back(net.points[v]);
    }

    const NurbsCurve curve = NurbsCurve::create_interpolated(points);
    std::vector<double> parameters{curve.domain().first};
    std::vector<double> chords{0.0};
    for (size_t k = 0; k + 1 < points.size(); k++)
        chords.push_back(chords.back() + (points[k + 1] - points[k]).magnitude());

    for (size_t k = 1; k < points.size(); k++)
        parameters.push_back(curve.domain().first + (curve.domain().second - curve.domain().first) * chords[k] / chords.back());

    const int count = std::max(2, static_cast<int>(std::ceil(chords.back() / lamella.sample)));
    std::vector<double> ts = parameters;
    for (int k = 0; k <= count; k++) {
        const double t = curve.domain().first + (curve.domain().second - curve.domain().first) * k / count;
        bool free = true;
        for (const double node : parameters)
            free = free && std::abs(t - node) > (curve.domain().second - curve.domain().first) / count / 2.0;

        if (free)
            ts.push_back(t);
    }

    std::sort(ts.begin(), ts.end());
    for (const double t : parameters) {
        const Darboux d = compute_line_darboux(curve, parameters, normals, t);
        corners.emplace_back(d.x, d.t, d.u);
    }

    for (const double t : ts) {
        const Darboux d = compute_line_darboux(curve, parameters, normals, t);
        double distance = 1e300;
        for (const Point& p : joints)
            distance = std::min(distance, (p - d.x).magnitude());

        stations.emplace_back(d.x, d.t, d.u);
        rulings.push_back(compute_ruling(d, path, lamella, compute_blend(distance, lamella)));
        lean = std::max(lean, d.kg == 0.0 ? 90.0 : std::atan(std::abs(d.tg / d.kg)) * 180.0 / Tolerance::PI);
        capped += d.kg == 0.0 || std::abs(d.tg / d.kg) > lamella.ruling ? 1 : 0;
    }
}

/// The frame of a line at one of its vertices, the station of its board there: origin the vertex, z its shared normal, x along the cubic through the line made normal to it.
inline Plane compute_vertex_frame(const std::vector<size_t>& line, const std::vector<Plane>& corners, size_t vertex) {
    const size_t k = static_cast<size_t>(std::find(line.begin(), line.end(), vertex) - line.begin());
    return corners[k];
}

/// The gridshell on an optimised A-net: two boards per line through its stations, the first family a layer up the shared normals and the second a layer down, a stud on the shared normal at every vertex.
inline Gridshell compute_net_gridshell(const Net& net, const Lamella& lamella) {

    Gridshell gridshell;
    gridshell.path = Path::normal_curvature(0.0);
    const double lift = lamella.spacing / 2.0;
    const double shift = (lamella.gap + lamella.thickness) / 2.0;
    std::vector<long> board(net.lines.size(), -1);
    std::vector<std::vector<Plane>> corners(net.lines.size());
    std::vector<size_t> lamella_of;
    for (size_t l = 0; l < net.lines.size(); l++) {
        if (!net.lamellas[l])
            continue;

        if (l < net.firsts)
            gridshell.tops++;

        lamella_of.push_back(l);
        std::vector<Plane> stations;
        std::vector<Vector> rulings;
        if (net.lines[l].size() >= 2)
            compute_line_stations(net, net.lines[l], gridshell.path, lamella, stations, rulings, corners[l], gridshell.lean, gridshell.capped);

        gridshell.frames.push_back(stations);
        if (stations.size() < 2)
            continue;

        const bool upper = l < net.firsts;
        const size_t index = lamella_of.size() - 1;
        const std::string name = upper ? fmt::format("lamella_top_{}", index) : fmt::format("lamella_bottom_{}", index - gridshell.tops);
        std::vector<std::shared_ptr<BeamCurved>>& layer = upper ? gridshell.top : gridshell.bottom;
        board[l] = static_cast<long>(layer.size());
        layer.push_back(compute_board(stations, rulings, upper ? lift : -lift, shift, lamella, name + "_a"));
        layer.push_back(compute_board(stations, rulings, upper ? lift : -lift, -shift, lamella, name + "_b"));
    }

    for (size_t v = 0; v < net.points.size(); v++) {
        if (net.pairs[v].first == static_cast<size_t>(-1))
            continue;

        const size_t top = net.pairs[v].first;
        const size_t bottom = net.pairs[v].second;
        if (board[top] < 0 || board[bottom] < 0)
            continue;

        gridshell.nodes.push_back({top, bottom, 0.0, 0.0});
        gridshell.studs.push_back(compute_stud(compute_vertex_frame(net.lines[top], corners[top], v), compute_vertex_frame(net.lines[bottom], corners[bottom], v), lamella, fmt::format("stud_{}_{}", top, bottom)));
        gridshell.pairs.emplace_back(static_cast<size_t>(board[top]) / 2, static_cast<size_t>(board[bottom]) / 2);
    }

    return gridshell;
}

/// The gridshell of an A-net on a mesh from two families of asymptotic polylines: the crossing net optimised by guided projection, refined once by bilinear subdivision and optimised again, then boards on the lamella lines and studs at the crossings; the net's residual before and after, the rounds taken and the refined vertex count are written into the gridshell.
inline Gridshell compute_mesh_gridshell(const Carrier& carrier, const std::vector<std::vector<Point>>& firsts, const std::vector<std::vector<Point>>& seconds, const Lamella& lamella, int rounds) {

    Net coarse = compute_net(carrier, firsts, seconds, lamella.step / 5.0);
    const double traced = compute_residual(coarse);
    const std::pair<int, double> first = compute_optimised(coarse, carrier, rounds);
    Net net = compute_refined(coarse);
    const std::pair<int, double> second = compute_optimised(net, carrier, rounds);
    Gridshell gridshell = compute_net_gridshell(net, lamella);
    gridshell.traced = traced;
    gridshell.optimised = compute_residual(net);
    gridshell.residual = second.second;
    gridshell.rounds = first.first + second.first;
    gridshell.vertices = net.points.size();

    return gridshell;
}

/// The gridshell of a mesh: both families traced from seeds put on the mesh, the first family along the asymptotic direction closer to the first one at the mesh point nearest centre, the second along the other, then compute_mesh_gridshell.
inline Gridshell compute_traced_gridshell(const Carrier& carrier, const std::vector<Point>& seeds_top, const std::vector<Point>& seeds_bottom, const Point& centre, const Lamella& lamella, int rounds) {

    const std::vector<Vector> directions = carrier.compute_directions(carrier.compute_foot(centre));
    std::vector<std::vector<std::vector<Point>>> families(2);
    for (int f = 0; f < 2 && directions.size() == 2; f++) {
        Vector reference = directions[f];
        for (const Point& seed : f == 0 ? seeds_top : seeds_bottom) {
            const std::vector<Point> trace = compute_mesh_trace(carrier, seed, reference, lamella.step);
            if (trace.size() < 2)
                continue;

            families[f].push_back(trace);
            const std::vector<Vector> local = carrier.compute_directions(carrier.compute_foot(seed));
            reference = std::abs(local[0].dot(reference)) >= std::abs(local[1].dot(reference)) ? local[0] : local[1];
        }
    }

    return compute_mesh_gridshell(carrier, families[0], families[1], lamella, rounds);
}

/// The gridshell of a rotational mesh about axis through centre: one asymptotic curve traced from seed, count copies turned about the axis as the first family and their mirror images as the second, then compute_mesh_gridshell.
inline Gridshell compute_rotational_gridshell(const Carrier& carrier, const Point& seed, const Point& centre, const Vector& axis, int count, const Lamella& lamella, int rounds) {

    const std::vector<Vector> directions = carrier.compute_directions(carrier.compute_foot(seed));
    if (directions.size() < 2)
        return Gridshell();

    const std::vector<Point> trace = compute_mesh_trace(carrier, seed, directions[0], lamella.step);
    const std::pair<std::vector<std::vector<Point>>, std::vector<std::vector<Point>>> families = compute_rotational(trace, centre, axis, count);

    return compute_mesh_gridshell(carrier, families.first, families.second, lamella, rounds);
}

} // namespace wood_gridshell
