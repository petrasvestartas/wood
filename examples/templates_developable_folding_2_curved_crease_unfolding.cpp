#include "wood_session.h"
#include "src/templates/developable_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_developable_folding;

/// Monoclastic corrugated arch by curved-crease unfolding on parallel planes.
int main() {

    CurvedCreaseUnfolding unfolding(CurvedCreaseUnfoldingGuide::monoclastic());
    std::cout << unfolding << std::endl;
    std::cout << fmt::format(
        "curved-crease unfolding: {} plates, {} flat pieces, {} contacts, development distortion {:.2e}, drift {:.3f} mm\n",
        unfolding.get_elements_placed<FaceCurvedElement>("plate").size(),
        unfolding.get_elements_placed<FaceCurvedElement>("flat").size(),
        unfolding.get_contacts().size(),
        unfolding.distortion(),
        unfolding.drift()
    );
    unfolding.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 2 of the developable folding templates: monoclastic corrugated vault by curved-crease unfolding (CCU), CurvedCreaseUnfoldingGuide::monoclastic. A 9 m semicircle (4.5 m rise, 55-degree reflection angle) is mirrored across thirteen parallel planes 450 mm apart: twelve mid-surface strips without thickness in strips, their creases in creases and their 2D crease pattern in pattern. The thickness comes last: every strip becomes a 30 mm plate hinged on its faces (axis shift, intrados to intrados and extrados to extrados) and mitered on the planes, an exact developable FaceCurvedElement sharing miter contacts with its neighbours, and its flat piece for cutting in flat. Groups: strips, creases, pattern, plates, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_2_curved_crease_unfolding --parallel 6 && ./build/templates_developable_folding_2_curved_crease_unfolding && ../bash/publish-scene.sh --target templates_developable_folding_2_curved_crease_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
