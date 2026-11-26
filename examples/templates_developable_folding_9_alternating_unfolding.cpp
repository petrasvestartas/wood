#include "wood_session.h"
#include "src/templates/developable_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// Corrugated CCU vault with alternating plane inclinations around the mid-height span axis.
int main() {

    CurvedCreaseUnfolding unfolding(
        Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)),
        55.0 * Tolerance::TO_RADIANS,
        CurvedCreaseUnfolding::zigzag(
            Point(0.0, 0.0, 2250.0),
            Vector(0.0, 1.0, 0.0),
            Vector(1.0, 0.0, 0.0),
            12,
            450.0,
            5.0 * Tolerance::TO_RADIANS
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
        "alternating unfolding: {} plates, {} closed solids, {} contacts, development distortion "
        "{:.2e}, support drift {:.0f} mm\n",
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
Step 9 of the developable folding templates: corrugated CCU vault with alternating osculating-plane inclinations about the actuation axis at mid-height. Thirteen planes 450 mm apart over a 9 m semicircle (4.5 m rise, 55-degree opening, 30 mm plates), every odd plane inclined +-5 degrees about the horizontal span axis at z = 2250 mm so corrugation depth and width articulate alternatingly at the crown and supports across three full waves while keeping every crease planar during deployment. Groups: plates, creases, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_9_alternating_unfolding --parallel 6 && ./build/templates_developable_folding_9_alternating_unfolding && ../bash/publish-scene.sh --target templates_developable_folding_9_alternating_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
