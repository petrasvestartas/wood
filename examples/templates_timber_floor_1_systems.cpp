#include "wood_session.h"
#include "src/templates/timber_floor/timber_floor.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// Eight timber floors of a home over 5 m under a heavy build-up, each as deep as the table asks.
int main() {

    const size_t homes = 0;
    const double span = 5000.0;
    const bool heavy = true;

    TimberFloorGuide ribbed = TimberFloorGuide::box(homes, span, heavy);
    ribbed.rib_depth = 240.0;
    ribbed.rib_width = 100.0;
    ribbed.bottom = 0.0;

    TimberFloorGuide planks;
    planks.span = span;
    planks.panel = 180.0;
    planks.panel_width = 200.0;
    planks.edge = 1;

    TimberFloorGuide cross_laminated;
    cross_laminated.span = span;
    cross_laminated.panel = 180.0;
    cross_laminated.panel_width = 1200.0;
    cross_laminated.edge = 3;

    TimberFloors floors({
        {"joists", TimberFloorGuide::joists(homes, span, heavy)},
        {"ribbed", ribbed},
        {"box", TimberFloorGuide::box(homes, span, heavy)},
        {"hollow", TimberFloorGuide::hollow(homes, span, heavy)},
        {"planks", planks},
        {"dowelled", TimberFloorGuide::dowelled(homes, span, heavy)},
        {"cross_laminated", cross_laminated},
        {"composite", TimberFloorGuide::composite(homes, span, heavy)},
    });
    std::cout << floors << std::endl;
    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 1 of the timber floors: one bay of every floor system over 5000, 2400 wide, for a home (2 kN/m2) under a heavy build-up (1.8 kN/m2), sized by the table where it has the system. joists: 160 x 280 at 600 under a 27 board. ribbed: 100 x 240 ribs at 500 under a glued 27 board. box: 80 x 180 ribs at 500 between two 27 boards. hollow: 200 deep box panels 1200 wide of 31 shells and webs. planks: glulam planks 180 x 200, tongue in groove. dowelled: 180 panels 600 wide of 40 boards on edge, 20 dowels through them. cross_laminated: 180 panels 1200 wide, a 27 x 160 board in their rebates over the joint. composite: 100 timber notched into 80 concrete.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_1_systems --parallel 6 && ./build/templates_timber_floor_1_systems && ../bash/publish-scene.sh --target templates_timber_floor_1_systems

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
