#include "wood_session.h"
#include "src/templates/timber_floor/earth_floor.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// Four floor elements of timber beams with rammed earth between them: arched, straight, flush and concealed.
int main() {

    EarthFloors floors({
        {"arch", EarthFloorGuide::arch()},
        {"straight", EarthFloorGuide::straight()},
        {"flush", EarthFloorGuide::flush()},
        {"concealed", EarthFloorGuide::concealed()},
    });
    std::cout << floors << std::endl;
    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 4 of the timber floors: the prefabricated timber and earth floor of Rematter, four elements 2100 x 4200 of three bays at 700. Beams 200 deep, each of two halves, wider at the foot, so the rammed earth between them rests on their flanks; a 27 three-layer panel on both, pinned to every half, 227 in all. arch: the earth 120 deep at the beams, its underside an arc rising 18 over 500, the beams running on below it. straight: the same, flat. flush: the earth as deep as the beams. concealed: flush, with a 20 board under it.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_4_earth --parallel 6 && ./build/templates_timber_floor_4_earth && ../bash/publish-scene.sh --target templates_timber_floor_4_earth

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
