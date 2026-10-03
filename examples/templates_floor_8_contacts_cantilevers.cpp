#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_brep_drill.h"
#include "src/templates/floor/floor.h"
#include <chrono>

using namespace session_cpp;
using namespace wood_session;

const bool BREPS = true; // write every cut element and every connector as its BRep, the dowels and the dowel and screw bores exact cylinders, instead of its mesh
const bool DUMP = true; // write name, volume and box centre of the carved columns and outer ribs to data/output/pb/floor_8_contacts_cantilevers.txt, the parity record against compas_tf

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

/// The outer ribs of every quarter, in quarter order.
std::vector<wood_floor::Member> outer_ribs(const wood_floor::FloorMembers& members) {

    std::vector<wood_floor::Member> ribs;

    for (const wood_floor::QuarterMembers& quarter : members.quarters)
        ribs.insert(ribs.end(), quarter.outer_ribs.begin(), quarter.outer_ribs.end());

    return ribs;
}

/// The connectors of one kind among those made.
std::vector<std::shared_ptr<JointBeam>> of_kind(const std::vector<std::shared_ptr<JointBeam>>& connectors, const std::string& prefix) {

    std::vector<std::shared_ptr<JointBeam>> result;

    for (const std::shared_ptr<JointBeam>& connector : connectors)
        if (connector->name.starts_with(prefix) && !std::isdigit(static_cast<unsigned char>(connector->name[prefix.size()])) && connector->name[prefix.size()] == '_' && connector->name.find('_', prefix.size() + 1) == std::string::npos)
            result.push_back(connector);

    return result;
}

/// The relationships of one kind, in connector order.
std::vector<wood_floor::Relationship> rows_of(const wood_floor::Floor& floor, wood_floor::Relation kind) {

    std::vector<wood_floor::Relationship> rows;

    for (const wood_floor::Relationship& row : wood_floor::relationships(floor))
        if (row.kind == kind)
            rows.push_back(row);

    return rows;
}

/// The midpoint of the top edge of a contact: the connector origin compas_tf's records carry.
Point top_edge_midpoint(const Polyline& contact) {

    std::vector<Point> points = contact.get_points();
    points.pop_back();
    double top = -1e300;

    for (const Point& point : points)
        top = std::max(top, point[2]);

    std::vector<Point> highest;

    for (const Point& point : points)
        if (top - point[2] <= 1e-6)
            highest.push_back(point);

    return Point::centroid(highest);
}

/// Prints every dowel set of a quarter: its two members, dowel count and length.
void print_dowels(const WoodSession& session, const wood_floor::QuarterMembers& quarter, const std::vector<std::shared_ptr<JointBeam>>& dowels, int index) {

    std::cout << fmt::format("quarter {} wedge-rib contacts, inset 50:", index) << std::endl;

    for (const std::shared_ptr<JointBeam>& set : dowels) {
        const std::shared_ptr<Element> rib = session.get_element<Element>(set->targets[0]);
        bool own = false;

        for (const wood_floor::Member& member : quarter.outer_ribs)
            own = own || member.element == rib;

        for (const wood_floor::Member& member : quarter.inner_ribs)
            own = own || member.element == rib;

        if (!own)
            continue;

        std::cout << fmt::format("   {} - {}: {} dowels {:.0f} long ({})", rib->name, session.get_element<Element>(set->targets[1])->name, set->drill_lines.size(), set->drill_lines.empty() ? 0.0 : set->drill_lines.front().length(), set->name) << std::endl;
    }
}

/// Computes and counts the BReps: every cut element exact or faceted with its bores, every connector part with its bores, written when BREPS is set.
void count_breps(WoodSession& session) {

    size_t exact = 0;
    size_t faceted = 0;
    size_t bores = 0;
    size_t connectors = 0;
    size_t part_bores = 0;
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

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

    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::cout << fmt::format("BReps of the cut elements: {} with {} exact bores, {} faceted, and of {} connectors with {} exact bores through their parts, in {:.0f} ms", exact, bores, faceted, connectors, part_bores, ms) << std::endl;
    std::cout << fmt::format("Every dowel bores every element it passes: {} dowel stretches through members and parts, {} exact bores found", dowel_crossings(session), bores + part_bores) << std::endl;
}

