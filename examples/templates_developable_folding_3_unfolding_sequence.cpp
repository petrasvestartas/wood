#include "wood_session.h"
#include "src/templates/developable_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// Kinematic deployment of a flat-foldable catenary CCU vault from closed stack (w = 0) to locked 3D shell (w = 1).
int main() {

    CurvedCreaseUnfolding sequence(
        CurvedCreaseUnfolding::catenary(6000.0, 2800.0),
        55.0 * Tolerance::TO_RADIANS,
        CurvedCreaseUnfolding::vault(
            Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 8, 420.0, 0.0
        ),
        {0.0, 0.35, 0.70, 1.0},
        30.0
    );
    std::cout << sequence << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<FaceCurvedElement>> plates =
        sequence.get_elements_placed<FaceCurvedElement>("stage");

    for (const std::shared_ptr<FaceCurvedElement>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format(
        "unfolding sequence: {} stages, {} plates, {} closed solids, {} contacts in locked stage, "
        "development distortion {:.2e}, drift {:.3f} mm\n",
        sequence.deployments.size(),
        plates.size(),
        solids,
        sequence.get_contacts().size(),
        sequence.distortion(),
        sequence.drift()
    );
    sequence.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the developable folding templates: one-degree-of-freedom accordion deployment of a curved-crease unfoldable (CCU) catenary shell from its closed flat-packed stack (w = 0, stack thickness 8 x 30 = 240 mm, planar unbent strips of curvature k_2d = k_3d cos 55 deg) through intermediate active-bending stages (w = 0.35, 0.70) to the miter-locked 3D corrugated vault (w = 1.0, 6 m span, 2.8 m rise). Axis-shift hinges alternate between intrados-to-intrados and extrados-to-extrados across the eight plates so the stack closes flat without collision while preserving zero neutral-surface strain. Groups: stage_0..stage_3, creases, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_3_unfolding_sequence --parallel 6 && ./build/templates_developable_folding_3_unfolding_sequence && ../bash/publish-scene.sh --target templates_developable_folding_3_unfolding_sequence

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
