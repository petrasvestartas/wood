#include "wood_session.h"
#include "src/templates/timber_floor/timber_floor.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// One floor on its beams, in their rebates and on battens glued to their faces.
int main() {

    TimberFloorGuide resting;
    resting.span = 4000.0;
    resting.width = 1800.0;
    resting.panel = 160.0;

    TimberFloorGuide rebated = resting;
    rebated.seat = 1;

    TimberFloorGuide battens = resting;
    battens.seat = 2;

    TimberFloors floors({{"resting", resting}, {"rebated", rebated}, {"battens", battens}}, "timber_floor_bearings");
    std::cout << floors << std::endl;
    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 2 of the timber floors: a 160 solid panel over 4000 on two beams 160 wide. resting: on top of the beams, the whole depth of both seen. rebated: in a rebate 60 deep in each beam, its top flush with theirs; the notched beam needs its tension across the grain checked. battens: on 60 x 80 battens glued to the beams' faces, flush too.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_2_bearings --parallel 6 && ./build/templates_timber_floor_2_bearings && ../bash/publish-scene.sh --target templates_timber_floor_2_bearings

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
