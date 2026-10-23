#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Counter-running folding, ridge to valley (Engel, p. 222): every warped strip folded along the diagonal through its low corners.
int main() {

    CounterFold fold(CounterFold::counter_running(5, 1500.0, 1500.0, 18000.0), Folding::ridge_to_valley, 240.0, "ridge_to_valley");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("ridge to valley: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 28 of the folding templates: counter-running folding of the same depth and elevation, ridge-to-valley folding, after Engel, Structure Systems, Folded plate structures (p. 222). The stations of step 5, a zigzag of five ridges at the start and its counter profile at the end of an 18 m span, but every warped strip folded along the diagonal through its two low corners, a valley running diagonally from a valley of one end to a valley of the other: 20 planar plates 240 mm thick. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_28_ridge_to_valley --parallel 6 && ./build/templates_folding_28_ridge_to_valley && ../bash/publish-scene.sh --target templates_folding_28_ridge_to_valley

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
