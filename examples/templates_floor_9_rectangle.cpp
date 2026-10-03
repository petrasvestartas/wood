#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_brep_drill.h"
#include "src/templates/floor/floor.h"
#include <chrono>

using namespace session_cpp;
using namespace wood_session;

const bool BREPS = true; // write every cut element and every connector as its BRep, the dowels and the dowel and screw bores exact cylinders, instead of its mesh
const bool DUMP = true; // write the contacts, the connector counts and the report to data/output/pb/floor_9_rectangle.txt; with --compas also every quarter's guide records in its corner frame, floor_9_rectangle_q<q>.txt, the R1 parity record against compas_tf
const double HALF_X = 3000.0; // half span along x: the bay is 6000 long
const double HALF_Y = 2400.0; // half span along y: the bay is 4800 wide

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

/// The connectors of one kind among those made.
std::vector<std::shared_ptr<JointBeam>> of_kind(const std::vector<std::shared_ptr<JointBeam>>& connectors, const std::string& prefix) {

    std::vector<std::shared_ptr<JointBeam>> result;

    for (const std::shared_ptr<JointBeam>& connector : connectors)
        if (connector->name.starts_with(prefix) && !std::isdigit(static_cast<unsigned char>(connector->name[prefix.size()])) && connector->name[prefix.size()] == '_' && connector->name.find('_', prefix.size() + 1) == std::string::npos)
            result.push_back(connector);

    return result;
}

/// Computes and counts the BReps: every cut element exact or faceted with its bores, every connector part with its bores, written when BREPS is set; returns the faceted count, the exact bores and the dowel stretches.
std::array<size_t, 3> count_breps(WoodSession& session) {

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
    const size_t stretches = dowel_crossings(session);
    std::cout << fmt::format("Every dowel bores every element it passes: {} dowel stretches through members and parts, {} exact bores found", stretches, bores + part_bores) << std::endl;

    return {faceted, bores + part_bores, stretches};
}

// ═══════════════════════════════════════════════════════════════════════════
// R1: a quarter in its corner frame
// ═══════════════════════════════════════════════════════════════════════════

/// A quarter's corner frame mapped onto compas_tf's quarter 0 of the FloorGuide with the quarter's half spans: corner at (-half span after, -half span before), x along the edge after the corner, y along the edge before it, reversed.
struct Frame {
    Point corner; // The bay corner.
    Vector x; // Along the edge after the corner.
    Vector y; // Along the edge before the corner, reversed.
    Point origin; // Where the corner lands in compas_tf's quarter 0.

    /// The point in compas_tf's quarter 0.
    Point point(const Point& p) const {
        const Vector d = p - corner;
        return Point(origin[0] + d.dot(x), origin[1] + d.dot(y), p[2]);
    }

    /// The direction in compas_tf's quarter 0.
    Point normal(const Vector& n) const {
        return Point(n.dot(x), n.dot(y), n[2]);
    }
};

/// One dump line: the name and every point mapped by the frame.
void dump(std::ofstream& file, const Frame& frame, const std::string& name, const std::vector<Point>& points) {

    file << name;

    for (const Point& point : points) {
        const Point p = frame.point(point);
        file << fmt::format(" {:.9f} {:.9f} {:.9f}", p[0], p[1], p[2]);
    }

    file << "\n";
}

/// One dump line for a plane: its origin and normal mapped by the frame.
void dump(std::ofstream& file, const Frame& frame, const std::string& name, const Plane& plane) {

    const Point o = frame.point(plane.origin());
    const Point n = frame.normal(plane.z_axis());
    file << fmt::format("{} {:.9f} {:.9f} {:.9f} {:.9f} {:.9f} {:.9f}\n", name, o[0], o[1], o[2], n[0], n[1], n[2]);
}

