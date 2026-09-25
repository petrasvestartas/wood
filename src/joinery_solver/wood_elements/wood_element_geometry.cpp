#include "pch.h"
#include "wood_element_geometry.h"

namespace wood_session {

using namespace session_cpp;

bool is_geometry_feature(std::string_view feature_type) {
    return feature_type == "outline" || feature_type == "axis" || feature_type == "section";
}

ElementFeature polyline_feature(std::string_view feature_type, const Polyline& polyline, int face_index) {
    return ElementFeature(std::string(feature_type), face_index, {polyline}, std::string(feature_type));
}

bool is_session_feature(std::string_view feature_type) {
    return feature_type == "joint" || feature_type == "contact";
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

Polyline square_section(const Point& at, const Vector& direction, const Vector& up, double radius) {

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
static double compute_flux(const Mesh& mesh, size_t a, size_t b, size_t c) {

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
                total += compute_flux(mesh, triangle[0], triangle[1], triangle[2]);
            continue;
        }

        const std::vector<size_t> ring = *mesh.face_vertices(face);
        for (size_t i = 1; i + 1 < ring.size(); i++)
            total += compute_flux(mesh, ring[0], ring[i], ring[i + 1]);
    }

    return std::abs(total) / 6.0;
}

BRep brep_between_loops(const std::vector<Polyline>& bottom, const std::vector<Polyline>& top) {

    std::vector<Polyline> faces{bottom[0], top[0]};
    std::vector<std::vector<Polyline>> holes(2);

    for (size_t loop = 1; loop < bottom.size(); loop++) {
        holes[0].push_back(bottom[loop]);
        holes[1].push_back(top[loop]);
    }

    for (size_t loop = 0; loop < bottom.size(); loop++) {
        const Polyline& lower = bottom[loop];
        const Polyline& upper = top[loop];
        const size_t segment_count = lower.point_count() - 1;
        for (size_t segment = 0; segment < segment_count; segment++) {
            faces.push_back(Polyline({lower.get_point(segment), lower.get_point(segment + 1), upper.get_point(segment + 1), upper.get_point(segment), lower.get_point(segment)}));
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

/// A ring moved along `along` onto cut `index`, then clipped by every other cut: the face that cut leaves on the member.
static Polyline end_section(const Polyline& ring, const Vector& along, const std::vector<Plane>& cuts, size_t index) {

    const double speed = along.dot(cuts[index].z_axis());
    if (std::abs(speed) < Tolerance::ZERO_TOLERANCE)
        return Polyline();

    std::vector<Point> moved;
    for (const Point& point : ring.get_points())
        moved.push_back(point - along * (signed_distance(cuts[index], point) / speed));

    return kept_ring(Polyline(moved), cuts, index);
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
        rings.push_back(start_cut < 0 ? kept_ring(sections[0], cuts, cuts.size()) : end_section(sections[first], points[first + 1] - points[first], cuts, start_cut));

    for (size_t i = first + 1; i <= last; i++) {

        kept.push_back(points[i]);

        if (ringed)
            rings.push_back(kept_ring(sections[i], cuts, cuts.size()));
    }

    kept.push_back(point_at(points, last, end));

    if (ringed)
        rings.push_back(end_cut < 0 ? kept_ring(sections[n], cuts, cuts.size()) : end_section(sections[last + 1], points[last + 1] - points[last], cuts, end_cut));

    return {Polyline(kept), rings};
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

} // namespace wood_session
