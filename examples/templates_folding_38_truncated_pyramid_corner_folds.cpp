#include "wood_session.h"
#include "src/templates/folding/truncated_pyramid.h"

using namespace session_cpp;
using namespace wood_session;

/// The second variation of the doubly folded truncated pyramid (Engel, p. 233): every corner line bent at a raised point near the
/// corner, every side fanning out from the middle of its base edge on the ground.
int main() {

    TruncatedPyramid fold(9000.0, 3000.0, 6800.0, FoldPoint{0.07, 0.0}, FoldPoint{0.22, 0.5}, 120.0, "truncated_pyramid_corner_folds");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("truncated pyramid corner folds: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 38 of the folding templates: the second variation for folding a given basic structure form after Engel, Structure Systems, Folded plate structures (p. 233). The truncated pyramid of step 15, an 18 m square with a 6 m top 6800 high; every corner line from a base corner to a top corner is bent at a point a fifth of the way along it raised to half the height, and the middle of every base edge stays on the ground, pulled in a little, so every side fans out from that middle into five triangles over its two bent corner lines and the top edge, its base edge rising from the middle to the corners' bends and down to the corners, the kinked silhouette and the V to the ground of Engel's elevations: twenty triangles and the top, 21 plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_38_truncated_pyramid_corner_folds --parallel 6 && ./build/templates_folding_38_truncated_pyramid_corner_folds && ../bash/publish-scene.sh --target templates_folding_38_truncated_pyramid_corner_folds

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
