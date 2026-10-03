#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/floor/floor.h"
#include <chrono>

using namespace session_cpp;
using namespace wood_session;

const bool DUMP = true; // write name, volume and box centre of the carved ring beams, the wedges and their dowels to data/output/pb/floor_6_contacts_floor.txt, the parity record against compas_tf

/// One dump line: name, volume and box centre of a mesh.
void dump(std::ofstream& file, const std::string& name, const Mesh& mesh) {

    const AABB box = AABB::from_mesh(mesh);
    file << fmt::format("{} {:.6f} {:.6f} {:.6f} {:.6f}\n", name, compute_volume(mesh), box.cx, box.cy, box.cz);
}

/// The contact records of the wedges as compas_tf's example 6 wrote them: the pair in search order, the thicker member, the wedge length, its dowels and the contact area.
void dump_contacts(std::ofstream& file, const wood_floor::Floor& floor, const wood_floor::FloorMembers& members, const std::vector<std::shared_ptr<JointBeam>>& wedges) {

    std::vector<wood_floor::Relationship> rows;

    for (const wood_floor::Relationship& row : wood_floor::relationships(floor))
        if (row.kind == wood_floor::Relation::seam_wedge || row.kind == wood_floor::Relation::oculus_wedge)
            rows.push_back(row);

    file << fmt::format("contacts {}\n", rows.size());

    for (size_t k = 0; k < rows.size(); k++) {
        const std::array<wood_floor::MemberRef, 2> pair = rows[k].scene_pair();
        const double thickness = std::max(members.thickness(pair[0]), members.thickness(pair[1]));
        const double length = (wedges[k]->parts[0][1].get_point(0) - wedges[k]->parts[0][0].get_point(0)).magnitude();
        file << fmt::format("wedge {} {} {} thickness {:.6f} length {:.6f} dowels {} area {:.6f}\n", k, pair[0].name(), pair[1].name(), thickness, length, wedges[k]->drill_lines.size(), rows[k].area());
    }
}

/// The contact records, the carved ring beams, then every wedge and its dowels apart, in compas_tf's names.
void dump(const WoodSession& session, const wood_floor::Floor& floor, const wood_floor::FloorMembers& members, const std::vector<std::shared_ptr<JointBeam>>& wedges, const std::string& path) {

    std::ofstream file(path);
    dump_contacts(file, floor, members, wedges);

    for (const std::shared_ptr<BeamVariable>& beam : session.beam_variables())
        if (beam->name.starts_with("inner_beams_") || (beam->name.starts_with("oculus_") && beam->name.size() == 8))
            dump(file, beam->name, beam->model_geometry_mesh());

    for (const std::shared_ptr<JointBeam>& wedge : wedges) {
        dump(file, wedge->name, Mesh::loft({wedge->parts[0][0]}, {wedge->parts[0][1]}, true));

        for (size_t i = 0; i < wedge->drill_lines.size(); i++)
            dump(file, fmt::format("{}_cylinder_{}", wedge->name, i), drill_mesh(wedge->drill_lines[i], wedge->line_radius, wedge->chord_tolerance));
    }
}

/// The square floor with the model's definitions, or in compas_tf's parity mode with --compas.
int main(int argc, char** argv) {

    const wood_floor::FloorPlan plan = wood_floor::FloorPlan::rectangle(3000.0, 3000.0);
    const wood_floor::Floor floor = argc > 1 && std::string(argv[1]) == "--compas" ? wood_floor::Floor::compas_parity(plan, wood_floor::FloorSizes{}) : wood_floor::Floor(plan, wood_floor::FloorSizes{});
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

    if constexpr (DUMP)
        dump(session, floor, members, wedges, std::filesystem::path(pb_path("floor_6_contacts_floor")).replace_extension(".txt").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 6 of the timber floor, port of compas_tf example_model_6_contacts_floor: the four quarters and the oculus, then a wedge joint on each of the floor's seam and oculus relationships, the contact polygon read from the members' outlines on their shared plane and checked against the kernel's contact search: the triangular wedge along the contact's top edge, its horizontal dowels, and in each beam a box pocket under the wedge face on its side plus the dowel holes. Prints the floor's report; DUMP writes the contact records, the carved ring beams, every wedge and its dowels, compared against compas_tf. Every connector node and every part and dowel nested under it carries the connector colour, red (wood_floor::CONNECTOR_COLOR on the tree node, which the pb keeps). The wedges sit in the tree by the members they join: the oculus wedges in oculus > connectors_oculus, the seam wedge of seam k in floor_model > seams > seam_k, each with its wedge and dowels nested under it.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_6_contacts_floor --parallel 6 && ./build/templates_floor_6_contacts_floor && ../bash/publish-scene.sh --target templates_floor_6_contacts_floor

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
