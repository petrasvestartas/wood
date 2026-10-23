#include "wood_session.h"
#include "src/templates/folding/cross_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// A composition of surfaces folded crosswise over square plans (Engel, p. 230): four by three units of the waterbomb fold, 6 m
/// each, side by side, their shared edges welded.
int main() {

    CrossFold fold(6000.0, 1, 0.0, 1200.0, 1800.0, 120.0, 4, 3, "fold_composition");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("fold composition: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 19 of the folding templates: the composition of surfaces folded crosswise over square plans after Engel, Structure Systems, Folded plate structures (p. 230). Twelve units of the waterbomb fold of step 16, 6 m squares in four columns and three rows, 24 by 18 m; in each the diagonals are valleys ending on the corners at 0 and the cross ridges ending at the edges' midpoints at 1200, rising to the unit's centre at 1800; the units share their edge points, so two units' plates meet mitered along their common edge: 96 planar triangles 120 mm thick, their corners cut back only where needed. Each step is a group: folds, mesh, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_19_fold_composition --parallel 6 && ./build/templates_folding_19_fold_composition && ../bash/publish-scene.sh --target templates_folding_19_fold_composition

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
