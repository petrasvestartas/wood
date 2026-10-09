#include "wood_session.h"
#include "src/templates/cross/vda_mesh.h"

using namespace session_cpp;
using namespace wood_session;

/// The vda mesh on its default dome: a mitred plate per face, two connector plates across every interior edge.
int main() {

    wood_cross::VdaMesh vda(
        wood_cross::VdaMesh::default_mesh(),
        40.0,
        {0.0},
        {2},
        {},
        {},
        300.0,
        300.0,
        40.0
    );
    std::cout << vda << std::endl;

    size_t corners = 0;
    size_t bisectors = 0;

    for (const std::vector<Plane>& planes : vda.bisector_planes())
        for (const Plane& plane : planes) {
            corners++;
            bisectors += plane.is_valid() ? 1 : 0;
        }

    std::cout << fmt::format("bisector planes: {} valid of {} corners", bisectors, corners) << std::endl;
    vda.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The vda mesh template on its default fifteen-face dome, 40 thick plates and 300 x 300 x 40 connectors: the welded mesh, the face, edge and bisector planes, one plate per face cut back by its edge planes so neighbours meet in mitres, and two connector frames and plates across every interior edge, each step in its group.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_cross_1_vda_mesh --parallel 6 && ./build/templates_cross_1_vda_mesh && ../bash/publish-scene.sh --target templates_cross_1_vda_mesh

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
