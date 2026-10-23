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
Step 4 of the timber modules: one room of two room modules after the timber modules of Blumer Lehmann, each 9000 x 2950 in plan on a 3000 grid, 50 apart, 2700 clear inside, 3180 high, side by side along their long sides. Each module is a frame: a floor of four 190 x 280 edge beams, a wall wide and flush all round, fourteen 80 x 200 joists across under a 27 mm deck flush with the edge beams' tops and a 15 mm board under them, the ceiling the floor turned over and lighter, 190 x 200 edge beams, 60 x 140 joists on a 15 mm lining flush with their undersides, four posts at its corners, and a framed wall between the posts on every closed side, 160 deep with a 15 mm sheet on each face; the side between the two modules is left open, a 190 x 300 downstand beam of each under its ceiling on a post at mid-length, so the room is 6 m wide. The corridor ends have a door, the facade ends a window. Every member and sheet that touches another is a face contact; no connector is made.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_4_room_module --parallel 6 && ./build/templates_module_4_room_module && ../bash/publish-scene.sh --target templates_module_4_room_module

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
