#include "wood_session.h"
#include "src/templates/folding/triangle_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Folded triangular surfaces over a triangular plan (Engel, p. 232): the triangle cut into four by the lines joining its edges'
/// midpoints, the middle triangle flat and raised.
int main() {

    TriangleFold fold(3, 12000.0, 0.0, 4500.0, 4500.0, 120.0, "triangle_triangle");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("triangle triangle: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 21 of the folding templates: folded triangular surfaces over a triangular plan after Engel, Structure Systems, Folded plate structures (p. 232). An equilateral triangle on a 12 m circumradius cut into four by the lines joining its edges' midpoints: the three corner triangles fold down from the midpoints at 4500 to the corners at 0, the middle triangle stays flat at 4500; four planar triangles 120 mm thick. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_21_triangle_triangle --parallel 6 && ./build/templates_folding_21_triangle_triangle && ../bash/publish-scene.sh --target templates_folding_21_triangle_triangle

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
