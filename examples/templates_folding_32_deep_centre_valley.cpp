#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The elevated centre profile deeper than the edge profiles, ridge to valley .
int main() {

    CounterFold fold(
        CounterFold::elevated_centre(4, 1500.0, 1000.0, 15000.0, 1800.0),
        Folding::ridge_to_valley,
        120.0,
        "deep_centre_valley"
    );
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("deep centre valley: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 32 of the folding templates: the elevated centre profile deeper than the edge profiles with ridge-to-valley folding: the stations of step 31, the diagonals from the ends' valleys to the centre's, crossing at the centre on the even lines, where the centre station creases at every fold: 32 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_32_deep_centre_valley --parallel 6 && ./build/templates_folding_32_deep_centre_valley && ../bash/publish-scene.sh --target templates_folding_32_deep_centre_valley

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
