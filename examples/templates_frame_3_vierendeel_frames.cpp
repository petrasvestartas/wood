#include "wood_session.h"
#include "src/templates/frame/frame.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_frame;

/// Four shear-gradated, biaxial concentric and crosswise-stacked Vierendeel frame systems after Engel, side by side.
int main() {

    Frames frames(
        {
            {"column_gradated_vierendeel", [](double x) { return FrameGuide(ColumnGradatedVierendeel{}, x); }},
            {"bay_gradated_vierendeel", [](double x) { return FrameGuide(BayGradatedVierendeel{}, x); }},
            {"concentric_vierendeel_grid", [](double x) { return FrameGuide(ConcentricVierendeelGrid{}, x); }},
            {"crosswise_stacked_vierendeel", [](double x) { return FrameGuide(CrosswiseStackedVierendeel{}, x); }},
        },
        8000.0,
        "templates_frame_3_vierendeel_frames"
    );
    std::cout << frames << std::endl;
    frames.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Four multi-panel Vierendeel frame systems after Heino Engel (Structure Systems, §3.2, pp. 194-199): (1) a seven-panel Vierendeel hall whose vertical post widths widen from midspan toward the end piers in proportion to transverse shear; (2) a double-cantilevered eight-panel Vierendeel bridge on central tapered piers with shear-gradated bay widths; (3) an open biaxial `#`-grid of story-deep Vierendeel frames spanning a 24 x 24 m column-free hall on eight perimeter fin columns; and (4) three storeys of open six-panel Vierendeel frames stacked crosswise with two-way cantilevers.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_frame_3_vierendeel_frames --parallel 6 && ./build/templates_frame_3_vierendeel_frames && ../bash/publish-scene.sh --target templates_frame_3_vierendeel_frames

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
