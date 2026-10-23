#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The centre fold elevated over the edge folds, alternate folding (Engel, p. 223): ridge to ridge and ridge to valley strip by strip.
int main() {

    CounterFold fold(CounterFold::counter_centre(5, 1500.0, 1500.0, 18000.0, 3000.0, 1500.0), Folding::alternate, 240.0, "alternate_fold");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("alternate fold: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 30 of the folding templates: counter-running folding with the centre fold elevated, alternate folding (Wechselfaltung), after Engel, Structure Systems, Folded plate structures (p. 223). The stations of step 6, the warped strips folded ridge to ridge and ridge to valley in turn: 40 planar plates 240 mm thick. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_30_alternate_fold --parallel 6 && ./build/templates_folding_30_alternate_fold && ../bash/publish-scene.sh --target templates_folding_30_alternate_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
