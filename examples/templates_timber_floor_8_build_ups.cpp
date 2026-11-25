#include "wood_session.h"
#include "src/templates/timber_floor/floor_build_up.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// Every floor build-up of the book's tables, 62 samples, a table a row of at most eight, their layers cut back one after the other.
int main() {

    FloorBuildUps floors(FloorBuildUp::library(), "timber_floor_build_ups", 8);
    std::cout << floors.build_ups.size() << " build-ups" << std::endl;
    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 8 of the timber floors: the 62 build-ups of tables 11 to 24 of the floor chapter, data/timber_floors/build_ups.json, each a sample 1400 x 1340 of its layers as printed, every layer ending 90 before the one under it: structural floors alone, joist floors exposed, partly exposed, clad and ballasted, solid floors with their insulation, boards, screeds, linings and fills, timber and concrete, and the systems of Lignatur, Lignotrend and Novatop. Membranes have no thickness and are not drawn.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_8_build_ups --parallel 6 && ./build/templates_timber_floor_8_build_ups && ../bash/publish-scene.sh --target templates_timber_floor_8_build_ups

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
