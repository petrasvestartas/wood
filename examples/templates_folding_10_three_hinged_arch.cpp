#include "wood_session.h"
#include "src/templates/folding/folded_frame.h"

using namespace session_cpp;
using namespace wood_session;

/// The three-hinged arch of Engel's folded plate structures (p. 227): six segments of a 24 m arch 9 m high, its folds deepest at the
/// haunches and narrowing to hinges at the feet and the crown.
int main() {

    FoldedFrame frame(FoldedFrame::arch(24000.0, 9000.0, 6), 4, 2000.0, {300.0, 1800.0, 2400.0, 300.0, 2400.0, 1800.0, 300.0});
    const std::vector<std::shared_ptr<Plate>> plates = frame.get_elements_numbered<Plate>("plate");
    std::cout << frame << std::endl;
    std::cout << fmt::format("three-hinged arch: {} stations, {} plates\n", frame.frame.point_count(), plates.size());
    frame.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 10 of the folding templates: the three-hinged arch after Engel, Structure Systems, Folded plate structures (p. 227). The arch's inner line has six segments with their corners on a circular arc 24 m wide and 9 m high; at every corner a zigzag of four ridges, plates 2000 across, stands in the plane halving the corner, 300 deep at the feet and the crown, the three hinges, 1800 and 2400 towards the haunches; the phase turning at every corner, every strip folds ridge to ridge into two planar 480 mm triangles: 96 plates. Each step is a group: frame, stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_10_three_hinged_arch --parallel 6 && ./build/templates_folding_10_three_hinged_arch && ../bash/publish-scene.sh --target templates_folding_10_three_hinged_arch

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
