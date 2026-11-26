#include "wood_session.h"
#include "src/templates/developable_folding/crease_quadrants.h"

using namespace session_cpp;
using namespace wood_session;

/// The four quadrants of one planar curved crease: CCF single-sheet folding (Q1, Q3) vs CCU two-sheet unfolding (Q2, Q4).
int main() {

    CreaseQuadrants quadrants(
        Primitives::arc(Point(-2000.0, 0.0, 0.0), Point(0.0, 0.0, 1500.0), Point(2000.0, 0.0, 0.0)),
        40.0 * Tolerance::TO_RADIANS,
        1200.0,
        40.0
    );
    std::cout << quadrants << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<FaceCurvedElement>> plates =
        quadrants.get_elements_placed<FaceCurvedElement>("quadrant");

    for (const std::shared_ptr<FaceCurvedElement>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    const std::array<double, 4>& sums = quadrants.sector_sums();
    std::cout << fmt::format(
        "crease quadrants: {} strips, {} closed solids, {} contacts, sector sums "
        "Q1={:.1f} deg Q2={:.1f} deg Q3={:.1f} deg Q4={:.1f} deg, development distortion {:.2e}\n",
        plates.size(),
        solids,
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
Step 4 of the developable folding templates: the four quadrants of a planar curved crease. An arc 4 m wide and 1.5 m high is the crease; a developable through it has rulings r on one side of the osculating plane and -r on the other, with mirror images Mr and -Mr. Two axis-shifted 40 mm strips 1200 mm wide form each quadrant: Q1 (r, -Mr) and Q3 (Mr, -r) fold a continuous single sheet (CCF, sector angles sum to 360 deg, unfolding flat into one plane at w = 0); Q2 (r, Mr) unfolds two mirror-symmetric sheets with a convex crease (synclastic CCU, sector angles < 360 deg, folding closed into an 80 mm stack at w = 0); Q4 (-r, -Mr) unfolds two mirror-symmetric sheets with a concave crease (anticlastic CCU, sector angles > 360 deg). Groups: quadrant_1..quadrant_4, creases, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_4_crease_quadrants --parallel 6 && ./build/templates_developable_folding_4_crease_quadrants && ../bash/publish-scene.sh --target templates_developable_folding_4_crease_quadrants

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
