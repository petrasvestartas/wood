#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// One plate per mesh face, its outlines mitered at every fold and its corners sharper than chamfer_angle cut back; a free edge lying
/// in one of edge_planes is cut by that plane instead of square to its plate, so the plates along a straight free edge end flush.
inline std::vector<std::shared_ptr<Plate>> mitered_plates(
    const Mesh& mesh,
    double thickness,
    double chamfer_bottom,
    double chamfer_top,
    double chamfer_angle,
    const std::vector<Plane>& edge_planes = {}
) {

    // miter_contours offsets the second outline by twice the value it is given, so half the thickness makes plates thickness thick
    std::vector<std::shared_ptr<Plate>> plates;
    const std::vector<std::tuple<std::vector<Point>, std::vector<Point>, std::vector<Point>, std::vector<Point>, Vector>> contours = Mesh::miter_contours(
        mesh,
        0.5 * thickness,
        chamfer_top,
        chamfer_bottom,
        false,
        chamfer_angle
    );

    for (const std::tuple<std::vector<Point>, std::vector<Point>, std::vector<Point>, std::vector<Point>, Vector>& contour : contours) {
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

                if (std::abs((top[i] - plane.origin()).dot(plane.z_axis())) > 0.01 || std::abs((top[j] - plane.origin()).dot(plane.z_axis())) > 0.01)
                    continue;

                const std::optional<Point> start = Intersection::line_plane(Line::from_points(bottom[(i + n - 1) % n], bottom[i]), plane, false);
                const std::optional<Point> end = Intersection::line_plane(Line::from_points(bottom[(j + 1) % n], bottom[j]), plane, false);

                if (start && end) {
                    moved[i] = *start;
                    moved[j] = *end;
                }
            }

        bottom = moved;
        top.push_back(top[0]);
        bottom.push_back(bottom[0]);
        const std::string name = fmt::format("plate_{}", plates.size());
        plates.push_back(std::make_shared<Plate>(Polyline(bottom), Polyline(top), name));
    }

    return plates;
}

