#include "wood_session.h"
#include "src/templates/cylindrical_folding/corner_fan.h"

using namespace session_cpp;
using namespace wood_session;

/// A corner fan: six developable strips folded from one corner of a 6 m square on parabolic creases to its two far sides.
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
Step 14 of the cylindrical folding templates: a corner fan inscribed in a 6 m square. Every fold is a parabola from three control points laid out in plan, the corner, the middle and the end on one of the square's two far sides, the end raised to 4 m, every second one 800 mm lower, and the middle lifted 1200 mm above the chord. All folds lifted alike, two neighbours differ by their ends' difference times the parameter, so every strip is a cylinder of straight rulings parallel to its top edge on the square, developable. 40 mm plates meeting at every fold on the bisector of their inner faces, cut at 15 percent of the way up. Groups: strips.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_cylindrical_folding_14_corner_fan --parallel 6 && ./build/templates_cylindrical_folding_14_corner_fan && ../bash/publish-scene.sh --target templates_cylindrical_folding_14_corner_fan

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
