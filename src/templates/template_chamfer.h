#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Face normals
// ═══════════════════════════════════════════════════════════════════════════

/// Newell's unit normal of the face through points given by its corner indices.
inline Vector newell_normal(const std::vector<Point>& points, const std::vector<size_t>& face) {

    Vector normal(0.0, 0.0, 0.0);

    for (size_t i = 0; i < face.size(); i++) {
        const Point& a = points[face[i]];
        const Point& b = points[face[(i + 1) % face.size()]];
        const double x = (a[1] - b[1]) * (a[2] + b[2]);
        const double y = (a[2] - b[2]) * (a[0] + b[0]);
        const double z = (a[0] - b[0]) * (a[1] + b[1]);
        normal = normal + Vector(x, y, z);
    }

    return normal.normalized();
}

// ═══════════════════════════════════════════════════════════════════════════
// Mitered plates
// ═══════════════════════════════════════════════════════════════════════════

/// One plate per mesh face mitered at every fold, its sharp corners cut back, free edges in edge_planes cut flush.
inline std::vector<std::shared_ptr<Plate>> mitered_plates(
    const Mesh& mesh,
    double thickness,
    double chamfer_bottom,
    double chamfer_top,
    double chamfer_angle,
    const std::vector<Plane>& edge_planes = {}
) {

    // contours: both outlines of every face, miter_contours offsetting by twice the half thickness it is given
    std::vector<std::shared_ptr<Plate>> plates;
    const std::vector<std::tuple<
        std::vector<Point>,
        std::vector<Point>,
        std::vector<Point>,
        std::vector<Point>,
        Vector>>
        contours = Mesh::miter_contours(
            mesh, 0.5 * thickness, chamfer_top, chamfer_bottom, false, chamfer_angle
        );

    for (const std::tuple<
             std::vector<Point>,
             std::vector<Point>,
             std::vector<Point>,
             std::vector<Point>,
             Vector>& contour : contours) {
        std::vector<Point> top = std::get<0>(contour);
        std::vector<Point> bottom = std::get<1>(contour);

        if (top.size() != bottom.size())
            continue;

        // free edges in an edge plane: the inner outline's edge moved onto the plane along its two neighbouring edges
        const size_t n = top.size();
        std::vector<Point> moved = bottom;

        for (const Plane& plane : edge_planes)
            for (size_t i = 0; i < n; i++) {
                const size_t j = (i + 1) % n;
                const double start_height = std::abs((top[i] - plane.origin()).dot(plane.z_axis()));
                const double end_height = std::abs((top[j] - plane.origin()).dot(plane.z_axis()));

                if (start_height > 0.01 || end_height > 0.01)
                    continue;

                const Line edge_in = Line::from_points(bottom[(i + n - 1) % n], bottom[i]);
                const Line edge_out = Line::from_points(bottom[(j + 1) % n], bottom[j]);
                const std::optional<Point> start = Intersection::line_plane(edge_in, plane, false);
                const std::optional<Point> end = Intersection::line_plane(edge_out, plane, false);

                if (start && end) {
                    moved[i] = *start;
                    moved[j] = *end;
                }
            }

        // plate: both outlines closed
        bottom = moved;
        top.push_back(top[0]);
        bottom.push_back(bottom[0]);
        const std::string name = fmt::format("plate_{}", plates.size());
        plates.push_back(std::make_shared<Plate>(Polyline(bottom), Polyline(top), name));
    }

    return plates;
}

// ═══════════════════════════════════════════════════════════════════════════
// Coplanar faces
// ═══════════════════════════════════════════════════════════════════════════

/// The root of face f's group of coplanar faces, halving the path to it on the way.
inline size_t coplanar_root(std::vector<size_t>& group, size_t f) {

    while (group[f] != f) {
        group[f] = group[group[f]];
        f = group[f];
    }

    return f;
}

