#include "wood_session.h"
#include "src/templates/folding/folded_frame.h"

using namespace session_cpp;
using namespace wood_session;

/// The two-hinged frame, ridge-to-valley folding (Engel, p. 225).
int main() {

    FoldedFrame fold(FoldedFrame::portal(30000.0, 5000.0, 2), 4, 2000.0, {300.0, 2000.0, 900.0, 3200.0, 900.0, 2000.0, 300.0}, true, Folding::ridge_to_valley, 120.0, "frame_ridge_to_valley");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("frame ridge to valley: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 33 of the folding templates: the two-hinged frame with ridge-to-valley folding after Engel, Structure Systems, Folded plate structures (p. 225): the portal and depths of step 8, every warped strip folded along its low diagonal, so valleys run diagonally across the frame: 96 planar plates 120 mm thick, flush along both sides. Each step is a group: frame, stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_33_frame_ridge_to_valley --parallel 6 && ./build/templates_folding_33_frame_ridge_to_valley && ../bash/publish-scene.sh --target templates_folding_33_frame_ridge_to_valley

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
