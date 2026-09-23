#pragma once
#include "wood_session.h"
#include "src/templates/plan.h"

namespace wood_grid {

// ═══════════════════════════════════════════════════════════════════════════
// Overlap
// ═══════════════════════════════════════════════════════════════════════════

/// Volume of other inside convex, the face half-spaces of convex pulled in by tolerance so faces that touch or lie within tolerance count for nothing: repeated Mesh::cut_by_plane, then Mesh::volume().
inline double compute_overlap(const session_cpp::Mesh& convex, const session_cpp::Mesh& other, double tolerance) {

    session_cpp::Mesh part = other;
    for (const session_cpp::Plane& plane : compute_planes(convex)) {
        part = part.cut_by_plane(session_cpp::Plane::from_point_normal(plane.origin() - plane.z_axis() * tolerance, -plane.z_axis()));
        if (part.number_of_faces() == 0)
            return 0.0;
    }

    return part.volume();
}

/// True when every vertex of a closed mesh lies inside or on every face plane within tolerance.
inline bool compute_convex(const session_cpp::Mesh& solid, double tolerance) {

    const std::vector<session_cpp::Plane> planes = compute_planes(solid);
    for (const size_t vertex : solid.vertices())
        for (const session_cpp::Plane& plane : planes)
            if ((*solid.vertex_point(vertex) - plane.origin()).dot(plane.z_axis()) > tolerance)
                return false;

    return true;
}

/// A solid as convex pieces: itself when convex, else one triangle prism per cap triangle of a loft between two planes, else itself.
inline std::vector<session_cpp::Mesh> compute_prisms(const session_cpp::Mesh& solid, double tolerance) {

    if (compute_convex(solid, tolerance))
        return {solid};

    const size_t half = solid.number_of_vertices() / 2;
    for (const size_t face : solid.faces()) {
        const std::vector<size_t> ring = *solid.face_vertices(face);
        if (!solid.get_triangulation().count(face) || std::any_of(ring.begin(), ring.end(), [&](size_t key) { return key >= half; }))
            continue;

        std::vector<session_cpp::Mesh> prisms;
        for (const std::array<size_t, 3>& triangle : solid.get_triangulation().at(face)) {
            std::vector<session_cpp::Point> bottom;
            std::vector<session_cpp::Point> top;
            for (const size_t key : triangle) {
                bottom.push_back(*solid.vertex_point(key));
                top.push_back(*solid.vertex_point(key + half));
            }
            prisms.push_back(session_cpp::Mesh::loft({to_polyline(bottom)}, {to_polyline(top)}, true));
        }

        return prisms;
    }

    return {solid};
}

/// The cut solid of a world element as a mesh.
inline session_cpp::Mesh compute_solid(const std::shared_ptr<session_cpp::Element>& element) {

    if (const std::shared_ptr<wood_session::Column> column = std::dynamic_pointer_cast<wood_session::Column>(element))
        return column->model_geometry_mesh();
    if (const std::shared_ptr<wood_session::Beam> beam = std::dynamic_pointer_cast<wood_session::Beam>(element))
        return beam->model_geometry_mesh();
    if (const std::shared_ptr<wood_session::Block> block = std::dynamic_pointer_cast<wood_session::Block>(element))
        return block->model_geometry_mesh();
    if (const std::shared_ptr<wood_session::Plate> plate = std::dynamic_pointer_cast<wood_session::Plate>(element))
        return plate->model_geometry_mesh();

    const session_cpp::Mesh* mesh = std::get_if<session_cpp::Mesh>(&element->geometry());

    return mesh ? *mesh : session_cpp::Mesh();
}

// ═══════════════════════════════════════════════════════════════════════════
// Clashes
// ═══════════════════════════════════════════════════════════════════════════

/// Every pair of world elements overlapping by more than tolerance as (guid, guid, volume), largest first, AABB pairs inflated by tolerance; a non-convex solid is the disjoint union of its cap-triangle prisms and its overlap the sum over them; empty means no clash.
inline std::vector<std::tuple<std::string, std::string, double>> compute_clashes(const wood_session::WoodSession& session, double tolerance = 1.0) {

    const std::vector<std::shared_ptr<session_cpp::Element>> elements = session.world_elements();
    std::vector<session_cpp::Mesh> solids;
    std::vector<std::vector<session_cpp::Mesh>> pieces;
    std::vector<session_cpp::AABB> boxes;
    for (const std::shared_ptr<session_cpp::Element>& element : elements) {
        solids.push_back(compute_solid(element));
        pieces.push_back(compute_prisms(solids.back(), tolerance));
        boxes.push_back(session_cpp::AABB::from_mesh(solids.back(), tolerance));
    }

    std::vector<std::tuple<std::string, std::string, double>> clashes;
    for (size_t i = 0; i < elements.size(); i++)
        for (size_t j = i + 1; j < elements.size(); j++) {
            if (solids[i].number_of_faces() == 0 || solids[j].number_of_faces() == 0 || !boxes[i].intersects(boxes[j]))
                continue;

            const bool first = pieces[i].size() == 1 && compute_convex(solids[i], tolerance);
            double volume = 0.0;
            for (const session_cpp::Mesh& prism : pieces[first ? i : j])
                volume += compute_overlap(prism, solids[first ? j : i], tolerance);
            if (volume > tolerance)
                clashes.emplace_back(elements[i]->guid(), elements[j]->guid(), volume);
        }

    std::sort(clashes.begin(), clashes.end(), [](const std::tuple<std::string, std::string, double>& a, const std::tuple<std::string, std::string, double>& b) { return std::get<2>(a) > std::get<2>(b); });

    return clashes;
}

} // namespace wood_grid
