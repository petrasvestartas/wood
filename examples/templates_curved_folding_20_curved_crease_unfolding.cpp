#include "wood_session.h"
#include "src/templates/curved_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// Curved-crease unfolding: sixteen flat plates with a circular edge, stacked and hinged on their curved edges, drawn flat-folded,
/// half open and open into a corrugated arch 9 m wide and 4.5 m high, every plate bent into an exact cone.
int main() {

    CurvedCreaseUnfolding unfolding(
        CurvedCreaseUnfolding::arc(9000.0, 4500.0),
        50.0 * Tolerance::TO_RADIANS,
        600.0,
        16,
        {0.0, 25.0 * Tolerance::TO_RADIANS, 50.0 * Tolerance::TO_RADIANS},
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
Step 20 of the curved folding templates: curved-crease unfolding after Scheder-Bieschin, Van Mele and Block (DMSB 2023). Sixteen plates 40 mm thick, each a flat band 600 mm wide between two concentric circular edges, are stacked and hinged to their neighbours along the curved edges, alternately on the top and the bottom face (the axis shift), and pulled open like an accordion. A straight edge would only hinge; the curved one forces every plate to bend about its rulings, so the stack rises into a corrugated arch: each crease keeps its length and its plane, and its curvature grows as 1 / cos of the opening. Drawn at 0 (the flat-folded stack), 25 and 50 degrees (the target semicircle 9 m wide and 4.5 m high); every plate an exact cone, a PlateCurved, its development checked flat and its mid-surface checked isometric to the plate as cut. Groups: plates (state_<i> > plate_<i>_<k>), hinges, flat (the plate as cut).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_20_curved_crease_unfolding --parallel 6 && ./build/templates_curved_folding_20_curved_crease_unfolding && ../bash/publish-scene.sh --target templates_curved_folding_20_curved_crease_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