/// The tie key as one mesh.
Mesh tie_key(const JointBeam& tie) {

    Mesh key;

    for (const std::array<Polyline, 2>& part : tie.parts)
        append_mesh(key, Mesh::loft({part[0]}, {part[1]}, true));

    return key;
}

/// The parity record of compas_tf's example 8, first half: the eight column contacts as compas_tf wrote them (column, rib, area, rib thickness, the plate origin on the contact's top edge and its x axis toward the rib), then the carved columns, the carved outer ribs and the tie keys.
void dump_parity(const WoodSession& session, const wood_floor::Floor& floor, const wood_floor::FloorMembers& members, const std::vector<std::shared_ptr<JointBeam>>& plates, const std::vector<std::shared_ptr<JointBeam>>& ties) {

    std::ofstream file(std::filesystem::path(pb_path("floor_8_contacts_cantilevers")).replace_extension(".txt").string());
    const std::vector<wood_floor::Relationship> rows = rows_of(floor, wood_floor::Relation::column_plate);
    file << fmt::format("contacts {}\n", rows.size());

    for (size_t i = 0; i < rows.size(); i++) {
        const Point origin = top_edge_midpoint(rows[i].contact);
        const Polyline& part = plates[i]->parts[0][0];
        const Vector x = (part.get_point(1) - part.get_point(0)).normalized();
        file << fmt::format("connector {} {} {} area {:.6f} thickness {:.6f} origin {:.6f} {:.6f} {:.6f} x {:.6f} {:.6f} {:.6f}\n", i, rows[i].a.name(), rows[i].b.name(), rows[i].area(), members.thickness(rows[i].b), origin[0], origin[1], origin[2], x[0], x[1], x[2]);
    }

    for (const std::shared_ptr<Column>& column : session.columns())
        dump(file, column->name, column->model_geometry_mesh());

    for (const wood_floor::Member& rib : outer_ribs(members))
        dump(file, rib.element->name, rib.element->model_geometry_mesh());

    for (const std::shared_ptr<JointBeam>& tie : ties)
        dump(file, tie->name, tie_key(*tie));
}

/// The parity record of compas_tf's example 8, second half: the four seam contacts as compas_tf wrote them (the pair in search order, area, the tie origin on the contact's top edge and the contact normal toward the first member), each with its key, then the carved outer ribs.
void dump_ties(const wood_floor::Floor& floor, const wood_floor::FloorMembers& members, const std::vector<std::shared_ptr<JointBeam>>& ties) {

    std::ofstream file(std::filesystem::path(pb_path("floor_8_ties")).replace_extension(".txt").string());
    const std::vector<wood_floor::Relationship> rows = rows_of(floor, wood_floor::Relation::seam_tie);
    file << fmt::format("contacts {}\n", rows.size());

    for (size_t i = 0; i < rows.size(); i++) {
        const std::array<wood_floor::MemberRef, 2> pair = rows[i].scene_pair();
        const Point origin = top_edge_midpoint(rows[i].contact);
        const Point first = members.get(pair[0])->model_geometry_mesh().centroid();
        Vector y(rows[i].plane.z_axis()[0], rows[i].plane.z_axis()[1], 0.0);
        y = y.normalized() * ((first - origin).dot(y) < 0.0 ? -1.0 : 1.0);
        file << fmt::format("tie {} {} {} area {:.6f} origin {:.6f} {:.6f} {:.6f} y {:.6f} {:.6f} {:.6f}\n", i, pair[0].name(), pair[1].name(), rows[i].area(), origin[0], origin[1], origin[2], y[0], y[1], y[2]);
        dump(file, ties[i]->name, tie_key(*ties[i]));
    }

    for (const wood_floor::Member& rib : outer_ribs(members))
        dump(file, rib.element->name, rib.element->model_geometry_mesh());
}

