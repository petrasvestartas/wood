#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Counter-running folding, ridge to valley : every warped strip folded along the diagonal through its low corners.
int main() {

    CounterFold fold(
        CounterFold::counter_running(4, 1500.0, 1500.0, 13500.0),
        Folding::ridge_to_valley,
        120.0,
        "ridge_to_valley"
    );
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("ridge to valley: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 28 of the folding templates: counter-running folding with ridge-to-valley folding: the stations of step 5, every warped strip folded along the diagonal through its two low corners, a valley running from the valley of one end to the valley of the other, so the zigzag of diagonals in the plan starts a strip later and the end elevation shows the pyramids's drawing: 16 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_28_ridge_to_valley --parallel 6 && ./build/templates_folding_28_ridge_to_valley && ../bash/publish-scene.sh --target templates_folding_28_ridge_to_valley

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
