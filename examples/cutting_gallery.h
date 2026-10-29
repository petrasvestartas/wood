#pragma once
#include "wood_session.h"

namespace cutting_gallery {
using namespace session_cpp;
using namespace wood_session;

inline const Color STOCK_COLOR(0.65f, 0.65f, 0.65f);
inline const Color CUTTER_COLOR(0.807f, 0.063f, 0.258f);
inline const Color RESULT_COLOR(0.24f, 0.54f, 0.86f);

inline Polyline rectangle(double x, double y, double z, double w, double h) {
    return Polyline::rectangle({x, y, z}, {1, 0, 0}, {0, 1, 0}, w, h);
}

inline void preview(
    WoodSession& scene,
    Mesh mesh,
    const std::string& name,
    Vector offset,
    Color color,
    const std::shared_ptr<TreeNode>& group
) {
    mesh = mesh.transformed(Xform::translation(offset[0], offset[1], offset[2]));
    mesh.set_objectcolor(color);
    scene.add(std::make_shared<Element>(mesh, name), group);
}

inline void finish(WoodSession& scene, const std::string& name) {
    scene.pb_dump(pb_path(name));
    scene.pb_dump(pb_path("live"));
}

inline void cut_case(
    WoodSession& scene,
    const std::string& name,
    const std::shared_ptr<Element>& stock,
    const std::shared_ptr<Joint>& cutter,
    double row
) {
    const std::shared_ptr<TreeNode> group = scene.add_group(name);
    const Xform move = Xform::translation(600, row, 0);
    preview(
        scene, stock->element_geometry_mesh(), "Stock", {0, row, 0}, STOCK_COLOR, group
    );
    stock->place(move);
    cutter->place(move);
    scene.add(stock, group);
    scene.add(cutter, group);
    cutter->targets = {stock->guid()};
    scene.add_interaction(cutter, stock, cutter->interaction(0));
    stock->name = "StockCut";
    stock->is_visible = false;
    stock->compute_geometry_mesh();
    preview(
        scene,
        stock->model_geometry_mesh(),
        "Result",
        {0, 0, 0},
        RESULT_COLOR,
        group
    );
    cutter->place(Xform::translation(-300, 0, 0));
    cutter->is_visible = true;
    cutter->name = "Cutter (display copy of applied definition)";
    cutter->compute_geometry_mesh();
    Mesh cutter_mesh = cutter->geometry_mesh();
    cutter_mesh.set_objectcolor(CUTTER_COLOR);
    cutter->set_geometry(cutter_mesh);
}
}
