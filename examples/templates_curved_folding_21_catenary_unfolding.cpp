#include "wood_session.h"
#include "src/templates/curved_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// Curved-crease unfolding to a catenary: two plates opened 45 degrees into a catenary vault 10 m wide and 6 m high.
int main() {

    CurvedCreaseUnfolding unfolding(
        CurvedCreaseUnfolding::catenary(10000.0, 6000.0),
        45.0 * Tolerance::TO_RADIANS,
        CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 2, 1500.0, 0.0),
        40.0
    );
    std::cout << unfolding << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> plates = unfolding.get_elements_placed<PlateCurved>("plate");

    for (const std::shared_ptr<PlateCurved>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("catenary unfolding: {} plates, {} closed solids, {} contacts, development distortion {:.2e}, drift {:.3f} mm\n", plates.size(), solids, unfolding.get_contacts().size(), unfolding.distortion(), unfolding.drift());
    unfolding.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 21 of the curved folding templates: curved-crease unfolding to a catenary. Three parallel planes 1500 mm apart, the crease a catenary 10 m wide and 6 m high, the plates opened 45 degrees. Each plate is a PlateCurved mitred on its two crease planes, sharing one face with its neighbour, its development checked flat. Groups: plates, creases.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_21_catenary_unfolding --parallel 6 && ./build/templates_curved_folding_21_catenary_unfolding && ./bash/publish-scene.sh --target templates_curved_folding_21_catenary_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
