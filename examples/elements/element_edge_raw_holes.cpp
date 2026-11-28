#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

/// Exact cylindrical pin bores drilled orthogonally and obliquely across an irregular scanned trunk.
int main() {

    WoodSession scene("element_edge_raw_holes");

    const std::shared_ptr<EdgeRawElement> log = EdgeRawElement::from_stock(
        Line::from_points({0.0, 0.0, 0.0}, {1650.0, 0.0, 0.0}), 55.0, "raw_log_0", 15.0, true, "log"
    );
    scene.add(log);

    const std::array<Line, 3> bores = {
        Line::from_points({420.0, 0.0, 110.0}, {420.0, 0.0, -110.0}),
        Line::from_points({825.0, -110.0, 0.0}, {825.0, 110.0, 0.0}),
        Line::from_points({1170.0, 0.0, 110.0}, {1290.0, 0.0, -110.0}),
    };
    for (size_t i = 0; i < bores.size(); i++) {
        const std::shared_ptr<JointElement> hole = JointElement::drill(bores[i], 22.0);
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
Vertical, horizontal and inclined pin holes bored through a scanned raw spruce log: JointElement::drill cuts exact rational cylindrical NURBS bores across the irregular bark surface.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw_holes --parallel 6 && ./build/element_edge_raw_holes && ../bash/publish-scene.sh --target element_edge_raw_holes

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
