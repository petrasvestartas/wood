#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Exact cylindrical dowel holes bored through an irregular scanned trunk.
int main() {

    WoodSession scene("element_edge_raw_holes");

    const std::shared_ptr<EdgeRawElement> log = EdgeRawElement::from_stock(
        Line::from_points({0.0, 0.0, 0.0}, {1800.0, 0.0, 0.0}), 55.0, "raw_log_0", 0.0, true, "log"
    );
    scene.add(log);

    for (size_t i = 0; i < 2; i++) {
        const double x = 550.0 + 700.0 * i;
        const std::shared_ptr<JointElement> hole = JointElement::drill(
            Line::from_points({x, 0.0, 110.0}, {x, 0.0, -110.0}), 22.0
        );
        hole->name = fmt::format("hole_{}", i);
        hole->is_visible = false;
        scene.add(hole);
        scene.add_interaction(hole, log, hole->interaction(0));
    }

    std::cout << fmt::format(
        "{}: {} faces, solid {}, volume {:.0f} of {:.0f} mm3\n",
        log->name,
        log->model_geometry_brep().face_count(),
        log->model_geometry_brep().is_solid(),
        log->model_geometry_brep().volume_exact(),
        log->element_geometry_brep().volume_exact()
    );

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Two dowel holes bored through a scanned raw spruce log: JointElement::drill cuts exact rational cylindrical NURBS bores across the irregular bark surface.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_holes --parallel 6 && ./build/element_edge_raw_holes && ../bash/publish-scene.sh --target element_edge_raw_holes

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
