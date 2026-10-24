#include "wood_session.h"
#include "src/templates/folding/single_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Singly folded surfaces over a hexagonal plan, ridges sloping to the centre .
int main() {

    SingleFold fold(6, 12000.0, -2100.0, 8300.0, 5200.0, 120.0, "hexagon_fold");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("hexagon fold: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 13 of the folding templates: singly folded surfaces over a hexagonal plan with ridges sloping to the centre. A regular hexagon on a 12 m circumradius standing on its six corners; every edge's middle is pulled 2100 in from the edge and raised to 8300, the centre only 5200, so the six ridges slope down from the peaks to the centre and the eave triangles from the edges on the ground lean inward, seen in the plan; every sector is two planar triangles folded on its ridge: eighteen plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_13_hexagon_fold --parallel 6 && ./build/templates_folding_13_hexagon_fold && ../bash/publish-scene.sh --target templates_folding_13_hexagon_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