/// The mesh with every group of neighbouring coplanar faces merged into one face, two coplanar triangles one bigger triangle: the
/// boundary of the group walked along its outer edges, a vertex dropped where its two edges run straight on and either no face outside
/// the group uses it or the same face, or none, lies across both edges, so no two consecutive edges of a face share a miter plane.
inline Mesh merged_coplanar(const Mesh& mesh) {

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data = mesh.to_vertices_and_faces();
    const std::vector<Point>& points = data.first;
    const std::vector<std::vector<size_t>>& faces = data.second;
    std::vector<Vector> normals;
    std::map<std::pair<size_t, size_t>, size_t> owner;

    for (size_t f = 0; f < faces.size(); f++) {
        Vector normal(0.0, 0.0, 0.0);

        for (size_t i = 0; i < faces[f].size(); i++) {
            const Point& a = points[faces[f][i]];
            const Point& b = points[faces[f][(i + 1) % faces[f].size()]];
            normal = normal + Vector((a[1] - b[1]) * (a[2] + b[2]), (a[2] - b[2]) * (a[0] + b[0]), (a[0] - b[0]) * (a[1] + b[1]));
            owner[{faces[f][i], faces[f][(i + 1) % faces[f].size()]}] = f;
        }

        normal.normalize_self();
        normals.push_back(normal);
    }

    // groups: neighbours across an edge joined when their normals agree and each lies in the other's plane
    std::vector<size_t> group(faces.size());
    std::iota(group.begin(), group.end(), 0);
    const std::function<size_t(size_t)> root = [&](size_t f) {
        while (group[f] != f)
            f = group[f] = group[group[f]];

        return f;
    };

    for (size_t f = 0; f < faces.size(); f++)
        for (size_t i = 0; i < faces[f].size(); i++) {
            const std::map<std::pair<size_t, size_t>, size_t>::const_iterator neighbour = owner.find({faces[f][(i + 1) % faces[f].size()], faces[f][i]});

            if (neighbour == owner.end() || normals[f].dot(normals[neighbour->second]) < 1.0 - 1e-9)
                continue;

            bool flat = true;

            for (const size_t v : faces[neighbour->second])
                flat = flat && std::abs((points[v] - points[faces[f][0]]).dot(normals[f])) < 0.01;

            if (flat)
                group[root(f)] = root(neighbour->second);
        }

    // merged faces: per group its outer edges chained into one loop, straight vertices used only inside the group dropped
    std::map<size_t, std::vector<size_t>> members;
    std::map<size_t, std::set<size_t>> users;

    for (size_t f = 0; f < faces.size(); f++) {
        members[root(f)].push_back(f);

        for (const size_t v : faces[f])
            users[v].insert(root(f));
    }

    std::vector<std::vector<size_t>> merged;

    for (const std::pair<const size_t, std::vector<size_t>>& member : members) {
        if (member.second.size() == 1) {
            merged.push_back(faces[member.second[0]]);
            continue;
        }

        std::map<size_t, size_t> next;
        std::set<std::pair<size_t, size_t>> edges;

        for (const size_t f : member.second)
            for (size_t i = 0; i < faces[f].size(); i++)
                edges.insert({faces[f][i], faces[f][(i + 1) % faces[f].size()]});

        for (const std::pair<size_t, size_t>& edge : edges)
            if (edges.find({edge.second, edge.first}) == edges.end())
                next[edge.first] = edge.second;

        std::vector<size_t> loop = {next.begin()->first};

        while (loop.size() <= next.size() && next[loop.back()] != loop.front())
            loop.push_back(next[loop.back()]);

        std::vector<size_t> kept;

        for (size_t i = 0; i < loop.size(); i++) {
            const Point& before = points[loop[(i + loop.size() - 1) % loop.size()]];
            const Point& at = points[loop[i]];
            const Point& after = points[loop[(i + 1) % loop.size()]];
            const bool straight = (at - before).cross(after - at).magnitude() < 1e-6 * (at - before).magnitude() * (after - at).magnitude();

            const std::map<std::pair<size_t, size_t>, size_t>::const_iterator across_before = owner.find({loop[i], loop[(i + loop.size() - 1) % loop.size()]});
            const std::map<std::pair<size_t, size_t>, size_t>::const_iterator across_after = owner.find({loop[(i + 1) % loop.size()], loop[i]});
            const long long group_before = across_before == owner.end() ? -1 : static_cast<long long>(root(across_before->second));
            const long long group_after = across_after == owner.end() ? -1 : static_cast<long long>(root(across_after->second));

            if (!straight || (users[loop[i]].size() > 1 && group_before != group_after))
                kept.push_back(loop[i]);
        }

        merged.push_back(kept);
    }

    return Mesh::from_vertices_and_faces(points, merged);
}