/// The boundary loop of a group of coplanar faces, a straight corner dropped unless another group turns there.
inline std::vector<size_t> coplanar_loop(
    const std::vector<Point>& points,
    const std::vector<std::vector<size_t>>& faces,
    const std::vector<size_t>& members,
    const std::map<std::pair<size_t, size_t>, size_t>& owner,
    const std::map<size_t, std::set<size_t>>& users,
    std::vector<size_t>& group
) {

    // boundary: the group's edges with no twin in the group, chained into one loop
    std::map<size_t, size_t> next;
    std::set<std::pair<size_t, size_t>> edges;

    for (const size_t f : members)
        for (size_t i = 0; i < faces[f].size(); i++)
            edges.insert({faces[f][i], faces[f][(i + 1) % faces[f].size()]});

    for (const std::pair<size_t, size_t>& edge : edges)
        if (edges.find({edge.second, edge.first}) == edges.end())
            next[edge.first] = edge.second;

    std::vector<size_t> loop = {next.begin()->first};

    while (loop.size() <= next.size() && next[loop.back()] != loop.front())
        loop.push_back(next[loop.back()]);

    // corners: a straight one kept only where the groups across its two edges differ at a vertex other groups use
    std::vector<size_t> kept;
    const size_t m = loop.size();

    for (size_t i = 0; i < m; i++) {
        const size_t previous = loop[(i + m - 1) % m];
        const size_t following = loop[(i + 1) % m];
        const Vector in = points[loop[i]] - points[previous];
        const Vector out = points[following] - points[loop[i]];
        const bool straight = in.cross(out).magnitude() < 1e-6 * in.magnitude() * out.magnitude();

        const std::map<std::pair<size_t, size_t>, size_t>::const_iterator across_before =
            owner.find({loop[i], previous});
        const std::map<std::pair<size_t, size_t>, size_t>::const_iterator across_after =
            owner.find({following, loop[i]});
        const long long group_before =
            across_before == owner.end()
                ? -1
                : static_cast<long long>(coplanar_root(group, across_before->second));
        const long long group_after =
            across_after == owner.end()
                ? -1
                : static_cast<long long>(coplanar_root(group, across_after->second));

        if (!straight || (users.at(loop[i]).size() > 1 && group_before != group_after))
            kept.push_back(loop[i]);
    }

    return kept;
}

/// The mesh with every group of neighbouring coplanar faces merged into one face, its straight corners dropped.
inline Mesh merged_coplanar(const Mesh& mesh) {

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data =
        mesh.to_vertices_and_faces();
    const std::vector<Point>& points = data.first;
    const std::vector<std::vector<size_t>>& faces = data.second;

    // normals and owners: every face's normal, every directed edge's face
    std::vector<Vector> normals;
    std::map<std::pair<size_t, size_t>, size_t> owner;

    for (size_t f = 0; f < faces.size(); f++) {
        normals.push_back(newell_normal(points, faces[f]));

        for (size_t i = 0; i < faces[f].size(); i++)
            owner[{faces[f][i], faces[f][(i + 1) % faces[f].size()]}] = f;
    }

    // groups: neighbours across an edge joined when their normals agree and each lies in the other's plane
    std::vector<size_t> group(faces.size());
    std::iota(group.begin(), group.end(), 0);

    for (size_t f = 0; f < faces.size(); f++)
        for (size_t i = 0; i < faces[f].size(); i++) {
            const size_t next = faces[f][(i + 1) % faces[f].size()];
            const std::map<std::pair<size_t, size_t>, size_t>::const_iterator neighbour =
                owner.find({next, faces[f][i]});

            if (neighbour == owner.end() || normals[f].dot(normals[neighbour->second]) < 1.0 - 1e-9)
                continue;

            bool flat = true;

            for (const size_t v : faces[neighbour->second])
                flat = flat && std::abs((points[v] - points[faces[f][0]]).dot(normals[f])) < 0.01;

            if (flat)
                group[coplanar_root(group, f)] = coplanar_root(group, neighbour->second);
        }

    // members and users: every group's faces, every vertex's groups
    std::map<size_t, std::vector<size_t>> members;
    std::map<size_t, std::set<size_t>> users;

    for (size_t f = 0; f < faces.size(); f++) {
        members[coplanar_root(group, f)].push_back(f);

        for (const size_t v : faces[f])
            users[v].insert(coplanar_root(group, f));
    }

    // merged faces: a lone face as it is, a group by its boundary loop
    std::vector<std::vector<size_t>> merged;

    for (const std::pair<const size_t, std::vector<size_t>>& member : members)
        if (member.second.size() == 1)
            merged.push_back(faces[member.second[0]]);
        else
            merged.push_back(coplanar_loop(points, faces, member.second, owner, users, group));

    return Mesh::from_vertices_and_faces(points, merged);
}

