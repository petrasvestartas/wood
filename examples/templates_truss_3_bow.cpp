#include "wood_session.h"
#include "src/templates/truss/truss.h"

using namespace session_cpp;
using namespace wood_session;

/// Every roof truss and bridge frame of Bow's tables (1873), stored as graphs and built by the one Truss template,
/// each its own scene truss_bow_<number>.
int main() {

    const std::vector<wood_truss::Bow> designs = wood_truss::Bow::all();
    for (const wood_truss::Bow& design : designs) {
        wood_truss::Truss truss(wood_truss::TrussGuide(design), "truss_" + design.name());
        truss.pb_dump(pb_path("truss_" + design.name()));
    }
    std::cout << designs.size() << " Bow trusses" << std::endl;
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Every roof truss and bridge frame of Bow's tables (1873), stored as graphs in wood_truss::Bow and built by the one Truss template from a TrussGuide on each graph, every design written as its own scene truss_bow_<number> with its members and joints.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_truss_3_bow --parallel 6 && ./build/templates_truss_3_bow && ../bash/publish-scene.sh --target templates_truss_3_bow

|||||||| TIME ||||||||
31.83 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
