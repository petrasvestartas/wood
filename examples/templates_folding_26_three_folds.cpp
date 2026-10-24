#include "wood_session.h"
#include "src/templates/folding/prismatic_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The system with three folds.
int main() {

    PrismaticFold fold(
        PrismaticFold::polygon(3, 12000.0, 3500.0), 12000.0, 120.0, 2, 0.0, "three_folds"
    );
    const std::vector<std::shared_ptr<Plate>> plates = fold.get_elements_numbered<Plate>("plate");
    std::cout << fold << std::endl;
    std::cout << fmt::format("three folds: {} plates\n", plates.size());
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 26 of the folding templates: the system with three folds: four plates with their corners on a 12 m wide arch rising 3500, carried straight over a 12 m span; the edge stresses at the side folds have opposite tendency and largely compensate each other through shear. Four 120 mm plates and a diaphragm at each end. Each step is a group: profile, folds, mesh, plates, diaphragms.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_26_three_folds --parallel 6 && ./build/templates_folding_26_three_folds && ../bash/publish-scene.sh --target templates_folding_26_three_folds

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
