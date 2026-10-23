#include "wood_session.h"
#include "src/templates/folding/polygon_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Interpenetrating folded surfaces over a hexagonal plan, ridges sloping to the centre (Engel, p. 228): six sectors, the ridges
/// highest at the edge and falling inwards.
int main() {

    PolygonFold fold(6, 12000.0, {0.0, 3600.0, 1500.0, 3000.0, 900.0, 2400.0}, 600.0, false, 240.0, 0.0, "hexagon_fold");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("hexagon fold: {} sides, {} plates\n", fold.sides, plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 13 of the folding templates: singly folded surfaces over a hexagonal plan with ridges sloping to the centre after Engel, Structure Systems, Folded plate structures (p. 228). The plan a regular hexagon on a 12 m circumradius; six concentric rings, valleys and ridges at 0, 3600, 1500, 3000, 900 and 2400, falling towards a centre at 600, every band a planar trapezoid, the six sectors meeting along the lines from the corners to the centre: 36 plates 240 mm thick. Each step is a group: rings, mesh, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_13_hexagon_fold --parallel 6 && ./build/templates_folding_13_hexagon_fold && ../bash/publish-scene.sh --target templates_folding_13_hexagon_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
