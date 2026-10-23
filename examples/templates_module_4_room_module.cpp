#include "wood_session.h"
#include "src/templates/module/room_module.h"

using namespace session_cpp;
using namespace wood_session;

/// One timber room module after Blumer Lehmann: a framed floor and ceiling and four framed walls, 9 x 3 m, 2.7 m clear inside.
int main() {

    wood_module::RoomModule module;
    std::cout << module << std::endl;

    const size_t frame = module.get_branch("contacts_frame").objects.meshes->size();
    const size_t sheathing = module.get_branch("contacts_sheathing").objects.meshes->size();
    std::cout << fmt::format("room module: {} x {} x {} mm, {} frame contacts, {} sheathing contacts", module.guide.length, module.guide.width, module.module_height(), frame, sheathing) << std::endl;
    module.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 4 of the timber modules: one room module after the timber modules of Blumer Lehmann, 9000 x 3000 in plan, 2700 clear inside, 3200 high. Six framed elements: a floor and a ceiling lying flat, 200 deep with a 25 mm deck on each face, rim joists along the module and joists across it on the 625 mm grid; and four walls standing on the floor deck under the ceiling, 160 deep with a 15 mm sheet on each face, the two long walls running past the two end walls, two windows in the front, one in the back and a door in the first end wall. Every member and sheet that touches another is a face contact, drawn in contacts_frame and contacts_sheathing; no connector is made.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_4_room_module --parallel 6 && ./build/templates_module_4_room_module && ../bash/publish-scene.sh --target templates_module_4_room_module

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
