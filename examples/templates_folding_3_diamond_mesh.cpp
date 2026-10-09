#include "wood_session.h"
#include "src/templates/folding/diamond_mesh.h"

using namespace session_cpp;
using namespace wood_session;

/// The diamond mesh on its default arch: the surface, the rhombus mesh and one 40 mm plate per triangle.
int main() {

    DiamondMesh diamond(
        DiamondMesh::default_surface(),
        8,
        4,
        40.0,
        10.0,
        180.0
    );
    const std::vector<std::shared_ptr<Plate>> plates = diamond.get_elements_numbered<Plate>("plate");
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
cmake --build build --target templates_folding_3_diamond_mesh --parallel 6 && ./build/templates_folding_3_diamond_mesh && ../bash/publish-scene.sh --target templates_folding_3_diamond_mesh

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
