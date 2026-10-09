#include "pch.h"
#include "wood_element_geometry.h"
#include "../src/boolean_polyline.h"

namespace wood_session {

using namespace session_cpp;

namespace {

bool inside_xy(const Point& point, const Polyline& polygon) {

    bool inside = false;
    const std::vector<Point> points = polygon.get_points();

    for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
        const Point& first = points[i];
        const Point& second = points[j];

        if ((first[1] > point[1]) != (second[1] > point[1]) && point[0] < (second[0] - first[0]) * (point[1] - first[1]) / (second[1] - first[1]) + first[0])
            inside = !inside;
    }

    return inside;
}

Polyline to_xy(const Polyline& ring, const Xform& local) {

    std::vector<Point> points;

    for (const Point& vertex : ring.get_points()) {
        const Point point = vertex.transformed(local);
        points.emplace_back(point[0], point[1], 0);
    }

    return Polyline(points).closed();
}

Polyline from_xy(const Polyline& ring, const Plane& plane, double height) {

    std::vector<Point> points;

    for (const Point& point : ring.get_points())
        points.push_back(plane.origin() + plane.x_axis() * point[0] + plane.y_axis() * point[1] + plane.z_axis() * height);

    return Polyline(points);
}

std::array<double, 2> compute_heights(const Mesh& mesh, const Plane& plane) {

    std::array<double, 2> heights = {std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};

    for (const std::pair<const size_t, VertexData>& entry : mesh.vertex) {
        const double height = (entry.second.position() - plane.origin()).dot(plane.z_axis());
        heights[0] = std::min(heights[0], height);
        heights[1] = std::max(heights[1], height);
    }

    return heights;
}

bool is_extrusion(
    const Mesh& mesh,
    const Xform& local,
    const std::array<double, 2>& heights,
    double tolerance
) {

    std::vector<Point> bottom;
    std::vector<Point> top;

    for (const std::pair<const size_t, VertexData>& entry : mesh.vertex) {
        const Point point = entry.second.position().transformed(local);

        if (std::abs(point[2] - heights[0]) <= tolerance)
            bottom.emplace_back(point[0], point[1], 0);
        else if (std::abs(point[2] - heights[1]) <= tolerance)
            top.emplace_back(point[0], point[1], 0);
        else
            return false;
    }

    if (bottom.size() != top.size())
        return false;

    for (const Point& point : bottom) {
        bool found = false;

        for (const Point& candidate : top)
            if (Point::distance(point, candidate) <= tolerance) {
                found = true;
                break;
            }

        if (!found)
            return false;
    }

    return true;
}

Polyline mesh_ring(const Mesh& mesh, const std::vector<size_t>& vertices) {

    std::vector<Point> points;

    for (size_t vertex : vertices)
        points.push_back(mesh.vertex.at(vertex).position());

    return Polyline(points);
}

std::vector<Polyline> bottom_rings(
    const Mesh& mesh,
    const Xform& local,
    double height,
    double tolerance
) {

    std::vector<Polyline> rings;

    for (const std::pair<const size_t, std::vector<size_t>>& face : mesh.face) {
        bool bottom = true;

        for (size_t vertex : face.second) {
            const Point position = mesh.vertex.at(vertex).position().transformed(local);

            if (std::abs(position[2] - height) > tolerance) {
                bottom = false;
                break;
            }
            }

        if (!bottom)
            continue;

        rings.push_back(to_xy(mesh_ring(mesh, face.second), local));
        const auto holes = mesh.face_holes.find(face.first);

        if (holes != mesh.face_holes.end())
            for (const std::vector<size_t>& ring : holes->second)
                rings.push_back(to_xy(mesh_ring(mesh, ring), local));
    }

    return rings;
}

Mesh loft_regions(const std::vector<Polyline>& rings, const Plane& plane, const std::array<double, 2>& heights) {

    std::vector<int> depth(rings.size(), 0);
    std::vector<int> parent(rings.size(), -1);

    for (size_t i = 0; i < rings.size(); ++i)
        for (size_t j = 0; j < rings.size(); ++j)
            if (i != j && inside_xy(rings[i][0], rings[j]))
                ++depth[i];

    for (size_t i = 0; i < rings.size(); ++i)
        for (size_t j = 0; j < rings.size(); ++j)
            if (depth[j] == depth[i] - 1 && inside_xy(rings[i][0], rings[j]))
                parent[i] = static_cast<int>(j);

    Mesh mesh;

    for (size_t i = 0; i < rings.size(); ++i) {
        if (depth[i] % 2)
            continue;

        std::vector<Polyline> bottom{from_xy(rings[i], plane, heights[0])};
        std::vector<Polyline> top{from_xy(rings[i], plane, heights[1])};

        for (size_t j = 0; j < rings.size(); ++j)
            if (parent[j] == static_cast<int>(i)) {
                bottom.push_back(from_xy(rings[j], plane, heights[0]));
                top.push_back(from_xy(rings[j], plane, heights[1]));
            }

        append_mesh(mesh, Mesh::loft(bottom, top));
    }

    return mesh;
}

}

std::optional<Mesh> compute_profile_cut(const Mesh& mesh, const InteractionFeatureSolid& cut) {

    if (cut.profile.empty() || cut.extrusion.magnitude_squared() == 0 || mesh.vertex.empty())
        return std::nullopt;

    const Vector normal = cut.extrusion.normalized();

    for (const Polyline& ring : cut.profile)
        for (const Point& point : ring.get_points())
            if (std::abs((point - cut.profile[0][0]).dot(normal)) > cut.tolerance)
                return std::nullopt;

    const Plane plane = Plane::from_point_normal(cut.profile[0][0], normal);
    const std::optional<Xform> local = Xform::frame_to_world(
        plane.origin(),
        plane.x_axis(),
        plane.y_axis(),
        normal
    ).inverse();

    if (!local)
        return std::nullopt;

    const std::array<double, 2> heights = compute_heights(mesh, plane);
    const double length = cut.extrusion.magnitude();

    if (heights[1] - heights[0] <= cut.tolerance || heights[0] < -cut.tolerance || heights[1] > length + cut.tolerance)
        return std::nullopt;

    if (cut.operation == SolidOperation::add && (std::abs(heights[0]) > cut.tolerance || std::abs(heights[1] - length) > cut.tolerance))
        return std::nullopt;

    if (!is_extrusion(
        mesh,
        *local,
        heights,
        cut.tolerance
    ))
        return std::nullopt;

    const std::vector<Polyline> subject = bottom_rings(
        mesh,
        *local,
        heights[0],
        cut.tolerance
    );

    if (subject.empty())
        return std::nullopt;

    std::vector<Polyline> clips;

    for (const Polyline& ring : cut.profile)
        clips.push_back(to_xy(ring, *local));

    return loft_regions(BooleanPolyline::compute_regions(subject, clips, static_cast<int>(cut.operation)), plane, heights);
}

}
