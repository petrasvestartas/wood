#include "wood_session.h"
#include "src/templates/curved_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// Curved-crease unfolding to a catenary: twenty plates cut to the catenary's edge flattened by cos 45, drawn flat-folded and open into a
/// corrugated catenary vault 10 m wide and 6 m high.
int main() {

    CurvedCreaseUnfolding unfolding(
        CurvedCreaseUnfolding::catenary(10000.0, 6000.0),
        45.0 * Tolerance::TO_RADIANS,
        500.0,
        20,
        {0.0, 45.0 * Tolerance::TO_RADIANS},
        40.0
    );
    std::cout << unfolding << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> plates = unfolding.get_elements_placed<PlateCurved>("plate");

    for (const std::shared_ptr<PlateCurved>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    for (size_t i = 0; i < unfolding.openings.size(); i++)
        std::cout << fmt::format("opening {:.0f} deg: span {:.0f} mm, rise {:.0f} mm\n", unfolding.openings[i] * Tolerance::TO_DEGREES, unfolding.span_rise(i).first, unfolding.span_rise(i).second);

    std::cout << fmt::format("curved-crease unfolding: {} plates, {} closed solids, development distortion {:.2e}, isometry {:.3f} mm\n", plates.size(), solids, unfolding.distortion(), unfolding.isometry());
    unfolding.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 21 of the curved folding templates: curved-crease unfolding (Scheder-Bieschin, Van Mele and Block, DMSB 2023, Fig. 9c) to a catenary vault. The target crease is a catenary 10 m wide and 6 m high at an opening of 45 degrees; the plates are cut to it with every curvature times cos 45, so flat-folded the stack is a lower, wider curve, and opening it multiplies every curvature back by 1 / cos 45. Twenty plates 500 mm wide and 40 mm thick, hinged alternately on their top and bottom faces; the developable strips between two catenary-shaped creases interpolated through the bent crease, each a PlateCurved with its development checked. Groups: plates (state_<i> > plate_<i>_<k>), hinges, flat (the plate as cut).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_21_catenary_unfolding --parallel 6 && ./build/templates_curved_folding_21_catenary_unfolding && ../bash/publish-scene.sh --target templates_curved_folding_21_catenary_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