/// One plate per mesh face by the exact face offset: every face moved thickness along its normal, the inner faces trimmed by each
/// other, so the plates meet on shared planar faces with no chamfer and no gap. Where three faces meet a vertex their offset planes
/// meet in one point. Where more meet, the offset planes cannot, and the vertex splits into several points, each where three of the planes meet, joined by short new edges between faces
/// that do not share an edge: of all the ways to split the vertex, the triangulations of its cycle of faces, the one where every
/// face's inner outline stays simple and turns as the face's corner does. A plate's sides are then the planar quads between its edges
/// and their offsets, shared with the neighbour across the fold, and the triangles between the vertex and every new edge it gets,
/// shared with the plate across that edge. A free edge is cut by the edge plane it lies in, or else square to its plate; such a plane
/// stays where it is while the faces move. Neighbouring coplanar faces are merged into one plate first.
inline std::vector<std::shared_ptr<Plate>> offset_plates(const Mesh& mesh, double thickness, const std::vector<Plane>& edge_planes = {}) {

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data = merged_coplanar(mesh).to_vertices_and_faces();
    const std::vector<Point>& points = data.first;
    const std::vector<std::vector<size_t>>& faces = data.second;

    // normals: every face's Newell normal, the side its plate's inner face lies on
    std::vector<Vector> normals;

    for (const std::vector<size_t>& face : faces) {
        Vector normal(0.0, 0.0, 0.0);

        for (size_t i = 0; i < face.size(); i++) {
            const Point& a = points[face[i]];
            const Point& b = points[face[(i + 1) % face.size()]];
            normal = normal + Vector((a[1] - b[1]) * (a[2] + b[2]), (a[2] - b[2]) * (a[0] + b[0]), (a[0] - b[0]) * (a[1] + b[1]));
        }

        normals.push_back(normal.normalized());
    }

    // corners: per vertex the faces at it with the vertex before and after it on each
    struct Corner {
        size_t face;
        size_t before;
        size_t after;
    };
    std::vector<std::vector<Corner>> corners(points.size());

    for (size_t f = 0; f < faces.size(); f++)
        for (size_t i = 0; i < faces[f].size(); i++)
            corners[faces[f][i]].push_back({f, faces[f][(i + faces[f].size() - 1) % faces[f].size()], faces[f][(i + 1) % faces[f].size()]});

    // a plane of a vertex's cone: n . x = c, a face's own moved thickness along its normal, a free edge's plane fixed
    struct Cut {
        Vector normal;
        double c;
        std::optional<size_t> face;
    };

    // a free edge's plane: an edge plane holding the edge, else the plane through the edge square to its face
    const std::function<Cut(size_t, size_t, size_t)> free_edge = [&](size_t a, size_t b, size_t f) {
        for (const Plane& plane : edge_planes) {
            const Vector n = plane.z_axis().normalized();

            if (std::abs((points[a] - plane.origin()).dot(n)) < 0.01 && std::abs((points[b] - plane.origin()).dot(n)) < 0.01)
                return Cut{n, n.dot(Vector(plane.origin()[0], plane.origin()[1], plane.origin()[2])), std::nullopt};
        }

        const Vector n = normals[f].cross(points[b] - points[a]).normalized();
        return Cut{n, n.dot(Vector(points[a][0], points[a][1], points[a][2])), std::nullopt};
    };

    // whether two planes are one
    const std::function<bool(const Cut&, const Cut&)> same = [](const Cut& a, const Cut& b) {
        return a.normal.dot(b.normal) > 1.0 - 1e-9 && std::abs(a.c - b.c) < 1e-6;
    };

    // the point where three planes meet, none when two are parallel
    const std::function<std::optional<Point>(const Cut&, const Cut&, const Cut&)> meet = [](const Cut& a, const Cut& b, const Cut& c) -> std::optional<Point> {
        const double determinant = a.normal.dot(b.normal.cross(c.normal));

        if (std::abs(determinant) < 1e-9)
            return std::nullopt;

        const Vector point = (b.normal.cross(c.normal) * a.c + c.normal.cross(a.normal) * b.c + a.normal.cross(b.normal) * c.c) * (1.0 / determinant);
        return Point(point[0], point[1], point[2]);
    };

    // the triangulations of a polygon of n corners, each as triangles of corner indices
    const std::function<std::vector<std::vector<std::array<size_t, 3>>>(size_t, size_t)> triangulations = [&](size_t first, size_t last) {
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
    };

    // chains: per face and per corner of it, the inner points from the side of its edge in to the side of its edge out
    std::map<std::pair<size_t, size_t>, std::vector<Point>> chains;

    for (size_t v = 0; v < points.size(); v++) {
        const std::vector<Corner>& at = corners[v];
        std::vector<bool> used(at.size(), false);

        // fans: the faces around the vertex in order, each next across its edge out, from a face with a free edge in, or any; a
        // vertex where two fans touch has its fans one after another
        while (std::find(used.begin(), used.end(), false) != used.end()) {
            size_t start = static_cast<size_t>(std::find(used.begin(), used.end(), false) - used.begin());

            for (size_t i = 0; i < at.size(); i++) {
                bool across = false;

                for (const Corner& other : at)
                    across = across || other.after == at[i].before;

                if (!used[i] && !across)
                    start = i;
            }

            std::vector<size_t> fan = {start};
            used[start] = true;

            while (true) {
                std::optional<size_t> next;

                for (size_t i = 0; i < at.size(); i++)
                    if (!used[i] && at[i].before == at[fan.back()].after)
                        next = i;

                if (!next)
                    break;

                fan.push_back(*next);
                used[*next] = true;
            }

            const bool closed = at[fan.back()].after == at[fan.front()].before && fan.size() > 2;
            const size_t k = fan.size();
            const Vector vertex(points[v][0], points[v][1], points[v][2]);

            // the cone: every face's plane moved thickness along its normal, an open fan closed by one corner outside, which is the
            // plane of the free edge in beside a later face and of the free edge out beside an earlier one
            std::vector<Cut> cuts;
            std::vector<Vector> edges;

            for (size_t i = 0; i < k; i++) {
                const size_t f = at[fan[i]].face;
                cuts.push_back(Cut{normals[f], normals[f].dot(vertex) + thickness, f});
                edges.push_back((points[at[fan[i]].after] - points[v]).normalized());
            }

            std::optional<Cut> free_in;
            std::optional<Cut> free_out;

            if (!closed) {
                free_in = free_edge(at[fan.front()].before, v, at[fan.front()].face);
                free_out = free_edge(v, at[fan.back()].after, at[fan.back()].face);
                cuts.push_back(Cut{Vector(0.0, 0.0, 0.0), 0.0, std::nullopt});
                edges.push_back((points[at[fan.front()].before] - points[v]).normalized());
            }

            // edges[i] runs between cut i and the next one round the cycle
            const size_t n = cuts.size();
            const std::function<Vector(size_t, size_t)> between = [&](size_t a, size_t b) {
                return (a + 1) % n == b ? edges[a] : edges[b];
            };

            // where face i meets the planes a and b
            const std::function<std::optional<Point>(size_t, size_t, size_t)> point_of = [&](size_t i, size_t a, size_t b) {
                const std::function<Cut(size_t, size_t)> plane = [&](size_t c, size_t other) {
                    return cuts[c].face ? cuts[c] : (i < other ? *free_in : *free_out);
                };
                return meet(cuts[i], plane(a, b), plane(b, a));
            };

            // the face's chain in a triangulation: the planes sharing a triangle with it from the one before it round to the one
            // after it, every two consecutive ones a point
            const std::function<std::optional<std::vector<Point>>(const std::vector<std::array<size_t, 3>>&, size_t)> chain = [&](const std::vector<std::array<size_t, 3>>& triangles, size_t i) -> std::optional<std::vector<Point>> {
                std::set<size_t> neighbours;

                for (const std::array<size_t, 3>& triangle : triangles)
                    if (std::find(triangle.begin(), triangle.end(), i) != triangle.end())
                        for (const size_t other : triangle)
                            if (other != i)
                                neighbours.insert(other);

                std::vector<size_t> order(neighbours.begin(), neighbours.end());
                std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return (a + n - i) % n > (b + n - i) % n; });

                // a face lying in this face's plane, touching it at the vertex alone, takes no part: no edge runs between them
                order.erase(std::remove_if(order.begin(), order.end(), [&](size_t other) { return cuts[other].face && same(cuts[other], cuts[i]); }), order.end());
                std::vector<Point> inner;

                for (size_t j = 0; j + 1 < order.size(); j++) {
                    // two faces in one plane meet this face in one line, a zero-length edge: no point
                    if (cuts[order[j]].face && cuts[order[j + 1]].face && same(cuts[order[j]], cuts[order[j + 1]]))
                        continue;

                    const std::optional<Point> point = point_of(i, order[j], order[j + 1]);

                    if (!point)
                        return std::nullopt;

                    inner.push_back(*point);
                }

                return inner;
            };

            // valid: the face's inner outline, with the rays along its two edges, simple and turning as its corner does
            const std::function<bool(const std::vector<Point>&, size_t)> valid = [&](const std::vector<Point>& inner, size_t i) {
                const Vector u = between((i + n - 1) % n, i);
                const Vector w = cuts[i].normal.cross(u).normalized();
                const double far = 1e6 * thickness;
                const Vector ray_in = between((i + n - 1) % n, i) * far;
                const Vector ray_out = between(i, (i + 1) % n) * far;
                const std::function<std::array<double, 2>(const Vector&)> flat = [&](const Vector& offset) {
                    return std::array<double, 2>{offset.dot(u), offset.dot(w)};
                };
                std::vector<std::array<double, 2>> outline;

                for (const Point& point : inner)
                    outline.push_back(flat(point - points[v]));

                outline.push_back(flat((inner.back() - points[v]) + ray_out));
                outline.push_back(flat((inner.front() - points[v]) + ray_in));
                const std::vector<std::array<double, 2>> corner = {{0.0, 0.0}, flat(ray_out), flat(ray_in)};

                const std::function<double(const std::vector<std::array<double, 2>>&)> area = [](const std::vector<std::array<double, 2>>& polygon) {
                    double sum = 0.0;

                    for (size_t j = 0; j < polygon.size(); j++)
                        sum += polygon[j][0] * polygon[(j + 1) % polygon.size()][1] - polygon[(j + 1) % polygon.size()][0] * polygon[j][1];

                    return 0.5 * sum;
                };

                if (area(outline) * area(corner) <= 0.0)
                    return false;

                // simple: no two edges that share no corner cross
                const size_t m = outline.size();
                const std::function<double(const std::array<double, 2>&, const std::array<double, 2>&, const std::array<double, 2>&)> side = [](const std::array<double, 2>& a, const std::array<double, 2>& b, const std::array<double, 2>& c) {
                    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
                };

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
            };

            // the split: of the triangulations the most faces take, the tightest, its inner points nearest the vertex
            std::vector<std::vector<Point>> best;
            size_t best_valid = 0;
            double best_reach = std::numeric_limits<double>::infinity();

            for (const std::vector<std::array<size_t, 3>>& triangles : triangulations(0, n - 1)) {
                std::vector<std::vector<Point>> inner(n);
                size_t count = 0;
                bool complete = true;

                for (size_t i = 0; i < k && complete; i++) {
                    const std::optional<std::vector<Point>> points_of = chain(triangles, i);
                    complete = points_of.has_value() && !points_of->empty();

                    if (complete) {
                        inner[i] = *points_of;
                        count += valid(inner[i], i) ? 1 : 0;
                    }
                }

                double reach = 0.0;

                for (size_t i = 0; i < k && complete; i++)
                    for (const Point& point : inner[i])
                        reach = std::max(reach, point.distance(points[v]));

                if (complete && (best.empty() || count > best_valid || (count == best_valid && reach < best_reach))) {
                    best = inner;
                    best_valid = count;
                    best_reach = reach;
                }
            }

            // a lone face: its corner where its plane meets the planes of its two free edges
            if (k == 1)
                if (const std::optional<Point> point = meet(cuts[0], *free_in, *free_out))
                    best = {{*point}};

            // a vertex on an edge plane none of its edges lies in, a foot: every face's inner outline there, with the rays along its
            // two edges, clipped to the face's side of the plane, so the plate ends flush on it
            for (size_t i = 0; i < k && i < best.size(); i++) {
                std::vector<Point> inner = best[i];
                const std::vector<size_t>& face = faces[at[fan[i]].face];

                for (const Plane& plane : edge_planes) {
                    const Vector normal = plane.z_axis().normalized();
                    const std::function<double(const Point&)> height = [&](const Point& point) {
                        return (point - plane.origin()).dot(normal);
                    };

                    if (inner.empty() || std::abs(height(points[v])) > 0.01)
                        continue;

                    double centre = 0.0;

                    for (const size_t corner : face)
                        centre += height(points[corner]);

                    const double sign = centre >= 0.0 ? 1.0 : -1.0;
                    const double far = 1e6 * thickness;
                    std::vector<Point> open = {inner.front() + between((i + n - 1) % n, i) * far};
                    open.insert(open.end(), inner.begin(), inner.end());
                    open.push_back(inner.back() + between(i, (i + 1) % n) * far);

                    if (std::all_of(open.begin(), open.end(), [&](const Point& point) { return sign * height(point) >= -1e-9; }))
                        continue;

                    std::vector<Point> clipped;

                    for (size_t j = 0; j < open.size(); j++) {
                        if (sign * height(open[j]) >= 0.0 && j > 0 && j + 1 < open.size())
                            clipped.push_back(open[j]);

                        if (j + 1 < open.size() && sign * height(open[j]) * height(open[j + 1]) * sign < 0.0) {
                            const double t = height(open[j]) / (height(open[j]) - height(open[j + 1]));
                            clipped.push_back(open[j] + (open[j + 1] - open[j]) * t);
                        }
                    }

                    if (!clipped.empty())
                        inner = clipped;
                }

                chains[{at[fan[i]].face, v}] = inner;
            }
        }
    }

    // plates: per face its inner outline, the chains of its corners in order, over its outline, every corner repeated for its chain
    std::vector<std::shared_ptr<Plate>> plates;

    for (size_t f = 0; f < faces.size(); f++) {
        std::vector<Point> inner;
        std::vector<Point> outer;

        for (const size_t v : faces[f]) {
            const std::map<std::pair<size_t, size_t>, std::vector<Point>>::const_iterator found = chains.find({f, v});

            if (found == chains.end())
                continue;

            for (const Point& point : found->second) {
                inner.push_back(point);
                outer.push_back(points[v]);
            }
        }

        if (inner.size() < 3)
            continue;

        inner.push_back(inner.front());
        outer.push_back(outer.front());
        plates.push_back(std::make_shared<Plate>(Polyline(inner), Polyline(outer), fmt::format("plate_{}", plates.size())));
    }

    return plates;
}

