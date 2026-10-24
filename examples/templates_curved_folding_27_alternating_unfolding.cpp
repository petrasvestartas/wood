#include "wood_session.h"
#include "src/templates/curved_folding/inclined_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// Curved-crease unfolding with every second crease plane tilted about the span at half height: ridges and valleys articulated
/// alternately at the apex and at the supports.
int main() {

    InclinedCreaseUnfolding unfolding(
        Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)),
        50.0 * Tolerance::TO_RADIANS,
        InclinedCreaseUnfolding::zigzag(Point(0.0, 0.0, 2250.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 16, 500.0, 4.0 * Tolerance::TO_RADIANS),
        40.0
    );
    std::cout << unfolding << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> plates = unfolding.get_elements_placed<PlateCurved>("plate");

    for (const std::shared_ptr<PlateCurved>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("alternating unfolding: {} plates, {} closed solids, development distortion {:.2e}, support drift {:.0f} mm\n", plates.size(), solids, unfolding.distortion(), unfolding.drift());
    unfolding.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 27 of the curved folding templates: plane inclination around the actuation axis (Scheder-Bieschin, Van Mele and Block, DMSB 2023, Fig. 11c). The semicircle 9 m wide and 4.5 m high; seventeen crease planes 500 mm apart, every second one tilted 4 degrees about a line along the span at half height: a plate is wide at the apex and narrow at the supports, the next the other way round, the ridges and valleys articulated alternately at the crown and at the feet. Groups: plates (plate_<k>), creases.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_27_alternating_unfolding --parallel 6 && ./build/templates_curved_folding_27_alternating_unfolding && ../bash/publish-scene.sh --target templates_curved_folding_27_alternating_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
