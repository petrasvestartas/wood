#include "wood_session.h"
#include "src/templates/timber_floor/timber_floor.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// Solid panels side by side, joined by a tongue, a strip, a cover board and a key.
int main() {

    TimberFloorGuide tongue;
    tongue.span = 3000.0;
    tongue.width = 2000.0;
    tongue.panel = 160.0;
    tongue.panel_width = 1000.0;
    tongue.edge = 1;

    TimberFloorGuide strip = tongue;
    strip.edge = 2;

    TimberFloorGuide board = tongue;
    board.edge = 3;

    TimberFloorGuide keys = tongue;
    keys.edge = 4;

    TimberFloors floors({{"tongue", tongue}, {"strip", strip}, {"board", board}, {"keys", keys}}, "timber_floor_joints");
    std::cout << floors << std::endl;
    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the timber floors: two solid panels 160 thick and 1000 wide over 3000, their joint along the span. tongue: a tongue a third of the thickness in a groove. strip: a 27 x 120 strip in a groove in both. board: a 27 x 160 board in a rebate in the top of both, pinned into each every 400. keys: a butterfly key across the joint, which takes tension across it.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_3_joints --parallel 6 && ./build/templates_timber_floor_3_joints && ../bash/publish-scene.sh --target templates_timber_floor_3_joints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
