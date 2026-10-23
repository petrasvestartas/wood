#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The centre fold elevated over the edge folds, alternate folding (Engel, p. 223): ridge to valley and ridge to ridge fold by fold.
int main() {

    CounterFold fold(CounterFold::elevated_centre(4, 1500.0, 1500.0, 15000.0, 1500.0), Folding::alternate, 120.0, "alternate_fold");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("alternate fold: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 30 of the folding templates: the centre fold elevated over the edge folds with alternate folding after Engel, Structure Systems, Folded plate structures (p. 223): the stations of step 6, the folds about the even fold lines folded ridge to valley and ridge to ridge by turns, the edge fold first ridge to valley. The odd fold lines then lie straight in one plane on either side and vanish, as do the centre's creases of every ridge-to-ridge fold, so the surface is planar quads between the diagonals, which all slope one way on each half of the span, with the stars of the plan at every second even line and the diamonds of Engel's end elevation without a line through them: 20 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_30_alternate_fold --parallel 6 && ./build/templates_folding_30_alternate_fold && ../bash/publish-scene.sh --target templates_folding_30_alternate_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
