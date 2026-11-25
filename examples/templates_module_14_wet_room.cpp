#include "wood_session.h"
#include "src/templates/module/module_building.h"

using namespace session_cpp;
using namespace wood_session;

/// A wet room set into each of two closed modules.
int main() {

    wood_module::ModuleBuildingGuide guide;
    guide.rooms = {1, 1};
    guide.storeys = 1;
    guide.pod = true;
    guide.modules[0].openings[2] = {{500.0, 1500.0, 300.0, 2200.0}};
    guide.modules[0].openings[3] = {{1700.0, 900.0, 0.0, 2100.0}};

    wood_module::ModuleBuilding building(guide, "wet_room");
    std::cout << building << std::endl;
    building.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 14 of the timber modules: a wet room in each of two closed modules side by side, a module of its own 2200 x 1650 of solid plates 60 thick, 2320 high under the 2500 clear, its door to the room. It is set on the module's floor before the ceiling goes on.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_14_wet_room --parallel 6 && ./build/templates_module_14_wet_room && ../bash/publish-scene.sh --target templates_module_14_wet_room

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
