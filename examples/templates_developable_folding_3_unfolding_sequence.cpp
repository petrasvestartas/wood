#include "wood_session.h"
#include "src/templates/developable_folding/curved_crease_unfolding.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_developable_folding;

/// Kinematic deployment of a flat-foldable catenary CCU vault from the closed stack (w = 0) to the locked shell (w = 1).
int main() {

    CurvedCreaseUnfolding sequence(
        CurvedCreaseUnfoldingGuide::unfolding_sequence(), "unfolding_sequence"
    );
    std::cout << sequence << std::endl;
    std::cout << fmt::format(
        "unfolding sequence: {} stages, {} plates, {} contacts in locked stage, development distortion {:.2e}, drift {:.3f} mm\n",
        sequence.guide.deployments.size(),
        sequence.get_elements_placed<FaceCurvedElement>("stage").size(),
        sequence.get_contacts().size(),
        sequence.distortion(),
        sequence.drift()
    );
    sequence.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 3 of the developable folding templates: one-degree-of-freedom accordion deployment of a curved-crease unfoldable (CCU) catenary shell, CurvedCreaseUnfoldingGuide::unfolding_sequence. The mid-surface strips deploy without thickness from the closed stack (w = 0, planar strips of curvature k_2d = k_3d cos 55 deg, all in one plane) through active-bending stages (w = 0.35, 0.70) to the target vault (w = 1.0, 6 m span, 2.8 m rise), side by side in stage_<s>_strips. The thickness comes last: every stage's strips become 30 mm plates hinged on their faces, the axis shift alternating intrados to intrados and extrados to extrados, so the closed stack is 8 x 30 = 240 mm thick and the target stage miter-locked. Groups: stage_0_strips..stage_3_strips, creases, pattern, stage_0..stage_3, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_3_unfolding_sequence --parallel 6 && ./build/templates_developable_folding_3_unfolding_sequence && ../bash/publish-scene.sh --target templates_developable_folding_3_unfolding_sequence

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
