#include "wood_session.h"
#include "src/templates/floor/floor.h"
#include <chrono>

using namespace session_cpp;
using namespace wood_session;

/// The square floor.
int main() {

    const wood_floor::FloorPlan plan = wood_floor::FloorPlan::rectangle(3000.0, 3000.0);
    const wood_floor::Floor floor(plan, wood_floor::FloorSizes{});
    std::cout << floor.check().str() << std::endl;
    WoodSession session("templates_floor_6_contacts_floor");
    const std::shared_ptr<TreeNode> root = session.add_group("floor_model");
    const wood_floor::FloorMembers members = wood_floor::add_floor(session, floor, root);
    const std::vector<wood_floor::Relation> kinds = {wood_floor::Relation::seam_wedge, wood_floor::Relation::oculus_wedge};
    const std::vector<wood_floor::ContactMismatch> mismatches = wood_floor::verify_contacts(session, floor, members, 1e-6, kinds);

    for (const wood_floor::ContactMismatch& mismatch : mismatches)
        std::cout << fmt::format("contact mismatch: {}: {}", mismatch.relation, mismatch.what) << std::endl;

    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    const std::vector<std::shared_ptr<JointBeam>> wedges = wood_floor::add_connectors(session, floor, members, kinds);
    session.pb_dump(pb_path("live"));
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    size_t dowels = 0;

    for (const std::shared_ptr<JointBeam>& wedge : wedges)
        dowels += wedge->drill_lines.size();

    std::cout << fmt::format("{} wedges, {} dowels: contacts, cuts and pb in {:.0f} ms", wedges.size(), dowels, ms) << std::endl;
    std::cout << fmt::format("{} of {} wedge contacts verified by the kernel's search", wedges.size() - mismatches.size(), wedges.size()) << std::endl;

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 6 of the timber floor: the four quarters and the oculus with a wedge connector on each seam and oculus contact: a triangular wedge along the contact's top edge with horizontal dowels, a box pocket under the wedge in each beam and the dowel holes. Every contact is read from the members' outlines and checked against the kernel's contact search. The connectors are red and sit in the tree by the members they join: oculus > connectors_oculus, floor_model > seams > seam_k.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_6_contacts_floor --parallel 6 && ./build/templates_floor_6_contacts_floor && ../bash/publish-scene.sh --target templates_floor_6_contacts_floor

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
