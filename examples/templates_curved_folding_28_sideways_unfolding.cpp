#include "wood_session.h"
#include "src/templates/curved_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// Curved-crease unfolding with every second crease plane tilted about a vertical axis at the crown: plates alternately wider at one
/// foot and at the other.
int main() {

    CurvedCreaseUnfolding unfolding(
        Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)),
        50.0 * Tolerance::TO_RADIANS,
        CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 16, 500.0, 4.0 * Tolerance::TO_RADIANS),
        40.0
    );
    std::cout << unfolding << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> plates = unfolding.get_elements_placed<PlateCurved>("plate");

    for (const std::shared_ptr<PlateCurved>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("sideways unfolding: {} plates, {} closed solids, {} contacts, development distortion {:.2e}, support drift {:.0f} mm\n", plates.size(), solids, unfolding.get_contacts().size(), unfolding.distortion(), unfolding.drift());
    unfolding.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 28 of the curved folding templates: plane inclination perpendicular to the actuation axis. The semicircle 9 m wide and 4.5 m high; seventeen crease planes 500 mm apart, every second one turned 4 degrees about a vertical axis through the crown: the plane distance shrinks toward one foot and grows toward the other, plate by plate the other way, the vault articulated alternately to its sides. Groups: plates (plate_<k>), creases.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_28_sideways_unfolding --parallel 6 && ./build/templates_curved_folding_28_sideways_unfolding && ./bash/publish-scene.sh --target templates_curved_folding_28_sideways_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
