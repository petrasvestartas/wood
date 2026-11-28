#include "wood_session.h"
#include "src/templates/cylindrical_folding/cylinder_carving.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_cylindrical_folding;

/// A cylinder carved by tilted cylinders in sequence: a - b, then - c, then - d, side by side, only the carved patches.
int main() {

    WoodSession stages("cylinder_carving");

    for (size_t count = 1; count <= 3; count++) {
        // every stage moved along x, the stock carved by its first count carvers
        const Vector shift(9000.0 * static_cast<double>(count - 1), 0.0, 0.0);
        CylinderCarving carving(CylinderCarvingGuide::column(count, shift), fmt::format("carving_{}", count));
        const size_t plates = carving.get_elements_placed<FaceCurvedElement>("plate").size();
        std::cout << fmt::format("{} carvers: {} plates, {} contacts, development distortion {:.2e}\n", count, plates, carving.get_contacts().size(), carving.distortion());
        stages.graft(carving, stages.add_group(fmt::format("stage_{}", count)));
    }

    stages.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 16 of the cylindrical folding templates: a cylinder carved by cylinders in sequence. The stock a, 2 m radius and 12 m high, standing on its axis, is carved by b of 1.5 m radius, then by c of 1.8 m, then by d of 1.3 m, their axes 40 degrees apart round one side, every one cutting 700 mm deep and turned a few degrees off a's axis; the three stages a - b, (a - b) - c and ((a - b) - c) - d stand side by side. Only the carved patches are made: every carver's surface inside the stock and outside the other carvers, curving inward, one 60 mm plate behind it. The carving is validated first: every carver must leave a patch, none may cut another's patch in two, and two patches must meet at 10 degrees or more. Groups: stage_<carvers>.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_cylindrical_folding_16_cylinder_carving --parallel 6 && ./build/templates_cylindrical_folding_16_cylinder_carving && ../bash/publish-scene.sh --target templates_cylindrical_folding_16_cylinder_carving

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
