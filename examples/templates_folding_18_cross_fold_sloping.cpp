#include "wood_session.h"
#include "src/templates/folding/diagonal_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Cross-folded surfaces spanned diagonally over a square plan, ridges sloping toward the centre (Engel, p. 229): the corners the
/// highest, every diagonal ridge folded on its way down to the centre, the edges' middles on the ground.
int main() {

    DiagonalFold fold(18000.0, {7000.0, 0.0, 4300.0, 0.43, 3500.0}, 120.0, "cross_fold_sloping");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("cross fold sloping: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 18 of the folding templates: cross-folded surfaces spanned diagonally over a square plan with the ridges sloping toward the centre after Engel, Structure Systems, Folded plate structures (p. 229). An 18 m square whose corners are the highest points, 7000; every diagonal ridge drops steeply to a fold point less than half way along it, 4300 high, and on gently to the centre at 3500, the edges' middles on the ground carrying the structure, so every edge is a deep valley from corner to corner and the chevrons of Engel's plan point to the corners; every sector is four planar triangles: sixteen plates 120 mm thick, their edges on the plan's sides cut flush. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_18_cross_fold_sloping --parallel 6 && ./build/templates_folding_18_cross_fold_sloping && ../bash/publish-scene.sh --target templates_folding_18_cross_fold_sloping

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
