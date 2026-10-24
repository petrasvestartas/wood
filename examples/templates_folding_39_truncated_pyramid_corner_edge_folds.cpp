#include "wood_session.h"
#include "src/templates/folding/truncated_pyramid.h"

using namespace session_cpp;
using namespace wood_session;

/// The third variation of the doubly folded truncated pyramid .
int main() {

    TruncatedPyramid fold(
        9000.0,
        3000.0,
        6800.0,
        FoldPoint{0.28, 0.5},
        FoldPoint{0.22, 0.5},
        120.0,
        "truncated_pyramid_corner_edge_folds"
    );
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format(
        "truncated pyramid corner and edge folds: {}\n", plate_report(plates, fold.thickness)
    );
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 39 of the folding templates: the third variation for folding a given basic structure form. The truncated pyramid of step 15, an 18 m square with a 6 m top 6800 high; every corner line is bent at a point a fifth of the way along it raised to half the height, and the middle of every base edge is pulled in over a quarter of the side's depth and raised to the same half height, so a level fold runs from bend to middle to bend around the pyramid, every side fans out from its middle into five triangles above that fold and a steep sixth leans from the base edge on the ground up to it, the horizontal line through the X's elevations: 24 triangles and the top, 25 plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_39_truncated_pyramid_corner_edge_folds --parallel 6 && ./build/templates_folding_39_truncated_pyramid_corner_edge_folds && ../bash/publish-scene.sh --target templates_folding_39_truncated_pyramid_corner_edge_folds

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
