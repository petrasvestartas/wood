#include "pch.h"
#include "wood_brep_drill.h"
#include "wood_element_geometry.h"
#include "primitives.h"

namespace wood_session {

using namespace session_cpp;

constexpr bool TRACE = false;

const double WELD = 1e-6; // vertices closer than this are one
const double ON_SIDE = 1e-6; // a vertex this close to a straight side of another face, strictly between its ends, splits it: a T-vertex
const double CLEARANCE = 1e-6; // an edge must keep the drill radius plus this from the axis
const double STEEP = 0.2; // a drill crossing a face at a cosine below this is too oblique for a clean ellipse
const int SAMPLES = 64; // points sampled along a hole loop to find how far it reaches along the axis
const double VOLUME = 1e-3; // relative volume deviation the exact solid may show against the mesh it replaces
const double BORE_SLACK = 0.15; // share of the bored volume the kernel's tessellated volume may miss besides, its polygons lying inside the true circles
const double SQUARE = 1e-9; // a hole loop spanning less than this along its drill is square to it: the bore surface ends exactly on it, which the kernel meshes on its grid
const double AXIS = 1e-4; // two drills whose radii, directions, axis offsets and span gap all lie within this are one bore

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

/// Whether a point of the face's plane lies inside a loop of it, by ray crossing in the face frame.
static bool inside_loop(const PlanarFace& face, const std::vector<Point>& loop, const Point& point) {

    const Point& o = face.points[0];
    const double px = (point - o).dot(face.x);
    const double py = (point - o).dot(face.y);
    bool in = false;

    for (size_t i = 0, j = loop.size() - 1; i < loop.size(); j = i++) {
        const double xi = (loop[i] - o).dot(face.x);
        const double yi = (loop[i] - o).dot(face.y);
        const double xj = (loop[j] - o).dot(face.x);
        const double yj = (loop[j] - o).dot(face.y);

        if ((yi > py) != (yj > py) && px < (xj - xi) * (py - yi) / (yj - yi) + xi)
            in = !in;
    }

    return in;
}

/// Whether a point of the face's plane lies on the face: inside its outer loop and in none of its holes.
static bool inside(const PlanarFace& face, const Point& point) {

    if (!inside_loop(face, face.points, point))
        return false;

    for (const std::vector<Point>& hole : face.holes)
        if (inside_loop(face, hole, point))
            return false;

    return true;
}

double segment_distance(const Point& p0, const Point& p1, const Point& q0, const Point& q1) {

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

/// The root of a face in the union-find forest.
static size_t find_face(std::vector<size_t>& parent, size_t face) {

    while (parent[face] != face) {
        parent[face] = parent[parent[face]];
        face = parent[face];
    }

    return face;
}

/// The boundary loops of a region of mesh faces as vertex keys in winding order: every directed edge whose reverse the region lacks; empty when a vertex starts two of them.
static std::vector<std::vector<size_t>> region_loops(const std::vector<std::vector<size_t>>& rings) {

    std::set<std::pair<size_t, size_t>> directed;

    for (const std::vector<size_t>& ring : rings)
        for (size_t i = 0; i < ring.size(); i++)
            directed.insert({ring[i], ring[(i + 1) % ring.size()]});

    std::map<size_t, size_t> next;

    for (const std::pair<size_t, size_t>& edge : directed)
        if (!directed.count({edge.second, edge.first}) && !next.emplace(edge.first, edge.second).second)
            return {};

    std::vector<std::vector<size_t>> loops;
    std::set<size_t> used;

    for (const std::pair<const size_t, size_t>& start : next) {
        if (used.count(start.first))
            continue;

        std::vector<size_t> loop;
        size_t key = start.first;

        for (size_t step = 0; step <= next.size() && !used.count(key); step++) {
            used.insert(key);
            loop.push_back(key);
            key = next.at(key);
        }

        if (key != start.first)
            return {};

        loops.push_back(loop);
    }

    return loops;
}

/// The distinct vertices of all the faces' loops, within WELD.
static std::vector<Point> loop_vertices(const std::vector<PlanarFace>& faces) {

    std::vector<Point> vertices;

    for (const PlanarFace& face : faces) {
        std::vector<const std::vector<Point>*> loops = {&face.points};

        for (const std::vector<Point>& hole : face.holes)
            loops.push_back(&hole);

        for (const std::vector<Point>* loop : loops)
            for (const Point& point : *loop) {
                bool known = false;

                for (const Point& vertex : vertices)
                    if (vertex.distance(point) <= WELD) {
                        known = true;
                        break;
                    }

                if (!known)
                    vertices.push_back(point);
            }
    }

    return vertices;
}

/// The loop with every side split at the vertices lying on it strictly between its ends, in order along the side.
static std::vector<Point> split_loop(const std::vector<Point>& loop, const std::vector<Point>& vertices) {

    std::vector<Point> split;

    for (size_t i = 0; i < loop.size(); i++) {
        const Point& p = loop[i];
        const Point& q = loop[(i + 1) % loop.size()];
        const Vector d = q - p;
        const double length2 = d.dot(d);
        split.push_back(p);

        if (length2 <= WELD * WELD)
            continue;

        std::vector<std::pair<double, size_t>> inside;

        for (size_t v = 0; v < vertices.size(); v++) {
            const double t = (vertices[v] - p).dot(d) / length2;

            if (t <= 0.0 || t >= 1.0 || vertices[v].distance(p) <= WELD || vertices[v].distance(q) <= WELD)
                continue;

            if (vertices[v].distance(p + d * t) <= ON_SIDE)
                inside.push_back({t, v});
        }

        std::sort(inside.begin(), inside.end());

        for (const std::pair<double, size_t>& hit : inside) {
            if constexpr (TRACE)
                std::cout << "side " << p << " -> " << q << " split at " << vertices[hit.second] << std::endl;

            split.push_back(vertices[hit.second]);
        }
    }

    return split;
}

/// Splits every side of every loop at the vertices lying on it, so faces meeting along a line share its edges one-to-one: a T-vertex, a vertex of one face on the side of another, whether the mesh carried it or merging coplanar faces left it, would otherwise give one edge against two.
static void split_sides(std::vector<PlanarFace>& faces) {

    const std::vector<Point> vertices = loop_vertices(faces);

    for (PlanarFace& face : faces) {
        face.points = split_loop(face.points, vertices);

        for (std::vector<Point>& hole : face.holes)
            hole = split_loop(hole, vertices);
    }
}

/// The plane of every mesh face: its ring, unit normal and offset; empty when a face is not planar.
struct FacePlane {
    std::vector<size_t> ring; // Vertex keys around the face.
    Vector normal; // Unit normal.
    double offset = 0.0; // normal . point for any point of the face.
};

/// The planes of the mesh faces in key order; empty when a face is not planar.
static std::vector<FacePlane> mesh_planes(const Mesh& mesh) {

    std::vector<FacePlane> planes;

    for (const size_t key : mesh.faces()) {
        FacePlane plane;
        plane.ring = mesh.face_vertices(key).value();
        std::vector<Point> points;

        for (const size_t vertex : plane.ring)
            points.push_back(mesh.vertex_point(vertex).value());

        plane.normal = compute_newell(points).normalized();
        plane.offset = plane.normal.dot(points[0] - Point(0.0, 0.0, 0.0));

        for (const Point& point : points)
            if (std::abs((point - points[0]).dot(plane.normal)) > 1e-4)
                return {};

        planes.push_back(plane);
    }

    return planes;
}

/// The regions of edge-adjacent coplanar faces: per region the face its plane is taken from and the faces it holds in index order.
static std::vector<std::pair<size_t, std::vector<size_t>>> coplanar_regions(const std::vector<FacePlane>& planes) {

    std::map<std::pair<size_t, size_t>, std::vector<size_t>> edges;

    for (size_t f = 0; f < planes.size(); f++)
        for (size_t i = 0; i < planes[f].ring.size(); i++) {
            const size_t a = planes[f].ring[i];
            const size_t b = planes[f].ring[(i + 1) % planes[f].ring.size()];
            edges[{std::min(a, b), std::max(a, b)}].push_back(f);
        }

    std::vector<size_t> parent(planes.size());

    for (size_t f = 0; f < parent.size(); f++)
        parent[f] = f;

    for (const std::pair<const std::pair<size_t, size_t>, std::vector<size_t>>& edge : edges)
        if (edge.second.size() == 2) {
            const size_t f = edge.second[0];
            const size_t g = edge.second[1];

            if (planes[f].normal.dot(planes[g].normal) > 1.0 - 1e-9 && std::abs(planes[f].offset - planes[g].offset) < 1e-6)
                parent[find_face(parent, g)] = find_face(parent, f);
        }

    std::map<size_t, std::vector<size_t>> regions;

    for (size_t f = 0; f < planes.size(); f++)
        regions[find_face(parent, f)].push_back(f);

    return std::vector<std::pair<size_t, std::vector<size_t>>>(regions.begin(), regions.end());
}

/// A planar face from its outer loop, holes and normal, framed along its first side.
static PlanarFace framed_face(const std::vector<Point>& points, const std::vector<std::vector<Point>>& holes, const Vector& normal) {

    PlanarFace face;
    face.points = points;
    face.holes = holes;
    face.normal = normal;
    face.x = (points[1] - points[0]).normalized();
    face.y = normal.cross(face.x).normalized();

    return face;
}

/// The faces of one coplanar region: one face with its outer loop and holes when its boundary has exactly one loop winding with the normal, else every mesh face of the region on its own.
static std::vector<PlanarFace> region_faces(const Mesh& mesh, const std::vector<FacePlane>& planes, const Vector& normal, const std::vector<size_t>& region) {

    std::vector<std::vector<size_t>> members;

    for (const size_t f : region)
        members.push_back(planes[f].ring);

    const std::vector<std::vector<size_t>> loops = region_loops(members);
    size_t outer = loops.size();
    std::vector<std::vector<Point>> polygons;

    for (size_t l = 0; l < loops.size(); l++) {
        polygons.push_back({});

        for (const size_t vertex : loops[l])
            polygons.back().push_back(mesh.vertex_point(vertex).value());

        if (compute_newell(polygons.back()).dot(normal) > 0.0)
            outer = outer == loops.size() ? l : loops.size() + 1;
    }

    std::vector<PlanarFace> faces;

    if (outer < loops.size()) {
        std::vector<std::vector<Point>> holes;

        for (size_t l = 0; l < polygons.size(); l++)
            if (l != outer)
                holes.push_back(polygons[l]);

        faces.push_back(framed_face(polygons[outer], holes, normal));

        return faces;
    }

    for (const size_t f : region) {
        std::vector<Point> points;

        for (const size_t vertex : planes[f].ring)
            points.push_back(mesh.vertex_point(vertex).value());

        faces.push_back(framed_face(points, {}, planes[f].normal));
    }

    return faces;
}

std::vector<PlanarFace> planar_faces(const Mesh& mesh) {

    const std::vector<FacePlane> planes = mesh_planes(mesh);
    std::vector<PlanarFace> faces;

    if (planes.empty())
        return faces;

    for (const std::pair<size_t, std::vector<size_t>>& region : coplanar_regions(planes)) {
        const std::vector<PlanarFace> found = region_faces(mesh, planes, planes[region.first].normal, region.second);
        faces.insert(faces.end(), found.begin(), found.end());
    }

    return faces;
}

/// Where the line through start along the unit direction d crosses the faces: the parameter along it and the face, sorted along the line.
static std::vector<std::pair<double, int>> face_crossings(const std::vector<PlanarFace>& faces, const Point& start, const Vector& d) {

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

    return crossings;
}

/// The stretches of one drill inside the solid, from where its axis crosses the faces, clipped to the drill.
static std::vector<Stretch> compute_stretches(const std::vector<PlanarFace>& faces, const Drill& drill, size_t index) {

    const Point start = drill.axis.start();
    const Vector d = drill.axis.to_vector().normalized();
    const double length = drill.axis.length();
    const std::vector<std::pair<double, int>> crossings = face_crossings(faces, start, d);
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

            if (cosine < STEEP) {
                if constexpr (TRACE)
                    std::cout << fmt::format("drill {} crosses face {} too obliquely, cosine {:.3f}", stretch.drill, face, cosine) << std::endl;

                return false;
            }

            margin = std::max(margin, drill.radius / cosine + drill.radius);
        }

    const Point a = drill.axis.start() + d * (stretch.t0 - margin);
    const Point b = drill.axis.start() + d * (stretch.t1 + margin);

    for (const PlanarFace& face : faces) {
        std::vector<const std::vector<Point>*> loops = {&face.points};

        for (const std::vector<Point>& hole : face.holes)
            loops.push_back(&hole);

        for (const std::vector<Point>* loop : loops)
            for (size_t i = 0; i < loop->size(); i++)
                if (segment_distance(a, b, (*loop)[i], (*loop)[(i + 1) % loop->size()]) < drill.radius + CLEARANCE) {
                    if constexpr (TRACE)
                        std::cout << fmt::format("drill {} within {:.3f} of an edge of a face of {} corners at ({:.1f} {:.1f} {:.1f})", stretch.drill, segment_distance(a, b, (*loop)[i], (*loop)[(i + 1) % loop->size()]), loop->size(), (*loop)[i][0], (*loop)[i][1], (*loop)[i][2]) << std::endl;

                    return false;
                }
    }

    for (size_t o = 0; o < stretches.size(); o++) {
        if (stretches[o].drill == stretch.drill)
            continue;

        const Drill& other = drills[stretches[o].drill];
        const Vector e = other.axis.to_vector().normalized();
        const Point c = other.axis.start() + e * stretches[o].t0;
        const Point f = other.axis.start() + e * stretches[o].t1;

        if (segment_distance(drill.axis.start() + d * stretch.t0, drill.axis.start() + d * stretch.t1, c, f) < drill.radius + other.radius + CLEARANCE) {
            if constexpr (TRACE)
                std::cout << fmt::format("drill {} within {:.3f} of drill {}", stretch.drill, segment_distance(drill.axis.start() + d * stretch.t0, drill.axis.start() + d * stretch.t1, c, f), stretches[o].drill) << std::endl;

            return false;
        }
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
/// Points at count equal steps of a curve's parameter, both ends included: enough wherever the shape, not the spacing, matters, and free of the arc-length integration divide_by_count does.
static std::vector<Point> sample_curve(const NurbsCurve& curve, int count) {

    const std::pair<double, double> domain = curve.domain();
    std::vector<Point> points;
    points.reserve(count + 1);
    for (int i = 0; i <= count; i++)
        points.push_back(curve.point_at(domain.first + (domain.second - domain.first) * i / count));

    return points;
}

static double uv_area(const NurbsCurve& uv) {

    const std::vector<Point> points = sample_curve(uv, std::max(uv.cv_count() * 4, 32));
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

/// The wire of a polygon loop on a patch: shared straight edges in loop order, each with its pcurve on the patch.
static int polygon_wire(Builder& builder, const std::vector<Point>& loop, const Patch& patch) {

    std::vector<BRepRef> edges;

    for (size_t i = 0; i < loop.size(); i++) {
        const int a = builder.vertex(loop[i]);
        const int b = builder.vertex(loop[(i + 1) % loop.size()]);

        if (a == b)
            continue;

        const std::pair<int, bool> edge = builder.line(a, b);
        const NurbsCurve line = NurbsCurve::create(false, 1, {builder.vertices[std::min(a, b)], builder.vertices[std::max(a, b)]});
        builder.brep.add_pcurve(edge.first, patch.surface, builder.brep.add_curve_2d(on_patch(line, patch)));
        edges.push_back({edge.first, edge.second ? BRepOrientation::Reversed : BRepOrientation::Forward});
    }

    return builder.brep.add_wire(edges);
}

/// Adds one planar face of the solid: its outer loop, its own holes and the drill loops lying on it.
static int add_planar_face(Builder& builder, const PlanarFace& face, const std::vector<const Loop*>& drills) {

    std::vector<Point> span = face.points;

    for (const Loop* drill : drills) {
        const std::vector<Point> points = sample_curve(drill->curve, 16);
        span.insert(span.end(), points.begin(), points.end());
    }

    const Patch patch = builder.patch(span, face.points[0], face.x, face.y);
    std::vector<BRepRef> wires = {{polygon_wire(builder, face.points, patch), BRepOrientation::Forward}};

    for (const std::vector<Point>& hole : face.holes)
        wires.push_back({polygon_wire(builder, hole, patch), BRepOrientation::Forward});

    for (const Loop* drill : drills) {
        const NurbsCurve uv = on_patch(drill->curve, patch);
        builder.brep.add_pcurve(drill->edge, patch.surface, builder.brep.add_curve_2d(uv));
        wires.push_back({builder.brep.add_wire({{drill->edge, uv_area(uv) > 0.0 ? BRepOrientation::Reversed : BRepOrientation::Forward}}), BRepOrientation::Forward});
    }

    return builder.brep.add_face(patch.surface, wires);
}

/// Adds the flat bottom of a hole ending inside the solid: its loop as the outer wire of a disc facing out of the solid.
static int add_bottom_face(Builder& builder, const Loop& loop, const Point& centre, const Vector& e, const Vector& outward) {

    const std::vector<Point> span = sample_curve(loop.curve, 16);
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

/// The two drills as one when they share radius and axis, within AXIS, and their spans along it meet or overlap: the span from the lower start to the higher end along the first's direction.
static std::optional<Drill> joined(const Drill& a, const Drill& b) {

    if (std::abs(a.radius - b.radius) > AXIS)
        return std::nullopt;

    const Vector d = a.axis.to_vector().normalized();
    const Vector e = b.axis.to_vector().normalized();

    if (d.cross(e).magnitude() > AXIS)
        return std::nullopt;

    const Vector off = b.axis.start() - a.axis.start();

    if ((off - d * off.dot(d)).magnitude() > AXIS)
        return std::nullopt;

    const double a0 = 0.0;
    const double a1 = a.axis.length();
    const double b0 = std::min(off.dot(d), (b.axis.end() - a.axis.start()).dot(d));
    const double b1 = std::max(off.dot(d), (b.axis.end() - a.axis.start()).dot(d));

    if (b0 > a1 + AXIS || a0 > b1 + AXIS)
        return std::nullopt;

    return Drill{Line::from_points(a.axis.start() + d * std::min(a0, b0), a.axis.start() + d * std::max(a1, b1)), a.radius};
}

std::vector<Drill> merged_drills(std::vector<Drill> drills) {

    for (bool merged = true; merged;) {
        merged = false;

        for (size_t i = 0; i < drills.size() && !merged; i++)
            for (size_t j = i + 1; j < drills.size() && !merged; j++)
                if (const std::optional<Drill> one = joined(drills[i], drills[j])) {
                    drills[i] = *one;
                    drills.erase(drills.begin() + j);
                    merged = true;
                }
    }

    return drills;
}

/// The solid under construction: the builder, the hole loops of every stretch, which of them lie on each planar face, and the shell faces so far.
struct Drilling {
    Builder builder; // Vertices, edges and surfaces shared so far.
    std::vector<std::array<Loop, 2>> loops; // The two hole loops of every stretch.
    std::vector<std::vector<const Loop*>> holes; // Per planar face, the hole loops lying on it.
    std::vector<BRepRef> shell; // Faces added so far: every bore and every bottom disc.
    double removed = 0.0; // Volume the bores take out, for the check against the mesh.
};

/// The two hole loops of a stretch, on the faces it crosses or square to the drill at a bottom, each a closed edge from the vertex on its seam.
static void add_hole_loops(Drilling& drilling, const std::vector<PlanarFace>& faces, const Drill& drill, const Stretch& stretch, size_t s, const Vector& e, const Vector& f) {

    const Point start = drill.axis.start();
    const Vector d = drill.axis.to_vector().normalized();

    for (size_t end = 0; end < 2; end++) {
        const int face = end == 0 ? stretch.face0 : stretch.face1;
        const PlanarFace* plane = face >= 0 ? &faces[face] : nullptr;
        const Point centre = end_centre(plane, start, d, end == 0 ? stretch.t0 : stretch.t1);
        Loop& loop = drilling.loops[s][end];
        loop.curve = hole_loop(centre, e, f, d, plane ? plane->normal : d, drill.radius);
        const int vertex = drilling.builder.vertex(loop.curve.point_at(loop.curve.domain().first));
        loop.edge = drilling.builder.brep.add_edge(drilling.builder.brep.add_curve_3d(loop.curve), vertex, vertex);
    }
}

/// How far along the drill the loops of a stretch reach: the lowest and highest axis parameter of either, and whether each is square to the drill.
static std::array<std::array<double, 2>, 2> loop_reach(const std::array<Loop, 2>& loops, const Point& start, const Vector& d) {

    std::array<std::array<double, 2>, 2> reach = {{{1e300, -1e300}, {1e300, -1e300}}};

    for (size_t end = 0; end < 2; end++)
        for (const Point& point : sample_curve(loops[end].curve, SAMPLES)) {
            reach[end][0] = std::min(reach[end][0], (point - start).dot(d));
            reach[end][1] = std::max(reach[end][1], (point - start).dot(d));
        }

    return reach;
}

/// Adds the exact bore of one stretch: its two hole loops, the cylinder between them with its seam, and a flat disc at an end inside the solid, the loops on the faces crossed kept for those faces; false when the kernel circle does not match the cylinder's parameter.
static bool add_bore(Drilling& drilling, const std::vector<PlanarFace>& faces, const Drill& drill, const Stretch& stretch, size_t s, const NurbsCurve& circle) {

    const Point start = drill.axis.start();
    const Vector d = drill.axis.to_vector().normalized();
    const Vector e = d.cross(std::abs(d[2]) < 0.9 ? Vector(0.0, 0.0, 1.0) : Vector(1.0, 0.0, 0.0)).normalized();
    const Vector f = d.cross(e).normalized();
    Builder& builder = drilling.builder;
    std::array<Loop, 2>& loops = drilling.loops[s];

    add_hole_loops(drilling, faces, drill, stretch, s, e, f);

    const std::array<std::array<double, 2>, 2> reach = loop_reach(loops, start, d);
    const double low = std::min(reach[0][0], reach[1][0]);
    const double high = std::max(reach[0][1], reach[1][1]);
    const double pad_low = reach[0][1] - reach[0][0] < SQUARE ? 0.0 : drill.radius * 0.1;
    const double pad_high = reach[1][1] - reach[1][0] < SQUARE ? 0.0 : drill.radius * 0.1;
    const Point origin = start + d * (low - pad_low);
    const double height = high - low + pad_low + pad_high;
    Xform frame;
    frame.m = {e[0], e[1], e[2], 0.0, f[0], f[1], f[2], 0.0, d[0], d[1], d[2], 0.0, origin[0], origin[1], origin[2], 1.0};
    const NurbsSurface bore = Primitives::cylinder_surface(0.0, 0.0, 0.0, drill.radius, height).transformed(frame);
    const int surface = builder.brep.add_surface(bore);
    const std::pair<double, double> du = bore.domain(0);
    const std::pair<double, double> dv = bore.domain(1);

    if (std::abs(circle.domain().first - du.first) > 1e-12 || std::abs(circle.domain().second - du.second) > 1e-12 || circle.cv_count() != 2 * ((circle.cv_count() - 1) / 2) + 1)
        return false;

    for (Loop& loop : loops) {
        const NurbsCurve uv = on_cylinder(circle, loop, origin, d, height, dv);
        loop.v = uv.point_at(uv.domain().first)[1];
        builder.brep.add_pcurve(loop.edge, surface, builder.brep.add_curve_2d(uv));
    }

    const int bottom = builder.vertex(loops[0].curve.point_at(du.first));
    const int top = builder.vertex(loops[1].curve.point_at(du.first));
    const int seam = builder.brep.add_edge(builder.brep.add_curve_3d(NurbsCurve::create(false, 1, {builder.vertices[bottom], builder.vertices[top]})), bottom, top);
    builder.brep.add_pcurve(seam, surface, builder.brep.add_curve_2d(uv_line(du.second, loops[0].v, du.second, loops[1].v)), builder.brep.add_curve_2d(uv_line(du.first, loops[0].v, du.first, loops[1].v)));
    const int wire = builder.brep.add_wire({{loops[0].edge, BRepOrientation::Forward}, {seam, BRepOrientation::Forward}, {loops[1].edge, BRepOrientation::Reversed}, {seam, BRepOrientation::Reversed}});
    drilling.shell.push_back({builder.brep.add_face(surface, {{wire, BRepOrientation::Forward}}), BRepOrientation::Reversed});

    for (size_t end = 0; end < 2; end++) {
        const int face = end == 0 ? stretch.face0 : stretch.face1;
        const Point centre = start + d * (end == 0 ? stretch.t0 : stretch.t1);

        if (face >= 0)
            drilling.holes[face].push_back(&loops[end]);
        else
            drilling.shell.push_back({add_bottom_face(builder, loops[end], centre, e, end == 0 ? d : -d), BRepOrientation::Forward});
    }

    const Point c0 = end_centre(stretch.face0 >= 0 ? &faces[stretch.face0] : nullptr, start, d, stretch.t0);
    const Point c1 = end_centre(stretch.face1 >= 0 ? &faces[stretch.face1] : nullptr, start, d, stretch.t1);
    drilling.removed += M_PI * drill.radius * drill.radius * (c1 - c0).magnitude();

    return true;
}

/// Whether the built solid closes and holds the mesh's volume less the bores, within VOLUME and the kernel's tessellation slack on the bores.
static bool check_volume(const BRep& brep, const Mesh& mesh, double removed, const std::vector<Drill>& drills, const std::vector<Stretch>& stretches) {

    const double expected = compute_volume(mesh) - removed;

    if (brep.is_solid() && std::abs(brep.volume() - expected) <= VOLUME * expected + BORE_SLACK * removed)
        return true;

    if constexpr (TRACE) {
        std::cout << fmt::format("built solid {} with volume {:.1f} against {:.1f} expected, {:.1f} removed by {} drills in {} stretches", brep.is_solid(), brep.volume(), expected, removed, drills.size(), stretches.size()) << std::endl;

        for (const Stretch& stretch : stretches)
            std::cout << fmt::format("   drill {} radius {:.1f} length {:.1f}: stretch {:.1f} .. {:.1f} faces {} {}", stretch.drill, drills[stretch.drill].radius, drills[stretch.drill].axis.length(), stretch.t0, stretch.t1, stretch.face0, stretch.face1) << std::endl;
    }

    return false;
}

/// Prints every edge the faces do not use exactly twice.
static void trace_edge_uses(const BRep& brep) {

    std::map<int, int> uses;

    for (const BRepFace& face : brep.m_faces)
        for (const BRepRef& wire : face.wires)
            for (const BRepRef& edge : brep.m_wires[wire.index].edges)
                uses[edge.index]++;

    for (const std::pair<const int, int>& use : uses)
        if (use.second != 2)
            std::cout << "edge " << use.first << " used " << use.second << " times" << std::endl;
}

std::optional<BRep> drilled_brep(const Mesh& mesh, const std::vector<Drill>& given) {

    const std::vector<Drill> drills = merged_drills(given);
    std::vector<PlanarFace> faces = planar_faces(mesh);

    if (faces.empty()) {
        if constexpr (TRACE)
            std::cout << "a face is not planar" << std::endl;

        return std::nullopt;
    }

    split_sides(faces);

    const NurbsCurve circle = Primitives::circle(0.0, 0.0, 0.0, 1.0);
    std::vector<Stretch> stretches;

    for (size_t i = 0; i < drills.size(); i++) {
        const std::vector<Stretch> found = compute_stretches(faces, drills[i], i);
        stretches.insert(stretches.end(), found.begin(), found.end());
    }

    for (size_t s = 0; s < stretches.size(); s++)
        if (!is_clear(faces, drills, stretches, s))
            return std::nullopt;

    Drilling drilling;
    drilling.holes.resize(faces.size());
    drilling.loops.resize(stretches.size());

    for (size_t s = 0; s < stretches.size(); s++)
        if (!add_bore(drilling, faces, drills[stretches[s].drill], stretches[s], s, circle))
            return std::nullopt;

    for (size_t i = 0; i < faces.size(); i++)
        drilling.shell.push_back({add_planar_face(drilling.builder, faces[i], drilling.holes[i]), BRepOrientation::Forward});

    BRep& brep = drilling.builder.brep;
    brep.add_solid({{brep.add_shell(drilling.shell), BRepOrientation::Forward}});

    if (!check_volume(brep, mesh, drilling.removed, drills, stretches))
        return std::nullopt;

    if constexpr (TRACE)
        trace_edge_uses(brep);

    return brep;
}

std::vector<std::array<double, 2>> inside_stretches(const Mesh& mesh, const Line& line) {

    const std::vector<PlanarFace> faces = planar_faces(mesh);
    const Vector d = line.to_vector().normalized();
    const std::vector<std::pair<double, int>> crossings = face_crossings(faces, line.start(), d);
    std::vector<std::array<double, 2>> stretches;

    for (size_t i = 0; i + 1 < crossings.size(); i++)
        if (faces[crossings[i].second].normal.dot(d) < 0.0)
            stretches.push_back({crossings[i].first, crossings[i + 1].first});

    return stretches;
}

bool is_inside(const std::vector<PlanarFace>& faces, const Point& point) {

    const Vector ray = Vector(0.12345, 0.23456, 1.0).normalized();
    bool in = false;

    for (const PlanarFace& face : faces) {
        const double cosine = face.normal.dot(ray);

        if (std::abs(cosine) < 1e-12)
            continue;

        const double t = face.normal.dot(face.points[0] - point) / cosine;

        if (t > 0.0 && inside(face, point + ray * t))
            in = !in;
    }

    return in;
}

}
