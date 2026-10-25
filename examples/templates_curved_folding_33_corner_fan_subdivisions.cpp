#include "wood_session.h"
#include "src/templates/curved_folding/corner_fan.h"

using namespace session_cpp;
using namespace wood_session;

/// The corner fan divided into 2, 4, 8 and 12 strips, side by side, the same square, heights and lift.
int main() {

    WoodSession fans("corner_fan_subdivisions");
    const std::array<size_t, 4> counts = {2, 4, 8, 12};

    for (size_t i = 0; i < counts.size(); i++) {
        const Point corner(9000.0 * static_cast<double>(i), 0.0, 0.0);
        CornerFan fan(
            6000.0,
            4000.0,
            counts[i],
            1200.0,
            800.0,
            0.15,
            40.0,
            corner,
            fmt::format("corner_fan_{}", counts[i])
        );
        std::cout << fmt::format("corner fan of {} strips: {} contacts, development distortion {:.2e}\n", counts[i], fan.get_contacts().size(), fan.distortion());
        fans.graft(fan, fans.add_group(fmt::format("fan_{}", counts[i])));
    }

    fans.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 33 of the curved folding templates: the corner fan of step 32 divided into 2, 4, 8 and 12 strips, half on each of the square's two far sides, the same 6 m square, folds 4 m high, every second 800 mm lower, lifted 1200 mm, 40 mm plates; every fan has one contact per fold between two strips. Groups: fan_<strips>.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_curved_folding_33_corner_fan_subdivisions --parallel 6 && ./build/templates_curved_folding_33_corner_fan_subdivisions && ../bash/publish-scene.sh --target templates_curved_folding_33_corner_fan_subdivisions

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
