#include "wood_session.h"
#include "src/templates/developable_folding/curved_miura.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_developable_folding;

/// Generalized Miura-ori along a helix, six strips folding with one degree of freedom, every quad a mitered plate.
int main() {

    CurvedMiuraGuide guide = CurvedMiuraGuide::helix_miura();
    guide.deployments = {1.0, 0.7, 0.4};
    CurvedMiura miura(guide);
    const FoldMeasures& measures = miura.measures();
    std::cout << miura << std::endl;
    std::cout << fmt::format(
        "curved miura: {} stages of {} quads, {} plates, planarity {:.1e}, kawasaki {:.1e} rad, angle sum {:.1e} rad, openings {:.1f} to {:.1f} deg, edge change {:.1e} mm, {} crossings\n",
        miura.guide.deployments.size(),
        miura.guide.strips * (miura.guide.curve.point_count() - 1),
        miura.get_elements<FacePlanarElement>().size(),
        measures.planarity,
        measures.kawasaki,
        measures.developability,
        measures.openings[0] * Tolerance::TO_DEGREES,
        measures.openings[1] * Tolerance::TO_DEGREES,
        measures.stretch,
        measures.crossings
    );
    miura.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 14 of the developable folding templates: planar tessellation from a space curve (Tachi 2011), CurvedMiuraGuide::helix_miura. The V ridge along a helix of 2 m radius rising 3 m per turn (1.5 turns, 36 segments, opened 60 degrees) is repeated across: its arms trimmed so every far edge is parallel to its crease edge, then six strips 300 mm wide at the middle, every other strip on the same arm's rulings, a generalized Miura-ori bent and twisted along the curve. Every vertex is a copy of the ridge's, so the 216 planar quads fold with one degree of freedom, here at 60, 42 and 24 degrees side by side, with the crease pattern under them. The thickness comes last: every quad of every stage an 18 mm mitered plate, the first stage's put in one by one. Groups: curve, ribbon, stage_0_quads..stage_2_quads, creases, pattern, stage_0..stage_2.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_14_curved_miura --parallel 6 && ./build/templates_developable_folding_14_curved_miura && ../bash/publish-scene.sh --target templates_developable_folding_14_curved_miura

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
