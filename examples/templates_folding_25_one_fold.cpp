#include "wood_session.h"
#include "src/templates/folding/prismatic_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The system with one fold.
int main() {

    PrismaticFold fold(
        PrismaticFold::polygon(1, 12000.0, 3000.0), 12000.0, 120.0, 2, 0.0, "one_fold"
    );
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("one fold: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 25 of the folding templates: the system with one fold: two plates on a 12 m wide arch rising 3000, meeting at one ridge, carried straight over a 12 m span; the edge stresses at the common fold have the same tendency, so the stress distribution stays unchanged. Two 120 mm plates and a diaphragm at each end. Each step is a group: profile, folds, mesh, plates, diaphragms.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_25_one_fold --parallel 6 && ./build/templates_folding_25_one_fold && ../bash/publish-scene.sh --target templates_folding_25_one_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
