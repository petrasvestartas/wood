#include "wood_session.h"
#include "src/templates/developable_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// Curved-crease unfolding.
int main() {

    CurvedCreaseUnfolding unfolding(
        Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)),
        50.0 * Tolerance::TO_RADIANS,
        CurvedCreaseUnfolding::zigzag(
            Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 4, 1000.0, 0.0
        ),
        40.0
    );
    std::cout << unfolding << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> plates =
        unfolding.get_elements_placed<PlateCurved>("plate");

    for (const std::shared_ptr<PlateCurved>& plate : plates)
        solids += plate->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format(
        "curved-crease unfolding: {} plates, {} closed solids, {} contacts, development distortion "
        "{:.2e}, drift {:.3f} mm\n",
        plates.size(),
        solids,
        unfolding.get_contacts().size(),
        unfolding.distortion(),
        unfolding.drift()
    );
    unfolding.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 2 of the developable folding templates: curved-crease unfolding. Four 40 mm plates, flat bands with circular edges, hinged on their curved edge and opened: the curved hinge forces each plate to bend about its rulings, so the stack rises into an arch. Five parallel planes 1000 mm apart, the crease a semicircle 9 m wide and 4.5 m high, the plates opened 50 degrees; every plate an exact cone. Each plate is a PlateCurved mitred on its two crease planes, sharing one face with its neighbour, its development checked flat. Groups: plates, creases.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_2_curved_crease_unfolding --parallel 6 && ./build/templates_developable_folding_2_curved_crease_unfolding && ./bash/publish-scene.sh --target templates_developable_folding_2_curved_crease_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
