#include "wood_session.h"
#include "src/templates/folding/triangle_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Folded triangular surfaces over a hexagonal plan (Engel, p. 232): a triangle at every corner, six from the midpoints to the raised
/// centre.
int main() {

    TriangleFold fold(6, 12000.0, 0.0, 4000.0, 7000.0, 240.0, "triangle_hexagon");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("triangle hexagon: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 23 of the folding templates: folded triangular surfaces over a hexagonal plan after Engel, Structure Systems, Folded plate structures (p. 232). A regular hexagon on a 12 m circumradius; a triangle at every corner between its edges' midpoints, and the hexagon of the midpoints split into six triangles to the centre: corners at 0, midpoints at 4000, centre at 7000; twelve planar triangles 240 mm thick. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_23_triangle_hexagon --parallel 6 && ./build/templates_folding_23_triangle_hexagon && ../bash/publish-scene.sh --target templates_folding_23_triangle_hexagon

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
