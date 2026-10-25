#include "wood_session.h"
#include "src/templates/curved_folding/corner_fan.h"

using namespace session_cpp;
using namespace wood_session;

/// A corner fan: six developable strips folded from one bottom corner on parabolic creases, the folds ending high and low in turn.
int main() {

    CornerFan fan;
    std::cout << fan << std::endl;

    size_t solids = 0;
    const std::vector<std::shared_ptr<PlateCurved>> strips = fan.get_elements_placed<PlateCurved>("strip");

    for (const std::shared_ptr<PlateCurved>& strip : strips)
        solids += strip->element_geometry_brep().is_solid() ? 1 : 0;

    std::cout << fmt::format("corner fan: {} strips, {} closed solids, {} contacts, development distortion {:.2e}\n", strips.size(), solids, fan.get_contacts().size(), fan.distortion());
    fan.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 32 of the curved folding templates: a corner fan. Six strips fill the quarter between the two box faces at one bottom corner. Every fold is a parabola from three points, the corner, a control point lifted 1200 mm above the chord's middle and the top 6 m out and 4 m up, lying in a vertical plane through the corner; between two folds the strip is a parabolic cylinder of straight rulings, developable, and the next strip is its mirror image in the fold's plane. The rulings are tilted so the folds end 800 mm lower in every second plane, the straight top edges zigzagging. 40 mm plates meeting on the fold planes, cut at 15 percent of the way up. Groups: strips.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_32_corner_fan --parallel 6 && ./build/templates_curved_folding_32_corner_fan && ../bash/publish-scene.sh --target templates_curved_folding_32_corner_fan

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
