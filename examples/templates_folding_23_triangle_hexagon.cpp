#include "wood_session.h"
#include "src/templates/folding/triangle_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Folded triangular surfaces over a hexagonal plan .
int main() {

    TriangleFold fold(6, 12000.0, 4200.0, 5300.0, 120.0, "triangle_hexagon");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("triangle hexagon: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 23 of the folding templates: folded triangular surfaces over a hexagonal plan. A regular hexagon on a 12 m circumradius standing on its corners; every edge carries a vertical gable triangle up to its middle 4200 high, a triangle at every corner folds down from two peaks to the corner, and the hexagon of the middles is split into six triangles to the centre at 5300: eighteen planar triangles 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_23_triangle_hexagon --parallel 6 && ./build/templates_folding_23_triangle_hexagon && ../bash/publish-scene.sh --target templates_folding_23_triangle_hexagon

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
