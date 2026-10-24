#include "wood_session.h"
#include "src/templates/curved_folding/inclined_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// A corrugated dome of curved-crease unfolding: a meridian arc mirrored in 24 meridian planes around the vertical axis.
int main() {

    InclinedCreaseUnfolding unfolding(
        Primitives::arc(Point(6000.0, 0.0, 0.0), Point(6000.0 * std::cos(0.7), 0.0, 6000.0 * std::sin(0.7)), Point(6000.0 * std::cos(1.4), 0.0, 6000.0 * std::sin(1.4))),
        60.0 * Tolerance::TO_RADIANS,
        InclinedCreaseUnfolding::fan(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), Vector(1.0, 0.0, 0.0), 24, 15.0 * Tolerance::TO_RADIANS),
        40.0
    );
    std::cout << unfolding << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> plates = unfolding.get_elements_placed<PlateCurved>("plate");

    for (const std::shared_ptr<PlateCurved>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("corrugated dome: {} plates, {} closed solids, development distortion {:.2e}, support drift {:.0f} mm\n", plates.size(), solids, unfolding.distortion(), unfolding.drift());
    unfolding.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 29 of the curved folding templates: a corrugated dome by consecutive plane inclinations in a radial layout (Scheder-Bieschin, Van Mele and Block, DMSB 2023, Fig. 12b). The crease is a meridian arc of a 6 m sphere from the floor up to 80 degrees, an oculus of 2.1 m left open; the 25 crease planes are the meridian planes 15 degrees apart around the vertical axis. The rulings leave the arc square to it at 60 degrees to its plane toward the centre; every plate is the mirror image of its neighbour in their meridian plane, so the ridges are the arc turned by 30 degrees and the valleys its mirror images, a star in plan. 24 plates 40 mm thick, each a developable mitred on its two meridian planes. Groups: plates (plate_<k>), creases.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_29_corrugated_dome --parallel 6 && ./build/templates_curved_folding_29_corrugated_dome && ../bash/publish-scene.sh --target templates_curved_folding_29_corrugated_dome

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
