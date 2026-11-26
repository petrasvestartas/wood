#include "wood_session.h"
#include "src/templates/developable_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// Doubly-curved synclastic CCU vault by modulated parallel osculating plane distances.
int main() {

    CurvedCreaseUnfolding unfolding(
        Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)),
        55.0 * Tolerance::TO_RADIANS,
        CurvedCreaseUnfolding::vault(
            Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 12, 450.0, 95.0
        ),
        30.0
    );
    std::cout << unfolding << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<FaceCurvedElement>> plates =
        unfolding.get_elements_placed<FaceCurvedElement>("plate");

    for (const std::shared_ptr<FaceCurvedElement>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format(
        "synclastic unfolding: {} plates, {} closed solids, {} contacts, development distortion "
        "{:.2e}, support drift {:.3f} mm\n",
        plates.size(),
        solids,
        unfolding.get_contacts().size(),
        unfolding.distortion(),
        unfolding.drift()
    );
    unfolding.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 5 of the developable folding templates: doubly-curved synclastic corrugated vault by plane-distance modulation. Twelve 30 mm plates across thirteen parallel osculating planes whose inward valley-to-ridge distances step down away from the mid-span crease (450 mm base spacing, 95 mm step, 55-degree reflection angle): both the apex profile and the support plan curve smoothly inward along a barrel dome while every support foot remains flush on the horizontal floor (0 mm drift) and every crease remains planar throughout deployment. Groups: plates, creases, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_5_synclastic_unfolding --parallel 6 && ./build/templates_developable_folding_5_synclastic_unfolding && ../bash/publish-scene.sh --target templates_developable_folding_5_synclastic_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
