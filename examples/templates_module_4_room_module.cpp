#include "wood_session.h"
#include "src/templates/module/room_module.h"

using namespace session_cpp;
using namespace wood_session;

/// One room of two timber room modules after Blumer Lehmann, open between them: framed floors and ceilings on corner posts, walls on the closed sides.
int main() {

    wood_module::RoomModule module;
    std::cout << module << std::endl;

    const size_t frame = module.get_branch("contacts_frame").objects.meshes->size();
    const size_t sheathing = module.get_branch("contacts_sheathing").objects.meshes->size();
    std::cout << fmt::format("room module: {} modules of {} x {} x {} mm, {} frame contacts, {} sheathing contacts", module.columns(), module.guide.length, module.guide.width, module.module_height(), frame, sheathing) << std::endl;
    module.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 4 of the timber modules: one room of two room modules after the timber modules of Blumer Lehmann, each 9000 x 3000 in plan, 2700 clear inside, 3200 high, joined along their long sides. Each module is a frame: a floor and a ceiling element lying flat, 200 deep with a 25 mm deck on each face, a rim of 160 wide boundary beams around seven 100 wide joists across it in eight equal bays and a row of noggings at mid-width, four posts at its corners, and a framed wall between the posts on every closed side, 160 deep with a 15 mm sheet on each face; the side between the two modules is left open, a post of each at mid-length, so the room is 6 m wide. The corridor ends have a door, the facade ends a window. Every member and sheet that touches another is a face contact; no connector is made.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_4_room_module --parallel 6 && ./build/templates_module_4_room_module && ../bash/publish-scene.sh --target templates_module_4_room_module

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
