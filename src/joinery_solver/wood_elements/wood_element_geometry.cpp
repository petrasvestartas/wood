#include "pch.h"
#include "wood_element_geometry.h"
#include "wood_brep_drill.h"

namespace wood_session {

using namespace session_cpp;

bool is_geometry_feature(std::string_view feature_type) {
    return feature_type == "outline" || feature_type == "axis" || feature_type == "section" || feature_type == "top" || feature_type == "bottom";
}

ElementFeature polyline_feature(std::string_view feature_type, const Polyline& polyline, int face_index) {
    return ElementFeature(
        std::string(feature_type),
        face_index,
        {polyline},
        std::string(feature_type)
    );
}

bool is_session_feature(std::string_view feature_type) {
    return feature_type == "joint" || feature_type == "contact" || feature_type == "drill";
}

std::vector<ElementFeature> session_features(const Element& element) {

    std::vector<ElementFeature> kept;
    for (const ElementFeature& feature : element.features()) {

        if (!is_session_feature(feature.feature_type))
            continue;

        kept.push_back(feature);
        if (feature.has_guid())
            kept.back().guid() = feature.guid();
    }

    return kept;
}

Polyline square_section(
    const Point& at,
    const Vector& direction,
    const Vector& up,
    double radius
) {

    const Vector along = direction.normalized();
    Vector rise = up.is_parallel_to(along) == 0 ? up : Vector::z_axis();
    if (rise.is_parallel_to(along) != 0)
        rise = Vector::x_axis();

    const Vector side = rise.cross(along).normalized();
    rise = along.cross(side).normalized();

    const Vector s = side * radius;
    const Vector u = rise * radius;

    return Polyline({at - u - s, at - u + s, at + u + s, at + u - s, at - u - s});
}

/// The solid between matching bottom and top loops as a boundary representation: loop 0 the outer outline, the rest holes; one quad per edge of every loop.
Vector compute_newell(const std::vector<Point>& points) {

    const size_t count = points.size() > 1 && points.front().distance(points.back()) < Tolerance::APPROXIMATION ? points.size() - 1 : points.size();
    Vector normal(0.0, 0.0, 0.0);
    for (size_t i = 0; i < count; i++)
        normal += (points[i] - Point(0.0, 0.0, 0.0)).cross(points[(i + 1) % count] - Point(0.0, 0.0, 0.0));

    return normal.normalized();
}

Vector cgal_base1(const Vector& normal) {

    if (normal[0] == 0.0)
        return Vector(1.0, 0.0, 0.0);
    if (normal[1] == 0.0)
        return Vector(0.0, 1.0, 0.0);
    if (normal[2] == 0.0)
        return Vector(0.0, 0.0, 1.0);

    const double ax = std::abs(normal[0]);
    const double ay = std::abs(normal[1]);
    const double az = std::abs(normal[2]);
    if (ax <= ay && ax <= az)
        return Vector(0.0, -normal[2], normal[1]).normalized();
    if (ay <= ax && ay <= az)
        return Vector(-normal[2], 0.0, normal[0]).normalized();
    return Vector(-normal[1], normal[0], 0.0).normalized();
}

Clipper2Lib::PathD clipper_path(const Polyline& outline, const Point& origin, const Vector& x_axis, const Vector& y_axis, const bool open) {

    Clipper2Lib::PathD path;
    const size_t n = !open && outline.is_closed() ? outline.point_count() - 1 : outline.point_count();
    path.reserve(n);
    for (size_t k = 0; k < n; ++k) {
        const Vector d = outline.get_point(k) - origin;
        path.emplace_back(d.dot(x_axis), d.dot(y_axis));
    }

    return path;
}

std::vector<Plane> face_planes(const Mesh& mesh) {

    std::vector<Plane> planes;
    for (const Polyline& outline : mesh.face_outlines()) {
        std::vector<Point> points = outline.get_points();
        points.pop_back();
        planes.push_back(Plane::from_point_normal(Point::centroid(points), compute_newell(points)));
    }

    return planes;
}

/// The signed flux of one triangle of vertex keys about the origin, six times its tetrahedron volume.
static double compute_flux(
    const Mesh& mesh,
    size_t a,
    size_t b,
    size_t c
) {

    const Point p0 = *mesh.vertex_point(a);
    const Point p1 = *mesh.vertex_point(b);
    const Point p2 = *mesh.vertex_point(c);

    return p0[0] * (p1[1] * p2[2] - p1[2] * p2[1]) + p0[1] * (p1[2] * p2[0] - p1[0] * p2[2]) + p0[2] * (p1[0] * p2[1] - p1[1] * p2[0]);
}

double compute_volume(const Mesh& mesh) {

    double total = 0.0;
    for (const size_t face : mesh.faces()) {
        if (mesh.get_triangulation().count(face)) {
            for (const std::array<size_t, 3>& triangle : mesh.get_triangulation().at(face))
                total += compute_flux(
                    mesh,
                    triangle[0],
                    triangle[1],
                    triangle[2]
                );
            continue;
        }

        const std::vector<size_t> ring = *mesh.face_vertices(face);
        for (size_t i = 1; i + 1 < ring.size(); i++)
            total += compute_flux(
                mesh,
                ring[0],
                ring[i],
                ring[i + 1]
            );
    }

    return std::abs(total) / 6.0;
}

const double COPLANAR = 1e-6; // a side strip whose points lie within this of one plane becomes one face; closer points are one

/// The points of every section without the closing point, empty unless all share one count of at least three.
static std::vector<std::vector<Point>> compute_rings(const std::vector<Polyline>& sections) {

    std::vector<std::vector<Point>> rings;

    for (const Polyline& section : sections) {
        std::vector<Point> points = section.get_points();

        if (section.is_closed())
            points.pop_back();

        if (points.size() < 3 || (!rings.empty() && points.size() != rings.front().size()))
            return {};

        rings.push_back(points);
    }

    return rings;
}

/// True when every point lies within COPLANAR of the Newell plane through them.
static bool is_coplanar(const std::vector<Point>& points) {

    const Vector normal = compute_newell(points);
    const Point origin = Point::centroid(points);

    for (const Point& point : points)
        if (std::abs((point - origin).dot(normal)) > COPLANAR)
            return false;

    return true;
}

/// Six times the signed volume enclosed by the faces, positive when they face outwards.
static double compute_signed_volume(const std::vector<Point>& vertices, const std::vector<std::vector<size_t>>& faces) {

    double total = 0.0;

    for (const std::vector<size_t>& face : faces)
        for (size_t i = 1; i + 1 < face.size(); i++) {
            const Vector a = vertices[face[0]] - Point(0.0, 0.0, 0.0);
            const Vector b = vertices[face[i]] - Point(0.0, 0.0, 0.0);
            const Vector c = vertices[face[i + 1]] - Point(0.0, 0.0, 0.0);
            total += a.dot(b.cross(c));
        }

    return total;
}

/// The side faces of the strip under ring side j: one polygon when the strip is planar, else one quad per station pair.
static void add_strip(
    const std::vector<Point>& vertices,
    size_t stations,
    size_t count,
    size_t j,
    std::vector<std::vector<size_t>>& faces
) {

    const size_t k = (j + 1) % count;
    std::vector<size_t> strip = {j};

    for (size_t i = 0; i < stations; i++)
        strip.push_back(i * count + k);

    for (size_t i = stations - 1; i > 0; i--)
        strip.push_back(i * count + j);

    std::vector<Point> points;

    for (const size_t index : strip)
        points.push_back(vertices[index]);

    if (stations == 2 || is_coplanar(points)) {
        faces.push_back(strip);
        return;
    }

    for (size_t i = 0; i + 1 < stations; i++)
        faces.push_back({i * count + j, i * count + k, (i + 1) * count + k, (i + 1) * count + j});
}

/// The face's points with repeats dropped, a stepped section collapsing a side onto its neighbour; empty when fewer than three are left.
static std::vector<Point> compute_face_points(const std::vector<Point>& vertices, const std::vector<size_t>& face) {

    std::vector<Point> points;

    for (const size_t index : face)
        if (points.empty() || points.back().distance(vertices[index]) > COPLANAR)
            points.push_back(vertices[index]);

    while (points.size() > 1 && points.back().distance(points.front()) <= COPLANAR)
        points.pop_back();

    if (points.size() < 3)
        return {};

    return points;
}

Mesh loft_stations(const std::vector<Polyline>& sections) {

    const std::vector<std::vector<Point>> rings = compute_rings(sections);

    if (rings.size() < 2)
        return Mesh();

    const size_t stations = rings.size();
    const size_t count = rings.front().size();
    std::vector<Point> vertices;

    for (const std::vector<Point>& ring : rings)
        vertices.insert(vertices.end(), ring.begin(), ring.end());

    std::vector<std::vector<size_t>> faces;

    for (size_t j = 0; j < count; j++)
        add_strip(
            vertices,
            stations,
            count,
            j,
            faces
        );

    std::vector<size_t> start(count);
    std::vector<size_t> end(count);

    for (size_t j = 0; j < count; j++) {
        start[j] = count - 1 - j;
        end[j] = (stations - 1) * count + j;
    }

    faces.push_back(start);
    faces.push_back(end);

    const bool inward = compute_signed_volume(vertices, faces) < 0.0;
    std::vector<std::vector<Point>> polygons;

    for (std::vector<size_t>& face : faces) {
        if (inward)
            std::reverse(face.begin(), face.end());

        const std::vector<Point> polygon = compute_face_points(vertices, face);

        if (!polygon.empty())
            polygons.push_back(polygon);
    }

    return Mesh::from_polylines(polygons);
}

BRep brep_between_loops(const std::vector<Polyline>& bottom, const std::vector<Polyline>& top) {

    std::vector<Polyline> faces{bottom[0], top[0]};
    std::vector<std::vector<Polyline>> holes(2);

    for (size_t loop = 1; loop < bottom.size(); loop++) {
        holes[0].push_back(bottom[loop]);
        holes[1].push_back(top[loop]);
    }

    // the side faces are the walls Mesh::loft builds between the same loops, so the two enclose one volume: a quad whose four corners share a plane stays a quad, one whose corners do not becomes the two triangles the volume fans it into, and a wall the loft already made a triangle stays one
    const Mesh walls = Mesh::loft(bottom, top, false);
    for (const size_t face : walls.faces()) {
        const std::vector<size_t> ring = *walls.face_vertices(face);
        std::vector<Point> corners;
        for (const size_t vertex : ring)
            corners.push_back(*walls.vertex_point(vertex));
        std::vector<Polyline> sides;
        if (corners.size() == 4 && !is_coplanar(corners))
            sides = {Polyline({corners[0], corners[1], corners[2], corners[0]}), Polyline({corners[0], corners[2], corners[3], corners[0]})};
        else {
            corners.push_back(corners.front());
            sides = {Polyline(corners)};
        }
        for (const Polyline& side : sides) {
            faces.push_back(side);
            holes.push_back({});
        }
    }

    return BRep::from_polylines(faces, holes);
}

BRep brep_sections(const std::vector<Polyline>& sections) {

    if (sections.size() < 2)
        return BRep();

    std::vector<Polyline> faces{sections.front(), sections.back()};

    for (size_t i = 0; i + 1 < sections.size(); i++) {

        const Polyline& lower = sections[i];
        const Polyline& upper = sections[i + 1];
        if (lower.point_count() != upper.point_count())
            return BRep();

        const size_t segment_count = lower.point_count() - 1;
        for (size_t segment = 0; segment < segment_count; segment++)
            faces.push_back(Polyline({lower.get_point(segment), lower.get_point(segment + 1), upper.get_point(segment + 1), upper.get_point(segment), lower.get_point(segment)}));
    }

    return BRep::from_polylines(faces, std::vector<std::vector<Polyline>>(faces.size()));
}

Mesh sweep_sections(const std::vector<Polyline>& sections) {

    if (sections.size() < 2)
        return Mesh();

    std::vector<Point> vertices;
    size_t ring = 0;
    for (const Polyline& section : sections) {

        std::vector<Point> points = section.get_points();
        if (section.is_closed())
            points.pop_back();

        if (ring == 0)
            ring = points.size();
        if (points.size() != ring || ring < 3)
            return Mesh();

        vertices.insert(vertices.end(), points.begin(), points.end());
    }

    std::vector<std::vector<size_t>> faces;
    for (size_t i = 0; i + 1 < sections.size(); i++)
        for (size_t j = 0; j < ring; j++) {
            const size_t a = i * ring + j;
            const size_t b = i * ring + (j + 1) % ring;
            faces.push_back({a, b, b + ring, a + ring});
        }

    std::vector<size_t> start(ring);
    std::vector<size_t> end(ring);
    const size_t last = (sections.size() - 1) * ring;
    for (size_t j = 0; j < ring; j++) {
        start[j] = ring - 1 - j;
        end[j] = last + j;
    }
    faces.push_back(start);
    faces.push_back(end);

    return Mesh::from_vertices_and_faces(vertices, faces);
}

Mesh cut_mesh(const Mesh& geometry, const std::vector<Plane>& planes) {

    Mesh cut = geometry;
    for (const Plane& plane : planes)
        cut = cut.cut_by_plane(plane);

    return cut;
}

BRep cut_brep(const BRep& geometry, const std::vector<Plane>& planes) {

    BRep cut = geometry;
    for (const Plane& plane : planes)
        cut = cut.cut_by_plane(plane);

    return cut;
}

/// Distance of a point from the plane, positive on the side its normal points to.
static double signed_distance(const Plane& plane, const Point& point) {
    return (point - plane.origin()).dot(plane.z_axis());
}

/// The first axis parameter (i at point i) on the kept side of the plane; past the last point when none is.
static double enter_parameter(const std::vector<Point>& points, const Plane& plane) {

    for (size_t i = 0; i < points.size(); i++) {

        const double d = signed_distance(plane, points[i]);
        if (d < -Tolerance::APPROXIMATION)
            continue;

        if (i == 0)
            return 0.0;

        const double before = signed_distance(plane, points[i - 1]);

        return static_cast<double>(i - 1) + before / (before - d);
    }

    return static_cast<double>(points.size());
}

/// The last axis parameter on the kept side of the plane; below zero when none is.
static double exit_parameter(const std::vector<Point>& points, const Plane& plane) {

    for (size_t i = points.size(); i > 0; i--) {

        const double d = signed_distance(plane, points[i - 1]);
        if (d < -Tolerance::APPROXIMATION)
            continue;

        if (i == points.size())
            return static_cast<double>(i - 1);

        const double after = signed_distance(plane, points[i]);

        return static_cast<double>(i - 1) + d / (d - after);
    }

    return -1.0;
}

/// The axis point at parameter t on segment `segment`.
static Point point_at(const std::vector<Point>& points, size_t segment, double t) {
    return points[segment] + (points[segment + 1] - points[segment]) * (t - static_cast<double>(segment));
}

/// A closed ring cut by a plane, the part on the side its normal points to; empty when under three points are left.
static Polyline clip_ring(const Polyline& ring, const Plane& plane) {

    const size_t n = ring.is_closed() ? ring.point_count() - 1 : ring.point_count();
    std::vector<Point> kept;

    for (size_t i = 0; i < n; i++) {

        const Point a = ring.get_point(i);
        const Point b = ring.get_point((i + 1) % n);
        const double da = signed_distance(plane, a);
        const double db = signed_distance(plane, b);

        if (da >= -Tolerance::APPROXIMATION)
            kept.push_back(a);

        if ((da < -Tolerance::APPROXIMATION) != (db < -Tolerance::APPROXIMATION))
            kept.push_back(a + (b - a) * (da / (da - db)));
    }

    if (kept.size() < 3)
        return Polyline();

    kept.push_back(kept.front());

    return Polyline(kept);
}

/// A ring clipped by every cut but `skip`: the part of it inside the member.
static Polyline kept_ring(const Polyline& ring, const std::vector<Plane>& cuts, size_t skip) {

    Polyline face = ring;
    for (size_t k = 0; k < cuts.size() && face.point_count() > 0; k++)
        if (k != skip)
            face = clip_ring(face, cuts[k]);

    return face;
}

/// The face cut `index` leaves on a member lofted through `sections`: every corner's edge from station to station met by the plane, then clipped by every other cut.
static Polyline end_section(const std::vector<Polyline>& sections, const std::vector<Plane>& cuts, size_t index) {

    const Plane& plane = cuts[index];
    std::vector<Point> corners;

    for (size_t j = 0; j < sections.front().point_count(); j++) {

        // the station pair the corner crosses the plane between; a corner that never crosses extends its nearer end pair
        size_t i = 0;
        while (i + 1 < sections.size() - 1 && signed_distance(plane, sections[i].get_point(j)) * signed_distance(plane, sections[i + 1].get_point(j)) > 0.0)
            i++;

        if (signed_distance(plane, sections[i].get_point(j)) * signed_distance(plane, sections[i + 1].get_point(j)) > 0.0
            && std::abs(signed_distance(plane, sections.front().get_point(j))) < std::abs(signed_distance(plane, sections.back().get_point(j))))
            i = 0;

        const Point a = sections[i].get_point(j);
        const Point b = sections[i + 1].get_point(j);
        const double da = signed_distance(plane, a);
        const double db = signed_distance(plane, b);

        if (std::abs(da - db) < Tolerance::ZERO_TOLERANCE)
            return Polyline();

        corners.push_back(a + (b - a) * (da / (da - db)));
    }

    return kept_ring(Polyline(corners), cuts, index);
}

std::pair<Polyline, std::vector<Polyline>> trim_to_cuts(const Polyline& axis, const std::vector<Polyline>& sections, const std::vector<Plane>& cuts) {

    const std::vector<Point> points = axis.get_points();
    if (points.size() < 2 || cuts.empty())
        return {axis, sections.size() == points.size() ? sections : std::vector<Polyline>()};

    const size_t n = points.size() - 1;
    double start = 0.0;
    double end = static_cast<double>(n);
    int start_cut = -1;
    int end_cut = -1;

    for (size_t k = 0; k < cuts.size(); k++) {

        const double enter = enter_parameter(points, cuts[k]);
        if (enter > start) {
            start = enter;
            start_cut = static_cast<int>(k);
        }

        const double exit = exit_parameter(points, cuts[k]);
        if (exit < end) {
            end = exit;
            end_cut = static_cast<int>(k);
        }
    }

    if (start >= end)
        return {Polyline(), {}};

    const bool ringed = sections.size() == points.size();
    const size_t first = std::min(static_cast<size_t>(start), n - 1);
    const size_t last = std::min(static_cast<size_t>(std::max(std::ceil(end) - 1.0, 0.0)), n - 1);
    std::vector<Point> kept = {point_at(points, first, start)};
    std::vector<Polyline> rings;

    if (ringed)
        rings.push_back(start_cut < 0 ? kept_ring(sections[0], cuts, cuts.size()) : end_section(sections, cuts, start_cut));

    for (size_t i = first + 1; i <= last; i++) {

        kept.push_back(points[i]);

        if (ringed)
            rings.push_back(kept_ring(sections[i], cuts, cuts.size()));
    }

    kept.push_back(point_at(points, last, end));

    if (ringed)
        rings.push_back(end_cut < 0 ? kept_ring(sections[n], cuts, cuts.size()) : end_section(sections, cuts, end_cut));

    return {Polyline(kept), rings};
}

std::optional<Plane> frame_along(const Point& origin, const Vector& along, const Vector& z) {

    const Vector up = z.normalized();
    const Vector across = along - up * along.dot(up);

    if (across.magnitude() < Tolerance::ZERO_TOLERANCE || z.magnitude() < Tolerance::ZERO_TOLERANCE)
        return std::nullopt;

    const Vector x = across.normalized();

    return Plane::from_frame(
        origin,
        x,
        up.cross(x),
        up
    );
}

bool is_mirror(const Xform& xform) {

    const Vector x = xform.transform_vector(Vector::x_axis());
    const Vector y = xform.transform_vector(Vector::y_axis());
    const Vector z = xform.transform_vector(Vector::z_axis());

    return x.cross(y).dot(z) < 0.0;
}

std::vector<ElementFeature> transformed_features(const std::vector<ElementFeature>& features, const Xform& xform) {

    std::vector<ElementFeature> moved = clone(features);

    for (ElementFeature& feature : moved)
        feature.outlines = transformed_list(feature.outlines, xform);

    return moved;
}


static void offset_index(int& index, int amount) {

    if (index >= 0)
        index += amount;
}

void append_brep(BRep& target, BRep source) {

    for (BRepEdge& edge : source.m_edges) {
        offset_index(edge.curve_3d_index, target.m_curves_3d.size());
        offset_index(edge.start_vertex, target.m_vertices.size());
        offset_index(edge.end_vertex, target.m_vertices.size());

        for (BRepCurveOnSurface& curve : edge.pcurves) {
            offset_index(curve.surface_index, target.m_surfaces.size());
            offset_index(curve.curve_2d_index, target.m_curves_2d.size());
            offset_index(curve.curve_2d_index_2, target.m_curves_2d.size());
        }
    }

    for (BRepWire& wire : source.m_wires)
        for (BRepRef& reference : wire.edges)
            offset_index(reference.index, target.m_edges.size());

    for (BRepFace& face : source.m_faces) {
        offset_index(face.surface_index, target.m_surfaces.size());

        for (BRepRef& reference : face.wires)
            offset_index(reference.index, target.m_wires.size());
    }

    for (BRepShell& shell : source.m_shells)
        for (BRepRef& reference : shell.faces)
            offset_index(reference.index, target.m_faces.size());

    for (BRepSolid& solid : source.m_solids)
        for (BRepRef& reference : solid.shells)
            offset_index(reference.index, target.m_shells.size());

    target.m_surfaces.insert(target.m_surfaces.end(), std::make_move_iterator(source.m_surfaces.begin()), std::make_move_iterator(source.m_surfaces.end()));
    target.m_curves_3d.insert(target.m_curves_3d.end(), std::make_move_iterator(source.m_curves_3d.begin()), std::make_move_iterator(source.m_curves_3d.end()));
    target.m_curves_2d.insert(target.m_curves_2d.end(), std::make_move_iterator(source.m_curves_2d.begin()), std::make_move_iterator(source.m_curves_2d.end()));
    target.m_vertices.insert(target.m_vertices.end(), std::make_move_iterator(source.m_vertices.begin()), std::make_move_iterator(source.m_vertices.end()));
    target.m_edges.insert(target.m_edges.end(), std::make_move_iterator(source.m_edges.begin()), std::make_move_iterator(source.m_edges.end()));
    target.m_wires.insert(target.m_wires.end(), std::make_move_iterator(source.m_wires.begin()), std::make_move_iterator(source.m_wires.end()));
    target.m_faces.insert(target.m_faces.end(), std::make_move_iterator(source.m_faces.begin()), std::make_move_iterator(source.m_faces.end()));
    target.m_shells.insert(target.m_shells.end(), std::make_move_iterator(source.m_shells.begin()), std::make_move_iterator(source.m_shells.end()));
    target.m_solids.insert(target.m_solids.end(), std::make_move_iterator(source.m_solids.begin()), std::make_move_iterator(source.m_solids.end()));
}

void append_mesh(Mesh& target, const Mesh& source) {

    std::map<size_t, size_t> vertices;

    for (const std::pair<const size_t, VertexData>& entry : source.vertex)
        vertices[entry.first] = target.add_vertex(entry.second.position());

    for (const std::pair<const size_t, std::vector<size_t>>& face : source.face) {
        std::vector<size_t> indices;

        for (size_t vertex : face.second)
            indices.push_back(vertices.at(vertex));

        const std::optional<size_t> key = target.add_face(indices);

        if (!key)
            throw std::runtime_error("Cannot append solid face");

        const auto holes = source.face_holes.find(face.first);

        if (holes != source.face_holes.end()) {
            std::vector<std::vector<size_t>> rings = holes->second;

            for (std::vector<size_t>& ring : rings)
                for (size_t& vertex : ring)
                    vertex = vertices.at(vertex);

            target.set_face_holes(*key, std::move(rings));
        }

        const auto cached = source.get_triangulation().find(face.first);

        if (cached != source.get_triangulation().end()) {
            std::vector<std::array<size_t, 3>> triangles = cached->second;

            for (std::array<size_t, 3>& triangle : triangles)
                for (size_t& vertex : triangle)
                    vertex = vertices.at(vertex);

            target.set_face_triangulation(*key, std::move(triangles));
        }
    }
}

static Polyline mesh_face_ring(const Mesh& mesh, const std::vector<size_t>& indices) {

    std::vector<Point> points;

    for (size_t vertex : indices)
        points.push_back(mesh.vertex.at(vertex).position());

    return Polyline(points).closed();
}

BRep mesh_brep(const Mesh& mesh) {

    std::vector<Polyline> faces;
    std::vector<std::vector<Polyline>> holes;

    for (const std::pair<const size_t, std::vector<size_t>>& face : mesh.face) {
        faces.push_back(mesh_face_ring(mesh, face.second));
        holes.emplace_back();
        const auto found = mesh.face_holes.find(face.first);

        if (found != mesh.face_holes.end())
            for (const std::vector<size_t>& ring : found->second)
                holes.back().push_back(mesh_face_ring(mesh, ring));
    }

    return BRep::from_polylines(faces, holes);
}

Mesh apply_solid_features(Mesh mesh, const std::vector<InteractionFeatureSolid>& cuts, bool drills) {

    std::vector<Mesh> pending;

    for (const InteractionFeatureSolid& cut : cuts) {
        if (cut.operation != SolidOperation::subtract && !pending.empty()) {
            mesh = solid_difference(mesh, pending);
            pending.clear();
        }

        if (drills)
            for (const Line& drill : cut.drills)
                pending.push_back(drill_mesh(drill, cut.drill_radius, cut.drill_tolerance));

        if (!cut.mesh.number_of_faces())
            continue;

        if (std::optional<Mesh> result = compute_profile_cut(mesh, cut))
            mesh = std::move(*result);
        else if (cut.operation == SolidOperation::subtract)
            pending.push_back(cut.mesh);
        else
            mesh = solid_boolean(
                mesh,
                cut.mesh,
                cut.operation,
                cut.tolerance
            );
    }

    if (!pending.empty())
        mesh = solid_difference(mesh, pending);

    return mesh;
}

BRep solid_features_brep(const Mesh& mesh, const std::vector<InteractionFeatureSolid>& cuts) {

    std::vector<Drill> drills;
    double chord_tolerance = 0.0;

    for (const InteractionFeatureSolid& cut : cuts)
        for (const Line& drill : cut.drills) {
            drills.push_back({drill, cut.drill_radius});
            chord_tolerance = cut.drill_tolerance;
        }

    if (!drills.empty()) {
        const Mesh milled = apply_solid_features(mesh, cuts, false);

        if (std::optional<BRep> exact = drilled_brep(milled, drills))
            return *exact;

        // a bore that grazes an edge or another bore is subtracted faceted first, so every other bore stays an exact cylinder
        const std::array<std::vector<Drill>, 2> split = split_clear_drills(milled, drills);

        if (!split[0].empty() && !split[1].empty()) {
            std::vector<Mesh> grazing;

            for (const Drill& drill : split[1])
                grazing.push_back(drill_mesh(drill.axis, drill.radius, chord_tolerance));

            if (std::optional<BRep> exact = drilled_brep(solid_difference(milled, grazing), split[0]))
                return *exact;
        }
    }

    const Mesh cut = apply_solid_features(mesh, cuts);

    if (std::optional<BRep> planar = drilled_brep(cut, {}))
        return *planar;

    return mesh_brep(cut);
}

}
