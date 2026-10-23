#include "wood_session.h"
#include "src/templates/module/room_module.h"

using namespace session_cpp;
using namespace wood_session;

/// A block of timber room modules: two side by side along their long walls, three storeys stacked, the most Blumer Lehmann stacks.
int main() {

    wood_module::RoomModuleGuide guide;
    guide.columns = 2;
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
Step 5 of the timber modules: six room modules of step 4, two side by side along their long walls and three storeys stacked, the most the timber modules of Blumer Lehmann stack; each module keeps its own floor, walls and ceiling, so two long walls stand back to back between neighbours and a ceiling lies under the floor of the module above. Every member and sheet that touches another is a face contact, in a module and between modules; no connector is made.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_5_module_block --parallel 6 && ./build/templates_module_5_module_block && ../bash/publish-scene.sh --target templates_module_5_module_block

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
