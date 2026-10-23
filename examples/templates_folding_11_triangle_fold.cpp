#include "wood_session.h"
#include "src/templates/folding/polygon_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Interpenetrating folded surfaces over a triangular plan, horizontal ridges (Engel, p. 228): three sectors, their ridges all 1800
/// high, meeting along the lines from the corners to the centre.
int main() {

    PolygonFold fold(3, 12000.0, {0.0, 1800.0, 0.0, 1800.0, 0.0, 1800.0}, 0.0, false, 240.0, "triangle_fold");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("triangle fold: {} sides, {} plates\n", fold.sides, plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 11 of the folding templates: structure systems through interpenetration of folded surfaces after Engel, Structure Systems, Folded plate structures (p. 228), singly folded surfaces over a triangular plan with horizontal ridges. The plan an equilateral triangle on a 12 m circumradius; six concentric rings alternate valley at 0 and ridge at 1800, so every band between two rings in a sector is a planar trapezoid between two parallel edges, the three sectors' folds meeting along the lines from the corners to the centre, the last triangles meeting at the centre at 0: 18 plates 240 mm thick. Each step is a group: rings, mesh, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_11_triangle_fold --parallel 6 && ./build/templates_folding_11_triangle_fold && ../bash/publish-scene.sh --target templates_folding_11_triangle_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
