#include "wood_session.h"
#include "src/templates/module/wall.h"

using namespace session_cpp;
using namespace wood_session;

/// A framed wall module with a door and a window: rails, studs, the frame of each opening, sheathing on both faces and the contacts between them.
int main() {

    wood_module::Wall wall;
    std::cout << wall << std::endl;

    const size_t frame = wall.get_branch("contacts_frame").objects.polylines->size();
    const size_t sheathing = wall.get_branch("contacts_sheathing").objects.polylines->size();
    std::cout << fmt::format("wall: {} frame contacts, {} sheathing contacts", frame, sheathing) << std::endl;
    wall.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 1 of the timber modules: a 4800 x 2700 framed wall with the default WallGuide, a door and a window. Bottom and top rails, the bottom rail stopping at the door, studs every 625 mm, per opening two king studs, a header and for the window a sill with cripples above and below, then 15 mm sheathing panels on both faces split at the sheet width and at every opening edge. Every member end on the member it stands on and every panel on every member under it is a face contact, stored as their interaction and drawn in contacts_frame and contacts_sheathing; no connector is made. The wall is a group of beams and plates in the tree, nothing more.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_1_wall --parallel 6 && ./build/templates_module_1_wall && ../bash/publish-scene.sh --target templates_module_1_wall

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
