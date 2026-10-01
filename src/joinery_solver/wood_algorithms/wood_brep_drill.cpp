#include "pch.h"
#include "wood_brep_drill.h"
#include "wood_element_geometry.h"
#include "primitives.h"

namespace wood_session {

using namespace session_cpp;

const double WELD = 1e-6; // vertices closer than this are one
const double CLEARANCE = 1e-6; // an edge must keep the drill radius plus this from the axis
const double STEEP = 0.2; // a drill crossing a face at a cosine below this is too oblique for a clean ellipse
const int SAMPLES = 64; // points sampled along a hole loop to find how far it reaches along the axis
const double VOLUME = 1e-3; // relative volume deviation the exact solid may show against the mesh it replaces

/// One planar face of the mesh with its frame.
struct PlanarFace {
    std::vector<size_t> keys; // Mesh vertex keys, outward winding.
    std::vector<Point> points; // Their positions.
    Vector normal; // Outward unit normal.
    Vector x; // In-plane x along the first side.
    Vector y; // normal x x.
};

/// One stretch of a drill inside the solid: its axis parameters and, per end, the face it crosses there or -1 for a flat bottom.
struct Stretch {
    size_t drill = 0; // Drill index.
    double t0 = 0.0; // Start along the axis.
    double t1 = 0.0; // End along the axis.
    int face0 = -1; // Face crossed at the start, -1 for a bottom.
    int face1 = -1; // Face crossed at the end, -1 for a bottom.
};

/// A flat patch spanning a face, and the affine map between it and the face's plane.
struct Patch {
    int surface = -1; // BRep surface index.
    Point origin; // Patch point at the domain start.
    Vector eu; // Patch extent along u.
    Vector ev; // Patch extent along v.
    std::pair<double, double> du; // u domain.
    std::pair<double, double> dv; // v domain.
};

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

/// Whether a point of the face's plane lies inside its outline, by ray crossing in the face frame.
static bool inside(const PlanarFace& face, const Point& point) {

    const Point& o = face.points[0];
    const double px = (point - o).dot(face.x);
    const double py = (point - o).dot(face.y);
    bool in = false;

    for (size_t i = 0, j = face.points.size() - 1; i < face.points.size(); j = i++) {
        const double xi = (face.points[i] - o).dot(face.x);
        const double yi = (face.points[i] - o).dot(face.y);
        const double xj = (face.points[j] - o).dot(face.x);
        const double yj = (face.points[j] - o).dot(face.y);

        if ((yi > py) != (yj > py) && px < (xj - xi) * (py - yi) / (yj - yi) + xi)
            in = !in;
    }

    return in;
}

/// The shortest distance between two segments.
static double segment_distance(const Point& p0, const Point& p1, const Point& q0, const Point& q1) {

    const Vector d1 = p1 - p0;
    const Vector d2 = q1 - q0;
    const Vector r = p0 - q0;
    const double a = d1.dot(d1);
    const double e = d2.dot(d2);
    const double f = d2.dot(r);
    double s = 0.0;
    double t = 0.0;

    if (a <= 1e-24 && e <= 1e-24)
        return (p0 - q0).magnitude();

    if (a <= 1e-24) {
        t = std::clamp(f / e, 0.0, 1.0);
    } else {
        const double c = d1.dot(r);

        if (e <= 1e-24) {
            s = std::clamp(-c / a, 0.0, 1.0);
        } else {
            const double b = d1.dot(d2);
            const double denom = a * e - b * b;
            s = denom > 1e-24 ? std::clamp((b * f - c * e) / denom, 0.0, 1.0) : 0.0;
            t = (b * s + f) / e;

            if (t < 0.0) {
                t = 0.0;
                s = std::clamp(-c / a, 0.0, 1.0);
            } else if (t > 1.0) {
                t = 1.0;
                s = std::clamp((b - c) / a, 0.0, 1.0);
            }
        }
    }

    return ((p0 + d1 * s) - (q0 + d2 * t)).magnitude();
}

/// The planar faces of a mesh with their frames; empty when a face is degenerate.
static std::vector<PlanarFace> planar_faces(const Mesh& mesh) {

    std::vector<PlanarFace> faces;

    for (const size_t key : mesh.faces()) {
        PlanarFace face;
        face.keys = mesh.face_vertices(key).value();

        for (const size_t vertex : face.keys)
            face.points.push_back(mesh.vertex_point(vertex).value());

        face.normal = compute_newell(face.points).normalized();
        face.x = (face.points[1] - face.points[0]).normalized();
        face.y = face.normal.cross(face.x).normalized();

        for (const Point& point : face.points)
            if (std::abs((point - face.points[0]).dot(face.normal)) > 1e-4)
                return {};

        faces.push_back(face);
    }

    return faces;
}

/// The stretches of one drill inside the solid, from where its axis crosses the faces, clipped to the drill.
static std::vector<Stretch> compute_stretches(const std::vector<PlanarFace>& faces, const Drill& drill, size_t index) {

    const Point start = drill.axis.start();
    const Vector d = drill.axis.to_vector().normalized();
    const double length = drill.axis.length();
    std::vector<std::pair<double, int>> crossings;

    for (size_t i = 0; i < faces.size(); i++) {
        const double cosine = faces[i].normal.dot(d);

        if (std::abs(cosine) < 1e-12)
            continue;

        const double t = faces[i].normal.dot(faces[i].points[0] - start) / cosine;

        if (inside(faces[i], start + d * t))
            crossings.push_back({t, static_cast<int>(i)});
    }

    std::sort(crossings.begin(), crossings.end());
    std::vector<Stretch> stretches;

    for (size_t i = 0; i + 1 < crossings.size(); i++) {
        const PlanarFace& entry = faces[crossings[i].second];

        if (entry.normal.dot(d) > 0.0)
            continue;

        const double t0 = crossings[i].first;
        const double t1 = crossings[i + 1].first;

        if (t1 <= 0.0 || t0 >= length)
            continue;

        Stretch stretch;
        stretch.drill = index;
        stretch.t0 = std::max(t0, 0.0);
        stretch.t1 = std::min(t1, length);
        stretch.face0 = t0 > 0.0 ? crossings[i].second : -1;
        stretch.face1 = t1 < length ? crossings[i + 1].second : -1;
        stretches.push_back(stretch);
    }

    return stretches;
}

/// The axis point of a stretch end on the face's plane, or the axis point at t for a bottom.
static Point end_centre(const PlanarFace* face, const Point& start, const Vector& d, double t) {

    if (!face)
        return start + d * t;

    return start + d * (face->normal.dot(face->points[0] - start) / face->normal.dot(d));
}

/// True when every face edge and every stretch of another drill keeps clear of the stretch's cylinder and every crossing is steep enough.
static bool is_clear(const std::vector<PlanarFace>& faces, const std::vector<Drill>& drills, const std::vector<Stretch>& stretches, size_t s) {

    const Stretch& stretch = stretches[s];
    const Drill& drill = drills[stretch.drill];
    const Vector d = drill.axis.to_vector().normalized();
    double margin = drill.radius;

    for (const int face : {stretch.face0, stretch.face1})
        if (face >= 0) {
            const double cosine = std::abs(faces[face].normal.dot(d));

            if (cosine < STEEP)
                return false;

            margin = std::max(margin, drill.radius / cosine + drill.radius);
        }

    const Point a = drill.axis.start() + d * (stretch.t0 - margin);
    const Point b = drill.axis.start() + d * (stretch.t1 + margin);

    for (const PlanarFace& face : faces)
        for (size_t i = 0; i < face.points.size(); i++)
            if (segment_distance(a, b, face.points[i], face.points[(i + 1) % face.points.size()]) < drill.radius + CLEARANCE)
                return false;

    for (size_t o = 0; o < stretches.size(); o++) {
        if (stretches[o].drill == stretch.drill)
            continue;

        const Drill& other = drills[stretches[o].drill];
        const Vector e = other.axis.to_vector().normalized();
        const Point c = other.axis.start() + e * stretches[o].t0;
        const Point f = other.axis.start() + e * stretches[o].t1;

        if (segment_distance(drill.axis.start() + d * stretch.t0, drill.axis.start() + d * stretch.t1, c, f) < drill.radius + other.radius + CLEARANCE)
            return false;
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// Curves
// ═══════════════════════════════════════════════════════════════════════════

/// The circle or ellipse where a drill of radius meets a plane, as the unit circle mapped by the drill's axes lifted onto the plane along the axis, so its parameter runs with the cylinder's and starts on its seam.
static NurbsCurve hole_loop(const Point& centre, const Vector& e, const Vector& f, const Vector& d, const Vector& normal, double radius) {

    const Vector a = (e - d * (e.dot(normal) / d.dot(normal))) * radius;
    const Vector b = (f - d * (f.dot(normal) / d.dot(normal))) * radius;
    Xform map;
    map.m = {a[0], a[1], a[2], 0.0, b[0], b[1], b[2], 0.0, d[0], d[1], d[2], 0.0, centre[0], centre[1], centre[2], 1.0};

    return Primitives::circle(0.0, 0.0, 0.0, 1.0).transformed(map);
}

/// The straight 2D curve between two parameter points.
static NurbsCurve uv_line(double u0, double v0, double u1, double v1) {
    return NurbsCurve::create(false, 1, {Point(u0, v0, 0.0), Point(u1, v1, 0.0)});
}

/// A 3D curve on a flat patch as its exact pcurve: every control point mapped into the patch parameters, weights kept.
static NurbsCurve on_patch(const NurbsCurve& curve, const Patch& patch) {

    NurbsCurve uv(3, curve.is_rational(), curve.order(), curve.cv_count());

    for (int i = 0; i < curve.nurbsknot_count(); i++)
        uv.set_nurbsknot(i, curve.nurbsknot(i));

    for (int i = 0; i < curve.cv_count(); i++) {
        const std::tuple<double, double, double, double> cv = curve.get_cv_4d(i);
        const double w = std::get<3>(cv);
        const Vector p = Point(std::get<0>(cv) / w, std::get<1>(cv) / w, std::get<2>(cv) / w) - patch.origin;
        const double u = patch.du.first + p.dot(patch.eu) / patch.eu.dot(patch.eu) * (patch.du.second - patch.du.first);
        const double v = patch.dv.first + p.dot(patch.ev) / patch.ev.dot(patch.ev) * (patch.dv.second - patch.dv.first);
        uv.set_cv_4d(i, u * w, v * w, 0.0, w);
    }

    return uv;
}

/// Twice the signed area a closed pcurve encloses.
static double uv_area(const NurbsCurve& uv) {

    const std::vector<Point> points = uv.divide_by_count(std::max(uv.cv_count() * 4, 32), true).first;
    double area = 0.0;

    for (size_t i = 0; i + 1 < points.size(); i++)
        area += points[i][0] * points[i + 1][1] - points[i + 1][0] * points[i][1];

    return area;
}

// ═══════════════════════════════════════════════════════════════════════════
// Building
// ═══════════════════════════════════════════════════════════════════════════

/// Grows a BRep from shared vertices, shared straight edges and the faces.
struct Builder {
    BRep brep; // The solid being built.
    std::vector<Point> vertices; // Positions of the vertices added so far.
    std::map<std::pair<int, int>, int> lines; // Straight edge per vertex pair, smaller index first.

    /// The vertex at a point, added once.
    int vertex(const Point& point) {

        for (size_t i = 0; i < vertices.size(); i++)
            if (vertices[i].distance(point) <= WELD)
                return static_cast<int>(i);

        vertices.push_back(point);

        return brep.add_vertex(point);
    }

    /// The straight edge between two vertices, added once, and whether a to b runs against it.
    std::pair<int, bool> line(int a, int b) {

        const std::pair<int, int> key = {std::min(a, b), std::max(a, b)};

        if (!lines.count(key))
            lines[key] = brep.add_edge(brep.add_curve_3d(NurbsCurve::create(false, 1, {vertices[key.first], vertices[key.second]})), key.first, key.second);

        return {lines[key], a != key.first};
    }

    /// A flat patch over the points in the face's frame, a little larger than they reach.
    Patch patch(const std::vector<Point>& points, const Point& origin, const Vector& x, const Vector& y) {

        double umin = 1e300;
        double umax = -1e300;
        double vmin = 1e300;
        double vmax = -1e300;

        for (const Point& point : points) {
            umin = std::min(umin, (point - origin).dot(x));
            umax = std::max(umax, (point - origin).dot(x));
            vmin = std::min(vmin, (point - origin).dot(y));
            vmax = std::max(vmax, (point - origin).dot(y));
        }

        const double pad = std::max(umax - umin, vmax - vmin) * 0.01;
        const Point p00 = origin + x * (umin - pad) + y * (vmin - pad);
        const NurbsCurve edge = NurbsCurve::create(false, 1, {p00, origin + x * (umax + pad) + y * (vmin - pad)});
        const NurbsSurface surface = Primitives::create_extrusion(edge, y * (vmax - vmin + 2.0 * pad));

        Patch result;
        result.surface = brep.add_surface(surface);
        result.du = surface.domain(0);
        result.dv = surface.domain(1);
        result.origin = surface.point_at(result.du.first, result.dv.first);
        result.eu = surface.point_at(result.du.second, result.dv.first) - result.origin;
        result.ev = surface.point_at(result.du.first, result.dv.second) - result.origin;

        return result;
    }
};

/// The hole loops of one stretch end: a closed edge on its circle or ellipse.
struct Loop {
    NurbsCurve curve; // 3D curve, parameter along the cylinder's u.
    int edge = -1; // BRep edge.
    double v = 0.0; // Height of the loop's start on the cylinder, as a fraction of it.
};

/// Adds one planar face of the mesh with the hole loops lying on it.
static int add_planar_face(Builder& builder, const PlanarFace& face, const std::vector<const Loop*>& holes) {

    std::vector<Point> span = face.points;

    for (const Loop* hole : holes) {
        const std::vector<Point> points = hole->curve.divide_by_count(16, true).first;
        span.insert(span.end(), points.begin(), points.end());
    }

    const Patch patch = builder.patch(span, face.points[0], face.x, face.y);
    std::vector<BRepRef> outer;

    for (size_t i = 0; i < face.points.size(); i++) {
        const int a = builder.vertex(face.points[i]);
        const int b = builder.vertex(face.points[(i + 1) % face.points.size()]);
        const std::pair<int, bool> edge = builder.line(a, b);
        const Point& p = builder.vertices[std::min(a, b)];
        const Point& q = builder.vertices[std::max(a, b)];
        const NurbsCurve line = NurbsCurve::create(false, 1, {p, q});
        builder.brep.add_pcurve(edge.first, patch.surface, builder.brep.add_curve_2d(on_patch(line, patch)));
        outer.push_back({edge.first, edge.second ? BRepOrientation::Reversed : BRepOrientation::Forward});
    }

    std::vector<BRepRef> wires = {{builder.brep.add_wire(outer), BRepOrientation::Forward}};

    for (const Loop* hole : holes) {
        const NurbsCurve uv = on_patch(hole->curve, patch);
        builder.brep.add_pcurve(hole->edge, patch.surface, builder.brep.add_curve_2d(uv));
        wires.push_back({builder.brep.add_wire({{hole->edge, uv_area(uv) > 0.0 ? BRepOrientation::Reversed : BRepOrientation::Forward}}), BRepOrientation::Forward});
    }

    return builder.brep.add_face(patch.surface, wires);
}

/// Adds the flat bottom of a hole ending inside the solid: its loop as the outer wire of a disc facing out of the solid.
static int add_bottom_face(Builder& builder, const Loop& loop, const Point& centre, const Vector& e, const Vector& outward) {

    const std::vector<Point> span = loop.curve.divide_by_count(16, true).first;
    const Patch patch = builder.patch(span, centre, e, outward.cross(e).normalized());
    const NurbsCurve uv = on_patch(loop.curve, patch);
    builder.brep.add_pcurve(loop.edge, patch.surface, builder.brep.add_curve_2d(uv));
    const int wire = builder.brep.add_wire({{loop.edge, uv_area(uv) > 0.0 ? BRepOrientation::Forward : BRepOrientation::Reversed}});

    return builder.brep.add_face(patch.surface, {{wire, BRepOrientation::Forward}});
}

/// The pcurve of a loop on its cylinder, exact: the loop is the unit circle mapped affinely, so around the cylinder its parameter is the circle's own and along it the height is linear in the circle's coordinates; each quadratic circle span becomes a cubic rational span of (t, v(t)).
static NurbsCurve on_cylinder(const NurbsCurve& circle, const Loop& loop, const Point& origin, const Vector& d, double height, const std::pair<double, double>& dv) {

    const int spans = (circle.cv_count() - 1) / 2;
    std::vector<double> heights(circle.cv_count());
    std::vector<double> weights(circle.cv_count());

    for (int i = 0; i < circle.cv_count(); i++) {
        const std::tuple<double, double, double, double> unit = circle.get_cv_4d(i);
        const std::tuple<double, double, double, double> cv = loop.curve.get_cv_4d(i);
        weights[i] = std::get<3>(unit);
        const Point p(std::get<0>(cv) / weights[i], std::get<1>(cv) / weights[i], std::get<2>(cv) / weights[i]);
        heights[i] = dv.first + (p - origin).dot(d) / height * (dv.second - dv.first);
    }

    NurbsCurve uv(3, true, 4, 3 * spans + 1);

    for (int i = 0; i < uv.nurbsknot_count(); i++)
        uv.set_nurbsknot(i, circle.nurbsknot(0) + static_cast<double>(i / 3));

    for (int j = 0; j < spans; j++) {
        const double w0 = weights[2 * j];
        const double w1 = weights[2 * j + 1];
        const double w2 = weights[2 * j + 2];
        const double n0 = w0 * heights[2 * j];
        const double n1 = w1 * heights[2 * j + 1];
        const double n2 = w2 * heights[2 * j + 2];
        const double k = circle.nurbsknot(0) + j;
        const std::array<double, 4> w = {w0, (w0 + 2.0 * w1) / 3.0, (2.0 * w1 + w2) / 3.0, w2};
        const std::array<double, 4> u = {k * w[0], k * w[1] + w0 / 3.0, k * w[2] + 2.0 * w1 / 3.0, k * w[3] + w2};
        const std::array<double, 4> v = {n0, (n0 + 2.0 * n1) / 3.0, (2.0 * n1 + n2) / 3.0, n2};

        for (int c = 0; c < 4; c++)
            uv.set_cv_4d(3 * j + c, u[c], v[c], 0.0, w[c]);
    }

    return uv;
}

// ═══════════════════════════════════════════════════════════════════════════
// Drilled BRep
// ═══════════════════════════════════════════════════════════════════════════

std::optional<BRep> drilled_brep(const Mesh& mesh, const std::vector<Drill>& drills) {

    const std::vector<PlanarFace> faces = planar_faces(mesh);

    if (faces.empty())
        return std::nullopt;

    const NurbsCurve circle = Primitives::circle(0.0, 0.0, 0.0, 1.0);
    std::vector<Stretch> stretches;

    for (size_t i = 0; i < drills.size(); i++) {
        const std::vector<Stretch> found = compute_stretches(faces, drills[i], i);
        stretches.insert(stretches.end(), found.begin(), found.end());
    }

    for (size_t s = 0; s < stretches.size(); s++)
        if (!is_clear(faces, drills, stretches, s))
            return std::nullopt;

    Builder builder;
    std::vector<std::vector<const Loop*>> holes(faces.size());
    std::vector<std::array<Loop, 2>> loops(stretches.size());
    std::vector<BRepRef> shell;
    double removed = 0.0;

    for (size_t s = 0; s < stretches.size(); s++) {
        const Stretch& stretch = stretches[s];
        const Drill& drill = drills[stretch.drill];
        const Point start = drill.axis.start();
        const Vector d = drill.axis.to_vector().normalized();
        const Vector e = d.cross(std::abs(d[2]) < 0.9 ? Vector(0.0, 0.0, 1.0) : Vector(1.0, 0.0, 0.0)).normalized();
        const Vector f = d.cross(e).normalized();

        for (size_t end = 0; end < 2; end++) {
            const int face = end == 0 ? stretch.face0 : stretch.face1;
            const PlanarFace* plane = face >= 0 ? &faces[face] : nullptr;
            const Point centre = end_centre(plane, start, d, end == 0 ? stretch.t0 : stretch.t1);
            loops[s][end].curve = hole_loop(centre, e, f, d, plane ? plane->normal : d, drill.radius);
            const int vertex = builder.vertex(loops[s][end].curve.point_at(loops[s][end].curve.domain().first));
            loops[s][end].edge = builder.brep.add_edge(builder.brep.add_curve_3d(loops[s][end].curve), vertex, vertex);
        }

        std::array<double, 2> reach = {1e300, -1e300};

        for (const Loop& loop : loops[s])
            for (const Point& point : loop.curve.divide_by_count(SAMPLES, true).first) {
                reach[0] = std::min(reach[0], (point - start).dot(d));
                reach[1] = std::max(reach[1], (point - start).dot(d));
            }

        const double pad = drill.radius * 0.1;
        const Point origin = start + d * (reach[0] - pad);
        const double height = reach[1] - reach[0] + 2.0 * pad;
        Xform frame;
        frame.m = {e[0], e[1], e[2], 0.0, f[0], f[1], f[2], 0.0, d[0], d[1], d[2], 0.0, origin[0], origin[1], origin[2], 1.0};
        const NurbsSurface bore = Primitives::cylinder_surface(0.0, 0.0, 0.0, drill.radius, height).transformed(frame);
        const int surface = builder.brep.add_surface(bore);
        const std::pair<double, double> du = bore.domain(0);
        const std::pair<double, double> dv = bore.domain(1);

        if (std::abs(circle.domain().first - du.first) > 1e-12 || std::abs(circle.domain().second - du.second) > 1e-12 || circle.cv_count() != 2 * ((circle.cv_count() - 1) / 2) + 1)
            return std::nullopt;

        for (Loop& loop : loops[s]) {
            const NurbsCurve uv = on_cylinder(circle, loop, origin, d, height, dv);
            loop.v = uv.point_at(uv.domain().first)[1];
            builder.brep.add_pcurve(loop.edge, surface, builder.brep.add_curve_2d(uv));
        }

        const int bottom = builder.vertex(loops[s][0].curve.point_at(du.first));
        const int top = builder.vertex(loops[s][1].curve.point_at(du.first));
        const int seam = builder.brep.add_edge(builder.brep.add_curve_3d(NurbsCurve::create(false, 1, {builder.vertices[bottom], builder.vertices[top]})), bottom, top);
        builder.brep.add_pcurve(seam, surface, builder.brep.add_curve_2d(uv_line(du.second, loops[s][0].v, du.second, loops[s][1].v)), builder.brep.add_curve_2d(uv_line(du.first, loops[s][0].v, du.first, loops[s][1].v)));
        const int wire = builder.brep.add_wire({{loops[s][0].edge, BRepOrientation::Forward}, {seam, BRepOrientation::Forward}, {loops[s][1].edge, BRepOrientation::Reversed}, {seam, BRepOrientation::Reversed}});
        shell.push_back({builder.brep.add_face(surface, {{wire, BRepOrientation::Forward}}), BRepOrientation::Reversed});

        for (size_t end = 0; end < 2; end++) {
            const int face = end == 0 ? stretch.face0 : stretch.face1;
            const Point centre = start + d * (end == 0 ? stretch.t0 : stretch.t1);

            if (face >= 0)
                holes[face].push_back(&loops[s][end]);
            else
                shell.push_back({add_bottom_face(builder, loops[s][end], centre, e, end == 0 ? d : -d), BRepOrientation::Forward});
        }

        const Point c0 = end_centre(stretch.face0 >= 0 ? &faces[stretch.face0] : nullptr, start, d, stretch.t0);
        const Point c1 = end_centre(stretch.face1 >= 0 ? &faces[stretch.face1] : nullptr, start, d, stretch.t1);
        removed += M_PI * drill.radius * drill.radius * (c1 - c0).magnitude();
    }

    for (size_t i = 0; i < faces.size(); i++)
        shell.push_back({add_planar_face(builder, faces[i], holes[i]), BRepOrientation::Forward});

    builder.brep.add_solid({{builder.brep.add_shell(shell), BRepOrientation::Forward}});
    const double expected = compute_volume(mesh) - removed;

    if (!builder.brep.is_solid() || std::abs(builder.brep.volume() - expected) > VOLUME * expected)
        return std::nullopt;

    return builder.brep;
}

}