// ═══════════════════════════════════════════════════════════════════════════
// Exact offset
// ═══════════════════════════════════════════════════════════════════════════

/// One plate per mesh face by the exact face offset.
class OffsetPlates {
private:
    /// A face at a vertex with the vertices before and after it on that face.
    struct Corner {
        size_t face; // The face.
        size_t before; // The vertex before on the face.
        size_t after; // The vertex after on the face.
    };

    /// A plane n . x = c of a vertex's cone.
    struct Cut {
        Vector normal; // The unit normal n.
        double c; // The plane's offset along n.
        std::optional<size_t>
            face; // The face it moves; none for a free edge or the placeholder of an open fan.
    };

    /// The planes of one fan of faces around a vertex.
    struct Cone {
        size_t vertex; // The vertex.
        std::vector<size_t>
            faces; // The fan's faces in order, each next across the edge out of the last.
        std::vector<Cut> cuts; // Every face's moved plane, an open fan closed by one placeholder.
        std::vector<Vector>
            edges; // Unit edges from the vertex, edges[i] between cut i and the next round the cycle.
        std::optional<Cut> free_in; // An open fan's free edge in.
        std::optional<Cut> free_out; // An open fan's free edge out.
    };

    const double _thickness; // mm, every plate.
    const std::vector<Plane> _edge_planes; // Planes that cut the free edges lying in them.
    std::vector<Point> _points; // The merged mesh's vertices.
    std::vector<std::vector<size_t>> _faces; // The merged mesh's faces.
    std::vector<Vector> _normals; // Every face's normal, the side its plate's inner face lies on.
    std::vector<std::vector<Corner>> _corners; // Per vertex the faces at it.
    std::map<std::pair<size_t, size_t>, std::vector<Point>>
        _chains; // Per face and vertex its inner points.
    std::vector<std::shared_ptr<Plate>> _plates; // One plate per face.

public:
    /// The plates of mesh thickness thick, a free edge lying in one of edge_planes cut by it.
    OffsetPlates(const Mesh& mesh, double thickness, const std::vector<Plane>& edge_planes);

    /// One plate per face, named plate_<i>.
    const std::vector<std::shared_ptr<Plate>>& plates() const;

private:
    /// Per vertex the faces at it with the vertex before and after it on each.
    std::vector<std::vector<Corner>> compute_corners() const;

    /// The fans of faces around vertex v, each started at a face with a free edge in when there is one.
    std::vector<std::vector<size_t>> compute_fans(size_t v) const;

    /// The cone of fan at vertex v: every face's plane moved by the thickness and an open fan's free edges.
    Cone compute_cone(size_t v, const std::vector<size_t>& fan) const;

    /// Every fan face's inner points by the triangulation most faces take, its points nearest the vertex.
    std::vector<std::vector<Point>> compute_split(const Cone& cone) const;

    /// Face i's inner points clipped to every edge plane its vertex stands on, so a foot ends flush.
    std::vector<Point> compute_foot(const Cone& cone, size_t i, std::vector<Point> inner) const;

    /// One plate per face, its inner outline the chains of its corners over its outline.
    std::vector<std::shared_ptr<Plate>> compute_plates() const;

    /// The plane of the free edge a b of face f.
    Cut free_edge(size_t a, size_t b, size_t f) const;

