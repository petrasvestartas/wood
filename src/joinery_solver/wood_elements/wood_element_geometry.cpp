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

ElementGeometry cut_geometry(const ElementGeometry& geometry, const std::vector<Plane>& planes) {

    ElementGeometry cut = geometry;
    for (const Plane& plane : planes) {
        if (const Mesh* mesh = std::get_if<Mesh>(&cut))
            cut = mesh->cut_by_plane(plane);
        else if (const BRep* brep = std::get_if<BRep>(&cut))
            cut = brep->cut_by_plane(plane);
    }

    return cut;
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
