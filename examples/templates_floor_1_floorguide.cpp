#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// Quarter 0 of the square floor's guide: its plan, and every member's plan quad, face planes and parabolas under the member's name.
int main() {

    wood_floor::FloorGuide guide = wood_floor::FloorGuide::rectangle(3000.0, 3000.0);

    wood_session::WoodSession session = guide.get_branch("quarter_0");
    session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 1 of the timber floor: the guide alone, FloorGuide::rectangle(3000, 3000) with the default parameters, a session that draws its own construction, and quarter 0 of it taken out with get_branch("quarter_0") as a WoodSession. Every quarter_q holds plan_q (the quarter polygon, the column head, the oculus corner) and one group per member family, outer_ribs_q, inner_ribs_q, inner_beams_q, wedges_q and tsections_q, each in its own colour; under it one group per member named as the Floor names that member's element, e.g. outer_ribs_0_0, holding the member's plan quad at the floor datum, its two face planes face_0 and face_1, and for a rib its three parabolas: the soffit, the t-sections' top and the beds' top. No member is built; a Floor builds them from the guide, example 4 shows the same names as elements.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_1_floorguide --parallel 6 && ./build/templates_floor_1_floorguide && ../bash/publish-scene.sh --target templates_floor_1_floorguide

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
