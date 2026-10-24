#include "wood_session.h"
#include "src/templates/folding/diagonal_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Cross-folded surfaces spanned diagonally over a square plan, ridges folded upwardly (Engel, p. 229): the diagonal ridges folded up
/// at their middles, the corners and the centre level, the edges' middles on the ground.
int main() {

    DiagonalFold fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("cross fold: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 16 of the folding templates: cross-folded surfaces spanned diagonally over a square plan with the ridges folded upwardly after Engel, Structure Systems, Folded plate structures (p. 229). An 18 m square; the cross from the edges' middles to the centre divides it into four sectors, each a surface folded diagonally: the ridge from the corner to the centre is folded upward at its middle, 6200 high, while the corner and the centre are both 3300 high and the edges' middles stand on the ground, so every edge is a valley from corner to corner, the lines from middle to middle bend up over the ridges and the structure stands on the four middles; every sector is four planar triangles around its fold point: sixteen plates 120 mm thick, their edges on the plan's sides cut flush. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_16_cross_fold --parallel 6 && ./build/templates_folding_16_cross_fold && ../bash/publish-scene.sh --target templates_folding_16_cross_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
