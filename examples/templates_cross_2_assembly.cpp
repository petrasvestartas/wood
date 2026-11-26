#include "wood_session.h"
#include "src/templates/cross/vda_mesh.h"

using namespace session_cpp;
using namespace wood_session;

/// The vda mesh on its default dome, its connectors turned so every plate slides onto all of them at once.
int main() {

    wood_cross::VdaMesh vda(wood_cross::VdaMeshGuide::dome_assembly());
    std::cout << vda << std::endl;
    vda.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The vda mesh template on its default fifteen-face dome, made to go together: VdaMesh::assembly_order goes from one foot ring by ring, breadth first, up over the crown and down to the other foot; a set is a plate with its connectors to the plates after it plugged in, and VdaMesh::assembly_lines turns the connectors of every edge along the slide of its later plate, so a set slides onto the connectors of all its earlier neighbours along one vector. 40 thick plates, 260 x 260 x 40 connectors, two per edge, cross slots where they pass through the plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_cross_2_assembly --parallel 6 && ./build/templates_cross_2_assembly && ../bash/publish-scene.sh --target templates_cross_2_assembly

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
