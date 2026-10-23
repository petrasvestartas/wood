#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Counter-running folding with the centre fold elevated over the edge folds, identical depth, ridge to ridge (Engel, p. 223): the
/// zigzag at both ends of an 18 m span, its counter profile at mid-span 3 m higher.
int main() {

    CounterFold fold(CounterFold::counter_centre(5, 1500.0, 1500.0, 18000.0, 3000.0, 1500.0), Folding::ridge_to_ridge, 240.0, "elevated_fold");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("elevated fold: {} stations, {} plates\n", fold.stations.size(), plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 6 of the folding templates: a surface with counter-running folding after Engel, Structure Systems, Folded plate structures (p. 223), the centre fold elevated over the edge folds, identical depth of the profiles. A zigzag of five ridges, 1500 across and 1500 deep, at both ends of an 18 m span, and its counter profile at mid-span, raised 3000, so the folds rise to the middle like a gable; every warped strip folds ridge to ridge into two planar 240 mm triangles, forty plates. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_6_elevated_fold --parallel 6 && ./build/templates_folding_6_elevated_fold && ../bash/publish-scene.sh --target templates_folding_6_elevated_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
