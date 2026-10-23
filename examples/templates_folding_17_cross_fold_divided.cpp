#include "wood_session.h"
#include "src/templates/folding/cross_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The cross fold subdivided from the four corners (Engel, p. 229): every half edge divided into three, 24 fold lines alternating
/// valley and ridge from the corners, 24 planar triangles.
int main() {

    CrossFold fold(18000.0, 3, 0.0, 3000.0, 6000.0, 120.0, 1, 1, "cross_fold_divided");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("cross fold divided: {} divisions, {} plates\n", fold.divisions, plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 17 of the folding templates: the cross fold of step 16 subdivided from the four corners after Engel, Structure Systems, Folded plate structures (p. 229). Every half edge between a corner and a midpoint is divided into three, so 24 straight fold lines run from the centre to the edge, alternating valley at 0 and ridge at 3000 from every corner, the diagonals valleys and the cross ridges; every face between two of them is a planar triangle: 24 plates 120 mm thick. Each step is a group: folds, mesh, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_17_cross_fold_divided --parallel 6 && ./build/templates_folding_17_cross_fold_divided && ../bash/publish-scene.sh --target templates_folding_17_cross_fold_divided

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
