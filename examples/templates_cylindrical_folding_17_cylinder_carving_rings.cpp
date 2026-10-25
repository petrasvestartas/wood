#include "wood_session.h"
#include "src/templates/cylindrical_folding/cylinder_carving.h"

using namespace session_cpp;
using namespace wood_session;

/// Closed rings of carved patches: a cylinder carved all round by 4, 6 and 8 cylinders, their axes tilted in turn, side by side.
int main() {

    WoodSession rings("cylinder_carving_rings");
    const std::array<size_t, 3> counts = {4, 6, 8};
    const std::array<double, 3> radii = {2600.0, 2000.0, 1600.0};
    const std::array<double, 3> depths = {1400.0, 1100.0, 1100.0};

    for (size_t r = 0; r < counts.size(); r++) {
        // the stock along x, every carver at an equal step round it, cutting depth deep, turned off its axis one way and the other in turn
        const Vector shift(0.0, 9000.0 * static_cast<double>(r), 0.0);
        const CylinderCarving::Cylinder stock = {Point(0.0, 0.0, 0.0) + shift, Vector(1.0, 0.0, 0.0), 2000.0};
        std::vector<CylinderCarving::Cylinder> carvers;

        for (size_t i = 0; i < counts[r]; i++) {
            const double around = 2.0 * Tolerance::PI * static_cast<double>(i) / static_cast<double>(counts[r]);
            const Vector out(0.0, std::cos(around), std::sin(around));
            const Vector across(0.0, -std::sin(around), std::cos(around));
            const double tilt = i % 2 == 0 ? 0.05 : -0.05;
            const Point axis = Point(6000.0, 0.0, 0.0) + shift + out * (stock.radius + radii[r] - depths[r]);
            carvers.push_back({axis, Vector(1.0, 0.0, 0.0) + across * tilt, radii[r]});
        }

        CylinderCarving carving(
            stock,
            carvers,
            12000.0,
            60.0,
            10.0 * Tolerance::TO_RADIANS,
            true,
            fmt::format("ring_{}", counts[r])
        );
        const size_t plates = carving.get_elements_placed<PlateCurved>("plate").size();
        std::cout << fmt::format("ring of {}: {} plates, {} contacts, development distortion {:.2e}\n", counts[r], plates, carving.get_contacts().size(), carving.distortion());
        rings.graft(carving, rings.add_group(fmt::format("ring_{}", counts[r])));
    }

    rings.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 17 of the cylindrical folding templates: closed rings of carved patches. The stock, 2 m radius and 12 m long along x, is carved all round by 4 cylinders of 2.6 m radius cutting 1400 mm deep, by 6 of 2 m cutting 1100 mm and by 8 of 1.6 m cutting 1100 mm, every carver's axis turned off the stock's one way and the other in turn; the three rings stand side by side. Every patch meets both neighbours, so the plates close round the stock into a fluted tube, one contact per crease; the carving is validated closed, every point of the stock's surface carved, so the plates leave no gap. Groups: ring_<carvers>.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_cylindrical_folding_17_cylinder_carving_rings --parallel 6 && ./build/templates_cylindrical_folding_17_cylinder_carving_rings && ../bash/publish-scene.sh --target templates_cylindrical_folding_17_cylinder_carving_rings

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
