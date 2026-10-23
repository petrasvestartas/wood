#include "wood_session.h"
#include "src/templates/module/room_module.h"

using namespace session_cpp;
using namespace wood_session;

/// A block of timber room modules like the ZM10 school pavilions: two rooms of two modules per storey, three storeys stacked.
int main() {

    wood_module::RoomModuleGuide guide;
    guide.columns = 4;
    guide.storeys = 3;
    wood_module::RoomModule block(guide, "module_block");
    std::cout << block << std::endl;

    const size_t frame = block.get_branch("contacts_frame").objects.meshes->size();
    const size_t sheathing = block.get_branch("contacts_sheathing").objects.meshes->size();
    std::cout << fmt::format("module block: {} modules, {} frame contacts, {} sheathing contacts", guide.columns * guide.storeys, frame, sheathing) << std::endl;
    block.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 5 of the timber modules: twelve room modules of step 4 like the ZM10 school pavilions Blumer Lehmann builds for Zurich, four side by side and three storeys stacked, the most they stack: per storey two rooms of two modules each, open between the two modules of a room and a wall each between the rooms, the module ends with their windows forming the facade, the corridor ends with their doors on the other side. Every member and sheet that touches another is a face contact, in a module and between modules; no connector is made.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_5_module_block --parallel 6 && ./build/templates_module_5_module_block && ../bash/publish-scene.sh --target templates_module_5_module_block

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
