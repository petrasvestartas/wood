#include "wood_session.h"
#include "src/templates/folding/prismatic_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The singly folded plate of Engel's folded plate structures (pp. 219, 221): a zigzag of four ridges over a 12 m span, diaphragms
/// under the folds at both ends and mid-span, a 400 flange folded down at both free edges.
int main() {

    PrismaticFold fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("prismatic fold: {} plates over {} mm\n", plates.size(), fold.span);
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the folding templates: the singly folded plate after Engel, Structure Systems, Folded plate structures (pp. 219-221). A zigzag profile of four ridges, eight plates each 1500 across in plan and 1200 high, so 12 m wide, carried straight over a 12 m span, the depth a tenth of the span; one 120 mm plate per fold, mitered at the folds, acting as a slab between its folds, as a deep beam along the span and with its neighbours as a truss across it. Three transverse diaphragms under the folds, one at each end and one at mid-span, each one plate across the profile between the flanges down to their bottom, stiffen the profile against a change of its fold angles (p. 221, diaphragm below folds); a 400 flange folded down at each free edge, the folds being shallow, mitered like every other fold, stiffens it against buckling (p. 221, edge beam for shallow folding). Each step is a group: profile, folds, mesh, plates, diaphragms.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_3_prismatic_fold --parallel 6 && ./build/templates_folding_3_prismatic_fold && ../bash/publish-scene.sh --target templates_folding_3_prismatic_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
