#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The square floor.
int main() {

    const wood_floor::FloorPlan plan = wood_floor::FloorPlan::rectangle(3000.0, 3000.0);
    const wood_floor::Floor floor(plan, wood_floor::FloorSizes{});
    std::cout << floor.check().str() << std::endl;
    WoodSession session("templates_floor_5_oculus");
    const std::shared_ptr<TreeNode> group = session.add_group("oculus");
    wood_floor::add_oculus_model(session, floor, group);

    std::cout << fmt::format("{} elements: {} plates, {} variable beams", session.objects.elements->size(), session.plates().size(), session.beam_variables().size()) << std::endl;
    session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 5 of the timber floor: the oculus ring built from the four quarters' oculus edges as a pinwheel, each ring beam between its edge's tilted bearing plane and the ring's inner plane, lifted to the floor at 3500: four ring beams as variable beams, four bottom wedges and the inner plate as plates. Prints the floor's report and the element counts.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_5_oculus --parallel 6 && ./build/templates_floor_5_oculus && ../bash/publish-scene.sh --target templates_floor_5_oculus

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
