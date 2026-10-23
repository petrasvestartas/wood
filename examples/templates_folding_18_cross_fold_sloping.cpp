#include "wood_session.h"
#include "src/templates/folding/cross_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The cross fold with its ridges sloping toward the centre (Engel, p. 229): every half edge divided into three, the ridges ending
/// 4500 high at the edge and falling to the centre at 1500.
int main() {

    CrossFold fold(18000.0, 3, 1500.0, 4500.0, 1500.0, 240.0, 1, 1, "cross_fold_sloping");
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("cross fold sloping: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 18 of the folding templates: cross-folded surfaces spanned diagonally over a square plan, ridges sloping toward the centre, after Engel, Structure Systems, Folded plate structures (p. 229). An 18 m square, every half edge divided into three, 24 straight fold lines from the centre to the edge alternating valley ending at 1500 and ridge ending at 4500 from every corner, all falling to the centre at 1500, so the surface dips to the middle where Engel's rising version peaks; every face a planar triangle: 24 plates 240 mm thick. Each step is a group: folds, mesh, plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_18_cross_fold_sloping --parallel 6 && ./build/templates_folding_18_cross_fold_sloping && ../bash/publish-scene.sh --target templates_folding_18_cross_fold_sloping

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
