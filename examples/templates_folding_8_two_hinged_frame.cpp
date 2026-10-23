#include "wood_session.h"
#include "src/templates/folding/folded_frame.h"

using namespace session_cpp;
using namespace wood_session;

/// The two-hinged frame of Engel's folded plate structures, ridge-to-ridge folding (p. 225): a 30 m portal 5 m high,
/// its folds deepest at the knees of the columns and at mid-span, the phase turning at every corner.
int main() {

    FoldedFrame frame;
    const std::vector<std::shared_ptr<Plate>> plates = frame.get_elements_numbered<Plate>("plate");
    std::cout << frame << std::endl;
    std::cout << fmt::format("two-hinged frame: {} stations, {} plates\n", frame.frame.point_count(), plates.size());
    frame.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 8 of the folding templates: a linear structure system of folded surfaces after Engel, Structure Systems, Folded plate structures (p. 225), the two-hinged frame with ridge-to-ridge folding. The frame's inner line, the valleys of its folds, is a portal 30 m wide and 5 m high, each column straight with a corner at mid-height and the ceiling straight with a corner at mid-span; at every corner a zigzag of four ridges, plates 2000 across, stands in the plane halving the corner, its ridges offset outwards along the bisector by the depth: 300 at the hinged feet, 2000 at the knees of the columns, 900 at the eaves and 3200 at mid-span, so the columns bulge out and the roof rises to its ridge as in Engel's section. The phase turns at every corner, so every strip warps and folds ridge to ridge into two planar 60 mm triangles: 96 plates. Each step is a group: frame, stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_8_two_hinged_frame --parallel 6 && ./build/templates_folding_8_two_hinged_frame && ../bash/publish-scene.sh --target templates_folding_8_two_hinged_frame

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
