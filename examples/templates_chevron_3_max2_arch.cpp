#include "wood_session.h"
#include "src/templates/chevron/chevron.h"

using namespace session_cpp;
using namespace wood_session;

/// The chevron shell on the second Annen surface with the sliders of the 2017 MAX2 prototype: arch 22 as it was built.
int main() {

    const std::filesystem::path path = config::session_data_dir() / "annen_surfaces.json";
    const std::vector<NurbsSurface> surfaces = wood_chevron::annen_surfaces(path.string());
    Chevron chevron(
        surfaces[1],
        4,
        900.0,
        0.5,
        0.04993,
        760.0,
        80.0,
        40.0,
        1.1,
        0.45,
        80.0,
        15.0,
        7,
        2000.0,
        {1, 1, 1, 1},
        {132000.0, 126000.0, 3520.0}
    );
    const std::vector<std::shared_ptr<FacePlanarElement>> plates =
        chevron.get_elements_numbered<FacePlanarElement>("plate");
    std::cout << chevron << std::endl;
    std::cout << fmt::format(
        "chevron max2 arch 22: {} faces, {} plates\n",
        chevron.mesh().number_of_faces(),
        plates.size()
    );
    chevron.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The chevron template on the second of the 23 Annen surfaces with the slider values of the Grasshopper definition that drove Christopher Robeller's plugin for the 2017 MAX2 prototype of arches 21 and 22 (2017_08_08_MAX2_22.gh): rows from 900 growing by 0.04993, stopping 2000 before the middle, boxes 760 deep with 40 mm plates 80 in from the box faces, the chevron edge planes turned 1.1 degrees and moved 18 mm, the sides closed at y 132000 and 126000 and the ground at z 3520, tenons on 80 mm tabs and corner fingers turned 15 degrees; the 200 boxes of arch 22 with the plates and joints of the prototype's Rhino file, each step in its own group (surface, mesh, strips, edge_planes, bisectors, plates with a box_<face> group per face, insertion, joints).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_chevron_3_max2_arch --parallel 6 && ./build/templates_chevron_3_max2_arch && ../bash/publish-scene.sh --target templates_chevron_3_max2_arch

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
