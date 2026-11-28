#include "wood_session.h"
#include "src/templates/developable_folding/curved_crease_unfolding.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_developable_folding;

/// A corrugated dome of curved-crease unfolding: a meridian arc mirrored in 24 meridian planes around the vertical axis.
int main() {

    CurvedCreaseUnfoldingGuide guide = CurvedCreaseUnfoldingGuide::dome();
    guide.deployments = {0.0, 1.0};
    CurvedCreaseUnfolding unfolding(guide);
    std::cout << unfolding << std::endl;
    std::cout << fmt::format(
        "corrugated dome: {} plates, {} contacts, development distortion {:.2e}, support drift {:.0f} mm\n",
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
Step 11 of the developable folding templates: a corrugated dome by consecutive radial plane inclinations perpendicular to the actuation axis, CurvedCreaseUnfoldingGuide::dome. The crease is a meridian arc of a 6 m sphere rising to 80 degrees around a 2.1 m oculus; 25 radial meridian planes 15 degrees apart mirror 24 strips into a closed 360-degree star dome that folds into a flat meridian stack at w = 0. The thickness comes last: 40 mm plates hinged on their faces. Groups: strips, creases, pattern, plates, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_11_corrugated_dome --parallel 6 && ./build/templates_developable_folding_11_corrugated_dome && ../bash/publish-scene.sh --target templates_developable_folding_11_corrugated_dome

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
