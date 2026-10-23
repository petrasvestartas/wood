#include "wood_session.h"
#include "src/templates/folding/folded_frame.h"

using namespace session_cpp;
using namespace wood_session;

/// The two-hinged frame of Engel's folded plate structures, ridge-to-ridge folding (p. 225): a 30 m portal of four 4.5 m bays,
/// its columns bulging out at the knee, its roof folded from the eaves corners to the ridge.
int main() {

    FoldedFrame frame;
    const std::vector<std::shared_ptr<Plate>> plates = frame.get_elements_numbered<Plate>("plate");
    std::cout << frame << std::endl;
    std::cout << fmt::format("two hinged frame: {} plates\n", plates.size());
    frame.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 8 of the folding templates: a linear structure system of folded surfaces after Engel, Structure Systems, Folded plate structures (p. 225), the two-hinged frame with ridge-to-ridge folding, read off the book's plan and both elevations. Half a bay is one fold line from the foot to the crown, zigzagging between the bay's edge, the frame's inner line of valleys, and the bay's centre, the ridges: the foot, hinged along the whole bay, the knee bulging out 2200 at half the 5800 column, the eaves corner, the ridge top 8750 high at mid-span and the ceiling peak 6600 under it; every point makes a planar triangle with the last valley and the last ridge, so a column is a pyramid of plates around its knee, a vertical plate at every bay edge, and the roof folds from the eaves corners up to the ridge top, the ridge running straight from the knee through the frame's inner corner to the top, the diamonds of Engel's side elevation and the hexagons of his plan. Mirrored over mid-span and over the bay's centre and repeated over four 4.5 m bays: 64 planar plates 120 mm thick, flush along both sides. Each step is a group: zigzag, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_8_two_hinged_frame --parallel 6 && ./build/templates_folding_8_two_hinged_frame && ../bash/publish-scene.sh --target templates_folding_8_two_hinged_frame

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
