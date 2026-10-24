#include "wood_session.h"
#include "src/templates/folding/triangle_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// Folded triangular surfaces over a triangular plan (Engel, p. 232): a vertical gable on every edge, the roof cut into four by the
/// lines joining the gables' peaks, the middle triangle flat.
int main() {

    TriangleFold fold(3, 12000.0, 7500.0, 7500.0, 120.0, "triangle_triangle");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("triangle triangle: {}\n", plate_report(plates, fold.thickness));
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 21 of the folding templates: folded triangular surfaces over a triangular plan after Engel, Structure Systems, Folded plate structures (p. 232). An equilateral triangle on a 12 m circumradius standing on its corners; every edge carries a vertical gable triangle up to its middle 7500 high, the lines joining the middles cut the roof into four: the three corner triangles fold down from two peaks to the corner on the ground, the middle triangle stays flat at 7500, as the vertical lines of Engel's perspective show on the gables: seven plates 120 mm thick, their edges on the ground cut flush with it. Each step is a group: mesh, folds, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_21_triangle_triangle --parallel 6 && ./build/templates_folding_21_triangle_triangle && ../bash/publish-scene.sh --target templates_folding_21_triangle_triangle

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
