#pragma once
#include "wood_session.h"
#include "wood_profile.h"

namespace wood_gridshell {

using namespace session_cpp;
using namespace wood_session;

/// Board, gap and stud sizes of a two-layer lamella gridshell, in mm.
struct Lamella {
    double height = 140.0; // Board depth along the surface normal.
    double thickness = 20.0; // Board thickness across the lamella.
    double gap = 60.0; // Clear distance between the two boards of a lamella, the stud width across every flat.
    double spacing = 180.0; // Normal distance between the top and the bottom layer centrelines; below height the layers overlap.
    double overrun = 20.0; // Stud length past the outer face of each layer.
    double step = 100.0; // Runge-Kutta step of the tracing, in mm along the surface.
    double sample = 50.0; // Distance between the stations a board is swept through, in mm along the lamella.
    double ruling = 1.0; // Largest lean of a strip's ruling from the normal a board follows, as tan of the angle; a steeper ruling is clamped to it.
    double block = 100.0; // Length of a lamella at every node over which its section stands on the straight node axis, the spacer block; the ruling blends to the strip's over another block.
};

/// What a lamella follows on the surface: the u and v iso-curves, or the two directions of one normal curvature, 0 the asymptotic curves.
struct Path {
    bool iso = false; // True for the iso-curves, false for the normal curvature value.
    double value = 0.0; // The normal curvature every lamella keeps, in 1/mm.

    /// The u and v iso-curves.
    static Path isocurves() {
        Path path;
        path.iso = true;
        return path;
    }

    /// True for the asymptotic curves, where the rectifying developable of a lamella stands normal to the surface.
    bool is_asymptotic() const {
        return !iso && value == 0.0;
    }

