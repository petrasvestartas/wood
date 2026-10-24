#include "wood_session.h"
#include "src/templates/curved_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// Curved-crease unfolding drawn in: the crease planes fanned around a horizontal axis 20 m up, closer at the apex than at the supports,
/// a synclastic corrugated vault.
int main() {

    CurvedCreaseUnfolding unfolding(
        Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)),
        50.0 * Tolerance::TO_RADIANS,
        CurvedCreaseUnfolding::leaning(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 16, 500.0, 1.0 * Tolerance::TO_RADIANS),
        40.0
    );
    std::cout << unfolding << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> plates = unfolding.get_elements_placed<PlateCurved>("plate");

    for (const std::shared_ptr<PlateCurved>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("synclastic unfolding: {} plates, {} closed solids, {} contacts, development distortion {:.2e}, support drift {:.0f} mm\n", plates.size(), solids, unfolding.get_contacts().size(), unfolding.distortion(), unfolding.drift());
    unfolding.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 23 of the curved folding templates: a synclastic corrugated vault by plane inclination. The crease is a semicircle 9 m wide and 4.5 m high; the seventeen crease planes are fanned 1.5 degrees apart around a horizontal axis along the span 20 m above the floor, so they stand closer at the apex than at the supports and the end arches lean in. The rulings leave the crease square to it at 50 degrees to its plane toward its concave side; every plate is the mirror image of its neighbour in their shared crease plane, a developable cut flat, 40 mm thick, mitred on the crease planes. Groups: plates (plate_<k>), creases.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_23_synclastic_unfolding --parallel 6 && ./build/templates_curved_folding_23_synclastic_unfolding && ./bash/publish-scene.sh --target templates_curved_folding_23_synclastic_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
