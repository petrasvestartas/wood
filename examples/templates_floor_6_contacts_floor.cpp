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

/// The carved ring beams, then every wedge and its dowels apart, in compas_tf's names.
void dump(const WoodSession& session, const std::vector<std::shared_ptr<JointBeam>>& wedges, const std::string& path) {

    std::ofstream file(path);

    for (const std::shared_ptr<BeamVariable>& beam : session.beam_variables())
        if (beam->name.starts_with("inner_beams_") || (beam->name.starts_with("oculus_") && beam->name.size() == 8))
            dump(file, beam->name, beam->model_geometry_mesh());

    for (const std::shared_ptr<JointBeam>& wedge : wedges) {
        dump(file, wedge->name, Mesh::loft({wedge->parts[0][0]}, {wedge->parts[0][1]}, true));

        for (size_t i = 0; i < wedge->drill_lines.size(); i++)
            dump(file, fmt::format("{}_cylinder_{}", wedge->name, i), drill_mesh(wedge->drill_lines[i], wedge->line_radius, wedge->chord_tolerance));
    }
}

int main() {

    const wood_floor::Floor floor(wood_floor::FloorPlan::rectangle(3000.0, 3000.0), wood_floor::FloorSizes{}, wood_floor::CentralLayers::compas);
    WoodSession session("templates_floor_6_contacts_floor");
    const std::shared_ptr<TreeNode> root = session.add_group("floor_model");
    const std::shared_ptr<TreeNode> quarters = wood_floor::add_group(session, "quarters_model", root);
    std::vector<wood_floor::Member> ring;

    for (size_t q = 0; q < 4; q++) {
        const wood_floor::QuarterMembers quarter = wood_floor::add_quarter_model(session, floor.quarter(q), wood_floor::add_group(session, fmt::format("quarter_model_{}", q), quarters));
        ring.insert(ring.end(), quarter.inner_beams.begin(), quarter.inner_beams.end());
    }

    const std::vector<wood_floor::Member> oculus = wood_floor::add_oculus_model(session, floor, wood_floor::add_group(session, "oculus", root));
    ring.insert(ring.end(), oculus.begin(), oculus.end());

    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    const std::vector<std::shared_ptr<JointBeam>> wedges = wood_floor::add_wedges(session, ring, wood_floor::add_group(session, "connectors", root));
    session.pb_dump(pb_path("live"));
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    size_t dowels = 0;

    for (const std::shared_ptr<JointBeam>& wedge : wedges)
        dowels += wedge->drill_lines.size();

    std::cout << fmt::format("{} wedges, {} dowels: contacts, cuts and pb in {:.0f} ms", wedges.size(), dowels, ms) << std::endl;

    if constexpr (DUMP)
        dump(session, wedges, std::filesystem::path(pb_path("floor_6_contacts_floor")).replace_extension(".txt").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 6 of the timber floor, port of compas_tf example_model_6_contacts_floor: the four quarters and the oculus, then a wedge joint on every long-face contact among the inner beams and the four oculus boundary beams: the triangular wedge along the contact's top edge, its horizontal dowels, and in each beam a box pocket under the wedge face on its side plus the dowel holes. DUMP writes the carved ring beams, every wedge and its dowels, compared against compas_tf.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_6_contacts_floor --parallel 6 && ./build/templates_floor_6_contacts_floor && ../bash/publish-scene.sh --target templates_floor_6_contacts_floor

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