    /// Whether face i's inner outline, with the rays along its edges, is simple and turns as its corner does.
    bool valid(const Cone& cone, const std::vector<Point>& inner, size_t i) const;

    /// The cone's edge from the vertex between cuts a and b.
    static Vector between(const Cone& cone, size_t a, size_t b);

    /// Where cut i meets the cuts a and b, the placeholder taken as the free edge on its side.
    static std::optional<Point> point_of(const Cone& cone, size_t i, size_t a, size_t b);

    /// Face i's inner points in a triangulation, one where every two consecutive planes sharing a triangle with it meet.
    static std::optional<std::vector<Point>>
    chain(const Cone& cone, const std::vector<std::array<size_t, 3>>& triangles, size_t i);

    /// Whether two cuts are one plane.
    static bool same(const Cut& a, const Cut& b);

    /// The point where three planes meet, none when two are parallel.
    static std::optional<Point> meet(const Cut& a, const Cut& b, const Cut& c);

    /// The triangulations of the polygon of corners first to last, each as triangles of corner indices.
    static std::vector<std::vector<std::array<size_t, 3>>>
    triangulations(size_t first, size_t last);

    /// The coordinates of offset in the plane frame u, w.
    static std::array<double, 2> flat(const Vector& offset, const Vector& u, const Vector& w);

    /// The signed area of a polygon in a plane frame.
    static double area(const std::vector<std::array<double, 2>>& polygon);

    /// Twice the signed area of the triangle a b c, positive when c lies left of a b.
    static double side(
        const std::array<double, 2>& a,
        const std::array<double, 2>& b,
        const std::array<double, 2>& c
    );

    /// The height of point over plane along its unit normal.
    static double height(const Point& point, const Plane& plane, const Vector& normal);
};

inline OffsetPlates::OffsetPlates(
    const Mesh& mesh, double thickness, const std::vector<Plane>& edge_planes
)
    : _thickness(thickness), _edge_planes(edge_planes) {

    // faces: the mesh with its neighbouring coplanar faces merged
    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data =
        merged_coplanar(mesh).to_vertices_and_faces();
    _points = data.first;
    _faces = data.second;

    // normals: every face's normal, the side its plate's inner face lies on
    for (const std::vector<size_t>& face : _faces)
        _normals.push_back(newell_normal(_points, face));

    // corners: per vertex the faces at it with the vertex before and after it on each
    _corners = compute_corners();

    // chains: per fan of faces at a vertex every face's inner points, the vertex split where more than three planes meet
    for (size_t v = 0; v < _points.size(); v++)
        for (const std::vector<size_t>& fan : compute_fans(v)) {
            const Cone cone = compute_cone(v, fan);
            const std::vector<std::vector<Point>> split = compute_split(cone);

            for (size_t i = 0; i < cone.faces.size() && i < split.size(); i++)
                _chains[{cone.faces[i], v}] = compute_foot(cone, i, split[i]);
        }

    // plates: per face its inner outline over its outline
    _plates = compute_plates();
}

inline const std::vector<std::shared_ptr<Plate>>& OffsetPlates::plates() const {
    return _plates;
}

inline std::vector<std::vector<OffsetPlates::Corner>> OffsetPlates::compute_corners() const {

    std::vector<std::vector<Corner>> corners(_points.size());

    for (size_t f = 0; f < _faces.size(); f++) {
        const size_t count = _faces[f].size();

        for (size_t i = 0; i < count; i++) {
            const size_t before = _faces[f][(i + count - 1) % count];
            const size_t after = _faces[f][(i + 1) % count];
            corners[_faces[f][i]].push_back({f, before, after});
        }
    }

    return corners;
}

