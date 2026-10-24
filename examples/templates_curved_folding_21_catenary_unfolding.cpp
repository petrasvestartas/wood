#include "wood_session.h"
#include "src/templates/curved_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// Curved-crease unfolding to a catenary: twenty plates cut to the catenary's curvature times cos 45 degrees, opened 45 degrees into a corrugated catenary vault 10 m wide and 6 m high.
int main() {

    CurvedCreaseUnfolding unfolding(
        CurvedCreaseUnfolding::catenary(10000.0, 6000.0),
        45.0 * Tolerance::TO_RADIANS,
        CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 20, 354.0, 0.0),
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
Step 21 of the curved folding templates: curved-crease unfolding to a catenary. Twenty plates 500 mm wide and 40 mm thick, hinged on their curved edges, open 45 degrees on twenty-one parallel planes 354 mm apart into a corrugated catenary vault 10 m wide and 6 m high, the given crease the catenary itself; every plate the developable strip of rulings leaving the crease square to it at the opening. Every plate is a PlateCurved between the two planes of its creases, mitred on them so it shares one face with each neighbour, its development checked flat; the session's contact detection finds every shared face. Groups: plates (plate_<k>), creases.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_21_catenary_unfolding --parallel 6 && ./build/templates_curved_folding_21_catenary_unfolding && ./bash/publish-scene.sh --target templates_curved_folding_21_catenary_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
