#include "wood_session.h"
#include "src/templates/frame/frame.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_frame;

/// Four single-storey portal, gable, articulated T-frame and doubled-column halls after Engel, side by side.
int main() {

    Frames frames(
        {
            {"two_hinged_portal", [](double x) { return FrameGuide(TwoHingedPortal{}, x); }},
            {"three_hinged_gable", [](double x) { return FrameGuide(ThreeHingedGable{}, x); }},
            {"cantilever_t_frame", [](double x) { return FrameGuide(ArticulatedTFrame{}, x); }},
            {"doubled_column_cantilever", [](double x) { return FrameGuide(DoubledColumnCantilever{}, x); }},
        },
        7000.0,
        "templates_frame_1_portal_frames"
    );
    std::cout << frames << std::endl;
    frames.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Four section-active portal frame halls after Heino Engel (Structure Systems, §3.2, pp. 186-191): (1) a four-bent two-hinged portal hall (`Zweigelenkrahmen`) with base shoes, tapered legs, exact diagonal knee miters, haunched rafters, and roof purlins; (2) a pitched three-hinged gable hall (`Dreigelenk-Giebelrahmen`) tapering to base pins and a ridge crown hinge; (3) a two-bay articulated hall coupling outer L-frames to a central double-cantilever T-frame (`T-Rahmen`) via sloped half-lap Gerber step-hinges; and (4) a wide-overhang hall on doubled interior tapered columns carrying symmetric 7.6 m tapered cantilevers.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_frame_1_portal_frames --parallel 6 && ./build/templates_frame_1_portal_frames && ../bash/publish-scene.sh --target templates_frame_1_portal_frames

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