inline std::vector<std::vector<size_t>> OffsetPlates::compute_fans(size_t v) const {

    const std::vector<Corner>& at = _corners[v];
    std::vector<bool> used(at.size(), false);
    std::vector<std::vector<size_t>> fans;

    while (std::find(used.begin(), used.end(), false) != used.end()) {

        // start: an unused face no face at the vertex lies across its edge in from, else the first unused
        size_t start =
            static_cast<size_t>(std::find(used.begin(), used.end(), false) - used.begin());

        for (size_t i = 0; i < at.size(); i++) {
            bool across = false;

            for (const Corner& other : at)
                across = across || other.after == at[i].before;

            if (!used[i] && !across)
                start = i;
        }

        // fan: from the start every next face across the edge out of the last
        std::vector<size_t> fan = {start};
        used[start] = true;

        for (size_t step = 1; step < at.size(); step++) {
            std::optional<size_t> next;

            for (size_t i = 0; i < at.size(); i++)
                if (!used[i] && at[i].before == at[fan.back()].after)
                    next = i;

            if (!next)
                break;

            fan.push_back(*next);
            used[*next] = true;
        }

        fans.push_back(fan);
    }

    return fans;
}

inline OffsetPlates::Cone
OffsetPlates::compute_cone(size_t v, const std::vector<size_t>& fan) const {

    const std::vector<Corner>& at = _corners[v];
    const Corner& first = at[fan.front()];
    const Corner& last = at[fan.back()];
    const bool closed = last.after == first.before && fan.size() > 2;
    const Vector vertex(_points[v][0], _points[v][1], _points[v][2]);
    Cone cone = {v, {}, {}, {}, std::nullopt, std::nullopt};

    // planes: every face's own moved the thickness along its normal, with the edge out of it
    for (const size_t corner : fan) {
        const size_t f = at[corner].face;
        cone.faces.push_back(f);
        cone.cuts.push_back(Cut{_normals[f], _normals[f].dot(vertex) + _thickness, f});
        cone.edges.push_back((_points[at[corner].after] - _points[v]).normalized());
    }

    if (closed)
        return cone;

    // free edges: an open fan's two free edges, the fan closed by one placeholder plane
    cone.free_in = free_edge(first.before, v, first.face);
    cone.free_out = free_edge(v, last.after, last.face);
    cone.cuts.push_back(Cut{Vector(0.0, 0.0, 0.0), 0.0, std::nullopt});
    cone.edges.push_back((_points[first.before] - _points[v]).normalized());
    return cone;
}

inline std::vector<std::vector<Point>> OffsetPlates::compute_split(const Cone& cone) const {

    const size_t n = cone.cuts.size();
    const size_t k = cone.faces.size();
    const Point& vertex = _points[cone.vertex];
    std::vector<std::vector<Point>> best;
    size_t best_valid = 0;
    double best_reach = std::numeric_limits<double>::infinity();

    // best: of the triangulations the most faces take, the one with its inner points nearest the vertex
    for (const std::vector<std::array<size_t, 3>>& triangles : triangulations(0, n - 1)) {
        std::vector<std::vector<Point>> inner(n);
        size_t count = 0;
        bool complete = true;

        for (size_t i = 0; i < k && complete; i++) {
            const std::optional<std::vector<Point>> points_of = chain(cone, triangles, i);
            complete = points_of.has_value() && !points_of->empty();

            if (complete) {
                inner[i] = *points_of;
                count += valid(cone, inner[i], i) ? 1 : 0;
            }
        }

        double reach = 0.0;

        for (size_t i = 0; i < k && complete; i++)
            for (const Point& point : inner[i])
                reach = std::max(reach, point.distance(vertex));

        const bool better =
            best.empty() || count > best_valid || (count == best_valid && reach < best_reach);

        if (complete && better) {
            best = inner;
            best_valid = count;
            best_reach = reach;
        }
    }

    if (k != 1)
        return best;

    // a lone face: its corner where its plane meets the planes of its two free edges
    const std::optional<Point> lone = meet(cone.cuts[0], *cone.free_in, *cone.free_out);

    if (lone)
        best = {{*lone}};

    return best;
}

