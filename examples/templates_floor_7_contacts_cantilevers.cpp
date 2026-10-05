#include "wood_session.h"
#include "src/templates/floor/floor.h"
#include <chrono>

using namespace session_cpp;
using namespace wood_session;

const bool SEAM_THROUGH_RIBS = true; // run the seam beams on through the outer rib band to the bay's outer face, the ribs ending on them, instead of the ties
const bool BREPS = true; // write every cut member, connector part and dowel as its BRep, the dowel and screw bores exact cylinders, instead of its mesh

/// Prints every dowel set by quarter: its two members, dowel count and length.
void print_dowels(const wood_floor::Floor& floor, const std::vector<std::shared_ptr<JointBeam>>& dowels) {

    const std::vector<wood_floor::Relationship> rows = wood_floor::relationships(floor, wood_floor::Relation::block_dowels);

    for (size_t i = 0; i < rows.size(); i++) {
        if (i == 0 || rows[i].seam_or_corner != rows[i - 1].seam_or_corner)
            std::cout << fmt::format("quarter {} wedge-rib contacts, inset 50:", rows[i].seam_or_corner) << std::endl;

        const std::vector<Line>& lines = dowels[i]->drill_lines;
        std::cout << fmt::format("   {} - {}: {} dowels {:.0f} long ({})", rows[i].a.name(), rows[i].b.name(), lines.size(), lines.empty() ? 0.0 : lines.front().length(), dowels[i]->name) << std::endl;
    }
}

/// The square floor.
int main() {

    wood_floor::FloorSizes sizes;
    sizes.seam_through_ribs = SEAM_THROUGH_RIBS;
    const wood_floor::Floor model(wood_floor::FloorPlan::rectangle(3000.0, 3000.0), sizes);
    std::cout << model.check().str() << std::endl;
    WoodSession session("templates_floor_7_contacts_cantilevers");
    const std::shared_ptr<TreeNode> root = session.add_group("cantilever_model");
    wood_floor::FloorMembers members = wood_floor::add_floor(session, model, wood_floor::add_group(session, "floor_model", root));
    wood_floor::add_columns(session, model, wood_floor::add_group(session, "columns_model", root), members);

    const wood_floor::ContactCheck contacts = wood_floor::verify_contacts(session, model, members);
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    const std::vector<std::shared_ptr<JointBeam>> wedges = wood_floor::add_connectors(session, model, members, {wood_floor::Relation::seam_wedge, wood_floor::Relation::oculus_wedge});
    const std::vector<std::shared_ptr<JointBeam>> column_joints = wood_floor::add_connectors(session, model, members, {wood_floor::Relation::column_plate, wood_floor::Relation::cross_lap});
    const std::vector<std::shared_ptr<JointBeam>> ties = wood_floor::add_connectors(session, model, members, {wood_floor::Relation::seam_tie});
    const std::vector<std::shared_ptr<JointBeam>> dowels = wood_floor::add_connectors(session, model, members, {wood_floor::Relation::block_dowels});
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    const size_t plates = wood_floor::relationships(model, wood_floor::Relation::column_plate).size();
    size_t pins = 0;

    for (const std::shared_ptr<JointBeam>& joint : dowels)
        pins += joint->drill_lines.size();

    std::cout << fmt::format("{} elements, {} wedges, {} dowel sets of {} dowels, {} rectangle plates with {} cross laps, {} ties: contacts and cuts in {:.0f} ms", session.objects.elements->size(), wedges.size(), dowels.size(), pins, plates, column_joints.size() - plates, ties.size(), ms) << std::endl;
    std::cout << contacts.str() << std::endl;
    print_dowels(model, dowels);
    std::cout << wood_floor::check_breps(session).str() << std::endl;

    const std::vector<wood_floor::Relation> screw_kinds(wood_floor::SCREW_RELATIONS.begin(), wood_floor::SCREW_RELATIONS.end());
    const wood_floor::ContactCheck screw_contacts = wood_floor::verify_contacts(session, model, members, 1e-6, screw_kinds);
    const std::vector<std::shared_ptr<JointBeam>> screws = wood_floor::add_connectors(session, model, members, screw_kinds);
    std::cout << "screws: " << screw_contacts.str() << std::endl;
    std::cout << wood_floor::check_screws(session, model, screws).str() << std::endl;

    if constexpr (BREPS)
        wood_floor::compute_breps(session);

    session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 7 of the timber floor: the whole square bay, the four quarters, the oculus and the four columns on their supports, with every connector, each contact checked against the kernel's contact search: the wedges of step 6; a rectangle plate on every column-to-outer-rib contact (30 thick, 220 into the column, 265 into the rib, four dowels), the two plates of a column head half-lapped; a bow-tie key on every seam where two outer ribs meet; four Ø8 x 30 dowels on every wedge block to rib contact; and last the 72 assembly screws (200 x d4, pre-drilled, no member cut). Prints the counts, the dowel sets per quarter, the BRep check (every dowel crossing an exact bore) and the screw check (contacts and clearances). BREPS writes every cut member, connector part and dowel as its BRep. Connectors are red and nested in the tree by the members they join: quarter_model_q > connectors_q, oculus > connectors_oculus, column_model_q > connectors_column_q, floor_model > seams > seam_k.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_7_contacts_cantilevers --parallel 6 && ./build/templates_floor_7_contacts_cantilevers && ../bash/publish-scene.sh --target templates_floor_7_contacts_cantilevers

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
