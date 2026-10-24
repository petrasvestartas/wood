#include "wood_session.h"
#include "src/templates/curved_folding/crease_quadrants.h"

using namespace session_cpp;
using namespace wood_session;

/// The four quadrants of one curved crease: two strips on it folded as one sheet (Q1, Q3) or unfolded as two mirror sheets, convex (Q2)
/// or concave (Q4) for both.
int main() {

    CreaseQuadrants quadrants(
        Primitives::arc(Point(-2000.0, 0.0, 0.0), Point(0.0, 0.0, 1500.0), Point(2000.0, 0.0, 0.0)),
        40.0 * Tolerance::TO_RADIANS,
        1200.0,
        40.0
    );
    std::cout << quadrants << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> plates = quadrants.get_elements_placed<PlateCurved>("quadrant");

    for (const std::shared_ptr<PlateCurved>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("crease quadrants: {} strips, {} closed solids, {} contacts, development distortion {:.2e}\n", plates.size(), solids, quadrants.get_contacts().size(), quadrants.distortion());
    quadrants.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 22 of the curved folding templates: the four quadrants of a planar curved crease. An arc 4 m wide and 1.5 m high is the crease; a developable through it has its rulings r on one side of the arc's plane and -r on the other, and their mirror images Mr and -Mr. Two strips 1200 mm wide and 40 mm thick, each a PlateCurved cone, take one ruling each: Q1 (r, -Mr) and Q3 (Mr, -r) are one sheet folded along the crease, the crease convex for one strip and concave for the other; Q2 (r, Mr) are two mirror sheets unfolded, the crease convex for both, synclastic; Q4 (-r, -Mr) the same with the crease concave for both, anticlastic. The rulings stand 40 degrees off the crease plane, square to the crease; an unfolding is mitred on the crease plane, a folding on the plane through the crease square to it. Groups: quadrant_<q> (quadrant_<q>_<k>), creases.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_22_crease_quadrants --parallel 6 && ./build/templates_curved_folding_22_crease_quadrants && ./bash/publish-scene.sh --target templates_curved_folding_22_crease_quadrants

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
