#include "wood_session.h"
#include "src/templates/folding/folded_frame.h"

using namespace session_cpp;
using namespace wood_session;

/// The three-hinged arch of Engel's folded plate structures (p. 227): an octagonal arch 21.5 m wide and 20.5 m high on point
/// feet, hinged at the crown.
int main() {

    FoldedFrame frame(FoldedFrame::three_hinged_arch(), 5000.0, 5, 120.0, "three_hinged_arch");
    const std::vector<std::shared_ptr<Plate>> plates = frame.get_elements_numbered<Plate>("plate");
    std::cout << frame << std::endl;
    std::cout << fmt::format("three hinged arch: {} plates\n", plates.size());
    frame.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 10 of the folding templates: the three-hinged arch after Engel, Structure Systems, Folded plate structures (p. 227). Five 5 m bays over a 21.5 m span, 20.5 m high at Engel's proportions: from a point foot at the bay's centre the fold line rises to the ankle's level line at 0.29 of the height, the knee bulging out 0.23 of the span, the haunch's valley at 0.59 and its ridge at 0.67, the crown's corners over 0.27 of the span and the hinge at mid-span, where ridge and valley meet in one line along the bays; the diagonals from the crown corners cross at the mid-span bay edges, the big X of Engel's plan: 140 planar plates 120 mm thick. Each step is a group: zigzag, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_10_three_hinged_arch --parallel 6 && ./build/templates_folding_10_three_hinged_arch && ../bash/publish-scene.sh --target templates_folding_10_three_hinged_arch

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