/// The screw records: per screw its connector and index, relation, members, the members it also passes, head and tip.
void dump_screws(const wood_floor::Floor& floor, const std::vector<std::shared_ptr<JointBeam>>& screws, const std::string& path) {

    std::ofstream file(path);
    size_t next = 0;

    for (const wood_floor::Relationship& row : wood_floor::relationships(floor)) {
        if (row.screws.empty() || next >= screws.size())
            continue;

        const std::shared_ptr<JointBeam>& connector = screws[next++];

        std::string through;

        for (const wood_floor::MemberRef& ref : row.through)
            through += " through " + ref.name();

        for (size_t i = 0; i < connector->drill_lines.size(); i++) {
            const Line& screw = connector->drill_lines[i];
            file << fmt::format("{}/{} {} {} {}{} {:.6f} {:.6f} {:.6f} {:.6f} {:.6f} {:.6f}\n", connector->name, i, wood_floor::relation_name(row.kind), row.a.name(), row.b.name(), through, screw.start()[0], screw.start()[1], screw.start()[2], screw.end()[0], screw.end()[1], screw.end()[2]);
        }
    }
}

/// Adds the assembly screws of every screw relationship under group, after every other connector so nothing before changes, checks their contacts against the kernel's search and their clearances, prints both, and gives the screws' dowels their exact cylinders.
std::vector<std::shared_ptr<JointBeam>> add_screws(WoodSession& session, const wood_floor::Floor& floor, const wood_floor::FloorMembers& members, const std::shared_ptr<TreeNode>& group) {

    const std::vector<wood_floor::Relation> kinds(wood_floor::SCREW_RELATIONS.begin(), wood_floor::SCREW_RELATIONS.end());
    const std::vector<wood_floor::ContactMismatch> mismatches = wood_floor::verify_contacts(session, floor, members, 1e-6, kinds);
    const std::vector<std::shared_ptr<JointBeam>> screws = wood_floor::add_connectors(session, floor, members, group, kinds);

    for (const wood_floor::ContactMismatch& mismatch : mismatches)
        std::cout << fmt::format("screw contact mismatch: {}: {}", mismatch.relation, mismatch.what) << std::endl;

    std::cout << fmt::format("{} of {} screw contacts verified by the kernel's search", screws.size() - mismatches.size(), screws.size()) << std::endl;
    std::cout << wood_floor::check_screws(session, floor, screws).str() << std::endl;

    for (const std::shared_ptr<Dowel>& dowel : session.get_elements<Dowel>())
        if (BREPS && dowel->name.starts_with("connector_screws_"))
            dowel->compute_geometry_brep();

    return screws;
}

