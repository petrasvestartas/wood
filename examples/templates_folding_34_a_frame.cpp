#include "wood_session.h"
#include "src/templates/folding/folded_frame.h"

using namespace session_cpp;
using namespace wood_session;

/// The two-hinged A-frame, ridge-to-valley folding (Engel, p. 226): two legs 18 m apart meeting 12 m high, deepest at mid-leg.
int main() {

    FoldedFrame fold(Polyline({Point(0.0, 0.0, 0.0), Point(4500.0, 0.0, 6000.0), Point(9000.0, 0.0, 12000.0), Point(13500.0, 0.0, 6000.0), Point(18000.0, 0.0, 0.0)}), 4, 2000.0, {300.0, 2400.0, 600.0, 2400.0, 300.0}, true, Folding::ridge_to_valley, 240.0, "a_frame");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("a frame: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 34 of the folding templates: the two-hinged A-frame with ridge-to-valley folding after Engel, Structure Systems, Folded plate structures (p. 226). The frame's inner line two straight legs from supports 18 m apart to an apex 12 m high, each with a corner at mid-length; the fold 300 deep at the hinged feet, 2400 at mid-leg and 600 at the apex, the phase turning at every corner, every warped strip folded ridge to valley: 64 planar plates 240 mm thick. Each step is a group: frame, stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_34_a_frame --parallel 6 && ./build/templates_folding_34_a_frame && ../bash/publish-scene.sh --target templates_folding_34_a_frame

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
