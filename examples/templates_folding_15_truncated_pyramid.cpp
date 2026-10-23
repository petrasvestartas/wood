#include "wood_session.h"
#include "src/templates/folding/polygon_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The doubly folded truncated pyramid, the basic structure form of Engel's variations (p. 233): a square plan folded in rings,
/// closed by a flat top.
int main() {

    PolygonFold fold(4, 12000.0, {0.0, 1800.0, 900.0, 3000.0}, 0.0, true, 240.0, 0.0, "truncated_pyramid");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("truncated pyramid: {} sides, {} plates\n", fold.sides, plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 15 of the folding templates: the doubly folded truncated pyramid after Engel, Structure Systems, Folded plate structures (p. 233), the basic structure form his variations fold further. A square plan on a 12 m circumradius; four rings at 0, 1800, 900 and 3000, folded in a valley and a ridge, the innermost ring, a quarter of the plan, closed by a flat top: twelve trapezoids and the top, 13 plates 240 mm thick. Each step is a group: rings, mesh, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_15_truncated_pyramid --parallel 6 && ./build/templates_folding_15_truncated_pyramid && ../bash/publish-scene.sh --target templates_folding_15_truncated_pyramid

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
