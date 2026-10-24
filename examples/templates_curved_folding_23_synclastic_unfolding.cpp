#include "wood_session.h"
#include "src/templates/curved_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

/// Curved-crease unfolding drawn in: the crease planes leaning toward the middle, a synclastic vault.
int main() {

    CurvedCreaseUnfolding unfolding(
        Primitives::arc(Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)),
        50.0 * Tolerance::TO_RADIANS,
        CurvedCreaseUnfolding::leaning(
            Point(0.0, 0.0, 0.0),
            Vector(0.0, 1.0, 0.0),
            Vector(1.0, 0.0, 0.0),
            4,
            1000.0,
            5.0 * Tolerance::TO_RADIANS
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
        "synclastic unfolding: {} plates, {} closed solids, {} contacts, development distortion "
        "{:.2e}, support drift {:.0f} mm\n",
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
Step 23 of the curved folding templates: a synclastic vault by plane inclination. Five planes 1000 mm apart on the floor, each leaning 5 degrees more toward the middle, the crease a semicircle 9 m wide and 4.5 m high, the plates opened 50 degrees: the apex is drawn in. Each plate is a PlateCurved mitred on its two crease planes, sharing one face with its neighbour, its development checked flat. Groups: plates, creases.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_23_synclastic_unfolding --parallel 6 && ./build/templates_curved_folding_23_synclastic_unfolding && ./bash/publish-scene.sh --target templates_curved_folding_23_synclastic_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
