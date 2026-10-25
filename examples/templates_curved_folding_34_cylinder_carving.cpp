#include "wood_session.h"
#include "src/templates/curved_folding/cylinder_carving.h"

using namespace session_cpp;
using namespace wood_session;

/// A cylinder carved by tilted cylinders in sequence: a - b, then - c, then - d, side by side, only the carved patches.
int main() {

    WoodSession stages("cylinder_carving");
    const std::array<double, 3> radii = {1500.0, 1800.0, 1300.0};
    const std::array<double, 3> tilts = {0.06, -0.05, 0.04};

    for (size_t count = 1; count <= 3; count++) {
        // every stage moved along y; the stock a along x, every carver 40 degrees apart over its top, cutting 700 mm deep, turned off its axis
        const Vector shift(0.0, 9000.0 * static_cast<double>(count - 1), 0.0);
        const CylinderCarving::Cylinder stock = {Point(0.0, 0.0, 0.0) + shift, Vector(1.0, 0.0, 0.0), 2000.0};
        std::vector<CylinderCarving::Cylinder> carvers;

        for (size_t i = 0; i < count; i++) {
            const double around = (130.0 - 40.0 * static_cast<double>(i)) * Tolerance::TO_RADIANS;
            const Vector out(0.0, std::cos(around), std::sin(around));
            const Vector across(0.0, -std::sin(around), std::cos(around));
            const Point axis = Point(6000.0, 0.0, 0.0) + shift + out * (stock.radius + radii[i] - 700.0);
            carvers.push_back({axis, Vector(1.0, 0.0, 0.0) + across * tilts[i], radii[i]});
        }

        CylinderCarving carving(
            stock,
            carvers,
            12000.0,
            60.0,
            10.0 * Tolerance::TO_RADIANS,
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
Step 34 of the curved folding templates: a cylinder carved by cylinders in sequence. The stock a, 2 m radius and 12 m long along x, is carved by b of 1.5 m radius, then by c of 1.8 m, then by d of 1.3 m, their axes 40 degrees apart over its top, every one cutting 700 mm deep and turned a few degrees off a's axis; the three stages a - b, (a - b) - c and ((a - b) - c) - d stand side by side. Only the carved patches are made: every carver's surface inside the stock and outside the other carvers, curving inward, one 60 mm plate behind it. The carving is validated first: every carver must leave a patch, none may cut another's patch in two, and two patches must meet at 10 degrees or more. Groups: stage_<carvers>.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_34_cylinder_carving --parallel 6 && ./build/templates_curved_folding_34_cylinder_carving && ../bash/publish-scene.sh --target templates_curved_folding_34_cylinder_carving

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
