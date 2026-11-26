#include "wood_session.h"
#include "src/templates/developable_folding/curved_crease_unfolding.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_developable_folding;

/// A corrugated funnel column of curved-crease unfolding: a flaring arc mirrored in 32 meridian planes around the vertical axis.
int main() {

    CurvedCreaseUnfoldingGuide guide = CurvedCreaseUnfoldingGuide::funnel();
    guide.deployments = {0.0, 1.0};
    CurvedCreaseUnfolding unfolding(guide);
    std::cout << unfolding << std::endl;
    std::cout << fmt::format(
        "corrugated funnel: {} plates, {} contacts, development distortion {:.2e}, support drift {:.0f} mm\n",
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
Step 12 of the developable folding templates: a corrugated funnel column by consecutive radial plane inclinations, CurvedCreaseUnfoldingGuide::funnel. The crease is a concave quarter circle of 5 m radius flaring from a 0.8 m base stem to a 5.8 m rim at 5 m height; 33 radial meridian planes 11.25 degrees apart mirror 32 anticlastic CCU strips into a closed 360-degree funnel shell that folds into a radial stack at w = 0. The thickness comes last: 40 mm plates hinged on their faces. Groups: strips, creases, pattern, plates, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_12_corrugated_funnel --parallel 6 && ./build/templates_developable_folding_12_corrugated_funnel && ../bash/publish-scene.sh --target templates_developable_folding_12_corrugated_funnel

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
