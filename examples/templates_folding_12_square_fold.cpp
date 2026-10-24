#include "wood_session.h"
#include "src/templates/folding/single_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Singly folded surfaces over a square plan, ridges rising toward the centre (Engel, p. 228): four sectors folded once each, their
/// ridges rising from the gables' peaks to the apex, a vertical gable on every edge.
int main() {

    SingleFold fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("square fold: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 12 of the folding templates: singly folded surfaces over a square plan with ridges rising toward the centre after Engel, Structure Systems, Folded plate structures (p. 228). An 18 m square standing on its corners; every edge's middle is in line with the edge and 5400 high, the centre 6900, so every edge carries a vertical gable triangle from the ground up to its peak and the four ridges rise from the peaks to the apex; every sector is two planar triangles folded on its ridge, meeting its neighbours on the valleys from the corners to the apex, as Engel's front elevation shows them, the gables' verticals at the sides and the shallow gable top: twelve plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_12_square_fold --parallel 6 && ./build/templates_folding_12_square_fold && ../bash/publish-scene.sh --target templates_folding_12_square_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
