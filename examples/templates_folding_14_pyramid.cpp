#include "wood_session.h"
#include "src/templates/folding/polygon_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The pyramidal folded plate (Engel, p. 231): four triangles over a square plan rising to an apex, each pair of opposite faces
/// stiffening the other pair.
int main() {

    PolygonFold fold(4, 9000.0, {0.0}, 6000.0, false, 120.0, 0.0, "pyramid");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("pyramid: {} sides, {} plates\n", fold.sides, plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 14 of the folding templates: the pyramidal folded plate after Engel, Structure Systems, Folded plate structures (p. 231): over a square plan on a 9 m circumradius four triangular plates rise to an apex at 6000, each acting as a slab, as a plate and with its neighbours as a truss, each pair of opposite faces stiffening the other pair: four plates 120 mm thick. Each step is a group: rings, mesh, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_14_pyramid --parallel 6 && ./build/templates_folding_14_pyramid && ../bash/publish-scene.sh --target templates_folding_14_pyramid

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
