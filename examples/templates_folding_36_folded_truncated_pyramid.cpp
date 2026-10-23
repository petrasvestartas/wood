#include "wood_session.h"
#include "src/templates/folding/polygon_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// A variation of the doubly folded truncated pyramid (Engel, p. 233): every side trapezoid folded into three triangles from its outer
/// edge's raised midpoint.
int main() {

    PolygonFold fold(4, 12000.0, {0.0, 3000.0}, 0.0, true, 120.0, 1500.0, "folded_truncated_pyramid");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("folded truncated pyramid: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 36 of the folding templates: a variation for folding a given basic structure form after Engel, Structure Systems, Folded plate structures (p. 233). The truncated pyramid over a square on a 12 m circumradius, its top 3000 high; every side trapezoid folds into three planar triangles from the midpoint of its outer edge, raised 1500, a fold running from it to both top corners: twelve triangles and the flat top, 13 plates 120 mm thick. Each step is a group: rings, mesh, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_36_folded_truncated_pyramid --parallel 6 && ./build/templates_folding_36_folded_truncated_pyramid && ../bash/publish-scene.sh --target templates_folding_36_folded_truncated_pyramid

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
