#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Counter-running folding with the counter profile in the centre, ridge to ridge (Engel, p. 222): the zigzag at both ends, its counter
/// profile at mid-span, same depth and elevation.
int main() {

    CounterFold fold(CounterFold::counter_centre(5, 1500.0, 1500.0, 18000.0, 0.0, 1500.0), Folding::ridge_to_ridge, 120.0, "counter_centre");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("counter centre: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 27 of the folding templates: counter-running folding with the counter profile in the centre after Engel, Structure Systems, Folded plate structures (p. 222). A zigzag of five ridges, 1500 across and deep, at both ends of an 18 m span, its counter profile at mid-span at the same elevation, every warped strip folded ridge to ridge into two planar triangles: 40 plates 120 mm thick. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_27_counter_centre --parallel 6 && ./build/templates_folding_27_counter_centre && ../bash/publish-scene.sh --target templates_folding_27_counter_centre

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
