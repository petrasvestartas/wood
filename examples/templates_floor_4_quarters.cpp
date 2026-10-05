#include "wood_session.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The square floor.
int main() {

    const wood_floor::FloorPlan plan = wood_floor::FloorPlan::rectangle(3000.0, 3000.0);
    const wood_floor::Floor floor(plan, wood_floor::FloorSizes{});
    std::cout << floor.check().str() << std::endl;
    WoodSession session("templates_floor_4_quarters");
    const std::shared_ptr<TreeNode> root = session.add_group("quarters_model");

    for (size_t q = 0; q < 4; q++)
        wood_floor::add_quarter_model(session, floor.quarter(q), wood_floor::add_group(session, fmt::format("quarter_model_{}", q), root));

    std::cout << fmt::format("{} elements: {} plates, {} variable beams", session.objects.elements->size(), session.plates().size(), session.beam_variables().size()) << std::endl;
    session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 4 of the timber floor: the four quarters of the bay, each built in place at its corner and lifted to the floor at 3500, grouped as beds (one per row), tsections, outer_ribs, inner_ribs, wedges_inner_beams and inner_beams with the quarter index on every name. Ribs and inner beams are variable beams, every other member a plate. Prints the floor's report and the element counts.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_4_quarters --parallel 6 && ./build/templates_floor_4_quarters && ../bash/publish-scene.sh --target templates_floor_4_quarters

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
