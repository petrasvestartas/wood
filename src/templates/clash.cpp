#include "pch.h"
#include <numeric>
#include "src/templates/clash.h"
#include "wood_element_geometry.h"
#include "src/templates/grid_plan.h"

using namespace session_cpp;

namespace wood_grid::clash {

using namespace wood_grid::plan;

// ═══════════════════════════════════════════════════════════════════════════
// Pieces
// ═══════════════════════════════════════════════════════════════════════════

/// True when every vertex of a closed mesh lies inside or on every face plane within tolerance.
bool is_convex_solid(const Mesh& solid, double tolerance) {

    const std::vector<Plane> planes = compute_planes(solid);
    for (const size_t vertex : solid.vertices())
        for (const Plane& plane : planes)
            if ((*solid.vertex_point(vertex) - plane.origin()).dot(plane.z_axis()) > tolerance)
                return false;

    return true;
}

/// True when a face ring of a loft uses only the vertices of its first cap.
bool is_cap(const std::vector<size_t>& ring, size_t half) {

    for (const size_t key : ring)
        if (key >= half)
            return false;

    return true;
}

/// Ear clipping of a planar ring of vertex keys in its own plane: the triangles of a simple polygon; empty when no ear is found.
std::vector<std::array<size_t, 3>> compute_ears(const Mesh& solid, const std::vector<size_t>& ring) {

    std::vector<Point> points;
    for (const size_t key : ring)
        points.push_back(*solid.vertex_point(key));
    const Vector normal = wood_session::compute_newell(points);
    const Vector u = (std::abs(normal[0]) < 0.9 ? Vector(1.0, 0.0, 0.0) : Vector(0.0, 1.0, 0.0)).cross(normal).normalized();
    const Vector v = normal.cross(u);

    std::vector<Point> flat;
    for (const Point& point : points)
        flat.emplace_back((point - points[0]).dot(u), (point - points[0]).dot(v), 0.0);

    std::vector<size_t> left(ring.size());
    std::iota(left.begin(), left.end(), 0);
    std::vector<std::array<size_t, 3>> triangles;
    const size_t rounds = ring.size() * ring.size();
    for (size_t round = 0; round < rounds && left.size() > 2; round++) {
        const size_t i = round % left.size();
        const size_t a = left[(i + left.size() - 1) % left.size()];
        const size_t b = left[i];
        const size_t c = left[(i + 1) % left.size()];
        const double turn = (flat[b] - flat[a]).cross(flat[c] - flat[b])[2];
        if (turn < -1e-9)
            continue;

        bool empty = true;
        const Polyline ear = to_polyline({flat[a], flat[b], flat[c]});
        for (const size_t k : left)
            if (k != a && k != b && k != c && turn > 1e-9 && ear.point_in_polygon_2d(flat[k]))
                empty = false;
        if (!empty)
            continue;

        if (turn > 1e-9)
            triangles.push_back({ring[a], ring[b], ring[c]});
        left.erase(left.begin() + i);
    }

    return left.size() > 2 ? std::vector<std::array<size_t, 3>>() : triangles;
}

/// The triangles of a cap face: its cached triangulation, else an ear clipping of its ring; empty for a holed cap without a cache.
std::vector<std::array<size_t, 3>> compute_triangles(const Mesh& solid, size_t face) {

    if (solid.get_triangulation().count(face))
        return solid.get_triangulation().at(face);
    if (solid.get_face_holes().count(face))
        return {};

    return compute_ears(solid, *solid.face_vertices(face));
}

/// Convex pieces of a solid: itself when convex, else one triangle prism per cap triangle of a loft between two matching caps; empty when it is neither.
std::vector<Mesh> compute_prisms(const Mesh& solid, double tolerance) {

    if (solid.number_of_faces() == 0 || !solid.is_closed())
        return {};
    if (is_convex_solid(solid, tolerance))
        return {solid};

    const size_t half = solid.number_of_vertices() / 2;
    for (const size_t face : solid.faces()) {
        if (!is_cap(*solid.face_vertices(face), half))
            continue;

        std::vector<Mesh> prisms;
        for (const std::array<size_t, 3>& triangle : compute_triangles(solid, face)) {
            std::vector<Point> bottom;
            std::vector<Point> top;
            for (const size_t key : triangle) {
                bottom.push_back(*solid.vertex_point(key));
                top.push_back(*solid.vertex_point(key + half));
            }
            prisms.push_back(Mesh::loft({to_polyline(bottom)}, {to_polyline(top)}, true));
        }

        return prisms;
    }

    return {};
}

/// The uncut loft of a wood element and the planes it is cut by: Column, Beam and Block carry cuts, a Plate lofts its features already notched, anything else is its model solid.
std::pair<Mesh, std::vector<Plane>> compute_raw(const std::shared_ptr<Element>& element) {

    if (const std::shared_ptr<wood_session::Column> column = std::dynamic_pointer_cast<wood_session::Column>(element))
        return {column->element_geometry_mesh(), column->cuts};
    if (const std::shared_ptr<wood_session::Beam> beam = std::dynamic_pointer_cast<wood_session::Beam>(element))
        return {beam->element_geometry_mesh(), beam->cuts};
    if (const std::shared_ptr<wood_session::Block> block = std::dynamic_pointer_cast<wood_session::Block>(element))
        return {block->element_geometry_mesh(), block->cuts};

    return {element->model_geometry_mesh(), {}};
}

/// Convex pieces of a world element's solid: the prisms of its uncut loft, each cut by its planes; throws when a solid is open or cannot be decomposed, so no clash goes unjudged.
std::vector<Mesh> compute_pieces(const std::shared_ptr<Element>& element, double tolerance) {

    const std::pair<Mesh, std::vector<Plane>> raw = compute_raw(element);
    if (raw.first.number_of_faces() == 0)
        return {};

    const std::vector<Mesh> prisms = compute_prisms(raw.first, tolerance);
    if (prisms.empty())
        throw std::runtime_error(fmt::format("compute_clashes: {} {} is not a closed convex or lofted solid", element->name, element->guid()));

    std::vector<Mesh> pieces;
    for (const Mesh& prism : prisms) {
        Mesh piece = prism;
        for (const Plane& plane : raw.second)
            if (piece.number_of_faces() > 0)
                piece = piece.cut_by_plane(plane);
        if (piece.number_of_faces() > 0)
            pieces.push_back(piece);
    }

    return pieces;
}

/// True when clash a has the larger volume.
bool is_worse(const std::tuple<std::string, std::string, double>& a, const std::tuple<std::string, std::string, double>& b) {
    return std::get<2>(a) > std::get<2>(b);
}

} // namespace wood_grid::clash

