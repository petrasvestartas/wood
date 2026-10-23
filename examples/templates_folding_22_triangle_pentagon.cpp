#include "wood_session.h"
#include "src/templates/folding/triangle_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Folded triangular surfaces over a pentagonal plan (Engel, p. 232): a triangle at every corner, five from the midpoints to the raised
/// centre.
int main() {

    TriangleFold fold(5, 12000.0, 0.0, 4000.0, 7000.0, 120.0, "triangle_pentagon");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("triangle pentagon: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 22 of the folding templates: folded triangular surfaces over a pentagonal plan after Engel, Structure Systems, Folded plate structures (p. 232). A regular pentagon on a 12 m circumradius; a triangle at every corner between its edges' midpoints, and the pentagon of the midpoints split into five triangles to the centre: corners at 0, midpoints at 4000, centre at 7000; ten planar triangles 120 mm thick. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_22_triangle_pentagon --parallel 6 && ./build/templates_folding_22_triangle_pentagon && ../bash/publish-scene.sh --target templates_folding_22_triangle_pentagon

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