/// The plane pairs of one family.
void dump(std::ofstream& file, const Frame& frame, const std::string& key, const std::vector<std::array<Plane, 2>>& pairs) {

    for (size_t i = 0; i < pairs.size(); i++)
        for (size_t j = 0; j < 2; j++)
            dump(file, frame, fmt::format("planes/{}/{}/{}", key, i, j), pairs[i][j]);
}

/// The quads of one family.
void dump(std::ofstream& file, const Frame& frame, const std::string& key, const std::vector<Polyline>& quads) {

    for (size_t i = 0; i < quads.size(); i++)
        dump(file, frame, fmt::format("quads/{}/{}", key, i), quads[i].get_points());
}

/// The two loops of one member outline.
void dump(std::ofstream& file, const Frame& frame, const std::string& name, const wood_floor::Outline& outline) {

    dump(file, frame, name + "/top", outline.top.get_points());
    dump(file, frame, name + "/bottom", outline.bottom.get_points());
}

/// The outline pairs of one member group, numbered from first.
void dump(std::ofstream& file, const Frame& frame, const std::string& group, const std::vector<wood_floor::Outline>& outlines, size_t first = 0) {

    for (size_t i = 0; i < outlines.size(); i++)
        dump(file, frame, fmt::format("{}/{}", group, first + i), outlines[i]);
}

/// One bed row numbered through the rows from first, each plate with its row.
void dump_row(std::ofstream& file, const Frame& frame, const std::vector<wood_floor::Outline>& row, size_t index, size_t first) {

    for (size_t i = 0; i < row.size(); i++) {
        dump(file, frame, fmt::format("beds/{}", first + i), row[i]);
        file << fmt::format("beds/{}/row {:.9f}\n", first + i, static_cast<double>(index));
    }
}

/// The construction records of quarter q in compas_tf's quarter 0: polygons, planes, quads, parabolas and shadows, block levels and bed planes.
void dump_construction(std::ofstream& file, const Frame& frame, const wood_floor::Quarter& quarter) {

    const wood_floor::QuarterGeometry& geometry = quarter.geometry();
    dump(file, frame, "quarter_polygon", geometry.polygon);
    dump(file, frame, "quarter_column_polygon", quarter.column().head);

    const wood_floor::ConstructionPlanes& cp = geometry.planes;
    dump(file, frame, "outer_ribs", cp.outer_ribs);
    dump(file, frame, "inner_beams", cp.inner_beams);
    dump(file, frame, "inner_ribs", cp.inner_ribs);
    dump(file, frame, "wedges", cp.wedges);
    dump(file, frame, "t_sections", cp.t_sections);
    dump(file, frame, "outer_ribs", geometry.quads.outer_ribs);
    dump(file, frame, "inner_beams", geometry.quads.inner_beams);
    dump(file, frame, "inner_ribs", geometry.quads.inner_ribs);
    dump(file, frame, "wedges", geometry.quads.wedges);
    dump(file, frame, "t_sections", geometry.quads.t_sections);

    for (size_t i = 0; i < geometry.parabolas.size(); i++)
        for (size_t j = 0; j < 3; j++)
            dump(file, frame, fmt::format("parabolas/{}/{}", i, j), geometry.parabolas[i][j].get_points());

    file << fmt::format("block_level_bottom {:.9f}\nblock_level_top {:.9f}\n", geometry.block_level_bottom, geometry.block_level_top);

    for (size_t i = 0; i < geometry.bed_top_planes.size(); i++)
        dump(file, frame, fmt::format("bed_top_planes/{}", i), geometry.bed_top_planes[i]);
}

