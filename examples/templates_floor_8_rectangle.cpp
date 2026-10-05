#include "wood_session.h"
#include "src/templates/floor/floor.h"
#include <chrono>

using namespace session_cpp;
using namespace wood_session;

const bool BREPS = true; // write every cut member, connector part and dowel as its BRep, the dowel and screw bores exact cylinders, instead of its mesh
const double HALF_X = 3000.0; // half span along x: the bay is 6000 long
const double HALF_Y = 2400.0; // half span along y: the bay is 4800 wide

/// Prints the contacts the kernel's search disagrees with and how many of the count agree.
void print_contacts(const std::string& what, const std::vector<wood_floor::ContactMismatch>& mismatches, size_t count) {

    for (const wood_floor::ContactMismatch& mismatch : mismatches)
        std::cout << fmt::format("{} mismatch: {}: {}", what, mismatch.relation, mismatch.what) << std::endl;

    std::cout << fmt::format("{} of {} {}s verified by the kernel's search", count - mismatches.size(), count, what) << std::endl;
}

/// The number of dowels over a set of connectors.
size_t count_dowels(const std::vector<std::shared_ptr<JointBeam>>& connectors) {

    size_t count = 0;

    for (const std::shared_ptr<JointBeam>& connector : connectors)
        count += connector->drill_lines.size();

    return count;
}

/// The rectangular bay.
int main() {

    const wood_floor::Floor model(wood_floor::FloorPlan::rectangle(HALF_X, HALF_Y), wood_floor::FloorSizes{});
    std::cout << model.check().str() << std::endl;
    WoodSession session("templates_floor_8_rectangle");
    const std::shared_ptr<TreeNode> root = session.add_group("cantilever_model");
    wood_floor::FloorMembers members = wood_floor::add_floor(session, model, wood_floor::add_group(session, "floor_model", root));
    wood_floor::add_columns(session, model, wood_floor::add_group(session, "columns_model", root), members);

    const std::vector<wood_floor::ContactMismatch> mismatches = wood_floor::verify_contacts(session, model, members);
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    const std::vector<std::shared_ptr<JointBeam>> wedges = wood_floor::add_connectors(session, model, members, {wood_floor::Relation::seam_wedge, wood_floor::Relation::oculus_wedge});
    const std::vector<std::shared_ptr<JointBeam>> column_joints = wood_floor::add_connectors(session, model, members, {wood_floor::Relation::column_plate, wood_floor::Relation::cross_lap});
    const std::vector<std::shared_ptr<JointBeam>> ties = wood_floor::add_connectors(session, model, members, {wood_floor::Relation::seam_tie});
    const std::vector<std::shared_ptr<JointBeam>> dowels = wood_floor::add_connectors(session, model, members, {wood_floor::Relation::block_dowels});
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    const size_t plates = wood_floor::relationships(model, wood_floor::Relation::column_plate).size();

    std::cout << fmt::format("{} elements, {} wedges with {} dowels, {} dowel sets of {} dowels, {} rectangle plates with {} cross laps, {} ties: contacts and cuts in {:.0f} ms", session.objects.elements->size(), wedges.size(), count_dowels(wedges), dowels.size(), count_dowels(dowels), plates, column_joints.size() - plates, ties.size(), ms) << std::endl;
    print_contacts("contact", mismatches, wedges.size() + plates + ties.size() + dowels.size());
    std::cout << wood_floor::check_breps(session).str() << std::endl;

    const std::vector<wood_floor::Relation> screw_kinds(wood_floor::SCREW_RELATIONS.begin(), wood_floor::SCREW_RELATIONS.end());
    const std::vector<wood_floor::ContactMismatch> screw_mismatches = wood_floor::verify_contacts(session, model, members, 1e-6, screw_kinds);
    const std::vector<std::shared_ptr<JointBeam>> screws = wood_floor::add_connectors(session, model, members, screw_kinds);
    print_contacts("screw contact", screw_mismatches, screws.size());
    std::cout << wood_floor::check_screws(session, model, screws).str() << std::endl;

    if constexpr (BREPS)
        wood_floor::compute_breps(session);

    session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 8 of the timber floor: the same model on a 6000 x 4800 bay, Floor(FloorPlan::rectangle(3000, 2400), FloorSizes{}). The quarters are built in place at their own corners and mirror each other; the oculus is a square diamond of half-diagonal 1000; the central panel follows rule A, its inner ribs swept along one direction so the central bed is one planar-faced cylinder with every layer 27 thick; both outer ribs of a corner end at one level (the short ribs' run-in solved to 187.667), which is the middle cutter level, so every rib meets its column head within 0.307 mm. The connectors and screws are those of step 7. Prints the report, the counts, the BRep check and the screw check; BREPS writes the BReps.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_8_rectangle --parallel 6 && ./build/templates_floor_8_rectangle && ../bash/publish-scene.sh --target templates_floor_8_rectangle

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
