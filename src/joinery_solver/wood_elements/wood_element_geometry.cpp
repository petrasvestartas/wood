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

std::vector<ElementFeature> joint_features(const Element& element) {

    std::vector<ElementFeature> kept;
    for (const ElementFeature& feature : element.features()) {

        if (feature.feature_type != "joint")
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

    const Vector side = along.cross(rise).normalized();
    rise = side.cross(along).normalized();

    const Vector s = side * radius;
    const Vector u = rise * radius;

    return Polyline({at - u - s, at - u + s, at + u + s, at + u - s, at - u - s});
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

} // namespace wood_session
