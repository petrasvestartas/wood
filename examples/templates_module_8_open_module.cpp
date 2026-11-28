#include "wood_session.h"
#include "src/templates/module/spatial_module.h"

using namespace laika_cpp;
using namespace wood_session;

/// A frame module open on both long sides and a module open on one, braced for transport.
int main() {

    wood_module::SpatialModuleGuide frame;
    frame.length = 9000.0;
    frame.closed = {false, false, true, true};
    frame.side_posts = 1;
    frame.openings[2] = {{500.0, 1500.0, 300.0, 2200.0}};
    frame.openings[3] = {{800.0, 1000.0, 0.0, 2100.0}};

    wood_module::SpatialModuleGuide braced = frame;
    braced.length = 6000.0;
    braced.closed = {false, true, false, true};
    braced.side_posts = 0;
    braced.solid_floor = true;
    braced.solid_ceiling = true;
    braced.brace = 27.0;

    wood_module::ModuleTypes types({{"frame", frame}, {"braced", braced}}, "open_modules");
    std::cout << types << std::endl;
    types.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 8 of the timber modules: two open modules. frame: 9000 x 2950, both long sides open, posts as thick as the walls at the corners and at mid-length and a 150 x 240 downstand on them, so the loads come down at points; the ends framed walls. braced: 6000 x 2950 with a solid floor and ceiling, one long side and one end closed, the open side held by two crossed 120 x 27 boards on the inner face of its posts while the module is carried.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_8_open_module --parallel 6 && ./build/templates_module_8_open_module && ../bash/publish-scene.sh --target templates_module_8_open_module

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