/// The square floor with the model's definitions, or in compas_tf's parity mode with --compas.
int main(int argc, char** argv) {

    const wood_floor::FloorPlan plan = wood_floor::FloorPlan::rectangle(3000.0, 3000.0);
    const wood_floor::Floor model = argc > 1 && std::string(argv[1]) == "--compas" ? wood_floor::Floor::compas_parity(plan, wood_floor::FloorSizes{}) : wood_floor::Floor(plan, wood_floor::FloorSizes{});
    std::cout << model.check().str() << std::endl;
    WoodSession session("templates_floor_8_contacts_cantilevers");
    const std::shared_ptr<TreeNode> root = session.add_group("cantilever_model");
    const std::shared_ptr<TreeNode> floor = wood_floor::add_group(session, "floor_model", root);
    wood_floor::FloorMembers members = wood_floor::add_floor(session, model, floor);
    wood_floor::add_columns(session, model, wood_floor::add_group(session, "columns_model", root), members);
    const std::vector<wood_floor::ContactMismatch> mismatches = wood_floor::verify_contacts(session, model, members);

    for (const wood_floor::ContactMismatch& mismatch : mismatches)
        std::cout << fmt::format("contact mismatch: {}: {}", mismatch.relation, mismatch.what) << std::endl;

    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    const std::vector<std::shared_ptr<JointBeam>> wedges = wood_floor::add_connectors(session, model, members, wood_floor::add_group(session, "connectors", floor), {wood_floor::Relation::seam_wedge, wood_floor::Relation::oculus_wedge});
    const std::vector<std::shared_ptr<JointBeam>> column_joints = wood_floor::add_connectors(session, model, members, wood_floor::add_group(session, "connectors", root), {wood_floor::Relation::column_plate, wood_floor::Relation::cross_lap});
    const std::vector<std::shared_ptr<JointBeam>> plates = of_kind(column_joints, "connector");
    const std::vector<std::shared_ptr<JointBeam>> laps = of_kind(column_joints, "connector_cross_lap");
    const std::vector<std::shared_ptr<JointBeam>> ties = wood_floor::add_connectors(session, model, members, wood_floor::add_group(session, "outer_rib_connectors", root), {wood_floor::Relation::seam_tie});
    const std::vector<std::shared_ptr<JointBeam>> dowels = wood_floor::add_connectors(session, model, members, wood_floor::add_group(session, "quarter_connectors", floor), {wood_floor::Relation::block_dowels});
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    size_t pins = 0;

    for (const std::shared_ptr<JointBeam>& joint : dowels)
        pins += joint->drill_lines.size();

    std::cout << fmt::format("{} elements, {} wedges, {} dowel sets of {} dowels, {} rectangle plates with {} cross laps, {} ties: contacts and cuts in {:.0f} ms", session.objects.elements->size(), wedges.size(), dowels.size(), pins, plates.size(), laps.size(), ties.size(), ms) << std::endl;
    std::cout << fmt::format("{} of 44 contacts verified by the kernel's search", 44 - mismatches.size()) << std::endl;

    for (size_t i = 0; i < members.quarters.size(); i++)
        print_dowels(session, members.quarters[i], dowels, static_cast<int>(i));

    count_breps(session);
    const std::vector<std::shared_ptr<JointBeam>> screws = add_screws(session, model, members, wood_floor::add_group(session, "screw_connectors", floor));
    session.pb_dump(pb_path("live"));

    if constexpr (DUMP) {
        dump_parity(session, model, members, plates, ties);
        dump_ties(model, members, ties);
        dump_screws(model, screws, std::filesystem::path(pb_path("floor_8_screws")).replace_extension(".txt").string());
    }

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 8 of the timber floor, port of compas_tf example_model_8_contacts_cantilevers: the four quarters, the oculus and the four columns on their supports, every connector made from the floor's 76 relationships (the contact polygons read from the members' outlines, every one checked against the kernel's contact search), the wedges of step 6, and a rectangle plate joint on the contact of every column with every outer rib: a 30 mm plate 220 into the column and 265 into the rib with four dowels, cut as a pocket and dowel holes into both; and a tie on every seam where two outer ribs of neighbouring quarters meet end to end: the bow-tie key of compas_tf's OBJ template made parametric, with its two mirrored pockets. Beyond compas_tf: the two rectangle plates of every column head cross as a half lap, a cross lap joint slotting each plate half its height where the other passes; and assembly dowels within every quarter, on every contact of a wedge block with an outer or inner rib: four Ø8 dowels 30 long per contact, one exactly at each corner of the contact inset by 50, 15 into each member. Every connector is a nested group in the tree: the connector node holding the relation, under it its plate, wedge or key as an element with exact bores where its dowels pass through, and every dowel as an element of its own, an exact cylinder flush with the members it passes through, while the holes run on past every face a dowel leaves. BREPS writes every cut element and every connector as its BRep, the dowels and the dowel and screw bores exact cylinders. Prints the floor's report. DUMP writes the column and seam contacts, the carved columns, outer ribs and ties, compared against compas_tf with --compas. Then the assembly screws, after every other connector so nothing before them changes: pre-drilled lines 200 long, d 4, two per location at two heights, 12 per quarter (outer ribs into the seam beams, the seam beams into the oculus beam across the mitres, the inner rib ends through the beam corners), 8 at the ring's pinwheel corners and 16 toe screws from the ring into the quarters' oculus beams, 72 in all; each line stored once on its connector, which names every member it passes, no member cut, every screw a dowel child drawn as an exact cylinder; prints the screw count per kind, their contacts against the kernel's search and their closest approach to each other, to the dowel bores and to the pockets, and DUMP writes them with their heads and tips to floor_8_screws.txt. Every connector node and every part and dowel nested under it carries the connector colour, red (wood_floor::CONNECTOR_COLOR on the tree node, which the pb keeps).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_8_contacts_cantilevers --parallel 6 && ./build/templates_floor_8_contacts_cantilevers && ../bash/publish-scene.sh --target templates_floor_8_contacts_cantilevers

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
