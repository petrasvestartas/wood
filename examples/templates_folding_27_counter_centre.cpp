#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The counter profile in the centre (Engel, p. 222): the zigzag at both ends of the span, its counter profile at mid-span, the
/// same depth and elevation, the diagonals running valley to valley and crossing at the centre.
int main() {

    CounterFold fold(CounterFold::counter_centre(4, 1500.0, 1500.0, 13500.0), Folding::ridge_to_valley, 120.0, "counter_centre");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("counter centre: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 27 of the folding templates: counter-running folding with the counter profile in the centre after Engel, Structure Systems, Folded plate structures (p. 222): the zigzag of four ridges at both ends of a 13.5 m span and its counter profile at mid-span, the same depth and elevation, so every fold line rises or falls by the depth to the centre and the centre profile is a crease at every fold; the warped strips fold ridge to valley, the diagonals running from the ends' valleys to the centre's, where the four of two strips cross in the stars of Engel's plan, one at every second fold line, and reach the corners of the plan singly: 32 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_27_counter_centre --parallel 6 && ./build/templates_folding_27_counter_centre && ../bash/publish-scene.sh --target templates_folding_27_counter_centre

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
