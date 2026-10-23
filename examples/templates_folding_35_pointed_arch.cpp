#include "wood_session.h"
#include "src/templates/folding/folded_frame.h"

using namespace session_cpp;
using namespace wood_session;

/// The arch with a top hinge (Engel, p. 227): a tall arch 12 m wide and 11 m high, narrowing to hinges at the feet and the crown.
int main() {

    FoldedFrame fold(FoldedFrame::arch(12000.0, 11000.0, 6), 4, 2000.0, {300.0, 1500.0, 2000.0, 300.0, 2000.0, 1500.0, 300.0}, true, Folding::ridge_to_ridge, 120.0, "pointed_arch");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("pointed arch: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 35 of the folding templates: the arch with a hinge at the crown after Engel, Structure Systems, Folded plate structures (p. 227). The arch's inner line has six segments on a circular arc 12 m wide and 11 m high, steep like Engel's; the fold 300 deep at the feet and at the crown hinge, 1500 and 2000 towards the haunches, the phase turning at every corner, every warped strip folded ridge to ridge: 96 planar plates 120 mm thick. Each step is a group: frame, stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_35_pointed_arch --parallel 6 && ./build/templates_folding_35_pointed_arch && ../bash/publish-scene.sh --target templates_folding_35_pointed_arch

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
