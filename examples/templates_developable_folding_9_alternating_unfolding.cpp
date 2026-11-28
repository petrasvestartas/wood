#include "wood_session.h"
#include "src/templates/developable_folding/curved_crease_unfolding.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_developable_folding;

/// Corrugated CCU vault with alternating plane inclinations around the actuation axis at mid-height.
int main() {

    CurvedCreaseUnfoldingGuide guide = CurvedCreaseUnfoldingGuide::alternating();
    guide.deployments = {0.0, 1.0};
    CurvedCreaseUnfolding unfolding(guide);
    std::cout << unfolding << std::endl;
    std::cout << fmt::format(
        "alternating unfolding: {} plates, {} contacts, development distortion {:.2e}, support drift {:.0f} mm\n",
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
Step 9 of the developable folding templates: corrugated CCU vault with alternating plane inclinations around the actuation axis, CurvedCreaseUnfoldingGuide::alternating. Thirteen planes 450 mm apart over a 9 m semicircle (4.5 m rise, 55-degree opening), every other pair inclined +-5 degrees about the horizontal span axis at z = 2250 mm, so the corrugation depth and width articulate alternatingly at the crown and the supports while every crease stays planar. The strips come first without thickness, the 30 mm plates last. Groups: strips, creases, pattern, plates, flat.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_developable_folding_9_alternating_unfolding --parallel 6 && ./build/templates_developable_folding_9_alternating_unfolding && ../bash/publish-scene.sh --target templates_developable_folding_9_alternating_unfolding

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
