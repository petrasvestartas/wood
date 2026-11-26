#include "wood_session.h"
#include "src/templates/folding/diamond_mesh.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_folding;

/// The diamond mesh on its default arch: the surface, the rhombus mesh and one 40 mm plate per triangle.
int main() {

    DiamondMesh diamond(DiamondMeshGuide::diamond_mesh());
    const std::vector<std::shared_ptr<FacePlanarElement>> plates =
        diamond.get_elements_numbered<FacePlanarElement>("plate");
    std::cout << diamond << std::endl;
    std::cout << fmt::format("diamond mesh: {} plates\n", plates.size());
    diamond.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The diamond mesh template on its default arch, 3000 by 5000 with a rise of 1500: 8 by 4 cells split into rhombi between the cell centres, two triangles each, one 40 mm plate per triangle with 10 mm chamfers, each step in its own group (surface, mesh, plates).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_2_diamond_mesh --parallel 6 && ./build/templates_folding_2_diamond_mesh && ../bash/publish-scene.sh --target templates_folding_2_diamond_mesh

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
