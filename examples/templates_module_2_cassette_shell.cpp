#include "wood_session.h"
#include "src/templates/module/cassette_shell.h"

using namespace session_cpp;
using namespace wood_session;

/// A segmented shell of hollow cassettes on the VDA dome: two thin plates and mitered edge beams per cell, the contacts between cells detected.
int main() {

    wood_module::CassetteShell shell;
    std::cout << shell << std::endl;

    const size_t contacts = shell.get_branch("contacts").objects.polylines->size();
    std::cout << fmt::format("cassette shell: {} cassettes, {} contacts between them", shell.mesh.number_of_faces(), contacts) << std::endl;
    shell.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 2 of the timber modules: the segmented hex shell after the BUGA Wood Pavilion 2019 on the fifteen-face dome of the VDA mesh. Each cell is a hollow cassette 200 deep: a 20 mm top and bottom plate and between them an edge beam 60 wide on every face edge, its outer face on the plane the edge shares with the neighbouring cell, so it is cut at the angle between the two cells, its ends mitered with the next beam. Inside every cassette each plate touches each beam and every beam the next; between cassettes the two beams on an interior edge touch face to face on their shared plane, every such contact detected and drawn in contacts. No connector is made yet.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_module_2_cassette_shell --parallel 6 && ./build/templates_module_2_cassette_shell && ../bash/publish-scene.sh --target templates_module_2_cassette_shell

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
