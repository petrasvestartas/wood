#include "wood_session.h"
#include "src/templates/developable_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_developable_folding;

/// Doubly-curved synclastic CCU vault by modulated distances of parallel planes.
int main() {

    CurvedCreaseUnfoldingGuide guide = CurvedCreaseUnfoldingGuide::synclastic();
    guide.deployments = {0.0, 1.0};
    CurvedCreaseUnfolding unfolding(guide);
    std::cout << unfolding << std::endl;
    std::cout << fmt::format(
        "synclastic unfolding: {} plates, {} contacts, development distortion {:.2e}, support drift {:.3f} mm\n",
        unfolding.get_elements_placed<FaceCurvedElement>("stage").size(),
        unfolding.get_contacts().size(),
        unfolding.distortion(),
        unfolding.drift()
    );
    unfolding.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 5 of the developable folding templates: doubly-curved synclastic corrugated vault by plane distances, CurvedCreaseUnfoldingGuide::synclastic. Twelve strips across thirteen parallel planes whose distances step down towards the mid-span crease (450 mm base spacing, 95 mm step, 55-degree reflection angle): the apex profile and the support plan curve inward along a barrel dome while every support foot stays on the floor (0 mm drift) and every crease stays planar. The thickness comes last: 30 mm plates hinged on their faces and mitered on the planes. Groups: strips, creases, pattern, plates, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_5_synclastic_unfolding --parallel 6 && ./build/templates_developable_folding_5_synclastic_unfolding && ../bash/publish-scene.sh --target templates_developable_folding_5_synclastic_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
