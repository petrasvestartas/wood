#include "wood_session.h"
#include "src/templates/folding/fold_composition.h"

using namespace session_cpp;
using namespace wood_session;

/// A composition of surfaces folded crosswise over square plans .
int main() {

    FoldComposition fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("fold composition: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 19 of the folding templates: the composition of surfaces folded crosswise over square plans. Six by four cells of 12 m, 72 by 48 m; the field's nodes alternate like a chessboard between the ground and 4600, so every cell edge slopes from a high node to a low one and the field stands on its low nodes, and every cell is folded on its diagonals into four triangles to a peak 6500 over its centre, the zigzag's front elevation; the two by two block in the middle carries one cross fold spanned diagonally with its ridges folded upwardly, its fold points 15000 high, its corners the block's corner nodes raised to 8000 and its edges' middles the block's low nodes, so the cells around the block rise to it: eighty triangles and the block's sixteen, 96 plates 120 mm thick, their edges on the field's sides cut flush. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_19_fold_composition --parallel 6 && ./build/templates_folding_19_fold_composition && ../bash/publish-scene.sh --target templates_folding_19_fold_composition

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
