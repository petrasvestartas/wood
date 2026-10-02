#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_brep_drill.h"
#include "src/templates/floor/floor.h"
#include <chrono>

using namespace session_cpp;
using namespace wood_session;

const bool BREPS = true; // write every cut element and every connector as its BRep, the dowels and the dowel and screw bores exact cylinders, instead of its mesh
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

/// The number of exact bores in a BRep: its rational surfaces, cylinders.
size_t count_bores(const BRep& brep) {

    size_t bores = 0;

    for (const NurbsSurface& surface : brep.m_surfaces)
        if (surface.is_rational())
            bores++;

    return bores;
}

/// How many bores the dowels of a solid ask for: one per stretch of a dowel inside it, the solid cut by its pockets first, dowels meeting end to end on one axis joined into one.
size_t bore_stretches(const Mesh& solid, const std::vector<Drill>& drills) {

    size_t stretches = 0;

    for (const Drill& drill : merged_drills(drills))
        for (const std::array<double, 2>& stretch : inside_stretches(solid, drill.axis))
            if (stretch[1] > 1e-6 && stretch[0] < drill.axis.length() - 1e-6)
                stretches++;

    return stretches;
}

/// How many bores the joints' dowels and screws ask for over the scene: every stretch of one inside a target's pocketed solid or inside a connector's own part.
size_t dowel_crossings(const WoodSession& session) {

    std::map<std::string, std::vector<Drill>> drills;
    size_t crossings = 0;

    for (const std::shared_ptr<Joint>& joint : session.get_elements<Joint>()) {
        std::vector<Drill> own;

        for (const Line& dowel : joint->drill_axes())
            own.push_back({dowel, joint->line_radius});

        for (const std::string& target : joint->targets)
            drills[target].insert(drills[target].end(), own.begin(), own.end());

        if (const std::shared_ptr<JointBeam> connector = std::dynamic_pointer_cast<JointBeam>(joint))
            for (size_t i = 0; i < connector->parts.size(); i++)
                crossings += bore_stretches(apply_solid_cuts(connector->part_mesh(i), connector->solid_cuts, false), own);
    }

    for (const std::pair<const std::string, std::vector<Drill>>& element : drills) {
        const std::shared_ptr<Element> target = session.get_element<Element>(element.first);
        const std::vector<SolidCut>* cuts = solid_cuts_of(*target);
        crossings += bore_stretches(cuts ? apply_solid_cuts(target->element_geometry_mesh(), *cuts, false) : target->element_geometry_mesh(), element.second);
    }

    return crossings;
}

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
    const std::vector<std::shared_ptr<JointBeam>> laps = wood_floor::add_cross_laps(session, plates, wood_floor::add_group(session, "connectors", root));
    const std::vector<std::shared_ptr<JointBeam>> ties = wood_floor::add_ties(session, wood_floor::add_group(session, "outer_rib_connectors", root));
    std::vector<std::string> misfits;
    double minimum_distance = 0.0;
    size_t through = 0;
    const std::vector<std::shared_ptr<JointBeam>> dowels = wood_floor::add_quarter_dowels(session, GUIDE, wood_floor::add_group(session, "quarter_connectors", floor), 4.0, 30.0, 50.0, 20.0, &misfits, &minimum_distance, &through);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    size_t pins = 0;

    for (const std::shared_ptr<JointBeam>& joint : dowels)
        pins += joint->drill_lines.size();

    std::cout << fmt::format("{} elements, {} wedges, {} dowel sets of {} dowels at least {:.1f} apart, {} pairs meeting as one through bore, {} rectangle plates with {} cross laps, {} ties: contacts and cuts in {:.0f} ms", session.objects.elements->size(), wedges.size(), dowels.size(), pins, minimum_distance, through, plates.size(), laps.size(), ties.size(), ms) << std::endl;

    for (int quarter = 0; quarter < 4; quarter++)
        for (const bool rib : {true, false}) {
            std::cout << fmt::format("quarter {} {}:", quarter, rib ? "wedge-rib contacts, inset 50" : "other contacts, inset 50") << std::endl;

            for (const std::shared_ptr<JointBeam>& set : dowels) {
                const std::string a = session.get_element<Element>(set->targets[0])->name;
                const std::string b = session.get_element<Element>(set->targets[1])->name;

                if (a.ends_with(fmt::format("_{}", quarter)) && ((a.find("ribs_") != std::string::npos && b.starts_with("wedges_")) == rib))
                    std::cout << fmt::format("   {} - {}: {} dowels {:.0f} long ({})", a, b, set->drill_lines.size(), set->drill_lines.empty() ? 0.0 : set->drill_lines.front().length(), set->name) << std::endl;
            }
        }

    for (const std::string& misfit : misfits)
        std::cout << "dowels: " << misfit << std::endl;

    size_t exact = 0;
    size_t faceted = 0;
    size_t bores = 0;
    size_t connectors = 0;
    size_t part_bores = 0;
    const std::chrono::steady_clock::time_point breps = std::chrono::steady_clock::now();

    for (const std::shared_ptr<Element>& element : *session.objects.elements) {
        if (std::dynamic_pointer_cast<Dowel>(element) || std::dynamic_pointer_cast<ConnectorPart>(element)) {
            if (std::dynamic_pointer_cast<ConnectorPart>(element))
                part_bores += count_bores(element->model_geometry_brep());

            if constexpr (BREPS)
                element->compute_geometry_brep();

            continue;
        }

        if (const std::shared_ptr<JointBeam> connector = std::dynamic_pointer_cast<JointBeam>(element)) {
            connectors += !connector->parts.empty() || !connector->drill_lines.empty();
            continue;
        }

        if (std::dynamic_pointer_cast<Joint>(element) || element->model_geometry_mesh().number_of_vertices() == element->element_geometry_mesh().number_of_vertices())
            continue;

        const size_t round = count_bores(element->model_geometry_brep());
        bores += round;
        round > 0 ? exact++ : faceted++;

        if (round == 0)
            std::cout << "faceted: " << element->name << std::endl;

        if constexpr (BREPS)
            element->compute_geometry_brep();
    }

    const double brep_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - breps).count();
    std::cout << fmt::format("BReps of the cut elements: {} with {} exact bores, {} faceted, and of {} connectors with {} exact bores through their parts, in {:.0f} ms", exact, bores, faceted, connectors, part_bores, brep_ms) << std::endl;
    std::cout << fmt::format("Every dowel bores every element it passes: {} dowel stretches through members and parts, {} exact bores found", dowel_crossings(session), bores + part_bores) << std::endl;

    session.pb_dump(pb_path("live"));

    if constexpr (DUMP) {
        std::ofstream file(std::filesystem::path(pb_path("floor_8_contacts_cantilevers")).replace_extension(".txt").string());

        for (const std::shared_ptr<Column>& column : session.columns())
            dump(file, column->name, column->model_geometry_mesh());

        for (const std::shared_ptr<BeamVariable>& beam : session.beam_variables())
            if (beam->name.starts_with("outer_ribs_"))
                dump(file, beam->name, beam->model_geometry_mesh());

        for (const std::shared_ptr<JointBeam>& tie : ties) {
            Mesh key;

            for (const std::array<Polyline, 2>& part : tie->parts)
                append_mesh(key, Mesh::loft({part[0]}, {part[1]}, true));

            dump(file, tie->name, key);
        }
    }

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 8 of the timber floor, port of compas_tf example_model_8_contacts_cantilevers: the four quarters, the oculus and the four columns on their supports, the wedges of step 6, and a rectangle plate joint on the contact of every column with every outer rib: a 30 mm plate 220 into the column and 265 into the rib with four dowels, cut as a pocket and dowel holes into both; and a tie on every seam where two outer ribs of neighbouring quarters meet end to end: the bow-tie key of compas_tf's OBJ template made parametric, with its two mirrored pockets. Beyond compas_tf: the two rectangle plates of every column head cross as a half lap, a cross lap joint slotting each plate half its height where the other passes; and assembly dowels within every quarter, on every face contact among its wedge blocks, inner beams, outer ribs and inner ribs: four Ø8 dowels 30 long per contact, one exactly at each corner of the contact inset by 50, 15 into each member; a dowel leaving a member or closer than 20 to another is listed for information. Every connector is a nested group in the tree: the connector node holding the relation, under it its plate, wedge or key as an element with exact bores where its dowels pass through, and every dowel as an element of its own, an exact cylinder flush with the members it passes through, while the holes run on past every face a dowel leaves. BREPS writes every cut element and every connector as its BRep, the dowels and the dowel and screw bores exact cylinders. DUMP writes the carved columns and outer ribs and the ties, compared against compas_tf.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_8_contacts_cantilevers --parallel 6 && ./build/templates_floor_8_contacts_cantilevers && ../bash/publish-scene.sh --target templates_floor_8_contacts_cantilevers

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
