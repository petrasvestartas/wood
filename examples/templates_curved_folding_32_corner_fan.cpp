#include "wood_session.h"
#include "src/templates/curved_folding/corner_fan.h"

using namespace session_cpp;
using namespace wood_session;

/// A corner fan: six developable conical strips folded from one bottom corner of a 6 x 6 x 4 m box up to its far top edges.
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
Step 32 of the curved folding templates: a corner fan. Every fold is a straight line from one bottom corner of a box 6 m wide, 6 m deep and 4 m high to a point on its two far top edges. Between two folds each strip is a cone with its apex at the corner, its top edge a parabola rising 400 mm, every strip bulging the same way so two meet at an angle on every fold; a cone is developable, so every strip is cut from a flat panel and bent. The strips are 40 mm thick, cut a sixth of the way up from the corner, and two neighbours meet on the plane bisecting them at their fold. Groups: strips.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_32_corner_fan --parallel 6 && ./build/templates_curved_folding_32_corner_fan && ../bash/publish-scene.sh --target templates_curved_folding_32_corner_fan

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