namespace wood_grid {

// ═══════════════════════════════════════════════════════════════════════════
// Clashes
// ═══════════════════════════════════════════════════════════════════════════

double compute_overlap(const Mesh& convex, const Mesh& other, double tolerance) {

    Mesh part = other;
    for (const Plane& plane : plan::compute_planes(convex)) {
        part = part.cut_by_plane(Plane::from_point_normal(plane.origin() - plane.z_axis() * tolerance, -plane.z_axis()));
        if (part.number_of_faces() == 0)
            return 0.0;
    }

    return part.volume();
}

std::vector<std::tuple<std::string, std::string, double>> compute_clashes(const wood_session::WoodSession& session, double tolerance) {

    const std::vector<std::shared_ptr<Element>> elements = session.world_elements();
    std::vector<std::vector<Mesh>> pieces;
    std::vector<AABB> boxes;
    for (const std::shared_ptr<Element>& element : elements) {
        pieces.push_back(clash::compute_pieces(element, tolerance));
        boxes.push_back(AABB::from_mesh(element->model_geometry_mesh(), tolerance));
    }

    std::vector<std::tuple<std::string, std::string, double>> clashes;
    for (size_t i = 0; i < elements.size(); i++)
        for (size_t j = i + 1; j < elements.size(); j++) {
            if (pieces[i].empty() || pieces[j].empty() || !boxes[i].intersects(boxes[j]))
                continue;

            double volume = 0.0;
            for (const Mesh& a : pieces[i])
                for (const Mesh& b : pieces[j])
                    if (AABB::from_mesh(a, tolerance).intersects(AABB::from_mesh(b, tolerance)))
                        volume += compute_overlap(a, b, tolerance);
            if (volume > tolerance)
                clashes.emplace_back(elements[i]->guid(), elements[j]->guid(), volume);
        }

    std::sort(clashes.begin(), clashes.end(), clash::is_worse);

    return clashes;
}

} // namespace wood_grid