    /// The two directions of normal curvature value, 0 the asymptotic curves.
    static Path normal_curvature(double value) {
        Path path;
        path.value = value;
        return path;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// Curvature
// ═══════════════════════════════════════════════════════════════════════════

/// The second-order data of a surface at (u, v): partials, unit normal, principal curvatures k1 >= k2 and their unit directions in space (d1, d2) and in the parameter plane (u1, u2), as Bowerbird's PrincipalCurvature.
struct Curvature {
    Point x; // The surface point.
    Vector a1; // dS/du.
    Vector a2; // dS/dv.
    Vector a11; // d2S/du2.
    Vector a12; // d2S/dudv.
    Vector a22; // d2S/dv2.
    Vector n; // Unit normal a1 x a2.
    double k1 = 0.0; // Larger principal curvature.
    double k2 = 0.0; // Smaller principal curvature.
    Vector d1; // Unit principal direction of k1 in space.
    Vector d2; // Unit principal direction of k2 in space.
    Vector u1; // d1 in the parameter plane, (du, dv, 0).
    Vector u2; // d2 in the parameter plane, (du, dv, 0).
    bool valid = false; // False where the metric or the shape operator degenerates.
};

/// The principal directions of the shape operator (k11 k12; k21 k22) written into c; false where its eigenvalues are complex.
inline bool compute_principal(Curvature& c, double g11, double g12, double g22, double k11, double k12, double k21, double k22) {

    const double disc = 4.0 * k12 * k21 + (k11 - k22) * (k11 - k22);
    if (disc < 0.0)
        return false;

    c.k1 = 0.5 * (k11 + k22 + std::sqrt(disc));
    c.k2 = 0.5 * (k11 + k22 - std::sqrt(disc));
    const bool by_row = std::abs(k12) > std::abs(k21);
    double du1 = by_row ? k12 : c.k1 - k22;
    double dv1 = by_row ? c.k1 - k11 : k21;
    double du2 = by_row ? k12 : c.k2 - k22;
    double dv2 = by_row ? c.k2 - k11 : k21;

    const double l1 = std::sqrt(g11 * du1 * du1 + 2.0 * g12 * du1 * dv1 + g22 * dv1 * dv1);
    du1 /= l1;
    dv1 /= l1;
    c.u1 = Vector(du1, dv1, 0.0);
    c.d1 = c.a1 * du1 + c.a2 * dv1;

    const double l2 = std::sqrt(g11 * du2 * du2 + 2.0 * g12 * du2 * dv2 + g22 * dv2 * dv2);
    du2 /= l2;
    dv2 /= l2;
    c.u2 = Vector(du2, dv2, 0.0);
    c.d2 = c.a1 * du2 + c.a2 * dv2;

    return true;
}

/// The curvature data of the surface at (u, v), from its partials up to second order.
inline Curvature compute_curvature(const NurbsSurface& surface, double u, double v) {

    const std::vector<Vector> ders = surface.evaluate(u, v, 2);
    Curvature c;
    c.x = Point(ders[0][0], ders[0][1], ders[0][2]);
    c.a1 = ders[3];
    c.a2 = ders[1];
    c.a11 = ders[5];
    c.a12 = ders[4];
    c.a22 = ders[2];
    c.n = c.a1.cross(c.a2).normalized();

    const double g11 = c.a1.dot(c.a1);
    const double g12 = c.a1.dot(c.a2);
    const double g22 = c.a2.dot(c.a2);
    const double h11 = c.n.dot(c.a11);
    const double h12 = c.n.dot(c.a12);
    const double h22 = c.n.dot(c.a22);
    const double det = g11 * g22 - g12 * g12;
    if (det == 0.0)
        return c;

    const double k11 = (g22 * h11 - g12 * h12) / det;
    const double k12 = (g22 * h12 - g12 * h22) / det;
    const double k21 = (g11 * h12 - g12 * h11) / det;
    const double k22 = (g11 * h22 - g12 * h12) / det;
    const double eps = std::max(std::abs(k11), std::abs(k22)) * 1e-10;
    if (std::abs(k12) < eps && std::abs(k21) < eps) {
        c.k1 = k11;
        c.k2 = k22;
        c.u1 = Vector(1.0 / std::sqrt(g11), 0.0, 0.0);
        c.u2 = Vector(0.0, 1.0 / std::sqrt(g22), 0.0);
        c.d1 = c.a1 * c.u1[0];
        c.d2 = c.a2 * c.u2[1];
        c.valid = true;
    } else {
        c.valid = compute_principal(c, g11, g12, g22, k11, k12, k21, k22);
    }

    return c;
}

/// The unit direction cos * d1 + sin * d2 of the tangent plane at c as (du, dv, 0) and in space, as Bowerbird's FindNormalCurvature maps it.
inline void compute_turned(const Curvature& c, double cos, double sin, Vector& uv, Vector& d) {

    const double du = c.a2.dot(c.d2) * cos - c.a2.dot(c.d1) * sin;
    const double dv = c.a1.dot(c.d1) * sin - c.a1.dot(c.d2) * cos;
    d = c.a1 * du + c.a2 * dv;
    const double l = d.magnitude();
    uv = Vector(du / l, dv / l, 0.0);
    d = d * (1.0 / l);
}

/// The two unit directions of normal curvature value at c, in the parameter plane (u1, u2) and in space (d1, d2), the first at +alpha and the second at -alpha from d1; false where value lies outside [k2, k1].
inline bool compute_normal_curvature_directions(const Curvature& c, double value, Vector& u1, Vector& u2, Vector& d1, Vector& d2) {

    const double t = (2.0 * value - c.k1 - c.k2) / (c.k1 - c.k2);
    if (std::abs(t) > 1.0 || std::isnan(t))
        return false;

    const double alpha = 0.5 * std::acos(t);
    compute_turned(c, std::cos(alpha), std::sin(alpha), u1, d1);
    compute_turned(c, std::cos(alpha), -std::sin(alpha), u2, d2);

    return true;
}

/// The two direction pairs of the path at uv = (u, v, 0): unit in space (d1, d2) with their images in the parameter plane (u1, u2); false where the path has no direction there.
inline bool compute_directions(const NurbsSurface& surface, const Path& path, const Vector& uv, Vector& u1, Vector& u2, Vector& d1, Vector& d2) {

    const Curvature c = compute_curvature(surface, uv[0], uv[1]);
    if (!c.valid)
        return false;

    if (path.iso) {
        u1 = Vector(1.0 / c.a1.magnitude(), 0.0, 0.0);
        u2 = Vector(0.0, 1.0 / c.a2.magnitude(), 0.0);
        d1 = c.a1.normalized();
        d2 = c.a2.normalized();
        return true;
    }

    return compute_normal_curvature_directions(c, path.value, u1, u2, d1, d2);
}

// ═══════════════════════════════════════════════════════════════════════════
// Tracing
// ═══════════════════════════════════════════════════════════════════════════

/// The rectangle of the parameter plane the tracing stays in.
struct Boundary {
    double u0 = 0.0; // Domain start in u.
    double u1 = 1.0; // Domain end in u.
    double v0 = 0.0; // Domain start in v.
    double v1 = 1.0; // Domain end in v.

    /// The domain of a surface.
    static Boundary of(const NurbsSurface& surface) {
        Boundary boundary;
        boundary.u0 = surface.domain(0).first;
        boundary.u1 = surface.domain(0).second;
        boundary.v0 = surface.domain(1).first;
        boundary.v1 = surface.domain(1).second;
        return boundary;
    }

    /// Cuts the segment a-b at the rectangle, true when b lay outside: b is moved to where the segment leaves, exactly on the edge.
    bool clip(const Vector& a, Vector& b) const {

        double t = 1.0;
        const double lows[2] = {u0, v0};
        const double highs[2] = {u1, v1};
        for (int k = 0; k < 2; k++) {
            if (b[k] < lows[k])
                t = std::min(t, (lows[k] - a[k]) / (b[k] - a[k]));

            if (b[k] > highs[k])
                t = std::min(t, (highs[k] - a[k]) / (b[k] - a[k]));
        }

        if (t >= 1.0)
            return false;

        b = a + (b - a) * t;
        for (int k = 0; k < 2; k++)
            b[k] = std::clamp(b[k], lows[k], highs[k]);

        return true;
    }
};

/// Bowerbird's Path.Direction: the parameter step of length step along whichever path direction at uv lies closer to the last step in space, sign matched; zero where the path has no direction.
inline Vector compute_direction(const NurbsSurface& surface, const Path& path, const Vector& uv, const Vector& last, double step) {

    Vector u1, u2, d1, d2;
    if (!compute_directions(surface, path, uv, u1, u2, d1, d2))
        return Vector(0.0, 0.0, 0.0);

    const Vector s = last.normalized();
    const double e1 = d1.dot(s);
    const double e2 = d2.dot(s);
    if (std::abs(e1) > std::abs(e2))
        return e1 > 0.0 ? u1 * step : u1 * -step;

    return e2 > 0.0 ? u2 * step : u2 * -step;
}

/// A lamella's trace on the surface: its points in the parameter plane, (u, v, 0), and on the surface.
struct Trace {
    std::vector<Vector> parameters; // (u, v, 0) of every point.
    std::vector<Point> points; // The surface point of every parameter.
};

/// Bowerbird's FindPath: fourth-order Runge-Kutta steps of step from uv along direction through the parameter plane, appended to the trace until the boundary, a standstill or count points; returns the last step in space.
inline Vector compute_path(const NurbsSurface& surface, const Path& path, const Boundary& boundary, Vector uv, Vector direction, double step, size_t count, Trace& trace) {

    while (trace.points.size() < count) {
        const Vector d0 = compute_direction(surface, path, uv, direction, step);
        const Vector d1 = compute_direction(surface, path, uv + d0 * 0.5, direction, step);
        const Vector d2 = compute_direction(surface, path, uv + d1 * 0.5, direction, step);
        const Vector d3 = compute_direction(surface, path, uv + d2, direction, step);
        const Vector delta = (d0 + d1 * 2.0 + d2 * 2.0 + d3) * (1.0 / 6.0);
        if (delta[0] == 0.0 && delta[1] == 0.0)
            break;

        Vector next = uv + delta;
        const bool hit = boundary.clip(uv, next);
        uv = next;
        const Point x = surface.point_at(uv[0], uv[1]);
        const Vector moved = x - trace.points.back();
        if (moved.dot(moved) < 1e-20)
            break;

        direction = moved;
        trace.parameters.push_back(uv);
        trace.points.push_back(x);
        if (hit)
            break;
    }

    return direction;
}

/// The path direction at the seed (u, v, 0) closer in space to the reference, by |dot|: the one that continues the reference's family; zero where the path has no direction there.
inline Vector compute_seed_direction(const NurbsSurface& surface, const Path& path, const Vector& seed, const Vector& reference) {

    Vector u1, u2, d1, d2;
    if (!compute_directions(surface, path, seed, u1, u2, d1, d2))
        return Vector(0.0, 0.0, 0.0);

    return std::abs(d1.dot(reference)) >= std::abs(d2.dot(reference)) ? d1 : d2;
}

/// The direction of a family at the middle of the domain: the path's first or second direction there, the reference every seed of the family is matched against.
inline Vector compute_family_direction(const NurbsSurface& surface, const Path& path, bool first) {

    const Boundary boundary = Boundary::of(surface);
    const Vector centre((boundary.u0 + boundary.u1) / 2.0, (boundary.v0 + boundary.v1) / 2.0, 0.0);
    Vector u1, u2, d1, d2;
    if (!compute_directions(surface, path, centre, u1, u2, d1, d2))
        return Vector(0.0, 0.0, 0.0);

    return first ? d1 : d2;
}

/// Bowerbird's Pathfinder on one untrimmed surface: the path traced both ways from the seed (u, v, 0) along its direction there closer to the reference, a point every step until the domain boundary; empty where the path has no direction at the seed.
inline Trace compute_trace(const NurbsSurface& surface, const Path& path, const Vector& seed, const Vector& reference, double step, size_t count = 100000) {

    const Vector direction = compute_seed_direction(surface, path, seed, reference);
    if (direction.magnitude() == 0.0)
        return Trace();

    const Boundary boundary = Boundary::of(surface);
    Trace trace;
    trace.parameters.push_back(seed);
    trace.points.push_back(surface.point_at(seed[0], seed[1]));
    compute_path(surface, path, boundary, seed, direction * -1.0, step, count, trace);
    std::reverse(trace.parameters.begin(), trace.parameters.end());
    std::reverse(trace.points.begin(), trace.points.end());
    compute_path(surface, path, boundary, seed, direction, step, count, trace);

    return trace;
}

/// count seeds (u, v, 0) evenly spaced strictly inside the segment from a to b of the parameter plane.
inline std::vector<Vector> compute_seeds(const Vector& a, const Vector& b, int count) {

    std::vector<Vector> seeds;
    for (int i = 0; i < count; i++)
        seeds.push_back(a + (b - a) * ((i + 1.0) / (count + 1.0)));

    return seeds;
}

/// The ends of the seed line of a family: the segment through the middle of the domain perpendicular in the parameter plane to the family's direction there, cut at the boundary.
inline std::pair<Vector, Vector> compute_seed_line(const NurbsSurface& surface, const Path& path, bool first) {

    const Boundary boundary = Boundary::of(surface);
    const Vector centre((boundary.u0 + boundary.u1) / 2.0, (boundary.v0 + boundary.v1) / 2.0, 0.0);
    Vector u1, u2, d1, d2;
    compute_directions(surface, path, centre, u1, u2, d1, d2);
    const Vector along = first ? u1 : u2;
    const Vector across = Vector(-along[1], along[0], 0.0).normalized() * (boundary.u1 - boundary.u0 + boundary.v1 - boundary.v0);
    Vector a = centre - across;
    Vector b = centre + across;
    boundary.clip(centre, a);
    boundary.clip(centre, b);

    return {a, b};
}

// ═══════════════════════════════════════════════════════════════════════════
// Curve on surface
// ═══════════════════════════════════════════════════════════════════════════

/// The cubic through the trace's parameters, a curve in the parameter plane evaluated on the surface as Bowerbird's CurveOnSurface; empty for a trace of one point.
inline NurbsCurve compute_curve(const Trace& trace) {

    if (trace.parameters.size() < 2)
        return NurbsCurve();

    std::vector<Point> points;
    for (const Vector& uv : trace.parameters)
        points.emplace_back(uv[0], uv[1], 0.0);

    return NurbsCurve::create_interpolated(points);
}

/// The surface point of the curve at t.
inline Point compute_point(const NurbsSurface& surface, const NurbsCurve& curve, double t) {
    const Point uv = curve.point_at(t);
    return surface.point_at(uv[0], uv[1]);
}

/// The frame of the curve at t: origin its surface point, x its unit tangent (the parameter derivative mapped by dS/du, dS/dv), z the surface normal.
inline Plane compute_frame(const NurbsSurface& surface, const NurbsCurve& curve, double t) {

    const std::vector<Vector> c = curve.evaluate(t, 1);
    const std::vector<Vector> s = surface.evaluate(c[0][0], c[0][1], 1);
    const Vector tangent = (s[2] * c[1][0] + s[1] * c[1][1]).normalized();
    const Vector normal = s[2].cross(s[1]).normalized();

    return Plane(Point(s[0][0], s[0][1], s[0][2]), tangent, normal.cross(tangent));
}

/// The Darboux frame and the invariants of the curve at t with respect to the surface (Schling et al. 2022, Sec. 2.1): t the unit tangent, n the surface normal, u = n x t, and along the curve the normal curvature, geodesic curvature and geodesic torsion.
struct Darboux {
    Point x; // The surface point.
    Vector t; // Unit tangent of the curve.
    Vector u; // Side vector n x t.
    Vector n; // Unit surface normal.
    double kn = 0.0; // Normal curvature, 1/mm.
    double kg = 0.0; // Geodesic curvature, 1/mm, signed towards u.
    double tg = 0.0; // Geodesic torsion, 1/mm, n' = -tg u + kn t along the curve.
};

/// The Darboux data of the curve at t: kn and tg by Euler's formula from the principal curvatures and the angle p of t against the k1 direction (Schling Eq. 3: kn = k1 cos^2 p + k2 sin^2 p, tg = (k2 - k1) / 2 sin 2p), kg the curvature vector of the curve on u, the curve's derivatives mapped by the surface partials as Bowerbird's CurveOnSurface.
inline Darboux compute_darboux(const NurbsSurface& surface, const NurbsCurve& curve, double t) {

    const std::vector<Vector> c = curve.evaluate(t, 2);
    const double u1 = c[1][0];
    const double v1 = c[1][1];
    const double u2 = c[2][0];
    const double v2 = c[2][1];
    const Curvature k = compute_curvature(surface, c[0][0], c[0][1]);

    const Vector x1 = k.a1 * u1 + k.a2 * v1;
    const Vector x2 = k.a1 * u2 + k.a2 * v2 + k.a11 * (u1 * u1) + k.a22 * (v1 * v1) + k.a12 * (2.0 * u1 * v1);
    const double speed2 = x1.dot(x1);
    const Vector bend = (x2 * speed2 - x1 * x1.dot(x2)) * (1.0 / (speed2 * speed2));

    Darboux d;
    d.x = k.x;
    d.t = x1.normalized();
    d.n = k.n;
    d.u = d.n.cross(d.t);
    const Vector d2 = k.n.cross(k.d1);
    const double phi = std::atan2(k.d1.cross(d.t).dot(k.n), k.d1.dot(d.t));
    d.kn = k.k1 * std::cos(phi) * std::cos(phi) + k.k2 * std::sin(phi) * std::sin(phi);
    d.tg = (k.k2 - k.k1) / 2.0 * std::sin(2.0 * phi);
    d.kg = bend.dot(d.u);

    return d;
}

/// The normal curvature, geodesic curvature and geodesic torsion of the curve at t, in 1/mm.
inline std::array<double, 3> compute_metrics(const NurbsSurface& surface, const NurbsCurve& curve, double t) {
    const Darboux d = compute_darboux(surface, curve, t);
    return {d.kn, d.kg, d.tg};
}

const double STRAIGHT = 1e-9; // Geodesic curvature in 1/mm below which a lamella counts as a straight line, where no strip normal to the surface is developable and the board follows the normal.

/// The share of the strip's ruling a station distance mm from its nearest node takes: 0 within half a block, where the section stands on the straight node axis, rising smoothly to 1 a block further out.
inline double compute_blend(double distance, const Lamella& lamella) {
    const double w = std::clamp((distance - lamella.block / 2.0) / lamella.block, 0.0, 1.0);
    return w * w * (3.0 - 2.0 * w);
}

/// The ruling of the lamella's board at d: for an asymptotic curve the rectifying developable's ruling tg t + kg n (Schling Eq. 4) scaled to unit height along the normal, n + tg / kg t, so the strip unrolls straight, its lean along the tangent clamped to lamella.ruling and scaled by the blend towards the normal at the nodes, where the physical node forces the strip through the straight node axis (Schling Sec. 3.4); the normal itself on other curves and along straight lines.
inline Vector compute_ruling(const Darboux& d, const Path& path, const Lamella& lamella, double blend) {

    if (!path.is_asymptotic() || std::abs(d.kg) < STRAIGHT)
        return d.n;

    return d.n + d.t * (blend * std::clamp(d.tg / d.kg, -lamella.ruling, lamella.ruling));
}

// ═══════════════════════════════════════════════════════════════════════════
// Crossings
// ═══════════════════════════════════════════════════════════════════════════

/// The fraction along a-b where it crosses c-d in the parameter plane, negative when the segments miss.
inline double compute_segment_crossing(const Vector& a, const Vector& b, const Vector& c, const Vector& d) {

    const double det = (b[0] - a[0]) * (d[1] - c[1]) - (b[1] - a[1]) * (d[0] - c[0]);
    if (std::abs(det) < 1e-300)
        return -1.0;

    const double s = ((c[0] - a[0]) * (d[1] - c[1]) - (c[1] - a[1]) * (d[0] - c[0])) / det;
    const double r = ((c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0])) / det;

    return s >= 0.0 && s <= 1.0 && r >= 0.0 && r <= 1.0 ? s : -1.0;
}

/// Newton on a(ta) = b(tb) in the parameter plane from the parameters nearest uv, (ta, tb) once the two curve points agree.
inline std::pair<double, double> compute_crossing_parameters(const NurbsCurve& a, const NurbsCurve& b, const Vector& uv) {

    const Point start(uv[0], uv[1], 0.0);
    double ta = a.closest_parameter(start);
    double tb = b.closest_parameter(start);
    for (int k = 0; k < 20; k++) {
        const std::vector<Vector> pa = a.evaluate(ta, 1);
        const std::vector<Vector> pb = b.evaluate(tb, 1);
        const Vector f = pa[0] - pb[0];
        if (f.dot(f) < 1e-24)
            break;

        const double det = pa[1][0] * -pb[1][1] - pa[1][1] * -pb[1][0];
        if (std::abs(det) < 1e-300)
            break;

        const double da = (-f[0] * -pb[1][1] - -f[1] * -pb[1][0]) / det;
        const double db = (pa[1][0] * -f[1] - pa[1][1] * -f[0]) / det;
        ta = std::clamp(ta + da, a.domain().first, a.domain().second);
        tb = std::clamp(tb + db, b.domain().first, b.domain().second);
    }

    return {ta, tb};
}

/// Where two lamellas cross: every crossing of their traces in the parameter plane refined onto their curves, (t on a, t on b) per crossing, one per point when a crossing falls on a trace vertex.
inline std::vector<std::pair<double, double>> compute_crossings(const Trace& ta, const NurbsCurve& a, const Trace& tb, const NurbsCurve& b) {

    std::vector<std::pair<double, double>> crossings;
    for (size_t i = 0; i + 1 < ta.parameters.size(); i++)
        for (size_t j = 0; j + 1 < tb.parameters.size(); j++) {
            const double s = compute_segment_crossing(ta.parameters[i], ta.parameters[i + 1], tb.parameters[j], tb.parameters[j + 1]);
            if (s < 0.0)
                continue;

            const Vector uv = ta.parameters[i] + (ta.parameters[i + 1] - ta.parameters[i]) * s;
            const std::pair<double, double> crossing = compute_crossing_parameters(a, b, uv);
            bool fresh = true;
            for (const std::pair<double, double>& known : crossings)
                fresh = fresh && (std::abs(known.first - crossing.first) > 1e-9 || std::abs(known.second - crossing.second) > 1e-9);

            if (fresh)
                crossings.push_back(crossing);
        }

    return crossings;
}

// ═══════════════════════════════════════════════════════════════════════════
// Elements
// ═══════════════════════════════════════════════════════════════════════════

/// The station parameters of a lamella: its nodes plus parameters every sample of the trace's length along the curve, none nearer a node than half a sample, sorted.
inline std::vector<double> compute_parameters(const Trace& trace, const NurbsCurve& curve, std::vector<double> nodes, double sample) {

    double length = 0.0;
    for (size_t k = 0; k + 1 < trace.points.size(); k++)
        length += (trace.points[k + 1] - trace.points[k]).magnitude();

    const int count = std::max(2, static_cast<int>(std::ceil(length / sample)));
    const double t0 = curve.domain().first;
    const double t1 = curve.domain().second;
    const double pitch = (t1 - t0) / count;
    std::vector<double> parameters = nodes;
    for (int k = 0; k <= count; k++) {
        const double t = t0 + pitch * k;
        bool free = true;
        for (const double node : nodes)
            free = free && std::abs(t - node) > pitch / 2.0;

        if (free)
            parameters.push_back(t);
    }

    std::sort(parameters.begin(), parameters.end());

    return parameters;
}

/// One board of a lamella: the rectangle section, lifted along the ruling by lift (heights measured along the normal) and shifted across by shift, swept through the stations with the ruling as its y direction, so the board is a slab of the strip's developable between two of its parallel geodesics.
inline std::shared_ptr<BeamCurved> compute_board(const std::vector<Plane>& stations, const std::vector<Vector>& rulings, double lift, double shift, const Lamella& lamella, const std::string& name) {

    std::vector<Point> points;
    for (const Plane& station : stations)
        points.push_back(station.origin());

    const Polyline section = profile_rectangle(lamella.thickness, lamella.height)[0].translated(Vector(shift, lift, 0.0));

    return std::make_shared<BeamCurved>(points, rulings, section, name);
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

/// A node of the gridshell: where a top lamella crosses a bottom one.
struct Node {
    size_t top = 0; // Position of the top lamella.
    size_t bottom = 0; // Position of the bottom lamella.
    double top_t = 0.0; // Parameter of the crossing on the top lamella's curve.
    double bottom_t = 0.0; // Parameter of the crossing on the bottom lamella's curve.
};

/// A two-directional lamella gridshell on two curve families of a NURBS surface: two upright boards gap apart per lamella, continuous through every node with the exact surface normal as up, the first family a layer up the normal, the second a layer down, a hexagonal stud on the surface normal in both gaps at every crossing.
struct Gridshell {
    NurbsSurface surface; // The carrier.
    Path path; // What the lamellas follow.
    std::vector<Trace> traces; // Tracing of every lamella, the top family first.
    std::vector<NurbsCurve> curves; // The curve in the parameter plane of every lamella, the top family first.
    size_t tops = 0; // Number of lamellas in the top family.
    std::vector<Node> nodes; // Every crossing of a top and a bottom lamella.
    std::vector<std::shared_ptr<BeamCurved>> top; // lamella_top_i_a and _b per lamella of the first family, spacing / 2 up the normal.
    std::vector<std::shared_ptr<BeamCurved>> bottom; // lamella_bottom_j_a and _b per lamella of the second family, spacing / 2 down the normal.
    std::vector<std::shared_ptr<Column>> studs; // stud_i_j where top lamella i crosses bottom lamella j, flats against the four boards at the node.
    std::vector<std::vector<Plane>> frames; // Stations of every lamella on the surface, x along it, z the normal; the top lamellas first.
    std::vector<std::pair<size_t, size_t>> pairs; // Top and bottom board pair of every stud, positions in top / 2 and bottom / 2.
    double lean = 0.0; // Largest lean of a rectifying ruling from the normal over every station of an asymptotic gridshell, in degrees, before the clamp.
    size_t capped = 0; // Stations whose ruling leaned past lamella.ruling and was clamped to it.

    /// The gridshell of a path on a surface: the top family traced from seeds_top along the path's first direction at the middle of the domain, the bottom family from seeds_bottom along its second, the boards swept through exact surface stations along the strips' rulings and a stud at every crossing.
    static Gridshell from_surface(const NurbsSurface& surface, const Path& path, const std::vector<Vector>& seeds_top, const std::vector<Vector>& seeds_bottom, const Lamella& lamella) {

        Gridshell gridshell;
        gridshell.surface = surface;
        gridshell.path = path;
        gridshell.compute_family(seeds_top, true, lamella);
        gridshell.tops = gridshell.traces.size();
        gridshell.compute_family(seeds_bottom, false, lamella);

        for (const Trace& trace : gridshell.traces)
            gridshell.curves.push_back(compute_curve(trace));

        gridshell.compute_nodes();
        gridshell.compute_elements(lamella);

        return gridshell;
    }

    /// One trace per seed, each along the direction closer to the family's: the family direction at the middle of the domain for the first seed, then the direction the previous seed took.
    void compute_family(const std::vector<Vector>& seeds, bool first, const Lamella& lamella) {

        Vector reference = compute_family_direction(surface, path, first);
        for (const Vector& seed : seeds) {
            traces.push_back(compute_trace(surface, path, seed, reference, lamella.step));
            const Vector taken = compute_seed_direction(surface, path, seed, reference);
            if (taken.magnitude() > 0.0)
                reference = taken;
        }
    }

    /// Every crossing of a top and a bottom lamella, in the parameter plane.
    void compute_nodes() {

        for (size_t i = 0; i < tops; i++)
            for (size_t j = tops; j < traces.size(); j++) {
                if (!curves[i].is_valid() || !curves[j].is_valid())
                    continue;

                for (const std::pair<double, double>& crossing : compute_crossings(traces[i], curves[i], traces[j], curves[j]))
                    nodes.push_back({i, j - tops, crossing.first, crossing.second});
            }
    }

    /// The stations of lamella i and the ruling at each: frames on the surface (x the tangent, z the normal) at the nodes and every sample between, the ruling the strip's blended to the normal near the nodes; lean and capped updated on an asymptotic path.
    void compute_stations(size_t i, const std::vector<double>& nodes, const Lamella& lamella, std::vector<Plane>& stations, std::vector<Vector>& rulings) {

        std::vector<Point> joints;
        for (const double t : nodes)
            joints.push_back(compute_point(surface, curves[i], t));

        for (const double t : compute_parameters(traces[i], curves[i], nodes, lamella.sample)) {
            const Darboux d = compute_darboux(surface, curves[i], t);
            double distance = 1e300;
            for (const Point& joint : joints)
                distance = std::min(distance, (joint - d.x).magnitude());

            stations.emplace_back(d.x, d.t, d.u);
            rulings.push_back(compute_ruling(d, path, lamella, compute_blend(distance, lamella)));
            if (path.is_asymptotic()) {
                lean = std::max(lean, std::abs(d.kg) < STRAIGHT ? 90.0 : std::atan(std::abs(d.tg / d.kg)) * 180.0 / Tolerance::PI);
                capped += std::abs(d.kg) < STRAIGHT || std::abs(d.tg / d.kg) > lamella.ruling ? 1 : 0;
            }
        }
    }

    /// The boards through the stations of every lamella and the studs at the nodes.
    void compute_elements(const Lamella& lamella) {

        std::vector<std::vector<double>> node_parameters(traces.size());
        for (const Node& node : nodes) {
            node_parameters[node.top].push_back(node.top_t);
            node_parameters[tops + node.bottom].push_back(node.bottom_t);
        }

        const double lift = lamella.spacing / 2.0;
        const double shift = (lamella.gap + lamella.thickness) / 2.0;
        std::vector<long> board(traces.size(), -1);
        for (size_t i = 0; i < traces.size(); i++) {
            std::vector<Plane> stations;
            std::vector<Vector> rulings;
            if (curves[i].is_valid())
                compute_stations(i, node_parameters[i], lamella, stations, rulings);

            frames.push_back(stations);
            if (stations.size() < 2)
                continue;

            const bool upper = i < tops;
            const std::string name = upper ? fmt::format("lamella_top_{}", i) : fmt::format("lamella_bottom_{}", i - tops);
            std::vector<std::shared_ptr<BeamCurved>>& layer = upper ? top : bottom;
            board[i] = static_cast<long>(layer.size());
            layer.push_back(compute_board(stations, rulings, upper ? lift : -lift, shift, lamella, name + "_a"));
            layer.push_back(compute_board(stations, rulings, upper ? lift : -lift, -shift, lamella, name + "_b"));
        }

        for (const Node& node : nodes) {
            if (board[node.top] < 0 || board[tops + node.bottom] < 0)
                continue;

            const Plane top_frame = compute_frame(surface, curves[node.top], node.top_t);
            const Plane bottom_frame = compute_frame(surface, curves[tops + node.bottom], node.bottom_t);
            studs.push_back(compute_stud(top_frame, bottom_frame, lamella, fmt::format("stud_{}_{}", node.top, node.bottom)));
            pairs.emplace_back(static_cast<size_t>(board[node.top]) / 2, static_cast<size_t>(board[tops + node.bottom]) / 2);
        }
    }
};

} // namespace wood_gridshell
