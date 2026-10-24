#include "wood_session.h"
#include "src/templates/folding/folded_frame.h"

using namespace session_cpp;
using namespace wood_session;

/// The three-hinged frame, ridge folding.
int main() {

    FoldedFrame frame(FoldedFrame::three_hinged_frame(), 4250.0, 4, 120.0, "three_hinged_frame");
    const std::vector<std::shared_ptr<Plate>> plates = frame.get_elements_numbered<Plate>("plate");
    std::cout << frame << std::endl;
    std::cout << fmt::format("three hinged frame: {} plates\n", plates.size());
    frame.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 9 of the folding templates: the three-hinged frame with ridge folding. A 30 m portal of four 4.25 m bays on point feet at the bay centres, 6250 to the eaves, the knee bulging out 1900 at half the column; the roof rises from the eaves to its ridge 8250 high over the quarter points of the span and dips to the hinge 6800 high at mid-span over a flat ceiling. The fold line runs foot, eaves corner, knee, quarter-point ridge, mid-span ceiling corner, hinge, so the diagonals from the eaves corners cross at the quarter-point ridge, the star's plan, and the diamonds of his side elevation stand on the knees: 64 planar plates 120 mm thick. Each step is a group: zigzag, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_9_three_hinged_frame --parallel 6 && ./build/templates_folding_9_three_hinged_frame && ../bash/publish-scene.sh --target templates_folding_9_three_hinged_frame

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
