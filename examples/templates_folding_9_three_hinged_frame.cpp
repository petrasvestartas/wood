#include "wood_session.h"
#include "src/templates/folding/folded_frame.h"

using namespace session_cpp;
using namespace wood_session;

/// The three-hinged frame of Engel's folded plate structures, ridge folding (p. 226): the portal's roof deepest at its quarter
/// points and narrowing to a hinge at mid-span.
int main() {

    FoldedFrame frame(FoldedFrame::portal(30000.0, 5000.0, 4), 4, 2000.0, {300.0, 2000.0, 900.0, 2800.0, 300.0, 2800.0, 900.0, 2000.0, 300.0});
    const std::vector<std::shared_ptr<Plate>> plates = frame.get_elements_numbered<Plate>("plate");
    std::cout << frame << std::endl;
    std::cout << fmt::format("three-hinged frame: {} stations, {} plates\n", frame.frame.point_count(), plates.size());
    frame.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 9 of the folding templates: the three-hinged frame with ridge folding after Engel, Structure Systems, Folded plate structures (p. 226). The portal of step 8 with corners at the ceiling's quarter points: the depth 300 at both feet and at mid-span, the three hinges, 2000 at the knees of the columns, 900 at the eaves and 2800 at the roof's quarter points, so the roof rises to two ridges and dips to the hinge between them as in Engel's section; the phase turning at every corner, every strip folds ridge to ridge into two planar 480 mm triangles: 128 plates. Each step is a group: frame, stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_9_three_hinged_frame --parallel 6 && ./build/templates_folding_9_three_hinged_frame && ../bash/publish-scene.sh --target templates_folding_9_three_hinged_frame

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
