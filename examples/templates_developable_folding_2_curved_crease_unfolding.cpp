#include "wood_session.h"
#include "src/templates/developable_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// Monoclastic corrugated arch by curved-crease unfolding on parallel osculating planes.
int main() {

    CurvedCreaseUnfolding unfolding(
        Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)),
        55.0 * Tolerance::TO_RADIANS,
        CurvedCreaseUnfolding::vault(
            Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 12, 450.0, 0.0
        ),
        30.0
    );
    std::cout << unfolding << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<FaceCurvedElement>> plates =
        unfolding.get_elements_placed<FaceCurvedElement>("plate");
    const std::vector<std::shared_ptr<FaceCurvedElement>> flat_pieces =
        unfolding.get_elements_placed<FaceCurvedElement>("flat");

    for (const std::shared_ptr<FaceCurvedElement>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    size_t flat_solids = 0;
    for (const std::shared_ptr<FaceCurvedElement>& piece : flat_pieces)
        flat_solids += piece->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format(
        "curved-crease unfolding: {} plates ({} solids), {} unfolded flat pieces ({} solids), {} contacts, "
        "development distortion {:.2e}, drift {:.3f} mm\n",
        plates.size(),
        solids,
        flat_pieces.size(),
        flat_solids,
        unfolding.get_contacts().size(),
        unfolding.distortion(),
        unfolding.drift()
    );
    unfolding.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 2 of the developable folding templates: monoclastic corrugated vault by curved-crease unfolding (CCU). Twelve 30 mm bending-active plates with alternating intrados/extrados axis-shift hinges and locking miters across thirteen parallel osculating planes 450 mm apart over a 9 m semicircular span (4.5 m rise, 55-degree reflection angle). Every plate is an exact developable cone solid (FaceCurvedElement) sharing miter contacts with its neighbours and unfolding flat into its 2D cutting pattern in group flat. Groups: plates, creases, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_2_curved_crease_unfolding --parallel 6 && ./build/templates_developable_folding_2_curved_crease_unfolding && ../bash/publish-scene.sh --target templates_developable_folding_2_curved_crease_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
