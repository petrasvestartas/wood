#pragma once
#include "wood_session.h"

namespace cutting_gallery {
using namespace session_cpp;
using namespace wood_session;

inline Polyline rectangle(double x, double y, double z, double w, double h) {
    return Polyline::rectangle({x, y, z}, {1, 0, 0}, {0, 1, 0}, w, h);
}

inline void preview(WoodSession& scene, Mesh mesh, const std::string& name, Vector offset, Color color,
                    const std::shared_ptr<TreeNode>& group) {
    mesh = mesh.transformed(Xform::translation(offset[0], offset[1], offset[2]));
    mesh.set_objectcolor(color);
    scene.add(std::make_shared<Element>(mesh, name), group);
}

inline void finish(WoodSession& scene, const std::string& name) {
    scene.pb_dump(pb_path(name));
    scene.pb_dump(pb_path("live"));
}

inline void cut_case(WoodSession& scene, const std::string& name, const std::shared_ptr<Element>& stock,
                     const std::shared_ptr<Joint>& cutter, double row) {
    const std::shared_ptr<TreeNode> group = scene.add_group(name);
    const Xform move = Xform::translation(600, row, 0);
    preview(scene, stock->element_geometry_mesh(), "Stock", {0, row, 0}, Color(0.72, 0.55, 0.34), group);
    stock->place(move);
    cutter->place(move);
    scene.add(stock, group);
    scene.add(cutter, group);
    cutter->targets = {stock->guid()};
    scene.add_joint(cutter);
    stock->name = "Result";
    stock->compute_geometry_mesh();
    const Mesh& result = stock->model_geometry_mesh();
    cutter->place(Xform::translation(-300, 0, 0));
    cutter->name = "Cutter (display copy of applied definition)";
    cutter->compute_geometry_mesh();
    Mesh cutter_mesh = cutter->model_geometry_mesh();
    cutter_mesh.set_objectcolor(Color(0.9, 0.35, 0.2));
    cutter->set_geometry(cutter_mesh);
    Mesh result_mesh = result;
    result_mesh.set_objectcolor(Color(0.24, 0.58, 0.7));
    stock->set_geometry(result_mesh);
}
}
