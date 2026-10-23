#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The elevated centre profile deeper than the edge profiles, ridge to valley (Engel, p. 224).
int main() {

    CounterFold fold(CounterFold::counter_centre(5, 1500.0, 1000.0, 18000.0, 2000.0, 2500.0), Folding::ridge_to_valley, 240.0, "deep_centre_valley");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("deep centre valley: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 32 of the folding templates: the stations of step 31 after Engel, Structure Systems, Folded plate structures (p. 224), every warped strip folded ridge to valley along its low diagonal: 40 planar plates 240 mm thick. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_32_deep_centre_valley --parallel 6 && ./build/templates_folding_32_deep_centre_valley && ../bash/publish-scene.sh --target templates_folding_32_deep_centre_valley

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
