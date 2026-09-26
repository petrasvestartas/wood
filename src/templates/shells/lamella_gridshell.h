#pragma once
#include "wood_session.h"
#include "wood_profile.h"

namespace wood_gridshell {

using namespace session_cpp;
using namespace wood_session;

const int ITERATIONS = 60; // block-coordinate descent sweeps of the net optimisation

/// Board, gap and stud sizes of a two-layer lamella gridshell, in mm.
struct Lamella {
    double height = 140.0; // Board depth along the surface normal.
    double thickness = 20.0; // Board thickness across the lamella.
    double gap = 60.0; // Clear distance between the two boards of a lamella, the stud width across every flat.
    double spacing = 180.0; // Normal distance between the top and the bottom layer centrelines; below height the layers overlap.
    double overrun = 20.0; // Stud length past the outer face of each layer.
    double step = 100.0; // Tracing step along a lamella on the surface; a sample nearer a crossing than twice the gap is dropped.
};

// ═══════════════════════════════════════════════════════════════════════════
// Fields
// ═══════════════════════════════════════════════════════════════════════════

/// The two directions (a, b, 0) of an orthonormal or parameter basis where l a^2 + 2 m a b + n b^2 = 0, none where that form is definite.
inline std::vector<Vector> compute_asymptotic(double l, double m, double n) {

    const double mean = (l + n) / 2.0;
    const double radius = std::hypot((l - n) / 2.0, m);
    std::vector<Vector> directions;
    if (radius < 1e-12 || std::abs(mean) > radius)
        return directions;

    const double phi = std::atan2(m, (l - n) / 2.0);
    const double spread = std::acos(-mean / radius);
    for (const double sign : {-1.0, 1.0})
        directions.emplace_back(std::cos((phi + sign * spread) / 2.0), std::sin((phi + sign * spread) / 2.0), 0.0);

    return directions;
}

/// What the lamellas are traced on: a state is a point of the field's own space, (u, v, 0) on a surface, xyz on a mesh.
struct Field {
    virtual ~Field() = default;

    /// The state both families are seeded from.
    virtual Point centre() const = 0;

    /// The two lamella directions at state in state space, each one mm long on the carrier; none where there are none.
    virtual std::vector<Vector> compute_directions(const Point& state) const = 0;

    /// The carrier vector of a state-space direction at state.
    virtual Vector compute_tangent(const Point& state, const Vector& direction) const = 0;

    /// The carrier point of state.
    virtual Point compute_point(const Point& state) const = 0;

    /// The unit carrier normal at state.
    virtual Vector compute_normal(const Point& state) const = 0;

    /// The state delta leads to from state, clipped at the carrier's edge, and true when clipped.
    virtual std::pair<Point, bool> compute_move(const Point& state, const Vector& delta) const = 0;

    /// The state of the carrier point nearest point, searched from hint.
    virtual Point compute_state(const Point& point, const Point& hint) const = 0;

    /// True when the two families are asymptotic curves, the only ones the net optimisation keeps.
    virtual bool is_asymptotic() const = 0;
};

/// The iso or asymptotic curves of a NURBS surface, states in (u, v, 0).
struct SurfaceField : Field {
    const NurbsSurface& surface; // Carrier.
    int curves; // 0 the u and v iso-curves, 1 the asymptotic curves.

    /// The field of curves on surface.
    SurfaceField(const NurbsSurface& surface, int curves) : surface(surface), curves(curves) {
    }

    /// The middle of the domain.
    Point centre() const override {
        return Point((surface.domain(0).first + surface.domain(0).second) / 2.0, (surface.domain(1).first + surface.domain(1).second) / 2.0, 0.0);
    }

    /// u and v for curves 0, for curves 1 the asymptotic directions where II(d, d) = 0, none where the Gaussian curvature is positive.
    std::vector<Vector> compute_directions(const Point& state) const override {

        const std::vector<Vector> derivatives = surface.evaluate(state[0], state[1], 2);
        std::vector<Vector> directions;
        if (curves == 0) {
            directions.push_back(Vector(1.0, 0.0, 0.0) / derivatives[3].magnitude());
            directions.push_back(Vector(0.0, 1.0, 0.0) / derivatives[1].magnitude());
            return directions;
        }

        const Vector normal = surface.normal_at(state[0], state[1]);
        for (const Vector& direction : compute_asymptotic(derivatives[5].dot(normal), derivatives[4].dot(normal), derivatives[2].dot(normal)))
            directions.push_back(direction / compute_tangent(state, direction).magnitude());

        return directions;
    }

    /// The surface vector of a (du, dv, 0) direction.
    Vector compute_tangent(const Point& state, const Vector& direction) const override {

        const std::vector<Vector> derivatives = surface.evaluate(state[0], state[1], 1);

        return derivatives[2] * direction[0] + derivatives[1] * direction[1];
    }

    /// The surface point at (u, v).
    Point compute_point(const Point& state) const override {
        return surface.point_at(state[0], state[1]);
    }

    /// The surface normal at (u, v).
    Vector compute_normal(const Point& state) const override {
        return surface.normal_at(state[0], state[1]);
    }

