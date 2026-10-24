#include "wood_session.h"
#include "src/templates/folding/triangle_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Folded triangular surfaces over a pentagonal plan .
int main() {

    TriangleFold fold(5, 12000.0, 4800.0, 6000.0, 120.0, "triangle_pentagon");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("triangle pentagon: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 22 of the folding templates: folded triangular surfaces over a pentagonal plan. A regular pentagon on a 12 m circumradius standing on its corners; every edge carries a vertical gable triangle up to its middle 4800 high, a triangle at every corner folds down from two peaks to the corner, and the pentagon of the middles is split into five triangles to the centre at 6000: fifteen planar triangles 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_22_triangle_pentagon --parallel 6 && ./build/templates_folding_22_triangle_pentagon && ../bash/publish-scene.sh --target templates_folding_22_triangle_pentagon

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
