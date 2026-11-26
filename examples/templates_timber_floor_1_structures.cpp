#include "wood_session.h"
#include "src/templates/timber_floor/timber_floor.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// The five structures of timber floors, sized for a home over 5 m by the book's tables, all the layers on them as one.
int main() {

    const size_t homes = 0;
    const double span = 5000.0;
    const bool heavy = true;

    TimberFloorGuide ribbed = TimberFloorGuide::box(homes, span, heavy);
    ribbed.rib_width = 100.0;
    ribbed.rib_depth = 240.0;
    ribbed.bottom = 0.0;

    TimberFloorGuide cross_laminated = TimberFloorGuide::dowelled(homes, span, heavy);
    cross_laminated.lamella = 0.0;
    cross_laminated.panel_width = 1200.0;
    cross_laminated.plies = 5;

    TimberFloors floors({
        {"joists", TimberFloorGuide::joists(homes, span, heavy)},
        {"ribbed", ribbed},
        {"box", TimberFloorGuide::box(homes, span, heavy)},
        {"hollow", TimberFloorGuide::hollow(homes, span, heavy)},
        {"dowelled", TimberFloorGuide::dowelled(homes, span, heavy)},
        {"cross_laminated", cross_laminated},
        {"composite", TimberFloorGuide::composite(homes, span, heavy)},
    }, "timber_floor_structures");
    std::cout << floors << std::endl;
    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 1 of the timber floors: the five structures for a home (2 kN/m2) over 5000 under a heavy build-up, 2400 wide on two beams, sized by the book's tables, all the layers on each as one of 110: joists under a board, ribs under a panel, a box and a hollow box, boards on edge dowelled and a cross-laminated panel of five plies, and timber notched into concrete.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_1_structures --parallel 6 && ./build/templates_timber_floor_1_structures && ../bash/publish-scene.sh --target templates_timber_floor_1_structures

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
