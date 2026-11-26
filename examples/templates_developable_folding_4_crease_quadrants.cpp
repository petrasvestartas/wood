#include "wood_session.h"
#include "src/templates/developable_folding/crease_quadrants.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_developable_folding;

/// The four quadrants of one planar curved crease: CCF single-sheet folding (Q1, Q3) and CCU two-sheet unfolding (Q2, Q4).
int main() {

    CreaseQuadrants quadrants(CreaseQuadrantsGuide::arc_crease());
    std::cout << quadrants << std::endl;
    const std::array<double, 4>& sums = quadrants.sector_sums();
    std::cout << fmt::format(
        "crease quadrants: {} strips, {} contacts, sector sums Q1={:.1f} deg Q2={:.1f} deg Q3={:.1f} deg Q4={:.1f} deg, development distortion {:.2e}\n",
        quadrants.get_elements_placed<FaceCurvedElement>("quadrant").size(),
        quadrants.get_contacts().size(),
        sums[0] * Tolerance::TO_DEGREES,
        sums[1] * Tolerance::TO_DEGREES,
        sums[2] * Tolerance::TO_DEGREES,
        sums[3] * Tolerance::TO_DEGREES,
        quadrants.distortion()
    );
    quadrants.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 4 of the developable folding templates: the four quadrants of a planar curved crease, CreaseQuadrantsGuide::arc_crease. An arc 4 m wide and 1.5 m high is the crease; a developable through it has rulings r on one side of the osculating plane and -r on the other, with mirror images Mr and -Mr. Two strips 1200 mm wide without thickness form each quadrant, in strips: Q1 (r, -Mr) and Q3 (Mr, -r) fold a continuous single sheet (CCF, sector angles sum to 360 deg); Q2 (r, Mr) unfolds two mirror-symmetric sheets with a convex crease (synclastic CCU, less than 360 deg); Q4 (-r, -Mr) two with a concave crease (anticlastic CCU, more than 360 deg). The thickness comes last: every strip becomes a 40 mm plate on its body side of the hinge face (axis shift), so Q1 and Q3 open into one flat sheet and Q2 and Q4 close into an 80 mm stack at w = 0. Groups: strips, creases, pattern, quadrant_1..quadrant_4, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_4_crease_quadrants --parallel 6 && ./build/templates_developable_folding_4_crease_quadrants && ../bash/publish-scene.sh --target templates_developable_folding_4_crease_quadrants

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
