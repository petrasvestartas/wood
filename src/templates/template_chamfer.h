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

/// One plate per mesh face, mitered at every fold; at a node where three or more plates meet, their inner corners spread apart, as
/// the bisector planes of the folds around it do not meet in one line, so every corner at that node is cut back along its two edges,
/// on both outlines, by just that spread, and no more, so the plates do not pass through each other; a node whose inner corners meet
/// in one point, as at a pyramid's apex, keeps its corners sharp. A free edge lying in one of edge_planes is cut by that plane.
inline std::vector<std::shared_ptr<Plate>> node_cut_plates(const Mesh& mesh, double thickness, const std::vector<Plane>& edge_planes = {}) {

    // miter_contours offsets the second outline by twice the value it is given, so half the thickness makes plates thickness thick
    const std::vector<std::tuple<std::vector<Point>, std::vector<Point>, std::vector<Point>, std::vector<Point>, Vector>> contours = Mesh::miter_contours(mesh, 0.5 * thickness, 0.0, 0.0, false, 180.0);

    // nodes: every outer corner, the mesh vertex it stands on, with the inner corners of all plates meeting there
    const std::function<std::array<long long, 3>(const Point&)> key = [](const Point& point) {
        return std::array<long long, 3>{std::llround(point[0] * 100.0), std::llround(point[1] * 100.0), std::llround(point[2] * 100.0)};
    };
    std::map<std::array<long long, 3>, std::vector<Point>> inner_corners;

    for (const std::tuple<std::vector<Point>, std::vector<Point>, std::vector<Point>, std::vector<Point>, Vector>& contour : contours)
        for (size_t i = 0; i < std::get<2>(contour).size() && i < std::get<3>(contour).size(); i++)
            inner_corners[key(std::get<2>(contour)[i])].push_back(std::get<3>(contour)[i]);

    // cut back: per node the largest distance between two of its plates' inner corners
    std::map<std::array<long long, 3>, double> cut;

    for (const std::pair<const std::array<long long, 3>, std::vector<Point>>& node : inner_corners) {
        double spread = 0.0;

        for (size_t a = 0; node.second.size() > 2 && a < node.second.size(); a++)
            for (size_t b = a + 1; b < node.second.size(); b++)
                spread = std::max(spread, (node.second[a] - node.second[b]).magnitude());

        cut[node.first] = spread;
    }

    std::vector<std::shared_ptr<Plate>> plates;

    for (const std::tuple<std::vector<Point>, std::vector<Point>, std::vector<Point>, std::vector<Point>, Vector>& contour : contours) {
        const std::vector<Point>& outer = std::get<2>(contour);
        const std::vector<Point>& inner = std::get<3>(contour);
        const size_t n = outer.size();

        if (inner.size() != n)
            continue;

        // corners: each cut back by its node's spread along both its edges, at most a third of either outline's shortest edge
        double shortest = std::numeric_limits<double>::max();

        for (size_t i = 0; i < n; i++)
            shortest = std::min({shortest, (outer[(i + 1) % n] - outer[i]).magnitude(), (inner[(i + 1) % n] - inner[i]).magnitude()});

        std::vector<Point> top;
        std::vector<Point> bottom;

        for (size_t i = 0; i < n; i++) {
            const double length = std::min(cut[key(outer[i])], shortest / 3.0);

            if (length <= 0.0) {
                top.push_back(outer[i]);
                bottom.push_back(inner[i]);
                continue;
            }

            for (const size_t neighbour : {(i + n - 1) % n, (i + 1) % n}) {
                top.push_back(outer[i] + (outer[neighbour] - outer[i]).normalized() * length);
                bottom.push_back(inner[i] + (inner[neighbour] - inner[i]).normalized() * length);
            }
        }

        // free edges in an edge plane: the inner outline's edge moved onto the plane along its two neighbouring edges
        const size_t m = top.size();
        std::vector<Point> moved = bottom;

        for (const Plane& plane : edge_planes)
            for (size_t i = 0; i < m; i++) {
                const size_t j = (i + 1) % m;

                if (std::abs((top[i] - plane.origin()).dot(plane.z_axis())) > 0.01 || std::abs((top[j] - plane.origin()).dot(plane.z_axis())) > 0.01)
                    continue;

                const std::optional<Point> start = Intersection::line_plane(Line::from_points(bottom[(i + m - 1) % m], bottom[i]), plane, false);
                const std::optional<Point> end = Intersection::line_plane(Line::from_points(bottom[(j + 1) % m], bottom[j]), plane, false);

                if (start && end) {
                    moved[i] = *start;
                    moved[j] = *end;
                }
            }

        bottom = moved;
        top.push_back(top[0]);
        bottom.push_back(bottom[0]);
        plates.push_back(std::make_shared<Plate>(Polyline(bottom), Polyline(top), fmt::format("plate_{}", plates.size())));
    }

    return plates;
}
