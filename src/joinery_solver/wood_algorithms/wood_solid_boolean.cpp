#include "pch.h"
#include "wood_solid_body.h"
#include "wood_element_geometry.h"
#include "remesh_cdt.h"
#include <manifold/manifold.h>

namespace wood_session {

using namespace laika_cpp;

// ═══════════════════════════════════════════════════════════════════════════
// Mesh to Manifold
// ═══════════════════════════════════════════════════════════════════════════

/// The points in the plane of their normal, x along the first side and y the normal cross x, so the face winds counter-clockwise.
static std::vector<std::pair<double, double>>
to_plane(const std::vector<Point>& points, const Point& origin, const Vector& x, const Vector& y) {

    std::vector<std::pair<double, double>> plane;
    plane.reserve(points.size());

    for (const Point& point : points)
        plane.push_back({(point - origin).dot(x), (point - origin).dot(y)});

    return plane;
}

/// Twice the signed 2D area of a polygon ring.
static double signed_area_2d(const std::vector<std::pair<double, double>>& ring) {

    double area = 0.0;

    for (size_t i = 0; i < ring.size(); i++) {
        const std::pair<double, double>& a = ring[i];
        const std::pair<double, double>& b = ring[(i + 1) % ring.size()];
        area += a.first * b.second - b.first * a.second;
    }

    return area;
}

/// The area-weighted normal of a set of triangles.
static Vector
triangles_normal(const Mesh& mesh, const std::vector<std::array<size_t, 3>>& triangles) {

    Vector sum(0.0, 0.0, 0.0);

    for (const std::array<size_t, 3>& triangle : triangles) {
        const Point a = mesh.vertex_point(triangle[0]).value();
        const Point b = mesh.vertex_point(triangle[1]).value();
        const Point c = mesh.vertex_point(triangle[2]).value();
        sum += (b - a).cross(c - a);
    }

    return sum;
}

/// The triangles of one face as vertex keys wound like the face.
static std::vector<std::array<size_t, 3>> face_triangles(const Mesh& mesh, size_t face) {

    const std::map<size_t, std::vector<std::array<size_t, 3>>>& cached = mesh.get_triangulation();

    if (const auto it = cached.find(face); it != cached.end())
        return it->second;

    const std::vector<size_t> ring = mesh.face_vertices(face).value();
    const auto holes_it = mesh.get_face_holes().find(face);
    const bool has_holes = holes_it != mesh.get_face_holes().end() && !holes_it->second.empty();

    if (ring.size() < 3)
        return {};

    if (ring.size() == 3 && !has_holes)
        return {{ring[0], ring[1], ring[2]}};

    std::vector<Point> points;
    points.reserve(ring.size());

    for (const size_t key : ring)
        points.push_back(mesh.vertex_point(key).value());

    const Vector normal = compute_newell(points);
    const Vector x = (points[1] - points[0]).normalized();
    const Vector y = normal.normalized().cross(x);
    std::vector<std::pair<double, double>> border_2d = to_plane(points, points[0], x, y);
    std::vector<size_t> flat_keys = ring;

    if (signed_area_2d(border_2d) < 0.0) {
        std::reverse(flat_keys.begin(), flat_keys.end());
        std::reverse(border_2d.begin(), border_2d.end());
    }

    std::vector<std::vector<std::pair<double, double>>> holes_2d;
    if (has_holes) {
        holes_2d.reserve(holes_it->second.size());
        for (std::vector<size_t> hole_keys : holes_it->second) {
            if (hole_keys.size() < 3)
                continue;
            std::vector<Point> hole_pts;
            hole_pts.reserve(hole_keys.size());
            for (const size_t key : hole_keys)
                hole_pts.push_back(mesh.vertex_point(key).value());
            std::vector<std::pair<double, double>> hole_2d = to_plane(hole_pts, points[0], x, y);
            if (signed_area_2d(hole_2d) > 0.0) {
                std::reverse(hole_keys.begin(), hole_keys.end());
                std::reverse(hole_2d.begin(), hole_2d.end());
            }
            flat_keys.insert(flat_keys.end(), hole_keys.begin(), hole_keys.end());
            holes_2d.push_back(std::move(hole_2d));
        }
    }

    std::vector<std::array<size_t, 3>> triangles;

    for (const std::array<int, 3>& t : cdt_triangulate(border_2d, holes_2d))
        triangles.push_back({flat_keys[t[0]], flat_keys[t[1]], flat_keys[t[2]]});

    if (triangles_normal(mesh, triangles).dot(normal) < 0.0)
        for (std::array<size_t, 3>& triangle : triangles)
            std::swap(triangle[1], triangle[2]);

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
    std::array<double, 3> low = {1e300, 1e300, 1e300};
    std::array<double, 3> high = {-1e300, -1e300, -1e300};
};

/// True when inner lies within outer on every axis.
static bool contains(const Bounds& outer, const Bounds& inner) {

    for (size_t i = 0; i < 3; i++)
        if (inner.low[i] < outer.low[i] || inner.high[i] > outer.high[i])
            return false;

    return true;
}

/// Turns every closed shell of the triangles outwards on its own.
static void orient_shells(manifold::MeshGL64& gl) {

    std::vector<uint64_t> parent(gl.NumVert());

    for (uint64_t v = 0; v < parent.size(); v++)
        parent[v] = v;

    for (size_t t = 0; t < gl.NumTri(); t++)
        for (size_t c = 1; c < 3; c++)
            parent[find_root(parent, gl.triVerts[3 * t + c])] =
                find_root(parent, gl.triVerts[3 * t]);

    std::map<uint64_t, double> volumes;
    std::map<uint64_t, Bounds> bounds;

    for (size_t t = 0; t < gl.NumTri(); t++) {
        const uint64_t root = find_root(parent, gl.triVerts[3 * t]);
        std::array<Vector, 3> corners;

        for (size_t c = 0; c < 3; c++) {
            const uint64_t v = gl.triVerts[3 * t + c];
            corners[c] = Vector(
                gl.vertProperties[3 * v], gl.vertProperties[3 * v + 1], gl.vertProperties[3 * v + 2]
            );

            for (size_t i = 0; i < 3; i++) {
                bounds[root].low[i] =
                    std::min(bounds[root].low[i], corners[c][static_cast<int>(i)]);
                bounds[root].high[i] =
                    std::max(bounds[root].high[i], corners[c][static_cast<int>(i)]);
            }
        }

        const Vector ref(
            gl.vertProperties[3 * root],
            gl.vertProperties[3 * root + 1],
            gl.vertProperties[3 * root + 2]
        );
        volumes[root] += (corners[0] - ref).dot((corners[1] - ref).cross(corners[2] - ref));
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

/// The mesh as a Manifold.
static manifold::Manifold build_manifold(const Mesh& mesh, uint64_t first_id) {

    manifold::MeshGL64 gl;
    std::unordered_map<size_t, uint64_t> index;
    const size_t n_verts = mesh.number_of_vertices();
    gl.vertProperties.reserve(n_verts * 3);
    index.reserve(n_verts);

    for (const std::pair<const size_t, VertexData>& entry : mesh.vertex) {
        index[entry.first] = gl.vertProperties.size() / 3;
        gl.vertProperties.insert(
            gl.vertProperties.end(), {entry.second.x, entry.second.y, entry.second.z}
        );
    }

    for (const size_t face : mesh.faces())
        for (const std::array<size_t, 3>& triangle : face_triangles(mesh, face)) {
            gl.triVerts.insert(
                gl.triVerts.end(),
                {index.at(triangle[0]), index.at(triangle[1]), index.at(triangle[2])}
            );
            gl.faceID.push_back(first_id + face);
        }

    orient_shells(gl);

    return manifold::Manifold(gl);
}

/// The mesh as a Manifold; throws unless the mesh is a closed manifold.
static manifold::Manifold to_manifold(const Mesh& mesh, uint64_t first_id) {

    const manifold::Manifold solid = build_manifold(mesh, first_id);

    if (solid.Status() != manifold::Manifold::Error::NoError)
        throw std::invalid_argument(
            "Solid booleans require closed meshes: Manifold status " +
            std::to_string(static_cast<int>(solid.Status()))
        );

    return solid;
}

bool manifold_solid(const Mesh& mesh) {
    return mesh.number_of_faces() > 0 &&
           build_manifold(mesh, 0).Status() == manifold::Manifold::Error::NoError;
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
static std::vector<std::vector<size_t>>
compute_regions(const manifold::MeshGL64& gl, const std::vector<size_t>& triangles) {

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

/// The boundary loops of a region of triangles as vertex indices in winding order: the outline and its holes, empty when a
/// boundary vertex starts two boundary edges.
static std::vector<std::vector<uint64_t>>
compute_loops(const manifold::MeshGL64& gl, const std::vector<size_t>& triangles) {

    std::set<std::pair<uint64_t, uint64_t>> directed;

    for (const size_t t : triangles)
        for (size_t c = 0; c < 3; c++)
            directed.insert({gl.triVerts[3 * t + c], gl.triVerts[3 * t + (c + 1) % 3]});

    std::map<uint64_t, uint64_t> next;

    for (const std::pair<uint64_t, uint64_t>& edge : directed)
        if (!directed.count({edge.second, edge.first}) &&
            !next.emplace(edge.first, edge.second).second)
            return {};

    std::vector<std::vector<uint64_t>> loops;

    while (!next.empty()) {
        std::vector<uint64_t> loop = {next.begin()->first};

        while (true) {
            const std::map<uint64_t, uint64_t>::iterator found = next.find(loop.back());
            if (found == next.end())
                return {};
            const uint64_t following = found->second;
            next.erase(found);
            if (following == loop.front())
                break;
            loop.push_back(following);
        }

        loops.push_back(loop);
    }

    return loops;
}

/// Twice the area a loop of Manifold vertices encloses, by Newell: the length of the loop's normal.
static double loop_area(const manifold::MeshGL64& gl, const std::vector<uint64_t>& loop) {

    if (loop.empty())
        return 0.0;
    const Point origin(
        gl.vertProperties[3 * loop[0]],
        gl.vertProperties[3 * loop[0] + 1],
        gl.vertProperties[3 * loop[0] + 2]
    );
    Vector normal(0.0, 0.0, 0.0);

    for (size_t i = 0; i < loop.size(); i++) {
        const uint64_t a = loop[i];
        const uint64_t b = loop[(i + 1) % loop.size()];
        const Point p(
            gl.vertProperties[3 * a], gl.vertProperties[3 * a + 1], gl.vertProperties[3 * a + 2]
        );
        const Point q(
            gl.vertProperties[3 * b], gl.vertProperties[3 * b + 1], gl.vertProperties[3 * b + 2]
        );
        normal += (p - origin).cross(q - origin);
    }

    return normal.magnitude();
}

/// Twice the area of a set of Manifold triangles: the sum of their cross products' lengths.
static double triangles_area(const manifold::MeshGL64& gl, const std::vector<size_t>& triangles) {

    double area = 0.0;

    for (const size_t t : triangles) {
        std::array<Point, 3> corner;

        for (size_t c = 0; c < 3; c++) {
            const uint64_t v = gl.triVerts[3 * t + c];
            corner[c] = Point(
                gl.vertProperties[3 * v], gl.vertProperties[3 * v + 1], gl.vertProperties[3 * v + 2]
            );
        }

        area += (corner[1] - corner[0]).cross(corner[2] - corner[0]).magnitude();
    }

    return area;
}

/// Adds one region: one face bounded by its outline, with the region's other loops as its holes, when the loops enclose
/// exactly the triangles' area and no vertex lies inside; else a face per triangle.
static void add_region(
    Mesh& mesh,
    const manifold::MeshGL64& gl,
    const std::vector<size_t>& keys,
    const std::vector<size_t>& triangles
) {

    const std::vector<std::vector<uint64_t>> loops = compute_loops(gl, triangles);
    const double area = triangles_area(gl, triangles);

    // the outline is the loop enclosing the most; every other loop is a hole in it
    size_t outline = 0;
    size_t boundary = 0;
    double holes_area = 0.0;
    for (size_t i = 0; i < loops.size(); i++) {
        boundary += loops[i].size();
        if (loop_area(gl, loops[i]) > loop_area(gl, loops[outline]))
            outline = i;
    }
    for (size_t i = 0; i < loops.size(); i++)
        if (i != outline)
            holes_area += loop_area(gl, loops[i]);

    if (!loops.empty() && loops[outline].size() >= 3 && boundary + 2 * (loops.size() - 1) == triangles.size() + 2 &&
        std::abs(loop_area(gl, loops[outline]) - holes_area - area) <= 1e-6 * area) {
        std::vector<size_t> face;

        for (const uint64_t vertex : loops[outline])
            face.push_back(keys[vertex]);

        const std::optional<size_t> key = mesh.add_face(face);

        if (key) {
            std::vector<std::vector<size_t>> rings;
            for (size_t i = 0; i < loops.size(); i++) {
                if (i == outline)
                    continue;
                rings.emplace_back();
                for (const uint64_t vertex : loops[i])
                    rings.back().push_back(keys[vertex]);
            }
            if (!rings.empty())
                mesh.set_face_holes(*key, rings);

            if (triangles.size() > 1) {
                std::vector<std::array<size_t, 3>> triangulation;

                for (const size_t t : triangles)
                    triangulation.push_back(
                        {keys[gl.triVerts[3 * t]],
                         keys[gl.triVerts[3 * t + 1]],
                         keys[gl.triVerts[3 * t + 2]]}
                    );

                mesh.set_face_triangulation(*key, triangulation);
            }

            return;
        }
    }

    for (const size_t t : triangles)
        mesh.add_face(
            {keys[gl.triVerts[3 * t]], keys[gl.triVerts[3 * t + 1]], keys[gl.triVerts[3 * t + 2]]}
        );
}

namespace {

/// Vertex v of a Manifold mesh as a Point.
Point manifold_vertex(const manifold::MeshGL64& gl, uint64_t v) {
    return Point(
        gl.vertProperties[3 * v], gl.vertProperties[3 * v + 1], gl.vertProperties[3 * v + 2]
    );
}

}

/// The Manifold as a mesh.
static Mesh from_manifold(const manifold::Manifold& solid, bool merge_planes = false) {

    const manifold::MeshGL64 gl = solid.GetMeshGL64();
    Mesh mesh;
    std::vector<size_t> keys;

    for (size_t v = 0; v < gl.NumVert(); v++)
        keys.push_back(mesh.add_vertex(manifold_vertex(gl, v)));

    std::map<uint64_t, std::vector<size_t>> faces;

    // a triangle thinner than 1e-4 mm, left where two cutters share a plane, stays a face of its own: in its face's group it
    // would join pieces that only touch along a line
    std::vector<size_t> slivers;
    for (size_t t = 0; t < gl.NumTri(); t++) {
        const Point a = manifold_vertex(gl, gl.triVerts[3 * t]);
        const Point b = manifold_vertex(gl, gl.triVerts[3 * t + 1]);
        const Point c = manifold_vertex(gl, gl.triVerts[3 * t + 2]);
        const double longest = std::max({Point::distance(a, b), Point::distance(b, c), Point::distance(c, a)});
        if ((b - a).cross(c - a).magnitude() < 1e-4 * longest)
            slivers.push_back(t);
        else
            faces[gl.faceID[t]].push_back(t);
    }

    if (merge_planes) {
        std::vector<std::pair<uint64_t, Plane>> planes;
        std::map<uint64_t, std::vector<size_t>> grouped;

        for (const std::pair<const uint64_t, std::vector<size_t>>& face : faces) {
            Vector sum(0.0, 0.0, 0.0);
            const Point origin = manifold_vertex(gl, gl.triVerts[3 * face.second.front()]);

            for (size_t t : face.second) {
                const Point a = manifold_vertex(gl, gl.triVerts[3 * t]);
                const Point b = manifold_vertex(gl, gl.triVerts[3 * t + 1]);
                const Point c = manifold_vertex(gl, gl.triVerts[3 * t + 2]);
                sum += (b - a).cross(c - a);
            }

            const Plane plane = Plane::from_point_normal(origin, sum.normalized());
            uint64_t into = face.first;

            for (const std::pair<uint64_t, Plane>& other : planes)
                if (other.second.z_axis().dot(plane.z_axis()) > 1.0 - 1e-9 &&
                    std::abs(other.second.signed_distance(origin)) < 1e-6) {
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
    for (const size_t sliver : slivers)
        add_region(mesh, gl, keys, {sliver});

    return mesh;
}

/// The largest solid of a Manifold that a cut may have split, by volume.
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

/// The axis-aligned box of a mesh's vertices.
static Bounds mesh_bounds(const Mesh& mesh) {

    Bounds box;

    for (const std::pair<const size_t, VertexData>& entry : mesh.vertex) {
        const VertexData& v = entry.second;
        box.low[0] = std::min(box.low[0], v.x);
        box.low[1] = std::min(box.low[1], v.y);
        box.low[2] = std::min(box.low[2], v.z);
        box.high[0] = std::max(box.high[0], v.x);
        box.high[1] = std::max(box.high[1], v.y);
        box.high[2] = std::max(box.high[2], v.z);
    }

    return box;
}

/// True when two boxes overlap within pad on every axis.
static bool overlaps(const Bounds& a, const Bounds& b, double pad = 1e-6) {

    if (a.low[0] > a.high[0] || b.low[0] > b.high[0])
        return false;

    for (size_t i = 0; i < 3; i++)
        if (a.high[i] + pad < b.low[i] || b.high[i] + pad < a.low[i])
            return false;

    return true;
}

/// The closed bodies of a cutter as separate Manifolds, so bodies that overlap.
static std::vector<manifold::Manifold> cutter_bodies(const Mesh& cutter, uint64_t first_id) {
    return to_manifold(cutter, first_id).Decompose();
}

Mesh solid_boolean(
    const Mesh& source, const Mesh& cutter, SolidOperation operation, double tolerance
) {

    if (!std::isfinite(tolerance) || tolerance <= 0)
        throw std::invalid_argument("Solid tolerance must be positive");

    if (!source.number_of_faces())
        return operation == SolidOperation::add ? cutter : Mesh();

    if (!cutter.number_of_faces())
        return operation == SolidOperation::intersect ? Mesh() : source;

    if (operation != SolidOperation::add &&
        !overlaps(mesh_bounds(source), mesh_bounds(cutter), std::max(tolerance, 1e-6)))
        return operation == SolidOperation::intersect ? Mesh() : source;

    const manifold::Manifold a = to_manifold(source, 0);
    std::vector<manifold::Manifold> bodies = cutter_bodies(cutter, face_id_span(source));

    if (operation == SolidOperation::subtract) {
        bodies.insert(bodies.begin(), a);
        return from_manifold(manifold::Manifold::BatchBoolean(bodies, manifold::OpType::Subtract));
    }

    const manifold::Manifold b = manifold::Manifold::BatchBoolean(bodies, manifold::OpType::Add);

    if (operation == SolidOperation::intersect)
        return from_manifold(a ^ b);

    return from_manifold(a + b, true);
}

std::vector<Mesh> solid_pieces(const Mesh& mesh) {

    std::vector<Mesh> pieces;

    for (const manifold::Manifold& piece : to_manifold(mesh, 0).Decompose())
        pieces.push_back(from_manifold(piece));

    return pieces;
}

Mesh shells_side_by_side(const std::vector<Mesh>& pieces) {

    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;

    for (const Mesh& piece : pieces) {
        std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data =
            piece.to_vertices_and_faces();
        const size_t first = points.size();
        points.insert(points.end(), data.first.begin(), data.first.end());

        for (std::vector<size_t>& face : data.second) {
            for (size_t& vertex : face)
                vertex += first;

            faces.push_back(face);
        }
    }

    if (faces.empty())
        return Mesh();

    return Mesh::from_vertices_and_faces(points, faces);
}

Mesh solid_merged(const Mesh& mesh) {
    return from_manifold(to_manifold(mesh, 0), true);
}

Mesh solid_hull(const std::vector<Point>& points) {

    std::vector<manifold::vec3> corners;
    for (const Point& point : points)
        corners.push_back({point[0], point[1], point[2]});

    return from_manifold(manifold::Manifold::Hull(corners), true);
}

Mesh solid_difference(const Mesh& source, const std::vector<Mesh>& cutters) {

    if (!source.number_of_faces() || cutters.empty())
        return source;

    const Bounds src_box = mesh_bounds(source);
    std::vector<const Mesh*> active;
    active.reserve(cutters.size());

    for (const Mesh& cutter : cutters)
        if (cutter.number_of_faces() && overlaps(src_box, mesh_bounds(cutter)))
            active.push_back(&cutter);

    if (active.empty())
        return source;

    std::vector<manifold::Manifold> solids = {to_manifold(source, 0)};
    uint64_t first_id = face_id_span(source);

    for (const Mesh* cutter : active) {
        const std::vector<manifold::Manifold> bodies = cutter_bodies(*cutter, first_id);
        solids.insert(solids.end(), bodies.begin(), bodies.end());
        first_id += face_id_span(*cutter);
    }

    return from_manifold(
        largest_piece(manifold::Manifold::BatchBoolean(solids, manifold::OpType::Subtract))
    );
}

// ═══════════════════════════════════════════════════════════════════════════
// SolidBody
// ═══════════════════════════════════════════════════════════════════════════

SolidBody::SolidBody(const Mesh& mesh) {

    if (!mesh.number_of_faces())
        return;

    bounds = AABB::from_mesh(mesh);
    const manifold::Manifold built = build_manifold(mesh, 0);
    if (built.Status() != manifold::Manifold::Error::NoError || built.IsEmpty())
        return;

    // one body: overlapping shells of one element are one solid, a cavity stays one
    const std::vector<manifold::Manifold> bodies = built.Decompose();
    bool cavity = false;
    for (const manifold::Manifold& body : bodies)
        cavity = cavity || body.Volume() < 0.0;
    solid = std::make_shared<const manifold::Manifold>(bodies.size() > 1 && !cavity ? manifold::Manifold::BatchBoolean(bodies, manifold::OpType::Add) : built);
}

bool SolidBody::is_solid() const {
    return solid != nullptr;
}

const AABB& SolidBody::box() const {
    return bounds;
}

AABB SolidBody::box_at(const Xform& placed) const {

    std::vector<Point> corners;
    for (const Point& corner : bounds.get_corners())
        corners.push_back(corner.transformed(placed));

    return AABB::from_points(corners);
}

/// The Manifold placed by placed.
static manifold::Manifold placed_manifold(const manifold::Manifold& solid, const Xform& placed) {

    manifold::mat3x4 matrix;
    for (int row = 0; row < 3; row++)
        for (int column = 0; column < 4; column++)
            matrix[column][row] = placed(row, column);

    return solid.Transform(matrix);
}

SharedWood SolidBody::shared_with(const SolidBody& other, const Xform& placed, const Xform& other_placed) const {

    if (!solid || !other.solid)
        return {};

    const manifold::Manifold common = placed_manifold(*solid, placed) ^ placed_manifold(*other.solid, other_placed);
    const double volume = common.Volume();
    const double area = common.SurfaceArea();

    return {volume, area > 0.0 ? 2.0 * volume / area : 0.0};
}

}
