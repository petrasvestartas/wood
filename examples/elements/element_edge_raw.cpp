#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

/// A scanned raw spruce trunk aligned along an axis.
int main() {

    WoodSession scene("element_edge_raw");

    const std::shared_ptr<EdgeRawElement> log = EdgeRawElement::from_stock(
        Line::from_points({0.0, 0.0, 0.0}, {1800.0, 0.0, 0.0}), 55.0, "raw_log_1", 20.0, true, "log"
    );
    scene.add(log);

    std::cout << fmt::format(
        "{}: {} faces, {} scanned points, solid {}, volume {:.0f} mm3\n",
        log->name,
        log->model_geometry_brep().face_count(),
        log->raw_pointcloud.point_count(),
        log->model_geometry_brep().is_solid(),
        log->model_geometry_brep().volume_exact()
    );

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A raw spruce trunk along a straight axis: EdgeRawElement::from_stock loads the scanned mesh and RGB point cloud from data/raw/ and places them onto the beam's base frame by raw_xform.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_edge_raw --parallel 6 && ./build/element_edge_raw && ../bash/publish-scene.sh --target element_edge_raw

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
