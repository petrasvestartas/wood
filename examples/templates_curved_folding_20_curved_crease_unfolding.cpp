#include "wood_session.h"
#include "src/templates/curved_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// Curved-crease unfolding: sixteen flat plates with a circular edge, hinged on their curved edges and opened 50 degrees into a corrugated arch 9 m wide and 4.5 m high, every plate bent into an exact cone.
int main() {

    CurvedCreaseUnfolding unfolding(
        Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)),
        50.0 * Tolerance::TO_RADIANS,
        CurvedCreaseUnfolding::zigzag(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 16, 460.0, 0.0),
        40.0
    );
    std::cout << unfolding << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> plates = unfolding.get_elements_placed<PlateCurved>("plate");

    for (const std::shared_ptr<PlateCurved>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("curved-crease unfolding: {} plates, {} closed solids, {} contacts, development distortion {:.2e}, drift {:.3f} mm\n", plates.size(), solids, unfolding.get_contacts().size(), unfolding.distortion(), unfolding.drift());
    unfolding.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 20 of the curved folding templates: curved-crease unfolding. Sixteen plates 40 mm thick, each a flat band 600 mm wide between two concentric circular edges, are stacked and hinged to their neighbours along the curved edges, alternately on the top and the bottom face (the axis shift), and pulled open like an accordion. A straight edge would only hinge; the curved one forces every plate to bend about its rulings, so the stack rises into a corrugated arch: each crease keeps its plane, and its curvature grows as the cut edge's over the cosine of the opening. Drawn open at 50 degrees on seventeen parallel planes 460 mm apart (the width times the sine of the opening), the given crease the semicircle 9 m wide and 4.5 m high, every plate an exact cone. Every plate is a PlateCurved between the two planes of its creases, mitred on them so it shares one face with each neighbour, its development checked flat; the session's contact detection finds every shared face. Groups: plates (plate_<k>), creases.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_20_curved_crease_unfolding --parallel 6 && ./build/templates_curved_folding_20_curved_crease_unfolding && ./bash/publish-scene.sh --target templates_curved_folding_20_curved_crease_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
