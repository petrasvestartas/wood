#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// One plate per mesh face, its outlines mitred at every fold and its corners sharper than chamfer_angle cut back.
inline std::vector<std::shared_ptr<Plate>> mitred_plates(
    const Mesh& mesh,
    double thickness,
    double chamfer_bottom,
    double chamfer_top,
    double chamfer_angle
) {

    std::vector<std::shared_ptr<Plate>> plates;
    const std::vector<std::tuple<std::vector<Point>, std::vector<Point>, std::vector<Point>, std::vector<Point>, Vector>> contours = Mesh::miter_contours(
        mesh,
        thickness,
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

        top.push_back(top[0]);
        bottom.push_back(bottom[0]);
        const std::string name = fmt::format("plate_{}", plates.size());
        plates.push_back(std::make_shared<Plate>(Polyline(bottom), Polyline(top), name));
    }

    return plates;
}
