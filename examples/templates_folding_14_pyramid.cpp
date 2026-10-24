#include "wood_session.h"
#include "src/templates/folding/truncated_pyramid.h"

using namespace session_cpp;
using namespace wood_session;

/// The pyramidal folded plate (Engel, p. 231): four triangles over a square plan rising to an apex, each pair of opposite faces
/// stiffening the other pair.
int main() {

    TruncatedPyramid fold(9000.0, 0.0, 6000.0, std::nullopt, std::nullopt, 120.0, "pyramid");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("pyramid: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 14 of the folding templates: the pyramidal folded plate after Engel, Structure Systems, Folded plate structures (p. 231): over an 18 m square on the ground four triangular plates rise to an apex at 6000, each acting as a slab, as a plate and with its neighbours as a truss, each pair of opposite faces stiffening the other pair: four plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_14_pyramid --parallel 6 && ./build/templates_folding_14_pyramid && ../bash/publish-scene.sh --target templates_folding_14_pyramid

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
