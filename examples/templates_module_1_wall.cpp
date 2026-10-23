#include "wood_session.h"
#include "src/templates/module/wall.h"

using namespace session_cpp;
using namespace wood_session;

/// A framed wall module with a door and a window: rails, studs, the frame of each opening, sheathing on both faces and the contacts between them.
int main() {

    wood_module::Wall wall;
    std::cout << wall << std::endl;

    const size_t frame = wall.get_branch("contacts_frame").objects.meshes->size();
    const size_t sheathing = wall.get_branch("contacts_sheathing").objects.meshes->size();
    std::cout << fmt::format("wall: {} frame contacts, {} sheathing contacts", frame, sheathing) << std::endl;
    wall.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 1 of the timber modules: a 4800 x 2700 framed wall with the default WallGuide, a door and a window. Bottom and top rails, the bottom rail stopping at the door, per opening a full-height jamb stud either side, a header between them and for the window a sill, then the studs on the 625 mm grid from the wall's start, a grid stud closer than half the spacing to a jamb or an edge stud left out, so no member doubles; cripples over the door and over and under the window; every member is 60 x 160. Then one 15 mm sheathing panel on each face over the whole wall, the door a notch and the window a hole. Every member end on the member it stands on and every panel on every member under it is a face contact, stored as their interaction and drawn in contacts_frame and contacts_sheathing; no connector is made. The wall is a group of beams and plates in the tree, nothing more.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_1_wall --parallel 6 && ./build/templates_module_1_wall && ../bash/publish-scene.sh --target templates_module_1_wall

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