inline std::vector<Point>
OffsetPlates::compute_foot(const Cone& cone, size_t i, std::vector<Point> inner) const {

    const size_t n = cone.cuts.size();
    const Point& vertex = _points[cone.vertex];
    const std::vector<size_t>& face = _faces[cone.faces[i]];
    const double far = 1e6 * _thickness;

    for (const Plane& plane : _edge_planes) {
        const Vector normal = plane.z_axis().normalized();

        if (inner.empty() || std::abs(height(vertex, plane, normal)) > 0.01)
            continue;

        // open: the inner points with the rays along the face's two edges, on the face's side of the plane
        double centre = 0.0;

        for (const size_t corner : face)
            centre += height(_points[corner], plane, normal);

        const double sign = centre >= 0.0 ? 1.0 : -1.0;
        std::vector<Point> open = {inner.front() + between(cone, (i + n - 1) % n, i) * far};
        open.insert(open.end(), inner.begin(), inner.end());
        open.push_back(inner.back() + between(cone, i, (i + 1) % n) * far);
        bool inside = true;

        for (const Point& point : open)
            inside = inside && sign * height(point, plane, normal) >= -1e-9;

        if (inside)
            continue;

        // clipped: the inner points on the face's side and where the outline crosses the plane
        std::vector<Point> clipped;

        for (size_t j = 0; j < open.size(); j++) {
            const double here = height(open[j], plane, normal);

            if (sign * here >= 0.0 && j > 0 && j + 1 < open.size())
                clipped.push_back(open[j]);

            if (j + 1 == open.size())
                continue;

            const double next = height(open[j + 1], plane, normal);

            if (sign * here * next * sign < 0.0) {
                const double t = here / (here - next);
                clipped.push_back(open[j] + (open[j + 1] - open[j]) * t);
            }
        }

        if (!clipped.empty())
            inner = clipped;
    }

    return inner;
}

inline std::vector<std::shared_ptr<Plate>> OffsetPlates::compute_plates() const {

    std::vector<std::shared_ptr<Plate>> plates;

    for (size_t f = 0; f < _faces.size(); f++) {
        std::vector<Point> inner;
        std::vector<Point> outer;

        // outlines: every corner's chain, the corner repeated over it on the outer outline
        for (const size_t v : _faces[f]) {
            const std::map<std::pair<size_t, size_t>, std::vector<Point>>::const_iterator found =
                _chains.find({f, v});

            if (found == _chains.end())
                continue;

            for (const Point& point : found->second) {
                inner.push_back(point);
                outer.push_back(_points[v]);
            }
        }

        if (inner.size() < 3)
            continue;

        inner.push_back(inner.front());
        outer.push_back(outer.front());
        const std::string name = fmt::format("plate_{}", plates.size());
        plates.push_back(std::make_shared<Plate>(Polyline(inner), Polyline(outer), name));
    }

    return plates;
}

inline OffsetPlates::Cut OffsetPlates::free_edge(size_t a, size_t b, size_t f) const {

    for (const Plane& plane : _edge_planes) {
        const Vector n = plane.z_axis().normalized();
        const Point& origin = plane.origin();
        const bool holds_a = std::abs((_points[a] - origin).dot(n)) < 0.01;
        const bool holds_b = std::abs((_points[b] - origin).dot(n)) < 0.01;

        if (holds_a && holds_b)
            return Cut{n, n.dot(Vector(origin[0], origin[1], origin[2])), std::nullopt};
    }

    const Vector n = _normals[f].cross(_points[b] - _points[a]).normalized();
    return Cut{n, n.dot(Vector(_points[a][0], _points[a][1], _points[a][2])), std::nullopt};
}

