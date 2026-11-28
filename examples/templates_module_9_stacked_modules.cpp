#include "wood_session.h"
#include "src/templates/module/module_building.h"

using namespace laika_cpp;
using namespace wood_session;

/// Modules stacked three high on elastic strips, coupled at their ends.
int main() {

    wood_module::ModuleBuildingGuide guide;
    guide.rooms = {1, 2};
    guide.modules[0].openings[2] = {{500.0, 1500.0, 300.0, 2200.0}};
    guide.modules[0].openings[3] = {{800.0, 1000.0, 0.0, 2100.0}};
    guide.modules[0].side_posts = 1;
    guide.coupler = 27.0;

    wood_module::ModuleBuilding building(guide, "stacked_modules");
    std::cout << building << std::endl;
    building.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 9 of the timber modules: nine modules that carry each other, three across and three high, the first a room of its own, the other two open to each other. Every module stands on two 80 x 40 battens along its long walls with a 12 strip on them, the acoustic decoupling, or on short ones under the posts of an open side; side by side 50 apart, so every wall and floor between two rooms is double. Plates 27 thick over every joint at the start ends couple the modules, each pinned into the two ceilings and the two floors that meet there.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_9_stacked_modules --parallel 6 && ./build/templates_module_9_stacked_modules && ../bash/publish-scene.sh --target templates_module_9_stacked_modules

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