/// One line on the plates as the folding examples print it: their count, the shortest edge of any outline, and either that every one
/// is thickness thick with both its outlines planar, or the names of those that are not.
inline std::string plate_report(const std::vector<std::shared_ptr<Plate>>& plates, double thickness) {

    std::string failed;
    double shortest = std::numeric_limits<double>::infinity();

    for (const std::shared_ptr<Plate>& plate : plates) {
        bool valid = std::abs(plate->thickness - thickness) < 0.1;

        for (size_t k = 0; k < 2 && k < plate->polylines.size() && k < plate->planes.size(); k++) {
            const Polyline& outline = plate->polylines[k];
            const Plane& plane = plate->planes[k];

            // an outer corner repeated for the facelets of an exact offset is one corner, no edge
            for (size_t i = 0; i + 1 < outline.point_count(); i++) {
                const double length = (outline[i + 1] - outline[i]).magnitude();
                valid = valid && std::abs((outline[i] - plane.origin()).dot(plane.z_axis().normalized())) < 0.1;
                shortest = length > 1e-9 ? std::min(shortest, length) : shortest;
            }
        }

        if (!valid)
            failed += (failed.empty() ? "" : ", ") + plate->name;
    }

    if (failed.empty())
        return fmt::format("{} plates, every one {} mm thick with planar outlines, shortest edge {:.1f} mm", plates.size(), thickness, shortest);

    return fmt::format("{} plates, shortest edge {:.1f} mm, not {} mm thick or planar: {}", plates.size(), shortest, thickness, failed);
}