inline bool OffsetPlates::valid(const Cone& cone, const std::vector<Point>& inner, size_t i) const {

    const size_t n = cone.cuts.size();
    const Point& vertex = _points[cone.vertex];
    const Vector u = between(cone, (i + n - 1) % n, i);
    const Vector w = cone.cuts[i].normal.cross(u).normalized();
    const double far = 1e6 * _thickness;
    const Vector ray_in = between(cone, (i + n - 1) % n, i) * far;
    const Vector ray_out = between(cone, i, (i + 1) % n) * far;

    // turning: the inner outline closed by the rays' far ends turns as the corner between the rays does
    std::vector<std::array<double, 2>> outline;

    for (const Point& point : inner)
        outline.push_back(flat(point - vertex, u, w));

    outline.push_back(flat((inner.back() - vertex) + ray_out, u, w));
    outline.push_back(flat((inner.front() - vertex) + ray_in, u, w));
    const std::vector<std::array<double, 2>> corner = {
        {0.0, 0.0}, flat(ray_out, u, w), flat(ray_in, u, w)
    };

    if (area(outline) * area(corner) <= 0.0)
        return false;

    // simple: no two edges that share no corner cross
    const size_t m = outline.size();

    for (size_t a = 0; a < m; a++)
        for (size_t b = a + 2; b < m; b++) {
            if (a == 0 && b == m - 1)
                continue;

            const std::array<double, 2>& p = outline[a];
            const std::array<double, 2>& q = outline[(a + 1) % m];
            const std::array<double, 2>& r = outline[b];
            const std::array<double, 2>& s = outline[(b + 1) % m];

            if (side(p, q, r) * side(p, q, s) < 0.0 && side(r, s, p) * side(r, s, q) < 0.0)
                return false;
        }

    return true;
}

inline Vector OffsetPlates::between(const Cone& cone, size_t a, size_t b) {

    const size_t n = cone.cuts.size();
    return (a + 1) % n == b ? cone.edges[a] : cone.edges[b];
}

inline std::optional<Point> OffsetPlates::point_of(const Cone& cone, size_t i, size_t a, size_t b) {

    const Cut& plane_a =
        cone.cuts[a].face ? cone.cuts[a] : (i < b ? *cone.free_in : *cone.free_out);
    const Cut& plane_b =
        cone.cuts[b].face ? cone.cuts[b] : (i < a ? *cone.free_in : *cone.free_out);
    return meet(cone.cuts[i], plane_a, plane_b);
}

inline std::optional<std::vector<Point>> OffsetPlates::chain(
    const Cone& cone, const std::vector<std::array<size_t, 3>>& triangles, size_t i
) {

    const size_t n = cone.cuts.size();
    std::set<size_t> neighbours;

    for (const std::array<size_t, 3>& triangle : triangles)
        if (std::find(triangle.begin(), triangle.end(), i) != triangle.end())
            for (const size_t other : triangle)
                if (other != i)
                    neighbours.insert(other);

    // order: from the neighbour before i round to the one after it, a face in i's plane touching it at the vertex alone left out
    std::vector<size_t> order;

    for (size_t offset = n - 1; offset > 0; offset--) {
        const size_t other = (i + offset) % n;
        const bool coplanar = cone.cuts[other].face && same(cone.cuts[other], cone.cuts[i]);

        if (neighbours.count(other) == 1 && !coplanar)
            order.push_back(other);
    }

    // points: every two consecutive planes, none for two faces in one plane, one for four planes meeting in one point
    std::vector<Point> inner;

    for (size_t j = 0; j + 1 < order.size(); j++) {
        const Cut& first = cone.cuts[order[j]];
        const Cut& second = cone.cuts[order[j + 1]];

        if (first.face && second.face && same(first, second))
            continue;

        const std::optional<Point> point = point_of(cone, i, order[j], order[j + 1]);

        if (!point)
            return std::nullopt;

        if (inner.empty() || inner.back().distance(*point) > 1e-3)
            inner.push_back(*point);
    }

    return inner;
}

inline bool OffsetPlates::same(const Cut& a, const Cut& b) {
    return a.normal.dot(b.normal) > 1.0 - 1e-9 && std::abs(a.c - b.c) < 1e-6;
}

inline std::optional<Point> OffsetPlates::meet(const Cut& a, const Cut& b, const Cut& c) {

    const double determinant = a.normal.dot(b.normal.cross(c.normal));

    if (std::abs(determinant) < 1e-9)
        return std::nullopt;

    const Vector along_a = b.normal.cross(c.normal) * a.c;
    const Vector along_b = c.normal.cross(a.normal) * b.c;
    const Vector along_c = a.normal.cross(b.normal) * c.c;
    const Vector point = (along_a + along_b + along_c) * (1.0 / determinant);
    return Point(point[0], point[1], point[2]);
}

