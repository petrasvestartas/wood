#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The centre fold elevated over the edge folds, ridge to valley .
int main() {

    CounterFold fold(
        CounterFold::elevated_centre(4, 1500.0, 1500.0, 15000.0, 1500.0),
        Folding::ridge_to_valley,
        120.0,
        "elevated_ridge_to_valley"
    );
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("elevated ridge to valley: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 29 of the folding templates: the centre fold elevated over the edge folds with ridge-to-valley folding: the stations of step 6, every warped strip folded along the diagonal through its low corners, from the ends' odd fold lines, their valleys, to the centre's even ones, so the diagonals cross at the centre on the even lines and the centre station creases at every fold, the vertical line's plan: 32 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_29_elevated_ridge_to_valley --parallel 6 && ./build/templates_folding_29_elevated_ridge_to_valley && ../bash/publish-scene.sh --target templates_folding_29_elevated_ridge_to_valley

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
