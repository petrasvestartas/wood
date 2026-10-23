#include "wood_session.h"
#include "src/templates/folding/cross_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The waterbomb fold of a square, cross-folded over a square plan (Engel, p. 229): the diagonals valleys, the cross ridges, eight
/// planar triangles meeting at the raised centre.
int main() {

    CrossFold fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("cross fold: {} divisions, {} plates\n", fold.divisions, plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 16 of the folding templates: a cross-folded surface over a square plan after Engel, Structure Systems, Folded plate structures (p. 229), the waterbomb fold of a square sheet. An 18 m square, fold lines from the centre to its four corners, the diagonals, valleys ending at 0, and to its edges' midpoints, the cross, ridges ending at 3000, all meeting at the centre 6000 high, the ridges rising toward it; every face between two straight fold lines through the centre is a planar triangle: eight plates 120 mm thick, their corners cut back only where needed so the eight meeting at the centre do not pass through each other. Each step is a group: folds, mesh, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_16_cross_fold --parallel 6 && ./build/templates_folding_16_cross_fold && ../bash/publish-scene.sh --target templates_folding_16_cross_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
