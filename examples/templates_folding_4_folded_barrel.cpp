#include "wood_session.h"
#include "src/templates/folding/prismatic_fold.h"

using namespace session_cpp;
using namespace wood_session;

/// The system with many folds of Engel's folded plate structures (p. 220): seven folds on a circular arc over 12 m, carried over a
/// 12 m span, its form and action approaching a cylindrical shell, a diaphragm at each end.
int main() {

    PrismaticFold barrel(PrismaticFold::polygon(7, 12000.0, 4000.0), 12000.0, 60.0, 2, 0.0, 0.0, "folded_barrel");
    const std::vector<std::shared_ptr<Plate>> plates = barrel.get_elements_numbered<Plate>("plate");
    std::cout << barrel << std::endl;
    std::cout << fmt::format("folded barrel: {} plates over {} mm\n", plates.size(), barrel.span);
    barrel.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 4 of the folding templates: the system with many folds after Engel, Structure Systems, Folded plate structures (p. 220). The more folds a profile has, the more the edge stresses of its plates balance each other through shear, until its form and action approach those of a cylindrical shell: seven folds, eight 60 mm plates with their corners on a circular arc 12 m wide and 4 m high, carried straight over a 12 m span, a diaphragm closing each end under the folds. Each step is a group: profile, folds, mesh, plates, diaphragms.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_4_folded_barrel --parallel 6 && ./build/templates_folding_4_folded_barrel && ../bash/publish-scene.sh --target templates_folding_4_folded_barrel

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
