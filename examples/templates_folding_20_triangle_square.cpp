#include "wood_session.h"
#include "src/templates/folding/triangle_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Folded triangular surfaces over a square plan (Engel, p. 232): a vertical gable on every edge, a triangle at every corner between
/// the gables' peaks, and four triangles from the peaks to the raised centre.
int main() {

    TriangleFold fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("triangle square: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 20 of the folding templates: a structure system of folded triangular surfaces over a square plan after Engel, Structure Systems, Folded plate structures (p. 232). A square on a 12 m circumradius standing on its corners; every edge carries a vertical gable triangle up to its middle 5000 high, the lines joining the middles cut a triangle off every corner, folding down to the corner on the ground, and the cross from the centre at 6500 splits the square of the middles into four more: twelve planar triangles 120 mm thick, their corners cut back only where needed and their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_20_triangle_square --parallel 6 && ./build/templates_folding_20_triangle_square && ../bash/publish-scene.sh --target templates_folding_20_triangle_square

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
