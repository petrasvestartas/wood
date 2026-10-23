#include "wood_session.h"
#include "src/templates/folding/polygon_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Interpenetrating folded surfaces over a square plan, ridges rising toward the centre (Engel, p. 228): four sectors, the rings
/// alternating valley and ridge and rising, a pyramid at the centre.
int main() {

    PolygonFold fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("square fold: {} sides, {} plates\n", fold.sides, plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 12 of the folding templates: singly folded surfaces over a square plan with ridges rising toward the centre after Engel, Structure Systems, Folded plate structures (p. 228). The plan a square on a 12 m circumradius; six concentric rings at 0, 1800, 900, 2700, 1800 and 3600, valleys and ridges rising to an apex at 4500, every band in a sector a planar trapezoid, the four sectors meeting along the diagonals: 24 plates 240 mm thick. Each step is a group: rings, mesh, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_12_square_fold --parallel 6 && ./build/templates_folding_12_square_fold && ../bash/publish-scene.sh --target templates_folding_12_square_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
