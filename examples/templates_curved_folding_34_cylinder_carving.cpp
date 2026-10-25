#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_carving.h"

using namespace session_cpp;
using namespace wood_session;

/// A cylinder carved by tilted cylinders in sequence: a - b, then - c, then - d, side by side, only the carved patches.
int main() {

    WoodSession stages("cylinder_carving");

    for (size_t count = 1; count <= 3; count++) {
        // every stage moved along y, the stock a along x and the carvers of their own radii, turned a few degrees off its axis
        const Vector shift(0.0, 9000.0 * static_cast<double>(count - 1), 0.0);
        const CylinderCarving::Cylinder stock = {Point(0.0, 0.0, 0.0) + shift, Vector(1.0, 0.0, 0.0), 2000.0};
        const std::vector<CylinderCarving::Cylinder> all = {
            {Point(6000.0, 1700.0, 2100.0) + shift, Vector(1.0, 0.06, 0.0), 1500.0},
            {Point(6000.0, -100.0, 2700.0) + shift, Vector(1.0, -0.05, 0.02), 1800.0},
            {Point(6000.0, -2000.0, 2000.0) + shift, Vector(1.0, 0.04, -0.03), 1300.0},
        };
        const std::vector<CylinderCarving::Cylinder> carvers(all.begin(), all.begin() + static_cast<long>(count));
        CylinderCarving carving(
            stock,
            carvers,
            12000.0,
            60.0,
            fmt::format("carving_{}", count)
        );
        const size_t plates = carving.get_elements_placed<PlateCurved>("plate").size();
        std::cout << fmt::format("{} carvers: {} plates, {} contacts, development distortion {:.2e}\n", count, plates, carving.get_contacts().size(), carving.distortion());
        stages.graft(carving, stages.add_group(fmt::format("stage_{}", count)));
    }

    stages.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 34 of the curved folding templates: a cylinder carved by cylinders in sequence. The stock a, 2 m radius and 12 m long along x, is carved by b of 1.5 m radius, then by c of 1.8 m, then by d of 1.3 m, every axis turned a few degrees off a's and shifted over its top; the three stages a - b, (a - b) - c and ((a - b) - c) - d stand side by side. Only the carved patches are made: every carver's surface inside the stock and outside the other carvers, curving inward, one 60 mm plate behind it. Every generator is clipped exactly, so every plate is a cylinder; two patches meet on the curve where their cylinders cross, both plates ending on the same inner crease, and a patch ends at the stock's surface and its end planes. Groups: stage_<carvers>.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_34_cylinder_carving --parallel 6 && ./build/templates_curved_folding_34_cylinder_carving && ../bash/publish-scene.sh --target templates_curved_folding_34_cylinder_carving

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
