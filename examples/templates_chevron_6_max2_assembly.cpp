#include "wood_session.h"
#include "src/templates/chevron/chevron.h"
#include "src/templates/chevron/chevron_assembly.h"

using namespace session_cpp;
using namespace wood_session;

/// Arch 22 of the 2017 MAX2 prototype assembled box by box: the order its tenons allow, every box swept along its insertion vector, and the 4 x 4 block at the crown.
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
    const wood_chevron::Assembly assembly(chevron);

    // the 4 x 4 block at the crown: the faces run foot to foot in 2 u_divisions rows of boxes, so the four middle rows, the four boxes about the crown of each
    const int runs = 2 * chevron.u_divisions;
    const int length = static_cast<int>(chevron.mesh().number_of_faces()) / runs;
    std::vector<int> block;

    for (int run = runs / 2 - 2; run < runs / 2 + 2; run++)
        for (int position = length / 2 - 1; position < length / 2 + 3; position++)
            block.push_back(run * length + position);

    std::cout << assembly.report() << std::endl;
    std::cout << fmt::format("block: {}\n", wood_chevron::Assembly::groups(block));
    assembly.write_json(
        (std::filesystem::path(pb_path("live")).parent_path() / "assembly.json").string(), block
    );
    chevron.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Arch 22 of the 2017 MAX2 prototype, the chevron template on the second Annen surface with the prototype's sliders, assembled box by box: every box slides in along its one insertion vector into the boxes already placed, in the order its through-tenons allow, and is swept along its vector against the plates already placed, points just inside its faces and along its edges stepped along the way; the report lists the plate pairs that meet with how deep. The 4 x 4 block at the crown, two rows of the two middle strips, is written with the order and the vectors to data/output/pb/assembly.json for tools/render_assembly_gif.py.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_chevron_6_max2_assembly --parallel 6 && ./build/templates_chevron_6_max2_assembly && ../bash/publish-scene.sh --target templates_chevron_6_max2_assembly

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
