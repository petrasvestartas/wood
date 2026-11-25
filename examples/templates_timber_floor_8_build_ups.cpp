#include "wood_session.h"
#include "src/templates/timber_floor/floor_build_up.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_timber_floor;

/// Structures of the book's tables and all the layers on each as one, as thick as the tables have them.
int main() {

    std::vector<FloorBuildUp> build_ups;

    for (const std::array<size_t, 2>& place : std::vector<std::array<size_t, 2>>{
             {12, 1}, {12, 2}, {13, 4}, {24, 6},
             {24, 1}, {24, 4}, {24, 10}, {23, 1},
             {18, 1}, {22, 1}, {19, 4}, {22, 2},
         })
        build_ups.push_back(FloorBuildUp::find(place[0], place[1]));

    FloorBuildUps floors(build_ups);

    for (const FloorBuildUp& build_up : build_ups)
        std::cout << fmt::format("{} {}: {} on {}", build_up.table, build_up.number, build_up.top(), build_up.structure().front().name) << std::endl;

    floors.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 8 of the timber floors: twelve structures of tables 12 to 24 of the floor chapter, data/timber_floors/build_ups.json, samples 1200 x 1340, all the layers on each as one layer as thick as they are together: joists under 88 of dry layers, 75 of screed and insulation and 110 of screed and fill, ribs on a cross-laminated plate under 105; box elements under 58 and 160, a grid under 88, boards on edge and concrete under 120; planks under 35 and 95, boards on edge under 80 and 120.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_timber_floor_8_build_ups --parallel 6 && ./build/templates_timber_floor_8_build_ups && ../bash/publish-scene.sh --target templates_timber_floor_8_build_ups

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
