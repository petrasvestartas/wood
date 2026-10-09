#include "wood_session.h"
#include "src/templates/folding/reflex_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The reflex fold on its default arch and zigzag: the curves, the fold planes, the folded mesh and one plate per fold.
int main() {

    ReflexFold fold;
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("reflex fold: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The reflex fold template: a zigzag profile carried along an arch, folded onto the plane that halves the arch's angle at every point, one 10 mm plate per quad with 20 mm chamfers, each step in its own group (curves, fold_planes, mesh, plates).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_1_reflex_fold --parallel 6 && ./build/templates_folding_1_reflex_fold && ../bash/publish-scene.sh --target templates_folding_1_reflex_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