    /// The share of delta that stays inside the domain.
    std::pair<Point, bool> compute_move(const Point& state, const Vector& delta) const override {

        double share = 1.0;
        for (int dir = 0; dir < 2; dir++) {
            const std::pair<double, double> domain = surface.domain(dir);
            if (state[dir] + delta[dir] < domain.first)
                share = std::min(share, (domain.first - state[dir]) / delta[dir]);

            if (state[dir] + delta[dir] > domain.second)
                share = std::min(share, (domain.second - state[dir]) / delta[dir]);
        }

        share = std::max(share, 0.0);

        return {state + delta * share, share < 1.0};
    }

    /// True for curves 1.
    bool is_asymptotic() const override {
        return curves == 1;
    }

    /// The (u, v) of the surface point nearest point: Gauss-Newton steps from hint, clamped to the domain.
    Point compute_state(const Point& point, const Point& hint) const override {

        Point uv = hint;
        for (int k = 0; k < 6; k++) {
            const std::vector<Vector> derivatives = surface.evaluate(uv[0], uv[1], 1);
            const Vector offset = surface.point_at(uv[0], uv[1]) - point;
            const Vector& du = derivatives[2];
            const Vector& dv = derivatives[1];
            const double a = du.dot(du);
            const double b = du.dot(dv);
            const double c = dv.dot(dv);
            const double determinant = a * c - b * b;
            if (std::abs(determinant) < 1e-30)
                break;

            const double gu = du.dot(offset);
            const double gv = dv.dot(offset);
            uv = Point(std::clamp(uv[0] - (c * gu - b * gv) / determinant, surface.domain(0).first, surface.domain(0).second), std::clamp(uv[1] - (a * gv - b * gu) / determinant, surface.domain(1).first, surface.domain(1).second), 0.0);
        }

        return uv;
    }
};

/// Where a point lands on a triangle mesh.
struct Foot {
    size_t triangle; // Triangle index.
    Vector weights; // Barycentric weights of its three corners.
};

/// The asymptotic curves of a triangle or quad mesh, states in xyz: vertex normals and shape operators, blended linearly inside each triangle, the directions where the blended second fundamental form vanishes.
struct MeshField : Field {
    std::vector<Point> points; // Vertex positions.
    std::vector<std::array<size_t, 3>> triangles; // Faces fanned into triangles.
    std::vector<Vector> normals; // Unit vertex normals, Max's weights, exact on a sphere.
    std::vector<std::array<Vector, 3>> tensors; // Shape operator per vertex, its three rows, summed over the faces around weighted by twice their area.
    std::vector<bool> open; // True for the boundary vertices.
    double cell = 1.0; // Side of the grid cells the triangles are bucketed in, twice the mean edge.
    std::unordered_map<long long, std::vector<size_t>> buckets; // Triangles per grid cell their box touches.

