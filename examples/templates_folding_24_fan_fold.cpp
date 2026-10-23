#include "wood_session.h"
#include "src/templates/folding/fan_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Cross-folded surfaces with ridges rising toward the centre (Engel, p. 229): the cross divides an 18 m square into quarters, and in
/// each fold lines fan out from its corner to points along the cross and to the centre.
int main() {

    FanFold fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("fan fold: {} divisions, {} plates\n", fold.divisions, plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 24 of the folding templates: cross-folded surfaces over a square plan with ridges rising toward the centre after Engel, Structure Systems, Folded plate structures (p. 229). The cross from the edges' midpoints to the centre divides an 18 m square into four quarters; in every quarter straight fold lines fan out from its corner on the supports at 0 to three points along each half cross line and to the centre, the diagonal. The cross rises from the midpoints at 600 to the centre at 3000, every second point raised 1800 more, so the fan's lines alternate ridge and valley and the cross folds too, shared by the quarters beside it; every face between two lines from one corner is a planar triangle: 32 plates 240 mm thick, their corners cut back by the thickness. Each step is a group: folds, mesh, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_24_fan_fold --parallel 6 && ./build/templates_folding_24_fan_fold && ../bash/publish-scene.sh --target templates_folding_24_fan_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