/// The R1 record of quarter q: the construction and the members compas_tf gets right on a rectangle, outer ribs, inner beams, blocks 0 / 2, flanges 1 / 4, bed rows 0 / 2 and the cutters, numbered as compas_tf numbers them.
void dump_view(const wood_floor::Floor& floor, size_t q, const std::string& path) {

    const wood_floor::Quarter quarter = floor.quarter(q);
    const wood_floor::ColumnCorner& column = quarter.column();
    const double after = 0.5 * (floor.plan.corners[(q + 1) % 4] - floor.plan.corners[q]).magnitude();
    const double before = 0.5 * (floor.plan.corners[(q + 3) % 4] - floor.plan.corners[q]).magnitude();
    const Frame frame{column.corner, column.x_axis, column.y_axis, Point(-after, -before, 0.0)};
    std::ofstream file(path);
    dump_construction(file, frame, quarter);
    dump(file, frame, "outer_ribs", quarter.outer_ribs());
    dump(file, frame, "inner_beams", quarter.inner_beams());

    const std::vector<wood_floor::Outline> blocks = quarter.wedges_inner_beams();
    const std::vector<wood_floor::Outline> flanges = quarter.tsections();
    const std::vector<std::vector<wood_floor::Outline>> beds = quarter.beds();
    dump(file, frame, "wedges_inner_beams/0", blocks[0]);
    dump(file, frame, "wedges_inner_beams/2", blocks[2]);
    dump(file, frame, "tsections/0", flanges[0]);
    dump(file, frame, "tsections/5", flanges[5]);
    dump_row(file, frame, beds[0], 0, 0);
    dump_row(file, frame, beds[2], 2, beds[0].size() + beds[1].size());
    dump(file, frame, "column_cutters", quarter.column_cutters());
}

// ═══════════════════════════════════════════════════════════════════════════
// Rectangle
// ═══════════════════════════════════════════════════════════════════════════

