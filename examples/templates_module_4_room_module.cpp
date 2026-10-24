#include "wood_session.h"
#include "src/templates/module/room_module.h"

using namespace session_cpp;
using namespace wood_session;

/// One room of two timber room modules open between them: framed floors and ceilings on corner posts, walls on the closed sides.
int main() {

    wood_module::RoomModule module;
    std::cout << module << std::endl;

    const size_t frame = module.get_branch("contacts_frame").objects.meshes->size();
    const size_t sheathing = module.get_branch("contacts_sheathing").objects.meshes->size();
    std::cout << fmt::format(
                     "room module: {} modules of {} x {} x {} mm, {} frame contacts, {} sheathing "
                     "contacts",
                     module.columns(),
                     module.guide.length,
                     module.guide.width,
                     module.module_height(),
                     frame,
                     sheathing
                 )
              << std::endl;
    module.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 4 of the timber modules: one room of two room modules each 9000 x 3000 in plan, 2700 clear inside, 3150 high, side by side along their long sides. Each module is a frame: a floor 280 thick, two 190 x 280 edge beams a wall wide over the whole length and sixteen 80 x 240 joists across, the first and last at the ends, between a 15 mm board and a 25 mm deck, flush on both faces; a thinner ceiling, 170, sixteen 60 x 140 joists between two 15 mm boards, its two long edge beams 190 x 470 reaching 300 into the room; four corner posts on the floor's edge beams under the ceiling's, a framed wall between them on every closed long side, 2400 high, and an end wall between floor and ceiling at each end, 160 deep with a 15 mm sheet on each face; the side between the two modules is left open, a post of each at mid-length, so the room is 6 m wide. The corridor ends have a door, the facade ends a window 2100 wide from 600 to 2500. Every member and sheet that touches another is a face contact; no connector is made.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_4_room_module --parallel 6 && ./build/templates_module_4_room_module && ../bash/publish-scene.sh --target templates_module_4_room_module

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
