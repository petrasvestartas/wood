#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A variable edge with a hole through its width.
int main() {

    WoodSession scene("element_edge_variable_holes");

    const std::shared_ptr<EdgeVariableElement> edge =
        EdgeVariableElement::box({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 1200.0, 200.0, 300.0, "rib");
    scene.add(edge);

    const std::shared_ptr<JointElement> hole =
        JointElement::drill(Line::from_points({600.0, 150.0, 150.0}, {600.0, -150.0, 150.0}), 30.0);
    hole->name = "hole";
    hole->is_visible = false;
    scene.add(hole);
    scene.add_interaction(hole, edge, hole->interaction(0));

    std::cout << fmt::format(
        "{}: {} faces, solid {}, volume {:.0f} of {:.0f} mm3\n",
        edge->name,
        edge->model_geometry_brep().face_count(),
        edge->model_geometry_brep().is_solid(),
        edge->model_geometry_brep().volume_exact(),
        edge->element_geometry_brep().volume_exact()
    );

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A variable edge with a hole: a drill is a joint, its interaction bores the rib.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_variable_holes --parallel 6 && ./build/element_edge_variable_holes && ../bash/publish-scene.sh --target element_edge_variable_holes

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
