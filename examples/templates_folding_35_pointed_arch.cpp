#include "wood_session.h"
#include "src/templates/folding/folded_frame.h"

using namespace session_cpp;
using namespace wood_session;

/// The arch with a top hinge (Engel, p. 227): nearly vertical legs 26 m high, the ridge peaking either side of the hinge at
/// mid-span.
int main() {

    FoldedFrame frame(FoldedFrame::top_hinged_arch(), 4950.0, 4, 120.0, "pointed_arch");
    const std::vector<std::shared_ptr<Plate>> plates = frame.get_elements_numbered<Plate>("plate");
    std::cout << frame << std::endl;
    std::cout << fmt::format("pointed arch: {} plates\n", plates.size());
    frame.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 35 of the folding templates: the arch with a hinge at the crown after Engel, Structure Systems, Folded plate structures (p. 227). Four 4.95 m bays over a 17.9 m span, 26.1 m high at Engel's proportions: from a point foot at the bay's centre a narrow trunk rises to its valley at a third of the height and its knee at 0.57, the haunch's valley at 0.885 of the height a fifth of the span in, the ridge peaks over 0.36 of the span with its valley straight below it, and dips to the hinge at mid-span, 0.93 of the height, where the two halves meet on the level valleys: 96 planar plates 120 mm thick. Each step is a group: zigzag, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_35_pointed_arch --parallel 6 && ./build/templates_folding_35_pointed_arch && ../bash/publish-scene.sh --target templates_folding_35_pointed_arch

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
