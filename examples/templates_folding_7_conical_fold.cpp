#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Conical folding, a continuous fold profile with its upper edge cut by sloping planes (Engel, p. 224): one zigzag of five ridges over
/// an 18 m span, its valleys level, its ridges 600 high at the ends and 1800 at mid-span.
int main() {

    CounterFold fold(CounterFold::conical(5, 1500.0, 600.0, 18000.0, 1800.0), Folding::ridge_to_ridge, 480.0, "conical_fold");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("conical fold: {} stations, {} plates\n", fold.stations.size(), plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 7 of the folding templates: a surface with conical folding after Engel, Structure Systems, Folded plate structures (p. 224), a continuous fold profile with its upper edge cut by sloping planes. One zigzag of five ridges, 1500 across, over an 18 m span, its valleys level, its ridges 600 high at both ends and 1800 at mid-span; a strip between a level valley and a sloping ridge is warped, so it folds along its diagonal into two planar 480 mm triangles, the plates fanning out from the high ridges at mid-span: forty plates. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_7_conical_fold --parallel 6 && ./build/templates_folding_7_conical_fold && ../bash/publish-scene.sh --target templates_folding_7_conical_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
