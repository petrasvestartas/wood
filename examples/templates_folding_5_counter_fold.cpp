#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Counter-running folding of the same depth and elevation, ridge to ridge (Engel, p. 222): a zigzag of five ridges at the start,
/// its counter profile at the end of an 18 m span, every warped strip folded through its high corners into two planar triangles.
int main() {

    CounterFold fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("counter fold: {} stations, {} plates\n", fold.stations.size(), plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 5 of the folding templates: a surface with counter-running folding after Engel, Structure Systems, Folded plate structures (p. 222), same depth of fold profiles and same elevation above ground. A zigzag of five ridges, ten plates each 1500 across in plan and 1500 deep, 15 m wide, at the start of an 18 m span, and its counter profile, ridges where the start has its valleys, at the end; their points joined by straight fold lines, every strip between two fold lines is warped and folds ridge to ridge, along the diagonal through its two high corners, into two planar 240 mm plates: twenty triangles, the ridges running diagonally from the ridges of one end to the ridges of the other. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_5_counter_fold --parallel 6 && ./build/templates_folding_5_counter_fold && ../bash/publish-scene.sh --target templates_folding_5_counter_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
