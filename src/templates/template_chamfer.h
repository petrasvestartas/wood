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

/// One plate per mesh face, mitered at every fold; at a node where three or more plates meet, every corner is cut only as far as it
/// crosses into another plate's share of the node: for every other plate there, the plane bisecting the two plates' planes through the
/// node separates them, as the miter plane does two neighbours, and a corner on the wrong side of the deepest such plane is cut by it
/// moved 1 mm into the plate, on both outlines, so the cut is one planar face with no edge of zero length; a plane that would take
/// more than half an edge is not used. Plates on both sides of every bisector cannot meet, and a
/// corner that crosses none keeps its point. Neighbouring coplanar faces are merged into one plate first. A free edge lying in one of
/// edge_planes is cut by that plane.
inline std::vector<std::shared_ptr<Plate>> node_cut_plates(const Mesh& mesh, double thickness, const std::vector<Plane>& edge_planes = {}) {

    // miter_contours offsets the second outline by twice the value it is given, so half the thickness makes plates thickness thick
    const std::vector<std::tuple<std::vector<Point>, std::vector<Point>, std::vector<Point>, std::vector<Point>, Vector>> contours = Mesh::miter_contours(merged_coplanar(mesh), 0.5 * thickness, 0.0, 0.0, false, 180.0);

    // nodes: the plates meeting at every outer corner, keyed by its position
    const std::function<std::array<long long, 3>(const Point&)> key = [](const Point& point) {
        return std::array<long long, 3>{std::llround(point[0] * 100.0), std::llround(point[1] * 100.0), std::llround(point[2] * 100.0)};
    };
    std::map<std::array<long long, 3>, std::vector<size_t>> nodes;
    std::vector<Point> centres;

    for (size_t p = 0; p < contours.size(); p++) {
        const std::vector<Point>& outer = std::get<2>(contours[p]);
        Point centre(0.0, 0.0, 0.0);

        for (const Point& point : outer) {
            nodes[key(point)].push_back(p);
            centre = centre + Vector(point[0], point[1], point[2]) * (1.0 / static_cast<double>(outer.size()));
        }

        centres.push_back(centre);
    }

    std::vector<std::shared_ptr<Plate>> plates;

    for (size_t p = 0; p < contours.size(); p++) {
        const std::vector<Point>& outer = std::get<2>(contours[p]);
        const std::vector<Point>& inner = std::get<3>(contours[p]);
        const Vector normal = std::get<4>(contours[p]);
        const size_t n = outer.size();

        if (inner.size() != n)
            continue;

        std::vector<Point> top;
        std::vector<Point> bottom;

        for (size_t i = 0; i < n; i++) {
            const std::vector<size_t>& others = nodes[key(outer[i])];
            const size_t before = (i + n - 1) % n;
            const size_t after = (i + 1) % n;

            // the deepest bisector the corner crosses: per other plate at the node the plane through the node halving the two planes,
            // its normal towards this plate's centre
            std::optional<Plane> cut;
            double deepest = 0.01;
            bool on_edge_plane = false;

            // a corner on an edge plane is cut by that plane, not at its node
            for (const Plane& plane : edge_planes)
                on_edge_plane = on_edge_plane || std::abs((outer[i] - plane.origin()).dot(plane.z_axis())) < 0.01;

            for (size_t q = 0; !on_edge_plane && others.size() > 2 && q < others.size(); q++) {
                if (others[q] == p)
                    continue;

                // a neighbour across one of the corner's two edges is already separated by their miter plane
                bool neighbour = false;

                for (const Point& point : std::get<2>(contours[others[q]]))
                    neighbour = neighbour || key(point) == key(outer[before]) || key(point) == key(outer[after]);

                if (neighbour)
                    continue;

                Vector across = normal - std::get<4>(contours[others[q]]);

                if (!across.normalize_self())
                    continue;

                if ((centres[p] - outer[i]).dot(across) < 0.0)
                    across = across * -1.0;

                const double depth = -(inner[i] - outer[i]).dot(across);
                const Plane candidate = Plane::from_point_normal(outer[i] + across * 1.0, across);

                // a plane that crosses an edge of either outline beyond its far end would cut away the edge, no corner: skipped
                bool within = true;

                for (const std::vector<Point>* points : {&outer, &inner})
                    for (const size_t neighbour : {before, after}) {
                        const double start = ((*points)[i] - candidate.origin()).dot(candidate.z_axis());
                        const double end = ((*points)[neighbour] - candidate.origin()).dot(candidate.z_axis());
                        within = within && start < 0.0 && end > 0.0 && -start < 0.5 * (end - start);
                    }

                if (depth > deepest && within) {
                    deepest = depth;
                    cut = candidate;
                }
            }

            // corner: kept, or cut by the plane on each outline that crosses it, a kept outer corner doubled to match the inner cut
            for (const std::pair<const std::vector<Point>*, std::vector<Point>*>& outline : {std::make_pair(&outer, &top), std::make_pair(&inner, &bottom)}) {
                const std::vector<Point>& points = *outline.first;

                if (!cut || (points[i] - cut->origin()).dot(cut->z_axis()) > -1e-6) {
                    outline.second->push_back(points[i]);

                    if (cut)
                        outline.second->push_back(points[i]);

                    continue;
                }

                for (const size_t neighbour : {before, after}) {
                    const std::optional<Point> crossing = Intersection::line_plane(Line::from_points(points[i], points[neighbour]), *cut, false);
                    outline.second->push_back(crossing.value_or(points[i]));
                }
            }
        }

        // free edges in an edge plane: every inner point under an outer point on the plane, with an outer neighbour on it too, moved
        // onto the plane along its edge to the neighbour off the plane, or square to the free edge where both neighbours are on it; a
        // corner touching the plane alone, its inner point past it, cut on both outlines by the plane moved 1 mm inwards
        for (const Plane& plane : edge_planes) {
            const size_t m = top.size();
            const std::function<double(const Point&)> side = [&](const Point& point) {
                return (point - plane.origin()).dot(plane.z_axis());
            };
            const std::function<bool(size_t)> on = [&](size_t k) {
                return std::abs(side(top[k])) < 0.01;
            };
            double inside = 0.0;

            for (const Point& point : top)
                inside += side(point);

            std::vector<Point> top_cut;
            std::vector<Point> bottom_cut;

            for (size_t k = 0; k < m; k++) {
                const size_t before = (k + m - 1) % m;
                const size_t after = (k + 1) % m;

                if (!on(k)) {
                    top_cut.push_back(top[k]);
                    bottom_cut.push_back(bottom[k]);
                    continue;
                }

                if (on(before) || on(after)) {
                    Line along = Line::from_points(bottom[k], bottom[on(before) ? after : before]);

                    if (on(before) && on(after))
                        along = Line::from_points(bottom[k], bottom[k] + normal.cross(top[after] - top[before]));

                    const std::optional<Point> point = Intersection::line_plane(along, plane, false);
                    const bool near = point && (*point - bottom[k]).magnitude() < 10.0 * thickness;
                    top_cut.push_back(top[k]);
                    bottom_cut.push_back(near ? *point : bottom[k]);
                    continue;
                }

                if (side(bottom[k]) * inside >= 0.0) {
                    top_cut.push_back(top[k]);
                    bottom_cut.push_back(bottom[k]);
                    continue;
                }

                // both outlines cut by the plane moved 1 mm inwards, so the cut is one planar face with no edge of zero length
                const Plane inwards = Plane::from_point_normal(plane.origin() + plane.z_axis() * (inside > 0.0 ? 1.0 : -1.0), plane.z_axis());

                for (const size_t neighbour : {before, after}) {
                    const std::optional<Point> outer_point = Intersection::line_plane(Line::from_points(top[k], top[neighbour]), inwards, false);
                    const std::optional<Point> inner_point = Intersection::line_plane(Line::from_points(bottom[k], bottom[neighbour]), inwards, false);
                    top_cut.push_back(outer_point.value_or(top[k]));
                    bottom_cut.push_back(inner_point.value_or(bottom[k]));
                }
            }

            top = top_cut;
            bottom = bottom_cut;
        }

        top.push_back(top[0]);
        bottom.push_back(bottom[0]);
        plates.push_back(std::make_shared<Plate>(Polyline(bottom), Polyline(top), fmt::format("plate_{}", plates.size())));
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

        for (size_t k = 0; k < 2 && k < plate->polylines.size(); k++) {
            const Polyline& outline = plate->polylines[k];
            const Plane& plane = outline.get_plane();

            for (size_t i = 0; i + 1 < outline.point_count(); i++) {
                valid = valid && std::abs((outline[i] - plane.origin()).dot(plane.z_axis())) < 0.1;
                shortest = std::min(shortest, (outline[i + 1] - outline[i]).magnitude());
            }
        }

        if (!valid)
            failed += (failed.empty() ? "" : ", ") + plate->name;
    }

    if (failed.empty())
        return fmt::format("{} plates, every one {} mm thick with planar outlines, shortest edge {:.1f} mm", plates.size(), thickness, shortest);

    return fmt::format("{} plates, shortest edge {:.1f} mm, not {} mm thick or planar: {}", plates.size(), shortest, thickness, failed);
}
