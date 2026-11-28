#include "wood_session.h"
#include "src/templates/timber_floor/timber_floor.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// How a floor sits on its beams, how its panels join, and how concrete is held to it.
int main() {

    TimberFloorGuide resting;
    resting.span = 3000.0;
    resting.width = 2000.0;
    resting.panel = 160.0;

    TimberFloorGuide rebated = resting;
    rebated.seat = 1;

    TimberFloorGuide battens = resting;
    battens.seat = 2;

    TimberFloorGuide tongue = resting;
    tongue.panel_width = 1000.0;
    tongue.edge = 1;

    TimberFloorGuide strip = tongue;
    strip.edge = 2;

    TimberFloorGuide board = tongue;
    board.edge = 3;

    TimberFloorGuide keys = tongue;
    keys.edge = 4;

    TimberFloorGuide glued = resting;
    glued.panel = 120.0;
    glued.panel_width = 1000.0;
    glued.concrete = 80.0;
    glued.connector = 0;

    TimberFloorGuide notched = glued;
    notched.connector = 1;

    TimberFloorGuide pinned = TimberFloorGuide::joists(0, 3000.0, false);
    pinned.width = 2000.0;
    pinned.concrete = 80.0;
    pinned.connector = 2;
    pinned.layers = 0.0;

    TimberFloorGuide locked = glued;
    locked.lamella = 40.0;
    locked.connector = 3;

    TimberFloors floors({
        {"tongue", tongue},
        {"strip", strip},
        {"board", board},
        {"keys", keys},
        {"glued", glued},
        {"notched", notched},
        {"pinned", pinned},
        {"locked", locked},
        {"resting", resting},
        {"rebated", rebated},
        {"battens", battens},
    }, "timber_floor_joints", 4);
    std::cout << floors << std::endl;
    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the timber floors: on the beams, in their rebates and on battens glued to their faces; panels joined by a tongue in a groove, a strip in two grooves, a board in two rebates with its pins and keys across; and concrete held to the timber by glue, by notches, by crossed pins over joists and by flat steel locks across boards on edge.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_3_joints --parallel 6 && ./build/templates_timber_floor_3_joints && ../bash/publish-scene.sh --target templates_timber_floor_3_joints

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
