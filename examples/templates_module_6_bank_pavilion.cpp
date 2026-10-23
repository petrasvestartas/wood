#include "wood_session.h"
#include "src/templates/module/room_module.h"

using namespace session_cpp;
using namespace wood_session;

/// The bank pavilion of four modules from the Blumer Lehmann brochure: a meeting room module, an open office of two modules, and a module with the entrance porch.
int main() {

    wood_module::RoomModuleGuide guide;
    guide.length = 10270.0;
    guide.stud_depth = 180.0;
    guide.rooms = {1, 2, 1};
    guide.layouts = {
        {{{{}, {{4825.0, 1590.0, 0.0, 2100.0}}, {{330.0, 1620.0, 900.0, 2400.0}}, {{330.0, 1620.0, 900.0, 2400.0}}}}, 0.0},
        {{{{}, {}, {{330.0, 1620.0, 900.0, 2400.0}}, {{330.0, 1620.0, 900.0, 2400.0}}}}, 0.0},
        {{{{}, {}, {{330.0, 1620.0, 900.0, 2400.0}}, {{330.0, 1620.0, 900.0, 2400.0}}}}, 0.0},
        {{{{{395.0, 1550.0, 0.0, 2100.0}, {5295.0, 890.0, 0.0, 2100.0}, {7557.0, 890.0, 0.0, 2100.0}},
           {{1268.0, 720.0, 1500.0, 2400.0}},
           {{120.0, 910.0, 900.0, 2400.0}, {1295.0, 910.0, 900.0, 2400.0}},
           {{323.0, 1020.0, 900.0, 2400.0}}}}, 2730.0},
    };
    wood_module::RoomModule pavilion(guide, "bank_pavilion");
    std::cout << pavilion << std::endl;

    const size_t frame = pavilion.get_branch("contacts_frame").objects.meshes->size();
    const size_t sheathing = pavilion.get_branch("contacts_sheathing").objects.meshes->size();
    std::cout << fmt::format("bank pavilion: {} modules of {} x {} mm, {} frame contacts, {} sheathing contacts", pavilion.columns(), guide.length, guide.width, frame, sheathing) << std::endl;
    pavilion.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 6 of the timber modules: the temporary bank pavilion of four modules from the Blumer Lehmann brochure "Modulares Bauen" (2025, p. 17, "Grundriss mit 4 Modulen"), measured on its plan at the 3.0 m module axis grid, which its 0.7, 0.9 and 1.0 m doors confirm: modules 3000 x 10270 with walls 210 thick, so the open office of two modules is 6000 clear across and every module 9850 clear along, windows 1620 wide 330 from the inner corner at both ends. Module 1 is the meeting room, closed, its wall towards the office with a passage; modules 2 and 3 are the open office, open between them with a pair of posts at mid-length and open towards modules 1 and 4, whose walls close the office; module 4 holds the entrance porch, its start wall set back 2730, the entrance door in its wall towards the office, two doors further on, a WC window outside. Sills, heads and the 2700 clear height are not in the brochure and are assumed; partitions inside the modules and the facade cladding are not built.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_6_bank_pavilion --parallel 6 && ./build/templates_module_6_bank_pavilion && ../bash/publish-scene.sh --target templates_module_6_bank_pavilion

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
