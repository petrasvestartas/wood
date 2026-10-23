#include "wood_session.h"
#include "src/templates/folding/folded_frame.h"

using namespace session_cpp;
using namespace wood_session;

/// The two-hinged frame, ridge-to-valley folding (Engel, p. 225): the same portal on point feet, its roof folded from the knee to
/// the valleys at mid-span.
int main() {

    FoldedFrame frame(FoldedFrame::two_hinged_frame(30000.0, 4500.0, 5800.0, 6600.0, 8750.0, 2900.0, 2200.0, Folding::ridge_to_valley), 4500.0, 4, 120.0, "frame_ridge_to_valley");
    const std::vector<std::shared_ptr<Plate>> plates = frame.get_elements_numbered<Plate>("plate");
    std::cout << frame << std::endl;
    std::cout << fmt::format("frame ridge to valley: {} plates\n", plates.size());
    frame.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 33 of the folding templates: the two-hinged frame with ridge-to-valley folding after Engel, Structure Systems, Folded plate structures (p. 225). The portal of step 8 with the other fold line: from a point foot at the bay's centre up to the eaves corner and out to the knee, then from the knee straight to the ceiling peak at mid-span, the valley, and up to the ridge top, so every column is two plates on a point and the roof a fan of four plates around the knee, the lines of Engel's plan from the knee to the mid-span corners: 48 planar plates 120 mm thick, flush along both sides. Each step is a group: zigzag, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_33_frame_ridge_to_valley --parallel 6 && ./build/templates_folding_33_frame_ridge_to_valley && ../bash/publish-scene.sh --target templates_folding_33_frame_ridge_to_valley

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
