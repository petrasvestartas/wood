#include "wood_session.h"
#include "src/templates/folding/triangle_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Folded triangular surfaces over a square plan (Engel, p. 232): a triangle at every corner between the edges' midpoints, and four
/// triangles from the midpoints to the raised centre.
int main() {

    TriangleFold fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("triangle square: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 20 of the folding templates: a structure system of folded triangular surfaces over a square plan after Engel, Structure Systems, Folded plate structures (p. 232). A square on a 12 m circumradius; the lines joining its edges' midpoints cut a triangle off every corner, and the cross from the centre splits the square of the midpoints into four more: the corners on the supports at 0, the midpoints at 4000, the centre at 7000; eight planar triangles 240 mm thick, their corners cut back by the thickness. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_20_triangle_square --parallel 6 && ./build/templates_folding_20_triangle_square && ../bash/publish-scene.sh --target templates_folding_20_triangle_square

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