    /// The field of mesh, each face fanned from its first corner.
    explicit MeshField(const Mesh& mesh) {

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

        normals.assign(points.size(), Vector(0.0, 0.0, 0.0));
        for (const std::array<size_t, 3>& t : triangles)
            for (int k = 0; k < 3; k++) {
                const Vector a = points[t[(k + 1) % 3]] - points[t[k]];
                const Vector b = points[t[(k + 2) % 3]] - points[t[k]];
                normals[t[k]] = normals[t[k]] + a.cross(b) / (a.dot(a) * b.dot(b));
            }

        for (Vector& normal : normals)
            normal = normal.normalized();

        tensors.assign(points.size(), {Vector(0.0, 0.0, 0.0), Vector(0.0, 0.0, 0.0), Vector(0.0, 0.0, 0.0)});
        for (const std::array<size_t, 3>& t : triangles) {
            const std::array<Vector, 3> tensor = compute_face_tensor(t);
            const double area = (points[t[1]] - points[t[0]]).cross(points[t[2]] - points[t[0]]).magnitude();
            for (const size_t corner : t)
                for (int row = 0; row < 3; row++)
                    tensors[corner][row] = tensors[corner][row] + tensor[row] * area;
        }

        std::set<std::pair<size_t, size_t>> edges;
        for (const std::array<size_t, 3>& t : triangles)
            for (int k = 0; k < 3; k++) {
                const std::pair<size_t, size_t> edge(std::min(t[k], t[(k + 1) % 3]), std::max(t[k], t[(k + 1) % 3]));
                if (!edges.erase(edge))
                    edges.insert(edge);
            }

        open.assign(points.size(), false);
        for (const std::pair<size_t, size_t>& edge : edges) {
            open[edge.first] = true;
            open[edge.second] = true;
        }

        double length = 0.0;
        for (const std::array<size_t, 3>& t : triangles)
            length += (points[t[1]] - points[t[0]]).magnitude();

        cell = 2.0 * length / static_cast<double>(triangles.size());
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

    /// The point a foot stands on, lifted off its flat triangle by half of Phong tessellation, the blend of its projections onto the three vertex tangent planes, which puts it on the circle through a chord's ends.
    Point compute_blend(const Foot& foot) const {

        const std::array<size_t, 3>& t = triangles[foot.triangle];
        const Point flat = points[t[0]] + (points[t[1]] - points[t[0]]) * foot.weights[1] + (points[t[2]] - points[t[0]]) * foot.weights[2];
        Vector lift(0.0, 0.0, 0.0);
        for (int k = 0; k < 3; k++)
            lift = lift - normals[t[k]] * ((flat - points[t[k]]).dot(normals[t[k]]) * foot.weights[k]);

        return flat + lift * 0.5;
    }

    /// The nearest mesh point.
    Point centre() const override {

        Vector sum(0.0, 0.0, 0.0);
        for (const Point& point : points)
            sum = sum + (point - Point(0.0, 0.0, 0.0));

        const Point middle = Point(0.0, 0.0, 0.0) + sum / static_cast<double>(points.size());
        Point best = points[0];
        for (const Point& point : points)
            if ((point - middle).magnitude() < (best - middle).magnitude())
                best = point;

        return best;
    }

    /// The asymptotic directions of the blended shape operator in the tangent plane of the blended normal.
    std::vector<Vector> compute_directions(const Point& state) const override {

        const Foot foot = compute_foot(state);
        const Vector normal = compute_normal(state);
        const Vector x = (std::abs(normal[0]) < 0.9 ? Vector(1.0, 0.0, 0.0) : Vector(0.0, 1.0, 0.0)).cross(normal).normalized();
        const Vector y = normal.cross(x);
        Vector along_x(0.0, 0.0, 0.0);
        Vector along_y(0.0, 0.0, 0.0);
        for (int k = 0; k < 3; k++) {
            const std::array<Vector, 3>& tensor = tensors[triangles[foot.triangle][k]];
            along_x = along_x + Vector(tensor[0].dot(x), tensor[1].dot(x), tensor[2].dot(x)) * foot.weights[k];
            along_y = along_y + Vector(tensor[0].dot(y), tensor[1].dot(y), tensor[2].dot(y)) * foot.weights[k];
        }

        std::vector<Vector> directions;
        for (const Vector& direction : compute_asymptotic(x.dot(along_x), x.dot(along_y), y.dot(along_y)))
            directions.push_back(x * direction[0] + y * direction[1]);

        return directions;
    }

    /// The direction itself, states being xyz.
    Vector compute_tangent(const Point&, const Vector& direction) const override {
        return direction;
    }

    /// The nearest mesh point.
    Point compute_point(const Point& state) const override {
        return compute_blend(compute_foot(state));
    }

    /// The blended vertex normal at the nearest mesh point.
    Vector compute_normal(const Point& state) const override {

        const Foot foot = compute_foot(state);
        Vector normal(0.0, 0.0, 0.0);
        for (int k = 0; k < 3; k++)
            normal = normal + normals[triangles[foot.triangle][k]] * foot.weights[k];

        return normal.normalized();
    }

    /// True for the vertices of the triangles that touch the boundary, the band whose one-sided normals are not trusted.
    std::vector<bool> compute_band() const {

        std::vector<bool> band(points.size(), false);
        for (const std::array<size_t, 3>& t : triangles)
            if (open[t[0]] || open[t[1]] || open[t[2]])
                for (const size_t corner : t)
                    band[corner] = true;

        return band;
    }

    /// The mesh point nearest state plus delta; no move, clipped, where that point lies on a triangle touching the boundary.
    std::pair<Point, bool> compute_move(const Point& state, const Vector& delta) const override {

        const Foot foot = compute_foot(state + delta);
        const std::array<size_t, 3>& t = triangles[foot.triangle];
        if (open[t[0]] || open[t[1]] || open[t[2]])
            return {state, true};

        return {compute_blend(foot), false};
    }

    /// Always: a mesh carries asymptotic curves only.
    bool is_asymptotic() const override {
        return true;
    }

    /// The point itself, states being xyz; every reader projects it.
    Point compute_state(const Point& point, const Point&) const override {
        return point;
    }

    /// Largest share of mean curvature over the vertices off the boundary band, |k1 + k2| / sqrt(2 (k1^2 + k2^2)) from the vertex shape operators: 0 on a minimal surface, 1 on a sphere.
    double compute_mean_curvature() const {

        const std::vector<bool> band = compute_band();
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
// Tracing
// ═══════════════════════════════════════════════════════════════════════════

/// The state-space lamella direction at state nearest previous on the carrier, turned to agree with it; zero where there is none.
inline Vector compute_slope(const Field& field, const Point& state, const Vector& previous) {

    Vector slope(0.0, 0.0, 0.0);
    double best = -1.0;
    for (const Vector& direction : field.compute_directions(state)) {
        const double dot = field.compute_tangent(state, direction).dot(previous);
        if (std::abs(dot) > best) {
            best = std::abs(dot);
            slope = dot < 0.0 ? -direction : direction;
        }
    }

    return slope;
}

/// The frame of a lamella at state: x its direction nearest previous, y the normal cross x, z the normal.
inline Plane compute_frame(const Field& field, const Point& state, const Vector& previous) {

    const Vector tangent = field.compute_tangent(state, compute_slope(field, state, previous));

    return Plane(field.compute_point(state), tangent, field.compute_normal(state).cross(tangent));
}

/// States from seed along the lamella direction nearest direction: RK4 steps of step mm, the last clipped to the carrier's edge; ends early where the direction field does.
inline std::vector<Point> compute_path(const Field& field, const Point& seed, Vector direction, double step) {

    std::vector<Point> states{seed};
    for (int k = 0; k < 100000; k++) {
        const Point state = states.back();
        const Vector k1 = compute_slope(field, state, direction) * step;
        const Vector k2 = compute_slope(field, state + k1 * 0.5, direction) * step;
        const Vector k3 = compute_slope(field, state + k2 * 0.5, direction) * step;
        const Vector k4 = compute_slope(field, state + k3, direction) * step;
        if (k1.magnitude() == 0.0)
            break;

        const std::pair<Point, bool> next = field.compute_move(state, (k1 + k2 * 2.0 + k3 * 2.0 + k4) / 6.0);
        if ((next.first - state).magnitude() < 1e-9 * step)
            break;

        states.push_back(next.first);
        direction = field.compute_point(next.first) - field.compute_point(state);
        if (next.second)
            break;
    }

    return states;
}

/// The lamella through seed both ways along the direction nearest hint, from one end to the other.
inline std::vector<Point> compute_curve(const Field& field, const Point& seed, const Vector& hint, double step) {

    const Vector along = field.compute_tangent(seed, compute_slope(field, seed, hint));
    std::vector<Point> states = compute_path(field, seed, -along, step);
    const std::vector<Point> ahead = compute_path(field, seed, along, step);
    std::reverse(states.begin(), states.end());
    states.insert(states.end(), ahead.begin() + 1, ahead.end());

    return states;
}

/// count lamellas of family 0 or 1, seeded at even arc lengths along a spine of the other family through the field's centre.
inline std::vector<std::vector<Point>> compute_family(const Field& field, int family, int count, double step) {

    const Point centre = field.centre();
    const std::vector<Point> spine = compute_curve(field, centre, field.compute_tangent(centre, field.compute_directions(centre)[1 - family]), step);

    std::vector<double> lengths{0.0};
    for (size_t k = 0; k + 1 < spine.size(); k++)
        lengths.push_back(lengths.back() + (field.compute_point(spine[k + 1]) - field.compute_point(spine[k])).magnitude());

    std::vector<std::vector<Point>> lamellas;
    size_t k = 0;
    for (int i = 0; i < count; i++) {
        const double at = lengths.back() * (i + 0.5) / count;
        while (k + 2 < lengths.size() && lengths[k + 1] < at)
            k++;

        const Point seed = spine[k] + (spine[k + 1] - spine[k]) * ((at - lengths[k]) / (lengths[k + 1] - lengths[k]));
        const Vector chord = field.compute_point(spine[k + 1]) - field.compute_point(spine[k]);
        lamellas.push_back(compute_curve(field, seed, field.compute_normal(seed).cross(chord), step));
    }

    return lamellas;
}

/// The frame at every state of a lamella, each turned along the chord that leaves it, the last along the chord that reaches it.
inline std::vector<Plane> compute_frames(const Field& field, const std::vector<Point>& states) {

    std::vector<Plane> frames;
    for (size_t k = 0; k < states.size(); k++) {
        const size_t i = std::min(k, states.size() - 2);
        frames.push_back(compute_frame(field, states[k], field.compute_point(states[i + 1]) - field.compute_point(states[i])));
    }

    return frames;
}

// ═══════════════════════════════════════════════════════════════════════════
// Crossings
// ═══════════════════════════════════════════════════════════════════════════

/// Where a top lamella crosses a bottom lamella.
struct Crossing {
    size_t top; // Top lamella.
    size_t bottom; // Bottom lamella.
    double along_top; // Position along the top lamella, segment index plus fraction.
    double along_bottom; // Position along the bottom lamella, segment index plus fraction.
};

/// Every crossing of the two families: segment against segment in the tangent plane at the top segment's start, where the two segments come within their lengths of each other, once where two neighbouring segments both find it.
inline std::vector<Crossing> compute_crossings(const std::vector<std::vector<Plane>>& tops, const std::vector<std::vector<Plane>>& bottoms) {

    std::vector<Crossing> crossings;
    for (size_t a = 0; a < tops.size(); a++)
        for (size_t b = 0; b < bottoms.size(); b++)
            for (size_t i = 0; i + 1 < tops[a].size(); i++)
                for (size_t j = 0; j + 1 < bottoms[b].size(); j++) {
                    const Plane& frame = tops[a][i];
                    const Vector ahead = tops[a][i + 1].origin() - frame.origin();
                    const Vector across = bottoms[b][j + 1].origin() - bottoms[b][j].origin();
                    const Vector offset = bottoms[b][j].origin() - frame.origin();
                    if (offset.magnitude() > ahead.magnitude() + across.magnitude())
                        continue;

                    const Vector x = frame.x_axis();
                    const Vector y = frame.y_axis();
                    const double denominator = ahead.dot(x) * across.dot(y) - ahead.dot(y) * across.dot(x);
                    if (std::abs(denominator) < 1e-30)
                        continue;

                    const double top = (offset.dot(x) * across.dot(y) - offset.dot(y) * across.dot(x)) / denominator;
                    const double bottom = (offset.dot(x) * ahead.dot(y) - offset.dot(y) * ahead.dot(x)) / denominator;
                    bool seen = false;
                    for (const Crossing& crossing : crossings)
                        seen = seen || (crossing.top == a && crossing.bottom == b && std::abs(crossing.along_top - i - top) < 1.0);

                    if (top >= 0.0 && top < 1.0 && bottom >= 0.0 && bottom < 1.0 && !seen)
                        crossings.push_back(Crossing{a, b, i + top, j + bottom});
                }

    return crossings;
}

// ═══════════════════════════════════════════════════════════════════════════
// Net
// ═══════════════════════════════════════════════════════════════════════════

/// A discrete net of two curve families (Wang, Almaskin, Pottmann 2025): a node where a top lamella crosses a bottom one, each lamella the sequence of its nodes, one normal per node that both lamellas share.
struct Net {
    std::vector<Point> points; // Node positions.
    std::vector<Vector> normals; // Unit node normals.
    std::vector<Point> states; // Field state of the carrier point nearest each node.
    std::vector<std::vector<size_t>> lamellas; // Nodes along every lamella in order, the top lamellas first.
    std::vector<std::pair<size_t, size_t>> crossings; // Top and bottom lamella through every node; a sample holds its one lamella twice.
    std::vector<bool> joints; // True for a crossing of two lamellas, false for a traced sample on one.
};

/// The net of two traced families: a node at every crossing, at the top lamella's traced state there, and every traced sample farther than a quarter step from a crossing as a vertex of its own lamella, all ordered along each lamella.
inline Net compute_net(const Field& field, const std::vector<std::vector<Point>>& states, const std::vector<std::vector<Plane>>& traces, size_t tops, double step) {

    Net net;
    net.lamellas.resize(traces.size());
    std::vector<std::vector<std::pair<double, size_t>>> along(traces.size());
    for (const Crossing& crossing : compute_crossings({traces.begin(), traces.begin() + tops}, {traces.begin() + tops, traces.end()})) {
        const std::vector<Point>& path = states[crossing.top];
        const size_t i = static_cast<size_t>(crossing.along_top);
        const Point state = path[i] + (path[i + 1] - path[i]) * (crossing.along_top - i);
        along[crossing.top].emplace_back(crossing.along_top, net.points.size());
        along[tops + crossing.bottom].emplace_back(crossing.along_bottom, net.points.size());
        net.states.push_back(state);
        net.points.push_back(field.compute_point(state));
        net.normals.push_back(field.compute_normal(state));
        net.crossings.emplace_back(crossing.top, crossing.bottom);
        net.joints.push_back(true);
    }

    for (size_t l = 0; l < along.size(); l++) {
        const std::vector<std::pair<double, size_t>> nodes = along[l];
        for (size_t k = 0; k < states[l].size(); k++) {
            bool free = true;
            for (const std::pair<double, size_t>& node : nodes)
                free = free && (traces[l][k].origin() - net.points[node.second]).magnitude() > step / 4.0;

            if (!free)
                continue;

            along[l].emplace_back(static_cast<double>(k), net.points.size());
            net.states.push_back(states[l][k]);
            net.points.push_back(traces[l][k].origin());
            net.normals.push_back(traces[l][k].z_axis());
            net.crossings.emplace_back(l, l);
            net.joints.push_back(false);
        }
    }

    for (size_t l = 0; l < along.size(); l++) {
        std::sort(along[l].begin(), along[l].end());
        for (const std::pair<double, size_t>& node : along[l])
            net.lamellas[l].push_back(node.second);
    }

    return net;
}

/// The unit vector v minimising v^T m v, the smallest eigenvector of a symmetric 3 x 3 matrix given by rows, by power iteration on trace(m) I - m from start.
inline Vector compute_smallest(const std::array<Vector, 3>& m, Vector start) {

    const double trace = m[0][0] + m[1][1] + m[2][2];
    for (int k = 0; k < 60; k++)
        start = (start * trace - Vector(m[0].dot(start), m[1].dot(start), m[2].dot(start))).normalized();

    return start;
}

/// Every node normal the best fit to the node's star, normal to all its edges along both lamellas as the A-net condition asks, pulled weakly to the carrier normal and turned to agree with it.
inline void compute_normals(const Field& field, Net& net) {

    std::vector<std::array<Vector, 3>> stars(net.points.size(), {Vector(0.0, 0.0, 0.0), Vector(0.0, 0.0, 0.0), Vector(0.0, 0.0, 0.0)});
    for (const std::vector<size_t>& lamella : net.lamellas)
        for (size_t k = 0; k + 1 < lamella.size(); k++) {
            const Vector edge = (net.points[lamella[k + 1]] - net.points[lamella[k]]).normalized();
            for (const size_t node : {lamella[k], lamella[k + 1]})
                for (int row = 0; row < 3; row++)
                    stars[node][row] = stars[node][row] + edge * edge[row];
        }

    for (size_t i = 0; i < net.points.size(); i++) {
        const Vector carrier = field.compute_normal(net.states[i]);
        for (int row = 0; row < 3; row++)
            stars[i][row] = stars[i][row] + (Vector(row == 0, row == 1, row == 2) - carrier * carrier[row]) * 1e-3;

        const Vector normal = compute_smallest(stars[i], net.normals[i]);
        net.normals[i] = normal.dot(carrier) < 0.0 ? -normal : normal;
    }
}

/// One row of a sparse least-squares system: coefficients on unknowns and the right-hand side.
struct Row {
    std::vector<std::pair<size_t, double>> terms; // Unknown index and coefficient.
    double value; // Right-hand side.
};

/// The least-squares solution of rows by conjugate gradients on the normal equations, starting from x.
inline std::vector<double> compute_least_squares(const std::vector<Row>& rows, std::vector<double> x) {

    const size_t n = x.size();
    std::vector<double> gradient(n, 0.0);
    for (const Row& row : rows) {
        double residual = row.value;
        for (const std::pair<size_t, double>& term : row.terms)
            residual -= term.second * x[term.first];

        for (const std::pair<size_t, double>& term : row.terms)
            gradient[term.first] += term.second * residual;
    }

    std::vector<double> direction = gradient;
    double norm = 0.0;
    for (const double g : gradient)
        norm += g * g;

    for (int k = 0; k < 150 && norm > 1e-24; k++) {
        std::vector<double> product(n, 0.0);
        for (const Row& row : rows) {
            double dot = 0.0;
            for (const std::pair<size_t, double>& term : row.terms)
                dot += term.second * direction[term.first];

            for (const std::pair<size_t, double>& term : row.terms)
                product[term.first] += term.second * dot;
        }

        double curvature = 0.0;
        for (size_t i = 0; i < n; i++)
            curvature += direction[i] * product[i];

        const double step = norm / curvature;
        double next = 0.0;
        for (size_t i = 0; i < n; i++) {
            x[i] += step * direction[i];
            gradient[i] -= step * product[i];
            next += gradient[i] * gradient[i];
        }

        for (size_t i = 0; i < n; i++)
            direction[i] = gradient[i] + direction[i] * next / norm;

        norm = next;
    }

    return x;
}

/// The node positions for fixed normals, least squares over the asymptotic condition n_i . (p_j - p_i) = 0 at both ends of every edge (Eq. 1), fairness 2 p_i - p_i-1 - p_i+1 along every lamella (Eq. 8, weight 1e-3), the normal distance to the carrier (weight 0.1) and a damping to the current positions (weight 0.01); every row scaled by the mean edge length.
inline void compute_positions(const Field& field, Net& net) {

    double length = 0.0;
    size_t count = 0;
    for (const std::vector<size_t>& lamella : net.lamellas)
        for (size_t k = 0; k + 1 < lamella.size(); k++) {
            length += (net.points[lamella[k + 1]] - net.points[lamella[k]]).magnitude();
            count++;
        }

    const double scale = 1.0 / (length / std::max<size_t>(count, 1));
    const double fair = 1e-3;
    const double close = 1e-1;
    const double damping = 1e-2;
    std::vector<Row> rows;
    for (const std::vector<size_t>& lamella : net.lamellas)
        for (size_t k = 0; k + 1 < lamella.size(); k++) {
            const size_t a = lamella[k];
            const size_t b = lamella[k + 1];
            for (const size_t node : {a, b}) {
                Row row{{}, 0.0};
                for (int axis = 0; axis < 3; axis++) {
                    row.terms.emplace_back(3 * b + axis, net.normals[node][axis] * scale);
                    row.terms.emplace_back(3 * a + axis, -net.normals[node][axis] * scale);
                }

                rows.push_back(row);
            }

            if (k + 2 < lamella.size())
                for (int axis = 0; axis < 3; axis++)
                    rows.push_back(Row{{{3 * lamella[k] + axis, -fair * scale}, {3 * b + axis, 2.0 * fair * scale}, {3 * lamella[k + 2] + axis, -fair * scale}}, 0.0});
        }

    std::vector<double> x;
    for (size_t i = 0; i < net.points.size(); i++) {
        const Point foot = field.compute_point(net.states[i]);
        const Vector normal = field.compute_normal(net.states[i]);
        Row row{{}, close * normal.dot(foot - Point(0.0, 0.0, 0.0)) * scale};
        for (int axis = 0; axis < 3; axis++) {
            row.terms.emplace_back(3 * i + axis, close * normal[axis] * scale);
            rows.push_back(Row{{{3 * i + axis, damping * scale}}, damping * scale * net.points[i][axis]});
            x.push_back(net.points[i][axis]);
        }

        rows.push_back(row);
    }

    x = compute_least_squares(rows, x);
    for (size_t i = 0; i < net.points.size(); i++) {
        net.points[i] = Point(x[3 * i], x[3 * i + 1], x[3 * i + 2]);
        net.states[i] = field.compute_state(net.points[i], net.states[i]);
    }
}

/// Largest asymptotic residual of the net, |n_i . (p_j - p_i)| / |p_j - p_i| over both ends of every edge: the sine of the angle an edge leaves the node's tangent plane.
inline double compute_residual(const Net& net) {

    double worst = 0.0;
    for (const std::vector<size_t>& lamella : net.lamellas)
        for (size_t k = 0; k + 1 < lamella.size(); k++) {
            const Vector edge = (net.points[lamella[k + 1]] - net.points[lamella[k]]).normalized();
            worst = std::max({worst, std::abs(net.normals[lamella[k]].dot(edge)), std::abs(net.normals[lamella[k + 1]].dot(edge))});
        }

    return worst;
}

/// The net optimised by block-coordinate descent on the energy of Wang, Almaskin, Pottmann (2025), Eq. 11, with carrier closeness in place of first-strip approximation: normals in closed form, then positions by least squares, iterations times.
inline void compute_optimised(const Field& field, Net& net, int iterations) {

    for (int k = 0; k < iterations; k++) {
        compute_normals(field, net);
        compute_positions(field, net);
    }

    compute_normals(field, net);
}

/// The frame of one lamella at one of its nodes: origin the node, z the node normal, x along the lamella.
inline Plane compute_node_frame(const Net& net, const std::vector<size_t>& lamella, size_t node) {

    const size_t k = static_cast<size_t>(std::find(lamella.begin(), lamella.end(), node) - lamella.begin());
    const Vector normal = net.normals[node];
    Vector tangent = net.points[lamella[std::min(k + 1, lamella.size() - 1)]] - net.points[lamella[k == 0 ? 0 : k - 1]];
    tangent = (tangent - normal * tangent.dot(normal)).normalized();

    return Plane(net.points[node], tangent, normal.cross(tangent));
}

/// The frames of one lamella at its nodes, origin the node, z its normal, x along the lamella, with one a gap past each end on the end tangent.
inline std::vector<Plane> compute_stations(const Net& net, const std::vector<size_t>& lamella, const Lamella& sizes) {

    std::vector<Plane> stations;
    for (const size_t node : lamella)
        stations.push_back(compute_node_frame(net, lamella, node));

    const Plane first = stations.front();
    const Plane last = stations.back();
    stations.insert(stations.begin(), Plane(first.origin() - first.x_axis() * sizes.gap, first.x_axis(), first.y_axis()));
    stations.emplace_back(last.origin() + last.x_axis() * sizes.gap, last.x_axis(), last.y_axis());

    return stations;
}

// ═══════════════════════════════════════════════════════════════════════════
// Elements
// ═══════════════════════════════════════════════════════════════════════════

/// One board on the lamella's central axis through its stations, its section a lift along each station's normal and a shift across it, so every section is the rectangle the local normal and the normal cross the tangent span.
inline std::shared_ptr<BeamCurved> compute_board(const std::vector<Plane>& stations, double lift, double shift, const Lamella& lamella, const std::string& name) {

    std::vector<Point> points;
    std::vector<Vector> directions;
    for (const Plane& station : stations) {
        points.push_back(station.origin());
        directions.push_back(station.z_axis());
    }

    const Polyline section = profile_rectangle(lamella.thickness, lamella.height)[0].translated(Vector(shift, lift, 0.0));

    return std::make_shared<BeamCurved>(points, directions, section, name);
}

/// A stud along the normal through both layers: a hexagon of three flat pairs gap apart, one against each layer's boards and one across the long corners; a 60 degree crossing gives the regular hexagon.
inline std::shared_ptr<Column> compute_stud(const Plane& top, const Plane& bottom, const Lamella& lamella, const std::string& name) {

    const Vector normal = top.z_axis();
    const Vector across = top.y_axis();
    const Vector other = bottom.y_axis().dot(normal.cross(across)) < 0.0 ? -bottom.y_axis() : bottom.y_axis();
    const std::vector<Vector> flats = across.dot(other) >= 0.0
        ? std::vector<Vector>{across, other, (other - across).normalized(), -across, -other, -(other - across).normalized()}
        : std::vector<Vector>{across, (across + other).normalized(), other, -across, -(across + other).normalized(), -other};

    const double reach = lamella.spacing / 2.0 + lamella.height / 2.0 + lamella.overrun;
    const Point base = top.origin() - normal * reach;
    std::vector<Point> points;
    for (size_t i = 0; i <= flats.size(); i++) {
        const Vector& first = flats[i % flats.size()];
        const Vector& second = flats[(i + 1) % flats.size()];
        points.push_back(base + (first + second) * (lamella.gap / 2.0 / (1.0 + first.dot(second))));
    }

    return std::make_shared<Column>(Line::from_points(base, top.origin() + normal * reach), Polyline(points), name);
}

/// A two-directional lamella gridshell on two curve families, on asymptotic curves a discrete A-net: two upright boards gap apart per lamella, continuous and twisting with the normal through every node, the first curve family a layer up the normal, the second a layer down, a hexagonal stud in both gaps at every crossing.
struct Gridshell {
    std::vector<std::shared_ptr<BeamCurved>> top; // lamella_top_i_a and _b per lamella of the first family, spacing / 2 up the normal.
    std::vector<std::shared_ptr<BeamCurved>> bottom; // lamella_bottom_j_a and _b per lamella of the second family, spacing / 2 down the normal.
    std::vector<std::shared_ptr<Column>> studs; // stud_i_j where top lamella i crosses bottom lamella j, flats against the four boards at the node.
    std::vector<std::vector<Plane>> frames; // Stations of every lamella on the carrier, x along it, z the normal; the top lamellas first.
    std::vector<std::pair<size_t, size_t>> nodes; // Top and bottom board pair of every stud, positions in top / 2 and bottom / 2.
    Net net; // The optimised net the boards and studs stand on.
    double traced = 0.0; // Largest asymptotic residual of the net before optimisation.

    /// The gridshell on count_top and count_bottom lamellas of a NURBS surface: curves 0 the u and v iso-curves, 1 the asymptotic curves (Gaussian curvature at most 0), each family seeded along a spine of the other through the middle of the domain.
    static Gridshell from_surface(const NurbsSurface& surface, int curves, int count_top, int count_bottom, const Lamella& lamella) {
        return from_field(SurfaceField(surface, curves), count_top, count_bottom, lamella);
    }

    /// The gridshell on count_top and count_bottom asymptotic lamellas of a triangle or quad mesh of negative Gaussian curvature, a minimal mesh above all, each family seeded along a spine of the other through the vertex nearest the centroid.
    static Gridshell from_mesh(const Mesh& mesh, int count_top, int count_bottom, const Lamella& lamella) {
        return from_field(MeshField(mesh), count_top, count_bottom, lamella);
    }

    /// The gridshell on the two families of a field: traced, turned into a net with a node at every crossing, an asymptotic net optimised and kept when that lowers its residual, the boards swept through the net and a stud at every crossing on its normal.
    static Gridshell from_field(const Field& field, int count_top, int count_bottom, const Lamella& lamella) {

        std::vector<std::vector<Point>> states = compute_family(field, 0, count_top, lamella.step);
        const std::vector<std::vector<Point>> bottoms = compute_family(field, 1, count_bottom, lamella.step);
        states.insert(states.end(), bottoms.begin(), bottoms.end());
        std::vector<std::vector<Plane>> traces;
        for (const std::vector<Point>& lamella_states : states)
            traces.push_back(compute_frames(field, lamella_states));

        const size_t tops = static_cast<size_t>(count_top);
        Gridshell gridshell;
        gridshell.net = compute_net(field, states, traces, tops, lamella.step);
        gridshell.traced = compute_residual(gridshell.net);
        if (field.is_asymptotic()) {
            Net optimised = gridshell.net;
            compute_optimised(field, optimised, ITERATIONS);
            if (compute_residual(optimised) < gridshell.traced)
                gridshell.net = optimised;
        }

        const Net& net = gridshell.net;
        std::vector<long> board(net.lamellas.size(), -1);
        const double lift = lamella.spacing / 2.0;
        const double shift = (lamella.gap + lamella.thickness) / 2.0;
        for (size_t i = 0; i < net.lamellas.size(); i++) {
            if (net.lamellas[i].size() < 2)
                continue;

            const bool upper = i < tops;
            const std::string name = upper ? fmt::format("lamella_top_{}", i) : fmt::format("lamella_bottom_{}", i - tops);
            std::vector<std::shared_ptr<BeamCurved>>& layer = upper ? gridshell.top : gridshell.bottom;
            board[i] = static_cast<long>(layer.size());
            gridshell.frames.push_back(compute_stations(net, net.lamellas[i], lamella));
            layer.push_back(compute_board(gridshell.frames.back(), upper ? lift : -lift, shift, lamella, name + "_a"));
            layer.push_back(compute_board(gridshell.frames.back(), upper ? lift : -lift, -shift, lamella, name + "_b"));
        }

        for (size_t n = 0; n < net.points.size(); n++) {
            if (!net.joints[n])
                continue;

            const size_t top = net.crossings[n].first;
            const size_t bottom = tops + net.crossings[n].second;
            if (board[top] < 0 || board[bottom] < 0)
                continue;

            const Plane top_frame = compute_node_frame(net, net.lamellas[top], n);
            const Plane bottom_frame = compute_node_frame(net, net.lamellas[bottom], n);
            gridshell.studs.push_back(compute_stud(top_frame, bottom_frame, lamella, fmt::format("stud_{}_{}", top, bottom - tops)));
            gridshell.nodes.emplace_back(static_cast<size_t>(board[top]) / 2, static_cast<size_t>(board[bottom]) / 2);
        }

        return gridshell;
    }
};

} // namespace wood_gridshell
