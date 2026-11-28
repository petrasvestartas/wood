#include "wood_session.h"
#include "src/templates/frame/frame.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_frame;

/// Four multi-storey coupled and tiered hinged portal frame halls after Engel, side by side.
int main() {

    Frames frames(
        {
            {"coupled_two_hinged", [](double x) { return FrameGuide(CoupledTwoHinged{}, x); }},
            {"coupled_three_hinged", [](double x) { return FrameGuide(CoupledThreeHinged{}, x); }},
            {"tiered_clerestory_hall", [](double x) { return FrameGuide(TieredClerestoryHall{}, x); }},
            {"tiered_outward_hall", [](double x) { return FrameGuide(TieredOutwardHall{}, x); }},
        },
        7000.0,
        "templates_frame_2_tiered_frames"
    );
    std::cout << frames << std::endl;
    frames.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Four multi-storey coupled and tiered hinged frame systems after Heino Engel (Structure Systems, §3.2, pp. 188, 192-193): (1) a three-storey coupled two-hinged portal frame building whose inward corridor cantilevers meet at central hinges; (2) a three-storey stacked three-hinged T-frame building (`Dreigelenk-Stockwerkrahmen`); (3) a tiered clerestory basilica hall where an upper two-hinged portal stands directly on the inward cantilevers of two lower portals; and (4) a bridge-set tiered hall where an upper wide portal stands on the outward cantilevers of a lower three-hinged portal.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_frame_2_tiered_frames --parallel 6 && ./build/templates_frame_2_tiered_frames && ../bash/publish-scene.sh --target templates_frame_2_tiered_frames

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