inline std::vector<std::vector<std::array<size_t, 3>>>
OffsetPlates::triangulations(size_t first, size_t last) {

    std::vector<std::vector<std::array<size_t, 3>>> all;

    if (last - first < 2)
        return std::vector<std::vector<std::array<size_t, 3>>>{{}};

    for (size_t apex = first + 1; apex < last; apex++)
        for (const std::vector<std::array<size_t, 3>>& left : triangulations(first, apex))
            for (const std::vector<std::array<size_t, 3>>& right : triangulations(apex, last)) {
                std::vector<std::array<size_t, 3>> triangles = left;
                triangles.insert(triangles.end(), right.begin(), right.end());
                triangles.push_back({first, apex, last});
                all.push_back(triangles);
            }

    return all;
}

inline std::array<double, 2>
OffsetPlates::flat(const Vector& offset, const Vector& u, const Vector& w) {
    return std::array<double, 2>{offset.dot(u), offset.dot(w)};
}

inline double OffsetPlates::area(const std::vector<std::array<double, 2>>& polygon) {

    double sum = 0.0;

    for (size_t j = 0; j < polygon.size(); j++) {
        const std::array<double, 2>& here = polygon[j];
        const std::array<double, 2>& next = polygon[(j + 1) % polygon.size()];
        sum += here[0] * next[1] - next[0] * here[1];
    }

    return 0.5 * sum;
}

inline double OffsetPlates::side(
    const std::array<double, 2>& a, const std::array<double, 2>& b, const std::array<double, 2>& c
) {
    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
}

inline double OffsetPlates::height(const Point& point, const Plane& plane, const Vector& normal) {
    return (point - plane.origin()).dot(normal);
}

/// One plate per mesh face by the exact face offset, a free edge lying in one of edge_planes cut by it.
inline std::vector<std::shared_ptr<Plate>>
offset_plates(const Mesh& mesh, double thickness, const std::vector<Plane>& edge_planes = {}) {

    const OffsetPlates offset(mesh, thickness, edge_planes);
    return offset.plates();
}

// ═══════════════════════════════════════════════════════════════════════════
// Report
// ═══════════════════════════════════════════════════════════════════════════

/// The folding examples' line on the plates: their count, the shortest edge, and whether each is thickness thick and planar.
inline std::string
plate_report(const std::vector<std::shared_ptr<Plate>>& plates, double thickness) {

    std::string failed;
    double shortest = std::numeric_limits<double>::infinity();

    for (const std::shared_ptr<Plate>& plate : plates) {
        bool valid = std::abs(plate->thickness - thickness) < 0.1;

        for (size_t k = 0; k < 2 && k < plate->polylines.size() && k < plate->planes.size(); k++) {
            const Polyline& outline = plate->polylines[k];
            const Plane& plane = plate->planes[k];
            const Vector normal = plane.z_axis().normalized();

            // edges: an outer corner repeated for the facelets of an exact offset is one corner, no edge
            for (size_t i = 0; i + 1 < outline.point_count(); i++) {
                const double length = (outline[i + 1] - outline[i]).magnitude();
                const double distance = std::abs((outline[i] - plane.origin()).dot(normal));
                valid = valid && distance < 0.1;
                shortest = length > 1e-9 ? std::min(shortest, length) : shortest;
            }
        }

        if (!valid)
            failed += (failed.empty() ? "" : ", ") + plate->name;
    }

    if (failed.empty())
        return fmt::format(
            "{} plates, every one {} mm thick with planar outlines, shortest edge {:.1f} mm",
            plates.size(),
            thickness,
            shortest
        );

    return fmt::format(
        "{} plates, shortest edge {:.1f} mm, not {} mm thick or planar: {}",
        plates.size(),
        shortest,
        thickness,
        failed
    );
}