/// The record of the whole bay: the report, every contact relationship with its area, and the counts.
void dump_bay(const wood_floor::Floor& floor, const std::string& counts, const std::string& path) {

    std::ofstream file(path);
    file << floor.check().str() << "\n";

    for (const wood_floor::Relationship& row : wood_floor::relationships(floor))
        if (row.contact.point_count() > 0 && row.kind != wood_floor::Relation::cutter)
            file << fmt::format("{} area {:.6f}\n", row.text(), row.area());

    file << counts << "\n";
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

/// The rectangular bay with the model's definitions, or with --compas in compas_tf's parity mode with its oculus rule, also writing every quarter view for R1.
int main(int argc, char** argv) {

    const bool compas = argc > 1 && std::string(argv[1]) == "--compas";
    const wood_floor::FloorPlan plan = compas ? wood_floor::FloorPlan::rectangle(HALF_X, HALF_Y, 1000.0, wood_floor::OculusRule::compas) : wood_floor::FloorPlan::rectangle(HALF_X, HALF_Y);
    const wood_floor::Floor model = compas ? wood_floor::Floor::compas_parity(plan, wood_floor::FloorSizes{}) : wood_floor::Floor(plan, wood_floor::FloorSizes{});
    std::cout << model.check().str() << std::endl;

    WoodSession session("templates_floor_9_rectangle");
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
    const std::vector<std::shared_ptr<JointBeam>> ties = wood_floor::add_connectors(session, model, members, wood_floor::add_group(session, "outer_rib_connectors", root), {wood_floor::Relation::seam_tie});
    const std::vector<std::shared_ptr<JointBeam>> dowels = wood_floor::add_connectors(session, model, members, wood_floor::add_group(session, "quarter_connectors", floor), {wood_floor::Relation::block_dowels});
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    size_t pins = 0;
    size_t wedge_dowels = 0;

    for (const std::shared_ptr<JointBeam>& joint : dowels)
        pins += joint->drill_lines.size();

    for (const std::shared_ptr<JointBeam>& wedge : wedges)
        wedge_dowels += wedge->drill_lines.size();

    const std::string counts = fmt::format("{} elements, {} wedges with {} dowels, {} dowel sets of {} dowels, {} rectangle plates with {} cross laps, {} ties", session.objects.elements->size(), wedges.size(), wedge_dowels, dowels.size(), pins, of_kind(column_joints, "connector").size(), of_kind(column_joints, "connector_cross_lap").size(), ties.size());
    std::cout << counts << fmt::format(": contacts and cuts in {:.0f} ms", ms) << std::endl;
    std::cout << fmt::format("{} of 44 contacts verified by the kernel's search", 44 - mismatches.size()) << std::endl;

    const std::array<size_t, 3> breps = count_breps(session);
    std::cout << fmt::format("G8: {} of 48 connectors, {} faceted, {} of {} dowel stretches exact bores", wedges.size() + column_joints.size() + ties.size() + dowels.size(), breps[0], breps[1], breps[2]) << std::endl;
    const std::vector<std::shared_ptr<JointBeam>> screws = add_screws(session, model, members, wood_floor::add_group(session, "screw_connectors", floor));
    session.pb_dump(pb_path("live"));

    if constexpr (DUMP) {
        dump_bay(model, counts, std::filesystem::path(pb_path(compas ? "floor_9_rectangle_compas" : "floor_9_rectangle")).replace_extension(".txt").string());

        dump_screws(model, screws, std::filesystem::path(pb_path(compas ? "floor_9_screws_compas" : "floor_9_screws")).replace_extension(".txt").string());

        for (size_t q = 0; compas && q < 4; q++)
            dump_view(model, q, std::filesystem::path(pb_path(fmt::format("floor_9_rectangle_q{}", q))).replace_extension(".txt").string());
    }

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Step 9 of the timber floor, the parametric model on a rectangle: Floor(FloorPlan::rectangle(3000, 2400), FloorSizes{}), a 6000 x 4800 bay. The four quarters are built in place at their own corners, so the bay is the mirror tiling of compas_tf's quarter: the two halves of every bay edge share one rib band and meet end to end at the seam, the seam beams share their seam planes, the ring is built from the four quarters' own oculus planes, and every column stands at its corner. The oculus is a square diamond of half-diagonal 1000 on the seams; the central panel of every quarter follows rule A, its inner ribs swept along one direction solved so the central bed is one planar-faced cylinder between them, with every layer exactly 27 thick; both outer ribs of every corner end on their fan planes at one level, the shallower of their compas_tf ends, the short ribs' straight run-in solved for it (187.667 instead of 240), and that level is the middle cutter level, so all eight rib faces meet the column head within 0.307 mm and rule A sweeps the inner ribs 0.474 deg off the chamfer; each side block between the ribs at the column spans its rib's run-in and the middle one 1.25 times their mean (240 / 267.292 / 187.667), so the blocks end level within 1 mm. The connectors come from the floor's 76 relationships as in example 8: 8 wedges, 8 rectangle plates with 4 cross laps, 4 ties and 24 dowel sets, every contact checked against the kernel's search. The report prints what compas_tf relied on silently. With --compas the bay uses compas_tf's oculus rule and parity definitions and also writes every quarter in its corner frame, compared against compas_tf's FloorGuide with that quarter's half spans (data/reference/floor/reference_floorguide_3000x2400.txt and _2400x3000.txt). BREPS writes every cut element and connector as its BRep. Then the assembly screws, after every other connector so nothing before them changes: pre-drilled lines 200 long, d 4, two per location at two heights, 12 per quarter (outer ribs into the seam beams, the seam beams into the oculus beam across the mitres, the inner rib ends through the beam corners), 8 at the ring's pinwheel corners and 16 toe screws from the ring into the quarters' oculus beams, 72 in all; each line stored once on its connector, which names every member it passes, no member cut, every screw a dowel child drawn as an exact cylinder; prints the screw count per kind, their contacts against the kernel's search and their closest approach to each other, to the dowel bores and to the pockets, and DUMP writes them with their heads and tips to floor_9_screws.txt. Every connector node and every part and dowel nested under it carries the connector colour, red (wood_floor::CONNECTOR_COLOR on the tree node, which the pb keeps).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_9_rectangle --parallel 6 && ./build/templates_floor_9_rectangle && ../bash/publish-scene.sh --target templates_floor_9_rectangle

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
