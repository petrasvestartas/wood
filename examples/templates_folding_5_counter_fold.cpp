#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Counter-running folding of the same depth and elevation, ridge to ridge .
int main() {

    CounterFold fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("counter fold: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 5 of the folding templates: a surface with counter-running folding, the same depth of the fold profiles and the same elevation above ground, ridge-to-ridge folding, read off the construction's plan and both elevations. A zigzag of four ridges, plates 1500 across and 1500 deep, stands at the start of a 13.5 m span, nine plates long, and its counter profile at the end, so every fold line runs from a valley to a ridge; every strip is warped and folds along the diagonal through its two high corners, a ridge running from the ridge of one end to the ridge of the other, the diagonals of neighbouring strips meeting at the ends in the zigzag's plan, the strips crossing in the X's of his end elevation: 16 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_5_counter_fold --parallel 6 && ./build/templates_folding_5_counter_fold && ../bash/publish-scene.sh --target templates_folding_5_counter_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
