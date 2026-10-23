#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The elevated centre profile deeper than the edge profiles, ridge to ridge (Engel, p. 224).
int main() {

    CounterFold fold(CounterFold::counter_centre(5, 1500.0, 1000.0, 18000.0, 2000.0, 2500.0), Folding::ridge_to_ridge, 120.0, "deep_centre");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("deep centre: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 31 of the folding templates: counter-running folding with an elevated centre profile whose depth is larger than that of the edge profiles, ridge-to-ridge folding, after Engel, Structure Systems, Folded plate structures (p. 224). A zigzag of five ridges 1000 deep at both ends of an 18 m span, its counter profile at mid-span raised 2000 and 2500 deep, every warped strip folded ridge to ridge: 40 planar plates 120 mm thick. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_31_deep_centre --parallel 6 && ./build/templates_folding_31_deep_centre && ../bash/publish-scene.sh --target templates_folding_31_deep_centre

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
