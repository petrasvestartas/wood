#include "wood_session.h"
#include "src/templates/developable_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// A corrugated funnel column of curved-crease unfolding: a flaring arc mirrored in 32 meridian planes around the vertical axis.
int main() {

    CurvedCreaseUnfolding unfolding(
        Primitives::arc(
            Point(800.0, 0.0, 0.0),
            Point(
                5800.0 - 5000.0 * std::cos(0.25 * Tolerance::PI),
                0.0,
                5000.0 * std::sin(0.25 * Tolerance::PI)
            ),
            Point(5800.0, 0.0, 5000.0)
        ),
        75.0 * Tolerance::TO_RADIANS,
        CurvedCreaseUnfolding::fan(
            Point(0.0, 0.0, 0.0),
            Vector(0.0, 0.0, 1.0),
            Vector(1.0, 0.0, 0.0),
            32,
            11.25 * Tolerance::TO_RADIANS
        ),
        40.0
    );
    std::cout << unfolding << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<FaceCurvedElement>> plates =
        unfolding.get_elements_placed<FaceCurvedElement>("plate");

    for (const std::shared_ptr<FaceCurvedElement>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format(
        "corrugated funnel: {} plates, {} closed solids, {} contacts, development distortion "
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
Step 12 of the developable folding templates: a corrugated funnel column by consecutive radial plane inclinations. The crease is a concave quarter-circle of 5 m radius flaring from a 0.8 m base stem to a 5.8 m rim at 5 m height; 33 radial meridian planes 11.25 degrees apart mirror 32 axis-shifted 40 mm anticlastic CCU plates into a closed 360-degree funnel shell that folds flat into a compact radial stack at w = 0. Groups: plates, creases, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_12_corrugated_funnel --parallel 6 && ./build/templates_developable_folding_12_corrugated_funnel && ../bash/publish-scene.sh --target templates_developable_folding_12_corrugated_funnel

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
