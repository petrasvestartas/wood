#include "wood_session.h"
#include "src/templates/frame/frame.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_frame;

/// Four cellular CLT slab box-frame systems (`Plattenrahmen`) after Engel, side by side.
int main() {

    Frames frames(
        {
            {"open_box_frames", [](double x) { return FrameGuide(OpenBoxFrames{}, x); }},
            {"closed_pilotis_box", [](double x) { return FrameGuide(ClosedPilotisBox{}, x); }},
            {"cantilevered_multi_box", [](double x) { return FrameGuide(CantileveredMultiBox{}, x); }},
            {"crosswise_stacked_boxes", [](double x) { return FrameGuide(CrosswiseStackedBoxes{}, x); }},
        },
        8000.0,
        "templates_frame_4_box_frames"
    );
    std::cout << frames << std::endl;
    frames.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Four cellular CLT slab box-frame systems (`Plattenrahmen`) after Heino Engel (Structure Systems, §3.4, p. 209): (1) open ground box-frame portals (`Offene Plattenrahmen`) surmounted by a raised central cross-box; (2) an elevated cellular box-frame grid on pilotis columns (`Geschlossene Plattenrahmen`) with side openings and skylight wells; (3) multi-storey staggered and cantilevered box frames (`Mehrgeschoss-Plattenrahmen`); and (4) three upper levels of tubular box frames stacked crosswise (`Kreuzweise gestapelte Plattenrahmen`) over twin portal box piers.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_frame_4_box_frames --parallel 6 && ./build/templates_frame_4_box_frames && ../bash/publish-scene.sh --target templates_frame_4_box_frames

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
