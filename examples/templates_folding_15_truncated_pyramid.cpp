#include "wood_session.h"
#include "src/templates/folding/truncated_pyramid.h"

using namespace session_cpp;
using namespace wood_session;

/// The doubly folded truncated pyramid.
int main() {

    TruncatedPyramid fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("truncated pyramid: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 15 of the folding templates: the doubly folded truncated pyramid, the basic structure form his variations fold further. An 18 m square on the ground, four trapezoids rising to a flat 6 m square top 6800 high, as his plan and perspective show: four trapezoids and the top, five plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_15_truncated_pyramid --parallel 6 && ./build/templates_folding_15_truncated_pyramid && ../bash/publish-scene.sh --target templates_folding_15_truncated_pyramid

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
