#include "wood_session.h"
#include "src/templates/folding/truncated_pyramid.h"

using namespace session_cpp;
using namespace wood_session;

/// The first variation of the doubly folded truncated pyramid (Engel, p. 233): every side folded around the middle of its base edge,
/// pulled in and raised half way, a steep triangle below it and a fan of three above.
int main() {

    TruncatedPyramid fold(9000.0, 3000.0, 6800.0, FoldPoint{0.21, 0.54}, std::nullopt, 120.0, "folded_truncated_pyramid");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("folded truncated pyramid: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 36 of the folding templates: the first variation for folding a given basic structure form after Engel, Structure Systems, Folded plate structures (p. 233). The truncated pyramid of step 15, an 18 m square with a 6 m top 6800 high; the middle of every base edge is pulled in a fifth of the side's depth and raised to 54 percent of the height, so every side fans out from it into three triangles to its corner lines and the top edge, and a steep fourth triangle leans from the base edge on the ground up to it, the X of Engel's elevations: sixteen triangles and the top, seventeen plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_36_folded_truncated_pyramid --parallel 6 && ./build/templates_folding_36_folded_truncated_pyramid && ../bash/publish-scene.sh --target templates_folding_36_folded_truncated_pyramid

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
