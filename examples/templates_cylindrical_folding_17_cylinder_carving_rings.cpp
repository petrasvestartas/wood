#include "wood_session.h"
#include "src/templates/cylindrical_folding/cylinder_carving.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_cylindrical_folding;

/// Closed rings of carved patches: a cylinder carved all round by 4, 6 and 8 cylinders, their axes tilted in turn, side by side.
int main() {

    WoodSession rings("cylinder_carving_rings");
    const std::array<size_t, 3> counts = {4, 6, 8};
    const std::array<double, 3> radii = {2000.0, 1600.0, 1200.0};
    const std::array<double, 3> depths = {1600.0, 1000.0, 800.0};

    for (size_t r = 0; r < counts.size(); r++) {
        // every ring moved along x, the stock carved all round by counts[r] carvers
        const Vector shift(9000.0 * static_cast<double>(r), 0.0, 0.0);
        CylinderCarving carving(CylinderCarvingGuide::ring(counts[r], radii[r], depths[r], shift), fmt::format("ring_{}", counts[r]));
        const size_t plates = carving.get_elements_placed<FaceCurvedElement>("plate").size();
        std::cout << fmt::format("ring of {}: {} plates, {} contacts, development distortion {:.2e}\n", counts[r], plates, carving.get_contacts().size(), carving.distortion());
        rings.graft(carving, rings.add_group(fmt::format("ring_{}", counts[r])));
    }

    rings.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 17 of the cylindrical folding templates: closed rings of carved patches. The stock, 2 m radius and 12 m high, standing on its axis, is carved all round by 4 cylinders of 2 m radius cutting 1600 mm deep, by 6 of 1.6 m cutting 1000 mm and by 8 of 1.2 m cutting 800 mm, every carver's axis turned off the stock's one way and the other in turn; the three rings stand side by side. Every patch meets both neighbours, so the plates close round the stock into a fluted tube, one contact per crease; the carving is validated closed, every point of the stock's surface carved, so the plates leave no gap. Groups: ring_<carvers>.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_cylindrical_folding_17_cylinder_carving_rings --parallel 6 && ./build/templates_cylindrical_folding_17_cylinder_carving_rings && ../bash/publish-scene.sh --target templates_cylindrical_folding_17_cylinder_carving_rings

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
