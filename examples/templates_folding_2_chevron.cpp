#include "wood_session.h"
#include "src/templates/folding/chevron.h"

using namespace session_cpp;
using namespace wood_session;

/// The chevron shell on the first Annen surface: the surface, the mesh, its strips and planes, four plates per face.
int main() {

    const std::filesystem::path path = config::session_data_dir() / "annen_surfaces.json";
    const std::vector<NurbsSurface> surfaces = wood_chevron::annen_surfaces(path.string());
    Chevron chevron(surfaces[0]);
    const std::vector<std::shared_ptr<Plate>> plates = chevron.get_elements_numbered<Plate>("plate");
    std::cout << chevron << std::endl;
    std::cout << fmt::format("chevron: {} faces, {} plates\n", chevron.mesh().number_of_faces(), plates.size());
    chevron.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The chevron template on the first of the 23 Annen surfaces in annen_surfaces.json: four strips of chevron rows growing towards the middle, a plane on every face edge with the chevron edges turned and offset, the corner bisectors, then per face a top and a bottom plate inset in the 760 mm box and a side plate on each chevron edge, with the insertion vectors, joint types, three-valence groups and adjacency the solver reads; each step in its own group (surface, mesh, strips, edge_planes, bisectors, plates, insertion).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_folding_2_chevron --parallel 6 && ./build/templates_folding_2_chevron && ../bash/publish-scene.sh --target templates_folding_2_chevron

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
