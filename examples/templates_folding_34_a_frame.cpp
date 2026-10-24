#include "wood_session.h"
#include "src/templates/folding/folded_frame.h"

using namespace session_cpp;
using namespace wood_session;

/// The two-hinged A-frame, ridge-to-valley folding .
int main() {

    FoldedFrame frame(FoldedFrame::a_frame(), 3850.0, 4, 120.0, "a_frame");
    const std::vector<std::shared_ptr<Plate>> plates = frame.get_elements_numbered<Plate>("plate");
    std::cout << frame << std::endl;
    std::cout << fmt::format("a frame: {} plates\n", plates.size());
    frame.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 34 of the folding templates: the two-hinged A-frame with ridge-to-valley folding. Four 3.85 m bays; every leg stands on point feet at the bay edges, bulges out 1450 at the knee 6000 high, and its fold line drops from the knee to the waist, 6100 in and 4000 high at the bay's centre, before climbing to the apex 12700 high at the bay edge and the ridge 18100 high over it: the diagonals from the feet to the apex cross at the waist, the star of six plates's plan, the kites of his side elevation hang from the ridge, and the legs' outer plates lean out over the feet: 48 planar plates 120 mm thick. Each step is a group: zigzag, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_34_a_frame --parallel 6 && ./build/templates_folding_34_a_frame && ../bash/publish-scene.sh --target templates_folding_34_a_frame

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
