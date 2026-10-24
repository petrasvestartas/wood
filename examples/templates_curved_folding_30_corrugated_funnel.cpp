#include "wood_session.h"
#include "src/templates/curved_folding/inclined_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// A corrugated funnel column of curved-crease unfolding: a flaring arc mirrored in 24 meridian planes around the vertical axis.
int main() {

    InclinedCreaseUnfolding unfolding(
        Primitives::arc(Point(800.0, 0.0, 0.0), Point(5800.0 - 5000.0 * std::cos(0.25 * Tolerance::PI), 0.0, 5000.0 * std::sin(0.25 * Tolerance::PI)), Point(5800.0, 0.0, 5000.0)),
        75.0 * Tolerance::TO_RADIANS,
        InclinedCreaseUnfolding::fan(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), Vector(1.0, 0.0, 0.0), 32, 11.25 * Tolerance::TO_RADIANS),
        40.0
    );
    std::cout << unfolding << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> plates = unfolding.get_elements_placed<PlateCurved>("plate");

    for (const std::shared_ptr<PlateCurved>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("corrugated funnel: {} plates, {} closed solids, development distortion {:.2e}, support drift {:.0f} mm\n", plates.size(), solids, unfolding.distortion(), unfolding.drift());
    unfolding.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 30 of the curved folding templates: a corrugated funnel column by consecutive plane inclinations in a radial layout (Scheder-Bieschin, Van Mele and Block, DMSB 2023, Fig. 12c). The crease is a quarter circle of 5 m radius flaring from a stem 0.8 m off the axis, vertical on the floor, to a rim 5.8 m out and 5 m up, horizontal there; the 33 crease planes are the meridian planes 11.25 degrees apart. The rulings leave the arc square to it at 75 degrees to its plane toward its concave side, outward and down; every plate is the mirror image of its neighbour in their meridian plane, a star in plan at the rim. 32 plates 40 mm thick, each a developable mitred on its two meridian planes. Groups: plates (plate_<k>), creases.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_30_corrugated_funnel --parallel 6 && ./build/templates_curved_folding_30_corrugated_funnel && ../bash/publish-scene.sh --target templates_curved_folding_30_corrugated_funnel

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
