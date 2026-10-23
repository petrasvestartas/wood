#include "wood_session.h"
#include "src/templates/folding/counter_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The centre fold elevated over the edge folds, identical depth, ridge to ridge (Engel, p. 223): the zigzag at both ends of a
/// 15 m span starting on a ridge, its counter profile at mid-span lifted by the depth, so the even fold lines run straight through.
int main() {

    CounterFold fold(CounterFold::elevated_centre(4, 1500.0, 1500.0, 15000.0, 1500.0), Folding::ridge_to_ridge, 120.0, "elevated_fold");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("elevated fold: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 6 of the folding templates: a surface with counter-running folding, the centre fold elevated over the edge fold with identical depth of profiles, ridge-to-ridge folding, after Engel, Structure Systems, Folded plate structures (p. 223), read off the book's plan and both elevations. A zigzag of four ridges, plates 1500 across and 1500 deep, starting on a ridge at the edge, stands at both ends of a 15 m span, and at mid-span its counter profile lifted by the depth: its valleys level with the ends' ridges, so the even fold lines run straight through the whole span, as the level line of Engel's front elevation, while the odd ones rise from the ends' valleys to the centre's ridges, twice the depth high. Folded ridge to ridge, the diagonals run from the ends' even fold lines to the centre's odd ones, crossing there in the stars of the plan; either side of a straight fold line the two triangles lie in one plane, so they are one plate and the centre station is no crease on the even lines, as the plan draws it: 24 planar plates 120 mm thick, flush along both sides. Each step is a group: stations, mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_6_elevated_fold --parallel 6 && ./build/templates_folding_6_elevated_fold && ../bash/publish-scene.sh --target templates_folding_6_elevated_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
