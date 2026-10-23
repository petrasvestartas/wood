#include "wood_session.h"
#include "src/templates/folding/curved_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The curved folded tent roof of Knie's Zauberhut (Blumer Lehmann, 2020): twelve curved folds on a 32 m roof rising into a round
/// tower 26 m high, a gable over the eaves between every two columns.
int main() {

    CurvedFold fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("curved fold: {} sectors, {} plates\n", fold.sectors, plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 37 of the folding templates: the curved folding of Knie's Zauberhut in Rapperswil (Blumer Lehmann with Carlos Martinez Architekten, 2020), a freestanding folded shell roof of 32 m diameter rising into a tower 26 m high with a round top of 7 m. Twelve sectors, their valleys running down to the columns at 4000 and their ridges ending in a gable 4000 over the eaves, the folds following a trumpet curve, flat at the eaves and vertical at the tower, their depth shrinking with the radius into the tower. The curves are discretised in fourteen rings, each the same star scaled, so every band between two rings is a planar trapezoid, a ruled strip of the curved fold: 312 plates 120 mm thick, mitered at every fold, their corners cut back only where three or more meet out of line. Each step is a group: meridians, mesh, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_37_curved_fold --parallel 6 && ./build/templates_folding_37_curved_fold && ../bash/publish-scene.sh --target templates_folding_37_curved_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
