#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The four quarters of the square floor.
int main() {

    wood_floor::Floor floor(wood_floor::FloorGuide::rectangle(3000.0, 3000.0));
    floor.add_quarters();
    floor.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 4 of the timber floor: the four quarters of the bay, each built in place at its corner and lifted to the floor at 3500, grouped as beds (one per row), tsections, outer_ribs, inner_ribs, wedges and inner_beams with the quarter index on every name. Ribs and inner beams are variable beams, every other member a plate.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_4_quarters --parallel 6 && ./build/templates_floor_4_quarters && ../bash/publish-scene.sh --target templates_floor_4_quarters

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
