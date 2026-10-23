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
