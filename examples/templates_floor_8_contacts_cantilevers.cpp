#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/floor/floor.h"
#include <chrono>

using namespace session_cpp;
using namespace wood_session;

const bool DUMP = true; // write name, volume and box centre of the carved columns and outer ribs to data/output/pb/floor_8_contacts_cantilevers.txt, the parity record against compas_tf

const wood_floor::FloorGuide GUIDE{
    .size_grid_x = 3000.0,
    .size_grid_y = 3000.0,
    .size_column_head = 220.0,
    .size_column_head_chamfer = 120.0,
    .size_outer_ribs = 100.0,
    .size_inner_ribs = 60.0,
    .size_inner_beams = 60.0,
    .size_wedge = 240.0,
    .height = 650.0,
    .rise = 453.0,
    .size_oculus = 1000.0,
};

/// One dump line: name, volume and box centre of a mesh.
void dump(std::ofstream& file, const std::string& name, const Mesh& mesh) {

    const AABB box = AABB::from_mesh(mesh);
    file << fmt::format("{} {:.6f} {:.6f} {:.6f} {:.6f}\n", name, compute_volume(mesh), box.cx, box.cy, box.cz);
}

int main() {

    WoodSession session("templates_floor_8_contacts_cantilevers");
    const std::shared_ptr<TreeNode> root = session.add_group("cantilever_model");
    const std::shared_ptr<TreeNode> floor = wood_floor::add_group(session, "floor_model", root);
    const std::shared_ptr<TreeNode> quarters = wood_floor::add_group(session, "quarters_model", floor);
    const std::shared_ptr<TreeNode> columns = wood_floor::add_group(session, "columns_model", root);

    for (int i = 0; i < 4; i++) {
        const std::string suffix = fmt::format("_{}", i);
        const Xform turn = Xform::rotation_z(i * 90.0, true);
        wood_floor::add_quarter_model(session, GUIDE, turn, wood_floor::add_group(session, "quarter_model" + suffix, quarters), suffix);
        wood_floor::add_column_model(session, GUIDE, turn, wood_floor::add_group(session, "column_model" + suffix, columns), suffix);
    }

    wood_floor::add_oculus_model(session, GUIDE, wood_floor::add_group(session, "oculus", floor));

    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    const std::vector<std::shared_ptr<JointBeam>> wedges = wood_floor::add_wedges(session, GUIDE, wood_floor::add_group(session, "connectors", floor));
    const std::vector<std::shared_ptr<JointBeam>> plates = wood_floor::add_rectangle_plates(session, GUIDE, wood_floor::add_group(session, "connectors", root));
    session.pb_dump(pb_path("live"));
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::cout << fmt::format("{} elements, {} wedges, {} rectangle plates: contacts, cuts and pb in {:.0f} ms", session.objects.elements->size(), wedges.size(), plates.size(), ms) << std::endl;

    for (const std::shared_ptr<JointBeam>& plate : plates)
        std::cout << fmt::format("{} {} {} origin {:.3f} {:.3f} {:.3f}", plate->name, session.get_element<Element>(plate->targets[0])->name, session.get_element<Element>(plate->targets[1])->name, plate->parts[0][0].get_point(3)[0], plate->parts[0][0].get_point(3)[1], plate->parts[0][0].get_point(3)[2]) << std::endl;

    if constexpr (DUMP) {
        std::ofstream file(std::filesystem::path(pb_path("floor_8_contacts_cantilevers")).replace_extension(".txt").string());

        for (const std::shared_ptr<Column>& column : session.columns())
            dump(file, column->name, column->model_geometry_mesh());

        for (const std::shared_ptr<BeamVariable>& beam : session.beam_variables())
            if (beam->name.starts_with("outer_ribs_"))
                dump(file, beam->name, beam->model_geometry_mesh());
    }

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 8 of the timber floor, port of compas_tf example_model_8_contacts_cantilevers: the four quarters, the oculus and the four columns on their supports, the wedges of step 6, and a rectangle plate joint on the contact of every column with every outer rib: a 30 mm plate 220 into the column and 265 into the rib with four dowels, cut as a pocket and dowel holes into both. DUMP writes the carved columns and outer ribs, compared against compas_tf.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_8_contacts_cantilevers --parallel 6 && ./build/templates_floor_8_contacts_cantilevers && ../bash/publish-scene.sh --target templates_floor_8_contacts_cantilevers

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
