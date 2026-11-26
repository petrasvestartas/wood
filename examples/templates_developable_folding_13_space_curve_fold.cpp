#include "wood_session.h"
#include "src/templates/developable_folding/space_curve_fold.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_developable_folding;

/// One curved fold along a zig-zagged helix folding with one degree of freedom from 60 degrees open to flat.
int main() {

    SpaceCurveFold fold(SpaceCurveFoldGuide::zigzag_fold());
    const FoldMeasures& measures = fold.measures();
    std::cout << fold << std::endl;
    std::cout << fmt::format(
        "space curve fold: {} stages of {} quads, {} plates, planarity {:.1e}, kawasaki {:.1e} rad, angle sum {:.1e} rad, openings {:.1f} to {:.1f} deg, edge change {:.1e} mm, {} crossings\n",
        fold.guide.deployments.size(),
        2 * (fold.guide.curve.point_count() - 1),
        fold.get_elements<FacePlanarElement>().size(),
        measures.planarity,
        measures.kawasaki,
        measures.developability,
        measures.openings[0] * Tolerance::TO_DEGREES,
        measures.openings[1] * Tolerance::TO_DEGREES,
        measures.stretch,
        measures.crossings
    );
    fold.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 13 of the developable folding templates: one-DOF rigid foldable curved fold from a space curve (Tachi 2011), SpaceCurveFoldGuide::zigzag_fold. A helix of 2 m radius rising 3 m per turn, 1.5 turns in 36 segments, has every other vertex moved 150 mm along its curvature vector and the others against it. Along the zig-zagged polyline the discrete ribbon 600 mm wide is built edge by edge (Eq. 6), then replaced by a V ridge: per edge two half-planes at 30 degrees to the ribbon, trimmed where neighbours meet, 72 planar quads whose vertices satisfy Kawasaki. The crease pattern is the ridge developed flat; folding it with the rulings fixed keeps every quad rigid and the crease at one opening, here 60, 30, 12, 3 and 1.2 degrees, the last all but flat without two quads crossing. Without a thickness there are no plates. Groups: curve, ribbon, stage_0_quads..stage_4_quads, creases, pattern.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_13_space_curve_fold --parallel 6 && ./build/templates_developable_folding_13_space_curve_fold && ../bash/publish-scene.sh --target templates_developable_folding_13_space_curve_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
