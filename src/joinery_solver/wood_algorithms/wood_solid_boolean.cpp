#include "pch.h"
#include "wood_element_geometry.h"
#include "remesh_cdt.h"
#include <manifold/manifold.h>

namespace wood_session {

using namespace session_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Mesh to Manifold
// ═══════════════════════════════════════════════════════════════════════════

/// The points in the plane of their normal, x along the first side and y the normal cross x, so the face winds counter-clockwise.
static std::vector<std::pair<double, double>> to_plane(const std::vector<Point>& points, const Vector& normal) {

    const Vector x = (points[1] - points[0]).normalized();
    const Vector y = normal.normalized().cross(x);
    std::vector<std::pair<double, double>> plane;

    for (const Point& point : points)
        plane.push_back({(point - points[0]).dot(x), (point - points[0]).dot(y)});

    return plane;
}

/// The triangles of one face as vertex keys wound like the face: its cached triangulation, the face itself, or a constrained Delaunay triangulation of its points.
static std::vector<std::array<size_t, 3>> face_triangles(const Mesh& mesh, size_t face) {

    const std::vector<size_t> ring = mesh.face_vertices(face).value();

    if (ring.size() == 3)
        return {{ring[0], ring[1], ring[2]}};

    std::vector<Point> points;

    for (const size_t key : ring)
        points.push_back(mesh.vertex_point(key).value());

    const Vector normal = compute_newell(points);
    std::vector<std::array<size_t, 3>> triangles;

    if (mesh.get_triangulation().count(face))
        triangles = mesh.get_triangulation().at(face);
    else
        for (const std::array<int, 3>& t : cdt_triangulate(to_plane(points, normal), {}))
            triangles.push_back({ring[t[0]], ring[t[1]], ring[t[2]]});

    for (std::array<size_t, 3>& triangle : triangles) {
        const Point a = mesh.vertex_point(triangle[0]).value();
        const Vector side = (mesh.vertex_point(triangle[1]).value() - a).cross(mesh.vertex_point(triangle[2]).value() - a);

        if (side.dot(normal) < 0.0)
            std::swap(triangle[1], triangle[2]);
    }

    return triangles;
}

/// The root of a vertex in the union-find forest, compressing the path on the way.
static uint64_t find_root(std::vector<uint64_t>& parent, uint64_t vertex) {

    while (parent[vertex] != vertex) {
        parent[vertex] = parent[parent[vertex]];
        vertex = parent[vertex];
    }

    return vertex;
}

/// The box of the vertices of one shell.
struct Bounds {
    std::array<double, 3> low = {1e300, 1e300, 1e300}; // Smallest coordinate per axis.
    std::array<double, 3> high = {-1e300, -1e300, -1e300}; // Largest coordinate per axis.
};

/// True when inner lies within outer on every axis.
static bool contains(const Bounds& outer, const Bounds& inner) {

    for (size_t i = 0; i < 3; i++)
        if (inner.low[i] < outer.low[i] || inner.high[i] > outer.high[i])
            return false;

    return true;
}

/// Turns every closed shell of the triangles outwards on its own: a shell enclosing a negative volume is reversed unless it lies inside an outward shell, where it is a cavity; so a cutter joining an outward pocket with inward drills cuts with both.
static void orient_shells(manifold::MeshGL64& gl) {

    std::vector<uint64_t> parent(gl.NumVert());

    for (uint64_t v = 0; v < parent.size(); v++)
        parent[v] = v;

    for (size_t t = 0; t < gl.NumTri(); t++)
        for (size_t c = 1; c < 3; c++)
            parent[find_root(parent, gl.triVerts[3 * t + c])] = find_root(parent, gl.triVerts[3 * t]);

    std::map<uint64_t, double> volumes;
    std::map<uint64_t, Bounds> bounds;

    for (size_t t = 0; t < gl.NumTri(); t++) {
        const uint64_t root = find_root(parent, gl.triVerts[3 * t]);
        std::array<Vector, 3> corners;

        for (size_t c = 0; c < 3; c++) {
            const uint64_t v = gl.triVerts[3 * t + c];
            corners[c] = Vector(gl.vertProperties[3 * v], gl.vertProperties[3 * v + 1], gl.vertProperties[3 * v + 2]);

            for (size_t i = 0; i < 3; i++) {
                bounds[root].low[i] = std::min(bounds[root].low[i], corners[c][static_cast<int>(i)]);
                bounds[root].high[i] = std::max(bounds[root].high[i], corners[c][static_cast<int>(i)]);
            }
        }

        volumes[root] += corners[0].dot(corners[1].cross(corners[2]));
    }

    std::set<uint64_t> inverted;

    for (const std::pair<const uint64_t, double>& shell : volumes) {
        if (shell.second >= 0.0)
            continue;

        bool cavity = false;

        for (const std::pair<const uint64_t, double>& other : volumes)
            if (other.second > 0.0 && contains(bounds[other.first], bounds[shell.first]))
                cavity = true;

        if (!cavity)
            inverted.insert(shell.first);
    }

    for (size_t t = 0; t < gl.NumTri(); t++)
        if (inverted.count(find_root(parent, gl.triVerts[3 * t])))
            std::swap(gl.triVerts[3 * t + 1], gl.triVerts[3 * t + 2]);
}

/// The mesh as a Manifold, every triangle tagged with its face key plus first_id so the faces come back out of a boolean; throws unless the mesh is a closed manifold.
static manifold::Manifold to_manifold(const Mesh& mesh, uint64_t first_id) {

    manifold::MeshGL64 gl;
    std::unordered_map<size_t, uint64_t> index;

    for (const size_t key : mesh.vertices()) {
        const Point point = mesh.vertex_point(key).value();
        index[key] = gl.vertProperties.size() / 3;
        gl.vertProperties.insert(gl.vertProperties.end(), {point[0], point[1], point[2]});
    }

    for (const size_t face : mesh.faces())
        for (const std::array<size_t, 3>& triangle : face_triangles(mesh, face)) {
            gl.triVerts.insert(gl.triVerts.end(), {index.at(triangle[0]), index.at(triangle[1]), index.at(triangle[2])});
            gl.faceID.push_back(first_id + face);
        }

    orient_shells(gl);
    const manifold::Manifold solid(gl);

    if (solid.Status() != manifold::Manifold::Error::NoError)
        throw std::invalid_argument("Solid booleans require closed meshes: Manifold status " + std::to_string(static_cast<int>(solid.Status())));

    return solid;
}

/// One past the largest face key, the id offset the next mesh's faces start from.
static uint64_t face_id_span(const Mesh& mesh) {

    uint64_t span = 0;

    for (const size_t face : mesh.faces())
        span = std::max<uint64_t>(span, face + 1);

    return span;
}

// ═══════════════════════════════════════════════════════════════════════════
// Manifold to Mesh
// ═══════════════════════════════════════════════════════════════════════════

/// The triangles of one face id split into edge-connected regions.
static std::vector<std::vector<size_t>> compute_regions(const manifold::MeshGL64& gl, const std::vector<size_t>& triangles) {

    std::map<std::pair<uint64_t, uint64_t>, std::vector<size_t>> edges;

    for (size_t i = 0; i < triangles.size(); i++)
        for (size_t c = 0; c < 3; c++) {
            const uint64_t a = gl.triVerts[3 * triangles[i] + c];
            const uint64_t b = gl.triVerts[3 * triangles[i] + (c + 1) % 3];
            edges[{std::min(a, b), std::max(a, b)}].push_back(i);
        }

    std::vector<size_t> region(triangles.size(), triangles.size());
    std::vector<std::vector<size_t>> regions;

    for (size_t seed = 0; seed < triangles.size(); seed++) {
        if (region[seed] != triangles.size())
            continue;

        regions.push_back({});
        std::vector<size_t> stack = {seed};
        region[seed] = regions.size() - 1;

        while (!stack.empty()) {
            const size_t i = stack.back();
            stack.pop_back();
            regions.back().push_back(triangles[i]);

            for (size_t c = 0; c < 3; c++) {
                const uint64_t a = gl.triVerts[3 * triangles[i] + c];
                const uint64_t b = gl.triVerts[3 * triangles[i] + (c + 1) % 3];

                for (const size_t j : edges.at({std::min(a, b), std::max(a, b)}))
                    if (region[j] == triangles.size()) {
                        region[j] = regions.size() - 1;
                        stack.push_back(j);
                    }
            }
        }
    }

    return regions;
}

/// The one boundary loop of a region of triangles as vertex indices in winding order; empty when the region has a hole or touches itself.
static std::vector<uint64_t> compute_loop(const manifold::MeshGL64& gl, const std::vector<size_t>& triangles) {

    std::set<std::pair<uint64_t, uint64_t>> directed;

    for (const size_t t : triangles)
        for (size_t c = 0; c < 3; c++)
            directed.insert({gl.triVerts[3 * t + c], gl.triVerts[3 * t + (c + 1) % 3]});

    std::map<uint64_t, uint64_t> next;

    for (const std::pair<uint64_t, uint64_t>& edge : directed)
        if (!directed.count({edge.second, edge.first}) && !next.emplace(edge.first, edge.second).second)
            return {};

    std::vector<uint64_t> loop = {next.begin()->first};

    for (size_t step = 0; step < next.size(); step++) {
        const uint64_t following = next.at(loop.back());

        if (following == loop.front())
            break;

        loop.push_back(following);
    }

    if (loop.size() != next.size())
        return {};

    return loop;
}

/// Adds one region: a polygon face carrying its triangles when it has a single boundary loop, else a face per triangle.
static void add_region(Mesh& mesh, const manifold::MeshGL64& gl, const std::vector<size_t>& keys, const std::vector<size_t>& triangles) {

    const std::vector<uint64_t> loop = compute_loop(gl, triangles);

    if (loop.size() >= 3) {
        std::vector<size_t> face;

        for (const uint64_t vertex : loop)
            face.push_back(keys[vertex]);

        const std::optional<size_t> key = mesh.add_face(face);

        if (key && triangles.size() > 1) {
            std::vector<std::array<size_t, 3>> triangulation;

            for (const size_t t : triangles)
                triangulation.push_back({keys[gl.triVerts[3 * t]], keys[gl.triVerts[3 * t + 1]], keys[gl.triVerts[3 * t + 2]]});

            mesh.set_face_triangulation(*key, triangulation);
        }

        return;
    }

    for (const size_t t : triangles)
        mesh.add_face({keys[gl.triVerts[3 * t]], keys[gl.triVerts[3 * t + 1]], keys[gl.triVerts[3 * t + 2]]});
}

/// The Manifold as a mesh: the triangles that came from one input face merged back into one polygon per connected region; for a union the input faces that lie in one plane merge too, so a glued block's face and the face it continues become one.
static Mesh from_manifold(const manifold::Manifold& solid, bool merge_planes = false) {

    const manifold::MeshGL64 gl = solid.GetMeshGL64();
    Mesh mesh;
    std::vector<size_t> keys;

    for (size_t v = 0; v < gl.NumVert(); v++)
        keys.push_back(mesh.add_vertex(Point(gl.vertProperties[3 * v], gl.vertProperties[3 * v + 1], gl.vertProperties[3 * v + 2])));

    std::map<uint64_t, std::vector<size_t>> faces;

    for (size_t t = 0; t < gl.NumTri(); t++)
        faces[gl.faceID[t]].push_back(t);

    if (merge_planes) {
        // every input face's plane from its area-weighted normal; a face joins the first one before it in the same plane
        const auto vertex = [&gl](uint64_t v) {
            return Point(gl.vertProperties[3 * v], gl.vertProperties[3 * v + 1], gl.vertProperties[3 * v + 2]);
        };
        std::vector<std::pair<uint64_t, Plane>> planes;
        std::map<uint64_t, std::vector<size_t>> grouped;

        for (const std::pair<const uint64_t, std::vector<size_t>>& face : faces) {
            Vector sum(0.0, 0.0, 0.0);
            const Point origin = vertex(gl.triVerts[3 * face.second.front()]);

            for (size_t t : face.second)
                sum += (vertex(gl.triVerts[3 * t + 1]) - vertex(gl.triVerts[3 * t])).cross(vertex(gl.triVerts[3 * t + 2]) - vertex(gl.triVerts[3 * t]));

            const Plane plane = Plane::from_point_normal(origin, sum.normalized());
            uint64_t into = face.first;

            for (const std::pair<uint64_t, Plane>& other : planes)
                if (other.second.z_axis().dot(plane.z_axis()) > 1.0 - 1e-9 && std::abs(other.second.signed_distance(origin)) < 1e-6) {
                    into = other.first;
                    break;
                }

            if (into == face.first)
                planes.push_back({face.first, plane});

            std::vector<size_t>& triangles = grouped[into];
            triangles.insert(triangles.end(), face.second.begin(), face.second.end());
        }

        faces = std::move(grouped);
    }

    for (const std::pair<const uint64_t, std::vector<size_t>>& face : faces)
        for (const std::vector<size_t>& region : compute_regions(gl, face.second))
            add_region(mesh, gl, keys, region);

    return mesh;
}

/// The largest solid of a Manifold that a cut may have split, by volume: the other solids are subtracted, so cavities inside the kept one, which Decompose also lists apart, stay open.
static manifold::Manifold largest_piece(const manifold::Manifold& solid) {

    const std::vector<manifold::Manifold> pieces = solid.Decompose();

    if (pieces.size() < 2)
        return solid;

    size_t best = 0;

    for (size_t i = 1; i < pieces.size(); i++)
        if (pieces[i].Volume() > pieces[best].Volume())
            best = i;

    std::vector<manifold::Manifold> offcuts;

    for (size_t i = 0; i < pieces.size(); i++)
        if (i != best && pieces[i].Volume() > 0.0)
            offcuts.push_back(pieces[i]);

    if (offcuts.empty())
        return solid;

    return solid - manifold::Manifold::BatchBoolean(offcuts, manifold::OpType::Add);
}

// ═══════════════════════════════════════════════════════════════════════════
// Booleans
// ═══════════════════════════════════════════════════════════════════════════

/// The closed bodies of a cutter as separate Manifolds, so bodies that overlap, a pocket and the drills through it, act as their union.
static std::vector<manifold::Manifold> cutter_bodies(const Mesh& cutter, uint64_t first_id) {
    return to_manifold(cutter, first_id).Decompose();
}

Mesh solid_boolean(const Mesh& source, const Mesh& cutter, SolidOperation operation, double tolerance) {

    if (!std::isfinite(tolerance) || tolerance <= 0)
        throw std::invalid_argument("Solid tolerance must be positive");

    if (!source.number_of_faces())
        return operation == SolidOperation::unite ? cutter : Mesh();

    if (!cutter.number_of_faces())
        return operation == SolidOperation::intersection ? Mesh() : source;

    const manifold::Manifold a = to_manifold(source, 0);
    std::vector<manifold::Manifold> bodies = cutter_bodies(cutter, face_id_span(source));

    if (operation == SolidOperation::difference) {
        bodies.insert(bodies.begin(), a);
        return from_manifold(manifold::Manifold::BatchBoolean(bodies, manifold::OpType::Subtract));
    }

    const manifold::Manifold b = manifold::Manifold::BatchBoolean(bodies, manifold::OpType::Add);

    if (operation == SolidOperation::intersection)
        return from_manifold(a ^ b);

    return from_manifold(a + b, true);
}

Mesh solid_difference(const Mesh& source, const std::vector<Mesh>& cutters) {

    if (!source.number_of_faces() || cutters.empty())
        return source;

    std::vector<manifold::Manifold> solids = {to_manifold(source, 0)};
    uint64_t first_id = face_id_span(source);

    for (const Mesh& cutter : cutters) {
        if (!cutter.number_of_faces())
            continue;

        const std::vector<manifold::Manifold> bodies = cutter_bodies(cutter, first_id);
        solids.insert(solids.end(), bodies.begin(), bodies.end());
        first_id += face_id_span(cutter);
    }

    return from_manifold(largest_piece(manifold::Manifold::BatchBoolean(solids, manifold::OpType::Subtract)));
}

}
