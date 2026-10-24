#include "wood_session.h"
#include "src/templates/folding/diagonal_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Cross-folded surfaces spanned diagonally over a square plan, ridges rising toward the centre (Engel, p. 229): the edges' middles
/// on the ground, every diagonal ridge rising from its corner and folded two thirds of the way up to the apex.
int main() {

    DiagonalFold fold(18000.0, {3100.0, 0.0, 4900.0, 0.67, 9000.0}, 120.0, "cross_fold_rising");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("cross fold rising: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 17 of the folding templates: cross-folded surfaces spanned diagonally over a square plan with the ridges rising toward the centre after Engel, Structure Systems, Folded plate structures (p. 229). An 18 m square standing on its four edges' middles; every diagonal ridge rises from its corner, 3100 high, to a fold point two thirds of the way along it, 4900 high, and on more steeply to the apex at 9000, so every edge is a valley from corner to corner, the lines from middle to middle fold over the ridges, the chevrons of Engel's plan pointing to the centre, and his front elevation's straight slope from the foot to the apex is the line from the middle to the centre; every sector is four planar triangles: sixteen plates 120 mm thick, their edges on the plan's sides cut flush. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_17_cross_fold_rising --parallel 6 && ./build/templates_folding_17_cross_fold_rising && ../bash/publish-scene.sh --target templates_folding_17_cross_fold_rising

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
