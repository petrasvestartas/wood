#include "wood_session.h"
#include "src/templates/developable_folding/space_curve_fold.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_developable_folding;

/// One curved fold along a densely sampled helix, every arm one curved plate, folding from 60 degrees until its arms would collide.
int main() {

    SpaceCurveFold fold(SpaceCurveFoldGuide::smooth_helix_fold(), "smooth_space_curve_fold");
    const FoldMeasures& measures = fold.measures();
    std::cout << fold << std::endl;
    std::cout << fmt::format(
        "smooth space curve fold: {} stages of {} quads, {} curved plates, development distortion {:.1e}, planarity {:.1e}, kawasaki {:.1e} rad, openings {:.1f} to {:.1f} deg, edge change {:.1e} mm, {} crossings\n",
        fold.guide.deployments.size(),
        2 * (fold.guide.curve.point_count() - 1),
        fold.get_elements<FaceCurvedElement>().size(),
        measures.distortion,
        measures.planarity,
        measures.kawasaki,
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
Step 15 of the developable folding templates: the smooth counterpart of step 13, SpaceCurveFoldGuide::smooth_helix_fold. The same helix without zig-zag, sampled in 180 segments, gives a V ridge of 360 planar quads close to the smooth constant-angle curved fold (Tachi 2011, 2.2). Folding it with the rulings fixed raises its curvature and torsion, so it rolls up: at 60, 30, 15 and 6 degrees no two quads cross, below about 5 degrees its arms collide and it cannot fold flat. The thickness comes last: every arm of every stage one 18 mm curved plate over its interpolated crease and edge, offset along the arm's normals away from the other arm, hinged on the crease, its flat development checked. Groups: curve, ribbon, stage_0_quads..stage_3_quads, creases, pattern, stage_0..stage_3.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_15_smooth_space_curve_fold --parallel 6 && ./build/templates_developable_folding_15_smooth_space_curve_fold && ../bash/publish-scene.sh --target templates_developable_folding_15_smooth_space_curve_fold

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
