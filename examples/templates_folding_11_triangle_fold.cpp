#include "wood_session.h"
#include "src/templates/folding/single_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Singly folded surfaces over a triangular plan, horizontal ridges .
int main() {

    SingleFold fold(3, 12000.0, 2900.0, 7000.0, 7000.0, 120.0, "triangle_fold");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("triangle fold: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 11 of the folding templates: structure systems through interpenetration of folded surfaces, singly folded surfaces over a triangular plan with horizontal ridges. The plan an equilateral triangle on a 12 m circumradius standing on its three corners; every edge's middle stands 2900 out from the edge and 7000 high, as high as the centre, so the three ridges from the middles to the centre are horizontal and the plan's outline is a six-pointed star; every sector is one singly folded surface, two planar triangles meeting on its ridge and its neighbours on the valleys from the corners to the centre, and a steep eave triangle from every edge on the ground leans outward up to its middle: nine plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_11_triangle_fold --parallel 6 && ./build/templates_folding_11_triangle_fold && ../bash/publish-scene.sh --target templates_folding_11_triangle_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
