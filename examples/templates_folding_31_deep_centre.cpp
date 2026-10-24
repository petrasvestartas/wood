#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The elevated centre profile deeper than the edge profiles, ridge to ridge .
int main() {

    CounterFold fold(
        CounterFold::elevated_centre(4, 1500.0, 1000.0, 15000.0, 1800.0),
        Folding::ridge_to_ridge,
        120.0,
        "deep_centre"
    );
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("deep centre: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 31 of the folding templates: counter-running folding with the elevated centre profile deeper than the edge profiles, ridge to ridge: the construction of step 6 with the edge profiles 1000 deep and the centre's 1800, its valleys level with the ends' ridges, so the even fold lines run straight and the odd ones rise 2800 from the ends' valleys to the centre's ridges, the tall diamonds's end elevation; the diagonals from the ends' even lines to the centre's odd ones, the centre no crease on the even lines: 24 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_31_deep_centre --parallel 6 && ./build/templates_folding_31_deep_centre && ../bash/publish-scene.sh --target templates_folding_31_deep_centre

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
