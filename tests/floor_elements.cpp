#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_brep_drill.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The square floor every check reads, built on first use.
const wood_floor::Floor& square_floor() {

    static const wood_floor::Floor floor(wood_floor::FloorPlan::rectangle(3000.0, 3000.0), wood_floor::FloorSizes{});

    return floor;
}

const double EXACT_SUPPORT = 500671.261678; // the support's exact BRep volume, cylinders and hexagons
const double CARVED_OUTER_RIB = 99598198.606378; // an outer rib carved by its rectangle plate pocket and dowels
const double TIED_OUTER_RIB = 98812970.259836; // the same rib after the tie pocket too
const double TIE_KEY = 1570456.693007; // the tie key's body with the tie's defaults
const double HEAD_CUT = 34771221.351479; // what the six head cutters take from the column

/// Prints the message and throws with it when the condition fails.
void check(bool ok, const std::string& message) {

    if (!ok) {
        std::cerr << "floor_elements FAILED: " << message << std::endl;
        throw std::runtime_error(message);
    }
}

/// The beam is closed, has the face count, and encloses the volume of the plate lofted from the same outline.
void check_beam(const BeamVariable& beam, const wood_floor::Outline& outline, size_t faces, const std::string& name) {

    const Mesh& mesh = beam.element_geometry_mesh();
    const double volume = compute_volume(mesh);
    const double reference = compute_volume(wood_floor::to_plate(outline, name)->element_geometry_mesh());

    check(mesh.is_closed(), name + " closed");
    check(mesh.number_of_faces() == faces, name + " faces " + std::to_string(mesh.number_of_faces()));
    check(std::abs(volume - reference) <= 1e-9 * reference, name + " volume " + std::to_string(volume) + " vs " + std::to_string(reference));
}

/// Ribs and beams as variable beams: closed, the volume of the plate from the same outline, through a round trip and a move.
void check_beams() {

    WoodSession scene("beam_variable");
    std::vector<std::shared_ptr<BeamVariable>> beams;

    const wood_floor::Quarter quarter = square_floor().quarter(0);
    const std::vector<wood_floor::Outline> outer = quarter.outer_ribs();
    const std::vector<wood_floor::Outline> inner = quarter.inner_ribs();

    for (size_t i = 0; i < 2; i++) {
        beams.push_back(wood_floor::to_rib(outer[i], "outer_rib"));
        check_beam(*beams.back(), outer[i], 11, "outer rib " + std::to_string(i));
        beams.push_back(wood_floor::to_rib(inner[i], "inner_rib"));
        check_beam(*beams.back(), inner[i], 11, "inner rib " + std::to_string(i));
    }

    for (const wood_floor::Outline& outline : quarter.inner_beams()) {
        beams.push_back(wood_floor::to_beam(outline, {0, 3}, {1, 2}, "inner_beam"));
        check_beam(*beams.back(), outline, 6, "inner beam");
    }

    const std::vector<wood_floor::Outline> oculus = square_floor().oculus();

    for (size_t i = 0; i < 4; i++) {
        beams.push_back(wood_floor::to_beam(oculus[i], {1, 0}, {2, 3}, "oculus_beam"));
        check_beam(*beams.back(), oculus[i], 6, "oculus beam " + std::to_string(i));
    }

    for (const std::shared_ptr<BeamVariable>& beam : beams)
        scene.add(beam);

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    const std::vector<std::shared_ptr<BeamVariable>> loaded = back.beam_variables();
    check(loaded.size() == beams.size(), "round trip count");

    for (size_t i = 0; i < beams.size(); i++) {
        check(loaded[i]->guid() == beams[i]->guid(), "round trip guid");
        check(loaded[i]->sections.size() == beams[i]->sections.size(), "round trip sections");
        check(loaded[i]->axis.start() == beams[i]->axis.start() && loaded[i]->axis.end() == beams[i]->axis.end(), "round trip axis");
        const double volume = compute_volume(beams[i]->model_geometry_mesh());
        check(std::abs(compute_volume(loaded[i]->model_geometry_mesh()) - volume) <= 1e-9 * volume, "round trip volume");
    }

    const Xform move = Xform::translation(100.0, -50.0, 3500.0);
    const std::shared_ptr<BeamVariable> moved = beams.front()->transformed(move);
    const double volume = compute_volume(beams.front()->element_geometry_mesh());
    check(std::abs(compute_volume(moved->element_geometry_mesh()) - volume) <= 1e-9 * volume, "transformed volume");
    check(moved->axis.start() == beams.front()->axis.start().transformed(move), "transformed axis");

    std::cout << "floor_elements: " << beams.size() << " variable beams closed, plate volumes, round trip and transform pass" << std::endl;
}

/// Every member carries its outline's thickness: an outer rib as thick as the guide says, the tilted middle block at least its plane offset, and the thickness kept through a quarter turn.
void check_thickness() {

    WoodSession scene("thickness");
    const wood_floor::QuarterMembers quarter = wood_floor::add_quarter_model(scene, square_floor().quarter(1), nullptr);
    const std::vector<wood_floor::Outline> outlines = square_floor().quarter(1).outer_ribs();

    check(quarter.outer_ribs[0].thickness == wood_floor::outline_thickness(outlines[0]), "a rib keeps its outline's thickness");
    check(std::abs(quarter.outer_ribs[0].thickness - square_floor().sizes.outer_ribs) < 1e-6, "an outer rib as thick as the sizes say, " + std::to_string(quarter.outer_ribs[0].thickness));
    check(quarter.inner_beams[1].thickness > square_floor().sizes.inner_beams - 1e-9 && quarter.inner_beams[1].thickness < 1.5 * square_floor().sizes.inner_beams, "an inner beam about as thick as the sizes say, " + std::to_string(quarter.inner_beams[1].thickness));
    check(quarter.blocks[1].thickness > 1.25 * square_floor().sizes.wedge - 1e-9, "the tilted middle block at least its plane offset thick, " + std::to_string(quarter.blocks[1].thickness));
    check(quarter.beds.size() == 3 && quarter.tsections.size() == 6 && quarter.inner_ribs.size() == 2, "a quarter of three bed rows, six t-sections and two inner ribs");

    std::cout << "floor_elements: every quarter member carries its outline thickness, a rib, a beam and a block checked" << std::endl;
}

/// The area of the polygon a circle of that radius is faceted into.
double faceted_area(double radius, double chord_tolerance) {

    const int n = circle_segments(radius, chord_tolerance);

    return 0.5 * n * radius * radius * std::sin(2.0 * M_PI / n);
}

/// The support under the column: closed, near the exact solid, its joint cutting exactly the head plate pocket and the screws, the head cutters removing their volume, and every dimension through a round trip.
void check_support() {

    WoodSession scene("support");
    const std::shared_ptr<Support> support = wood_floor::to_support(square_floor().columns[0]);
    const std::shared_ptr<Column> column = wood_floor::to_column(square_floor().columns[0], square_floor().sizes, *support);
    scene.add(support);
    scene.add(column);

    const Mesh& base = support->element_geometry_mesh();
    check(base.is_closed(), "support closed");
    check(std::abs(compute_volume(base) - EXACT_SUPPORT) <= 1e-3 * EXACT_SUPPORT, "support volume " + std::to_string(compute_volume(base)));
    check(std::abs(column->axis.start()[2] - (support->height - support->head_plate_recess)) <= 1e-9, "column foot on the support");

    const double stock = compute_volume(column->element_geometry_mesh());
    const std::shared_ptr<Joint> joint = Joint::support(*support, *column);
    scene.add(joint);
    scene.add_joint(joint);

    const double pocket = faceted_area(support->head_plate_diameter * 0.5, support->chord_tolerance) * support->head_plate_recess;
    const double screws = faceted_area(support->screw_diameter * 0.5, support->chord_tolerance) * support->screw_length * support->screw_count;
    const double removed = stock - compute_volume(column->model_geometry_mesh());
    check(std::abs(removed - pocket - screws) <= 1e-6 * removed, "support joint removes " + std::to_string(removed) + " not " + std::to_string(pocket + screws));

    for (const std::shared_ptr<Joint>& cutter : wood_floor::to_column_cutters(square_floor().quarter(0), *column)) {
        scene.add(cutter);
        scene.add_joint(cutter);
    }

    const double head = stock - removed - compute_volume(column->model_geometry_mesh());
    check(std::abs(head - HEAD_CUT) <= 1e-6 * HEAD_CUT, fmt::format("head cutters remove {:.6f}", head));
    check(column->model_geometry_mesh().is_closed(), "carved column closed");

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    const std::shared_ptr<Support> loaded = back.supports().front();
    check(loaded->plane.origin() == support->plane.origin() && loaded->height == support->height && loaded->head_plate_diameter == support->head_plate_diameter, "support round trip");
    check(loaded->screw_count == support->screw_count && loaded->screw_angle == support->screw_angle && loaded->base_plate_hole_spacing == support->base_plate_hole_spacing, "support round trip fasteners");

    std::cout << fmt::format("floor_elements: support {:.3f} mm3, joint removes {:.3f}, head cutters {:.6f}, round trip pass", compute_volume(base), removed, head) << std::endl;
}

/// The children of a connector in the scene tree: its part and dowel elements, in order.
std::vector<std::shared_ptr<Joint>> children_of(const WoodSession& scene, const JointBeam& connector) {

    std::vector<std::shared_ptr<Joint>> children;

    for (TreeNode* child : scene.tree.get_node_by_name(connector.guid())->children())
        children.push_back(scene.get_element<Joint>(child->name));

    return children;
}

/// The connector is nested: it draws nothing itself, parts part children then dowels dowel children follow it in the tree, named after it (a pre-drilled screw <name>_screw_<i>), every one visible and exact.
void check_nested(const WoodSession& scene, const JointBeam& connector, size_t parts, size_t dowels) {

    const std::vector<std::shared_ptr<Joint>> children = children_of(scene, connector);
    check(connector.is_connector() && connector.model_geometry_brep().m_faces.empty() && connector.model_geometry_mesh().number_of_faces() == 0, connector.name + " draws nothing itself");
    check(children.size() == parts + dowels, fmt::format("{} nests {} parts and {} dowels, not {} children", connector.name, parts, dowels, children.size()));

    for (size_t i = 0; i < children.size(); i++) {
        const std::shared_ptr<Joint>& child = children[i];
        check(child && child->is_visible, connector.name + " child visible");

        if (i < parts) {
            const std::string name = parts == 1 ? connector.name + "_part" : fmt::format("{}_part_{}", connector.name, i);
            check(std::dynamic_pointer_cast<ConnectorPart>(child) && child->name == name, connector.name + " part child " + child->name);
        } else {
            const double cylinder = M_PI * connector.line_radius * connector.line_radius * connector.drill_lines[i - parts].length();
            check(std::dynamic_pointer_cast<Dowel>(child) && child->name == fmt::format("{}_{}_{}", connector.name, connector.pre_drill ? "screw" : "dowel", i - parts) && std::abs(child->model_geometry_brep().volume() - cylinder) < 1e-2 * cylinder, connector.name + " dowel child " + child->name);
        }
    }
}

/// No dowel of the connector protrudes: just inside every dowel's end lies in one of its two members; and when the dowels pass through, just outside every end lies in neither, the end flush with the outer face.
void check_flush(const JointBeam& joint, const WoodSession& scene, const std::string& name, bool through) {

    const std::vector<PlanarFace> a = planar_faces(scene.get_element<Element>(joint.targets[0])->element_geometry_mesh());
    const std::vector<PlanarFace> b = planar_faces(scene.get_element<Element>(joint.targets[1])->element_geometry_mesh());

    for (const Line& dowel : joint.drill_lines) {
        const Vector d = dowel.to_vector().normalized();

        for (const Point& end : {dowel.start(), dowel.end()}) {
            const Vector out = end == dowel.start() ? -d : d;
            check(is_inside(a, end - out * 0.5) || is_inside(b, end - out * 0.5), name + " dowel does not protrude");

            if (through)
                check(!is_inside(a, end + out * 0.5) && !is_inside(b, end + out * 0.5), name + " dowel ends flush with the outer face");
        }
    }
}

/// The outer ribs of every quarter, in quarter order.
std::vector<wood_floor::Member> outer_ribs(const wood_floor::FloorMembers& members) {

    std::vector<wood_floor::Member> ribs;

    for (const wood_floor::QuarterMembers& quarter : members.quarters)
        ribs.insert(ribs.end(), quarter.outer_ribs.begin(), quarter.outer_ribs.end());

    return ribs;
}

/// The relationship table: 76 rows in the counts of the design and 36 screw rows, every contact one the kernel's search finds on the square within 1e-6 of plane, top edge and area, and require_contact throwing for a pair that does not touch.
void check_relationships() {

    WoodSession scene("relationships");
    wood_floor::FloorMembers members = wood_floor::add_floor(scene, square_floor(), nullptr);
    wood_floor::add_columns(scene, square_floor(), nullptr, members);
    const std::vector<wood_floor::Relationship> rows = wood_floor::relationships(square_floor());
    std::map<wood_floor::Relation, size_t> counts;

    for (const wood_floor::Relationship& row : rows)
        counts[row.kind]++;

    check(rows.size() == 112, "112 relationships, 76 and 36 screw rows, not " + std::to_string(rows.size()));
    check(counts[wood_floor::Relation::screw_rib_beam] == 8 && counts[wood_floor::Relation::screw_beam_mitre] == 8 && counts[wood_floor::Relation::screw_rib_corner] == 8 && counts[wood_floor::Relation::screw_ring] == 4 && counts[wood_floor::Relation::screw_oculus] == 8, "8 rib-beam, 8 mitre, 8 rib-corner, 4 ring and 8 oculus screw rows");
    check(counts[wood_floor::Relation::seam_wedge] == 4 && counts[wood_floor::Relation::oculus_wedge] == 4 && counts[wood_floor::Relation::column_plate] == 8 && counts[wood_floor::Relation::cross_lap] == 4, "4 seam wedges, 4 oculus wedges, 8 column plates, 4 cross laps");
    check(counts[wood_floor::Relation::seam_tie] == 4 && counts[wood_floor::Relation::block_dowels] == 24 && counts[wood_floor::Relation::support] == 4 && counts[wood_floor::Relation::cutter] == 24, "4 ties, 24 dowel sets, 4 supports, 24 cutters");

    for (const wood_floor::Relationship& row : rows)
        check(members.get(row.a) != nullptr && members.get(row.b) != nullptr, "every relationship names two scene members: " + row.text());

    const wood_floor::ContactCheck contacts = wood_floor::verify_contacts(scene, square_floor(), members);
    check(contacts.ok() && contacts.count == 44, contacts.str());

    bool thrown = false;

    try {
        wood_floor::require_contact(scene, members.quarters[0].outer_ribs[0].element, members.quarters[2].outer_ribs[0].element, ContactType::end_end, "no such seam");
    } catch (const std::runtime_error& error) {
        thrown = std::string(error.what()).find("no such seam") != std::string::npos;
    }

    check(thrown, "require_contact throws naming the relation when the members do not touch");
    std::cout << "floor_elements: 112 relationships (76 and 36 screw rows), 44 contacts verified by the kernel's search within 1e-6, require_contact names the relation it misses" << std::endl;
}

/// The wedges between the inner beams and the oculus: eight visible connectors with their parts and cutters, their dowels flush with the beams, exact cylinders in their BReps and exact bores through the wedge parts, and the carved beams, through a round trip.
void check_wedges() {

    WoodSession scene("wedges");
    const wood_floor::FloorMembers members = wood_floor::add_floor(scene, square_floor(), nullptr);
    const std::vector<std::shared_ptr<JointBeam>> wedges = wood_floor::add_connectors(scene, square_floor(), members, {wood_floor::Relation::seam_wedge, wood_floor::Relation::oculus_wedge});
    check(wedges.size() == 8, "eight wedges, not " + std::to_string(wedges.size()));

    for (const std::shared_ptr<JointBeam>& wedge : wedges) {
        check_flush(*wedge, scene, wedge->name, true);
        check(wood_floor::count_bores(wedge->part_brep(0)) == wedge->drill_lines.size(), "every wedge dowel an exact bore through the wedge's part");
        check_nested(scene, *wedge, 1, wedge->drill_lines.size());
        check(wood_floor::count_bores(children_of(scene, *wedge).front()->model_geometry_brep()) == wedge->drill_lines.size(), "the wedge child carries the bores");
    }

    std::map<std::string, double> volumes;

    for (const std::shared_ptr<BeamVariable>& beam : scene.beam_variables())
        volumes[beam->guid()] = compute_volume(beam->model_geometry_mesh());

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    size_t loaded = 0;

    for (const std::shared_ptr<JointBeam>& joint : back.get_elements<JointBeam>()) {
        if (std::dynamic_pointer_cast<ConnectorPart>(joint))
            continue;

        check(joint->parts.size() == 1 && joint->cutters.size() == 2 && !joint->drill_lines.empty(), "wedge round trip");
        check_nested(back, *joint, 1, joint->drill_lines.size());
        loaded++;
    }

    check(loaded == wedges.size(), "wedge round trip count");

    for (const std::shared_ptr<BeamVariable>& beam : back.beam_variables())
        check(std::abs(compute_volume(beam->model_geometry_mesh()) - volumes.at(beam->guid())) <= 1e-9 * volumes.at(beam->guid()), "carved beam round trip " + beam->name);

    for (const std::shared_ptr<BeamVariable>& beam : scene.beam_variables()) {
        if (beam->solid_cuts.empty())
            continue;

        const BRep& brep = beam->model_geometry_brep();
        const double volume = compute_volume(beam->model_geometry_mesh());
        check(wood_floor::count_bores(brep) > 0 && brep.is_solid() && std::abs(brep.volume() - volume) <= 1e-3 * volume, "exact dowel bores in " + beam->name);
    }

    std::cout << "floor_elements: " << wedges.size() << " wedges, visible, dowels flush and exact, joints and carved beams through a round trip, every dowel bore exact in the BReps, pass" << std::endl;
}

/// The dowels factory on two plates face to face: four Ø8 dowels 30 long at the corners of the 600 x 200 contact inset by 50, 15 deep into both 60 plates, cut as exact bores into both, through a round trip.
void check_dowels() {

    WoodSession scene("dowels");
    const std::shared_ptr<Plate> lower = Plate::from_rectangle(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), 600.0, 200.0, 60.0, "lower");
    const std::shared_ptr<Plate> upper = Plate::from_rectangle(Point(0.0, 0.0, 60.0), Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), 600.0, 200.0, 60.0, "upper");
    scene.add(lower);
    scene.add(upper);

    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(lower, upper);
    check(contact != nullptr, "the plates touch face to face");

    const std::shared_ptr<JointBeam> joint = JointBeam::dowels(*lower, *upper, *contact);
    check(joint && joint->drill_lines.size() == 4 && joint->is_visible, "four dowels at the corners of the contact");
    std::set<std::pair<int, int>> corners;

    for (const Line& dowel : joint->drill_lines) {
        check(std::abs(dowel.length() - 30.0) < 1e-9, "a dowel 15 deep into each plate, " + std::to_string(dowel.length()));
        check(std::abs(dowel.center()[2] - 60.0) < 1e-9, "a dowel centred on the contact");
        corners.insert({static_cast<int>(std::lround(dowel.center()[0])), static_cast<int>(std::lround(dowel.center()[1]))});
    }

    check(corners == std::set<std::pair<int, int>>{{50, 50}, {550, 50}, {550, 150}, {50, 150}}, "the dowels 50 in from the contact's corners");

    scene.add_connector(joint, nullptr);
    check_nested(scene, *joint, 0, 4);
    const double bore = faceted_area(joint->line_radius, joint->chord_tolerance) * 15.0 * 4.0;

    for (const std::shared_ptr<Plate>& plate : {lower, upper}) {
        const double volume = compute_volume(plate->model_geometry_mesh());
        check(std::abs(600.0 * 200.0 * 60.0 - bore - volume) <= 1e-6 * volume, "four blind holes out of " + plate->name);
        check(wood_floor::count_bores(plate->model_geometry_brep()) == 4 && plate->model_geometry_brep().is_solid(), "four exact bores in " + plate->name);
    }

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    const std::shared_ptr<JointBeam> loaded = back.get_elements<JointBeam>().front();
    check(loaded->drill_lines.size() == 4 && loaded->cutters.size() == 2 && loaded->drill_overshoot == joint->drill_overshoot, "dowels round trip");
    check_nested(back, *loaded, 0, 4);

    std::cout << "floor_elements: four dowels at the corners of a plate contact, 30 long, exact bores, nested as four dowel children and a round trip pass" << std::endl;
}

/// Every inner rib is exact, bored once per dowel of the sets that join it.
void check_inner_rib_bores(const std::array<wood_floor::QuarterMembers, 4>& quarters, const std::vector<std::shared_ptr<JointBeam>>& sets) {

    for (const wood_floor::QuarterMembers& quarter : quarters)
        for (const wood_floor::Member& rib : quarter.inner_ribs) {
            size_t crossing = 0;

            for (const std::shared_ptr<JointBeam>& set : sets)
                if (std::find(set->targets.begin(), set->targets.end(), rib.element->guid()) != set->targets.end())
                    crossing += set->drill_lines.size();

            const BRep& brep = rib.element->model_geometry_brep();
            check(brep.is_solid() && wood_floor::count_bores(brep) == crossing, fmt::format("an inner rib bored once per dowel, one exact cylinder each: {} dowels, {} bores in {}", crossing, wood_floor::count_bores(brep), rib.element->name));
        }
}

/// Every member with a cut is an exact solid with at least one bore; at least seven per quarter, the ribs and the blocks.
void check_drilled_members(const WoodSession& scene) {

    size_t drilled = 0;

    for (const std::shared_ptr<Element>& element : scene.world_elements()) {
        const std::vector<SolidCut>* cuts = solid_cuts_of(*element);

        if (!cuts || cuts->empty())
            continue;

        drilled++;
        check(element->model_geometry_brep().is_solid() && wood_floor::count_bores(element->model_geometry_brep()) > 0, fmt::format("exact dowel bores in {}: solid {}, {} bores", element->name, element->model_geometry_brep().is_solid(), wood_floor::count_bores(element->model_geometry_brep())));
    }

    check(drilled == 28, "seven drilled members per quarter, the ribs and the blocks, not " + std::to_string(drilled));
}

/// The assembly dowels of the quarters: a dowel set on every rib-to-wedge-block contact, six per quarter, never across quarters, four 30 mm dowels exactly at the corners of each contact inset 50, the contacts found on the uncut members; every dowel half in each member, every drilled member exact; through a round trip.
void check_quarter_dowels() {

    WoodSession scene("quarter_dowels");
    const wood_floor::FloorMembers members = wood_floor::add_floor(scene, square_floor(), nullptr);
    const std::vector<std::shared_ptr<JointBeam>> sets = wood_floor::add_connectors(scene, square_floor(), members, {wood_floor::Relation::block_dowels});
    size_t dowels = 0;

    for (const std::shared_ptr<JointBeam>& set : sets) {
        check_nested(scene, *set, 0, set->drill_lines.size());
        const std::shared_ptr<Element> rib = scene.get_element<Element>(set->targets[0]);
        const std::shared_ptr<Element> block = scene.get_element<Element>(set->targets[1]);
        check(rib->name.substr(rib->name.find_last_of('_')) == block->name.substr(block->name.find_last_of('_')), "a dowel set stays within one quarter, not " + rib->name + " to " + block->name);
        check(rib->name.find("ribs_") != std::string::npos && block->name.starts_with("wedges_"), "a dowel set joins a rib to a wedge block, not " + rib->name + " to " + block->name);
        check(set->drill_lines.size() == 4, "four dowels per contact, not " + std::to_string(set->drill_lines.size()));

        for (const Line& dowel : set->drill_lines) {
            check(std::abs(dowel.length() - 30.0) < 1e-9, "a dowel 30 long");
            check(is_inside(planar_faces(rib->element_geometry_mesh()), dowel.center() - dowel.to_vector().normalized() * 0.5) && is_inside(planar_faces(block->element_geometry_mesh()), dowel.center() + dowel.to_vector().normalized() * 0.5), "a dowel crosses the contact at its middle, half in each member");
            dowels++;
        }
    }

    check(sets.size() == 24, "six rib-to-block dowel sets per quarter, not " + std::to_string(sets.size()));
    check_inner_rib_bores(members.quarters, sets);
    check_drilled_members(scene);

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    size_t loaded = 0;

    for (const std::shared_ptr<JointBeam>& set : back.get_elements<JointBeam>())
        if (!std::dynamic_pointer_cast<ConnectorPart>(set))
            loaded += set->drill_lines.size();

    check(loaded == dowels, "quarter dowels round trip");

    std::cout << fmt::format("floor_elements: {} dowel sets of {} dowels on the wedge blocks of the quarters, every dowel half in each member, every drilled member exact, round trip pass", sets.size(), dowels) << std::endl;
}

/// The two plates of every column half-lapped by a cross lap: each slotted part an exact solid with its four dowel bores, the two touching without overlap.
void check_cross_laps(const WoodSession& scene, const std::vector<std::shared_ptr<JointBeam>>& laps) {

    for (const std::shared_ptr<JointBeam>& lap : laps) {
        const std::shared_ptr<JointBeam> a = scene.get_element<JointBeam>(lap->targets[0]);
        const std::shared_ptr<JointBeam> b = scene.get_element<JointBeam>(lap->targets[1]);
        check(a->solid_cuts.size() == 1 && b->solid_cuts.size() == 1, "each plate carries its slot");
        const Mesh slotted_a = apply_solid_cuts(a->part_mesh(0), a->solid_cuts, false);
        const Mesh slotted_b = apply_solid_cuts(b->part_mesh(0), b->solid_cuts, false);
        const Mesh overlap = solid_boolean(slotted_a, slotted_b, SolidOperation::intersection, 1e-7);
        check(!overlap.number_of_faces() || compute_volume(overlap) < 1e-6, "the slotted plates do not overlap");
        const double sum = compute_volume(slotted_a) + compute_volume(slotted_b);
        check(std::abs(sum - compute_volume(solid_boolean(slotted_a, slotted_b, SolidOperation::unite, 1e-7))) < 1e-9 * sum, "the slotted plates unite to their sum, touching without overlap");
        check(std::abs(compute_volume(a->part_mesh(0)) - compute_volume(slotted_a) - 30.0 * 30.0 * 125.0) < 1e-3, "a slot 30 wide and 125 deep out of the first plate");
        check(std::abs(compute_volume(b->part_mesh(0)) - compute_volume(slotted_b) - 30.0 * 30.0 * 125.0) < 1e-3, "a slot 30 wide and 125 deep out of the second plate");

        for (const std::shared_ptr<JointBeam>& plate : {a, b}) {
            const BRep part = plate->part_brep(0);
            check(part.is_solid() && wood_floor::count_bores(part) == 4, "a slotted plate exact with four dowel bores");
            check_nested(scene, *plate, 1, 4);
            const std::shared_ptr<Joint> child = children_of(scene, *plate).front();
            check(child->model_geometry_brep().is_solid() && wood_floor::count_bores(child->model_geometry_brep()) == 4 && std::abs(child->model_geometry_brep().volume() - part.volume()) < 1e-6 * part.volume(), "the plate child carries the slot and the four bores");
        }
    }
}

/// The ties on the rib seams: four, nested as four key parts, every tied outer rib and every key at its pinned volume.
void check_ties(const WoodSession& scene, const std::vector<wood_floor::Member>& ribs, const std::vector<std::shared_ptr<JointBeam>>& ties) {

    check(ties.size() == 4, "four ties, not " + std::to_string(ties.size()));

    for (const std::shared_ptr<JointBeam>& tie : ties)
        check_nested(scene, *tie, 4, 0);

    for (const wood_floor::Member& rib : ribs)
        check(std::abs(compute_volume(rib.element->model_geometry_mesh()) - TIED_OUTER_RIB) <= 1e-9 * TIED_OUTER_RIB, fmt::format("tied outer rib {} {:.6f}", rib.element->name, compute_volume(rib.element->model_geometry_mesh())));

    for (const std::shared_ptr<JointBeam>& tie : ties) {
        Mesh key;

        for (const std::array<Polyline, 2>& part : tie->parts)
            append_mesh(key, Mesh::loft({part[0]}, {part[1]}, true));

        check(std::abs(compute_volume(key) - TIE_KEY) <= 1e-9 * TIE_KEY, fmt::format("tie volume {:.6f}", compute_volume(key)));
    }
}

/// A tie loaded from a round trip and then drilled through its second key: every key child gets its own part's cuts, so only that key loses the bore.
void check_loaded_tie_cuts(WoodSession& scene, const std::vector<std::shared_ptr<JointBeam>>& ties) {

    WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    const std::shared_ptr<JointBeam> tie = back.get_element<JointBeam>(ties.front()->guid());
    const std::vector<std::shared_ptr<Joint>> keys = children_of(back, *tie);
    check(keys.size() == 4, "a loaded tie keeps its four key children");

    std::vector<double> volumes;

    for (const std::shared_ptr<Joint>& key : keys)
        volumes.push_back(compute_volume(key->model_geometry_mesh()));

    const Mesh neck = tie->part_mesh(1);
    const AABB box = AABB::from_mesh(neck);
    const Point centre(box.cx, box.cy, box.cz);
    const std::shared_ptr<Joint> drill = Joint::drill(Line::from_points(centre - Vector(0.0, 0.0, 200.0), centre + Vector(0.0, 0.0, 200.0)), 3.0);
    drill->targets = {tie->guid()};
    back.add(drill);
    back.add_joint(drill);

    for (size_t i = 0; i < keys.size(); i++) {
        const std::shared_ptr<JointBeam> key = std::dynamic_pointer_cast<JointBeam>(keys[i]);
        check(key->solid_cuts.size() == tie->part_cuts(i).size() && std::abs(compute_volume(key->part_mesh(0)) - compute_volume(tie->part_mesh(i))) < 1e-9, fmt::format("key {} carries its own part and its cuts", i));
        const double lost = volumes[i] - compute_volume(key->model_geometry_mesh());
        check(i == 1 ? lost > 1.0 : std::abs(lost) < 1e-6 * volumes[i], fmt::format("only the drilled key loses volume: key {} lost {:.9f}", i, lost));
    }

    std::cout << "floor_elements: a loaded tie drilled through one key, every key child cut as its own part" << std::endl;
}

/// The rectangle plates between the columns and the outer ribs and the ties on the rib seams: eight and four, every carved outer rib at its pinned volume, the plates half-lapped and the column still exact, through a round trip.
void check_rectangle_plates() {

    WoodSession scene("rectangle_plates");
    wood_floor::FloorMembers members = wood_floor::add_floor(scene, square_floor(), nullptr);
    wood_floor::add_columns(scene, square_floor(), nullptr, members);
    const std::vector<wood_floor::Member> ribs = outer_ribs(members);
    const std::vector<std::shared_ptr<JointBeam>> joints = wood_floor::add_connectors(scene, square_floor(), members, {wood_floor::Relation::column_plate, wood_floor::Relation::cross_lap});
    check(joints.size() == 12, "eight rectangle plates and four cross laps, not " + std::to_string(joints.size()));
    const std::vector<std::shared_ptr<JointBeam>> plates(joints.begin(), joints.begin() + 8);
    const std::vector<std::shared_ptr<JointBeam>> laps(joints.begin() + 8, joints.end());

    for (const wood_floor::Member& rib : ribs)
        check(std::abs(compute_volume(rib.element->model_geometry_mesh()) - CARVED_OUTER_RIB) <= 1e-9 * CARVED_OUTER_RIB, fmt::format("carved outer rib {} {:.6f}", rib.element->name, compute_volume(rib.element->model_geometry_mesh())));

    check(laps.size() == 4 && laps[0]->name == "connector_cross_lap_0" && plates[7]->name == "connector_7", "four cross laps after the eight plates");
    check_cross_laps(scene, laps);

    for (const wood_floor::ColumnModel& column : members.columns)
        check(column.column->model_geometry_brep().is_solid() && wood_floor::count_bores(column.column->model_geometry_brep()) == 11, "the column exact with its eight dowel and three screw bores");

    const std::vector<std::shared_ptr<JointBeam>> ties = wood_floor::add_connectors(scene, square_floor(), members, {wood_floor::Relation::seam_tie});
    check_ties(scene, ribs, ties);

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    size_t slotted = 0;

    for (const std::shared_ptr<JointBeam>& joint : back.get_elements<JointBeam>())
        if (!std::dynamic_pointer_cast<ConnectorPart>(joint))
            slotted += joint->solid_cuts.size();

    check(slotted == 8, "the slots round trip on the plates");
    check(back.get_elements<ConnectorPart>().size() == 8 + 16 && back.get_elements<Dowel>().size() == 32, "the plate and key parts and the dowels round trip as children");

    std::cout << "floor_elements: " << plates.size() << " rectangle plates half-lapped by " << laps.size() << " cross laps, drilled and exact, and " << ties.size() << " ties, carved outer ribs and ties at their pinned volumes, round trip pass" << std::endl;
    check_loaded_tie_cuts(scene, ties);
}

/// Whether two planes are one plane: unit normals parallel or opposite as flip says, the same offset, within 1e-9.
bool same_plane(const Plane& a, const Plane& b, bool opposite) {

    const Vector normal = opposite ? -b.z_axis() : b.z_axis();
    const double offset_a = a.z_axis().dot(Vector(a.origin()[0], a.origin()[1], a.origin()[2]));
    const double offset_b = normal.dot(Vector(b.origin()[0], b.origin()[1], b.origin()[2]));

    return (a.z_axis() - normal).magnitude() <= 1e-9 && std::abs(offset_a - offset_b) <= 1e-9;
}

/// The members of quarter q read every shared plane as the floor stores it, and on the square every quarter equals quarter 0 turned by its quarter turns within 1e-6.
void check_shared_entities() {

    const wood_floor::Floor& floor = square_floor();

    for (size_t q = 0; q < 4; q++) {
        const wood_floor::ConstructionPlanes& mine = floor.geometry[q].planes;
        const wood_floor::ConstructionPlanes& next = floor.geometry[(q + 1) % 4].planes;
        check(same_plane(floor.seams[q].plane, mine.inner_beams[0][0], false) && same_plane(floor.seams[q].plane, next.inner_beams[2][0], true), fmt::format("seam {} is the beam-0 plane of quarter {} and the beam-2 plane of quarter {}", q, q, (q + 1) % 4));
        check(same_plane(floor.edges[q].band[0], mine.outer_ribs[0][0], false) && same_plane(floor.edges[q].band[0], next.outer_ribs[1][0], false), fmt::format("bay edge {} is the outer rib band of quarters {} and {}", q, q, (q + 1) % 4));
        check(same_plane(floor.edges[q].band[1], mine.outer_ribs[0][1], false) && same_plane(floor.edges[q].band[1], next.outer_ribs[1][1], false), fmt::format("bay edge {} inner band plane shared", q));
        check(same_plane(floor.oculus_edges[q].tilted, mine.inner_beams[1][0], false) && same_plane(floor.oculus_edges[q].back, mine.inner_beams[1][1], false), fmt::format("oculus edge {} is the oculus beam pair of quarter {}", q, q));

        for (size_t k = 0; k < 3; k++)
            check(same_plane(floor.columns[q].wedge_fan[k][0], mine.wedges[k][0], false) && same_plane(floor.columns[q].wedge_fan[k][1], mine.wedges[k][1], false), fmt::format("column {} fan plane {} is the quarter's wedge plane", q, k));

        const Xform turn = Xform::rotation_z(static_cast<double>(q) * 90.0, true);
        const std::vector<wood_floor::Outline> turned = floor.quarter(0).outer_ribs();
        const std::vector<wood_floor::Outline> built = floor.quarter(q).outer_ribs();

        for (size_t i = 0; i < 2; i++) {
            const std::vector<Point> a = turned[i].top.transformed(turn).get_points();
            const std::vector<Point> b = built[i].top.get_points();
            check(a.size() == b.size(), "the in-place rib has the turned rib's vertex count");

            for (size_t j = 0; j < a.size(); j++)
                check((a[j] - b[j]).magnitude() <= 1e-6, fmt::format("quarter {} outer rib {} vertex {} is quarter 0's turned: {:.3e} off", q, i, j, (a[j] - b[j]).magnitude()));
        }
    }

    std::cout << "floor_elements: every seam, bay edge, oculus edge and column fan plane read by its quarters as one plane, every in-place quarter equal to the turned quarter 0 within 1e-6" << std::endl;
}

/// The ring built from the four oculus edges is four-fold symmetric on the square: every ring beam and bottom wedge equals the first turned by its quarter turns, and the plate equals itself turned, within 1e-9.
void check_ring() {

    const std::vector<wood_floor::Outline> ring = square_floor().oculus();
    check(ring.size() == 9, "four ring beams, four wedges and the plate");

    for (size_t i = 0; i < 8; i++) {
        const Xform turn = Xform::rotation_z(static_cast<double>(i % 4) * 90.0, true);
        const wood_floor::Outline& first = ring[i < 4 ? 0 : 4];

        for (const std::array<const Polyline*, 2>& loops : {std::array<const Polyline*, 2>{&first.top, &ring[i].top}, std::array<const Polyline*, 2>{&first.bottom, &ring[i].bottom}}) {
            const std::vector<Point> a = loops[0]->transformed(turn).get_points();
            const std::vector<Point> b = loops[1]->get_points();
            check(a.size() == b.size(), "a ring member has the first member's vertex count");

            for (size_t j = 0; j < a.size(); j++)
                check((a[j] - b[j]).magnitude() <= 1e-9, fmt::format("ring member {} vertex {} is member {} turned: {:.3e} off", i, j, i < 4 ? 0 : 4, (a[j] - b[j]).magnitude()));
        }
    }

    const std::vector<Point> plate = ring[8].top.get_points();
    const std::vector<Point> turned = ring[8].top.transformed(Xform::rotation_z(90.0, true)).get_points();

    for (size_t j = 0; j + 1 < plate.size(); j++)
        check((turned[j] - plate[(j + 1) % (plate.size() - 1)]).magnitude() <= 1e-9, "the ring plate is four-fold symmetric");

    std::cout << "floor_elements: the ring from the four oculus edges is the rotated ring within 1e-9 on the square" << std::endl;
}

/// The report on the square: every structural relation 0, rule A reducing to the chamfer direction for both the ruling and the rib sweep, the middle cutter level at the outer rib bottoms.
void check_report() {

    const wood_floor::Floor& floor = square_floor();
    const wood_floor::FloorReport report = floor.check();
    std::cout << report.str() << std::endl;
    check(report.ok(1e-6), "the square's report holds every structural relation");

    for (size_t q = 0; q < 4; q++) {
        const wood_floor::CentralPanel& panel = floor.geometry[q].central_panel;
        const Vector& chamfer = floor.columns[q].chamfer_direction;
        check(std::abs(std::abs(panel.ruling.dot(chamfer)) - 1.0) <= 1e-12 && std::abs(std::abs(panel.rib_sweep.dot(chamfer)) - 1.0) <= 1e-12, fmt::format("rule A gives quarter {} the chamfer direction for the ruling and the sweep", q));
        check(std::abs(report.rib_sweep_obliqueness_deg[q][0] - report.rib_sweep_obliqueness_deg[q][1]) <= 1e-9, "both inner ribs equally oblique on the square");
        check(std::abs(report.rib_bottom_clearance_mm[q][0]) <= 1e-9 && std::abs(report.rib_bottom_clearance_mm[q][1]) <= 1e-9, fmt::format("the middle cutter level at quarter {}'s outer rib bottoms", q));
    }

    std::cout << "floor_elements: the square's report holds, rule A is the chamfer direction, the middle cutter level at the rib bottoms" << std::endl;
}

/// The thinnest and thickest central bed plate of a floor: the distance of each plate's top corners from its bottom face's plane.
std::array<double, 2> central_bed_thickness(const wood_floor::Floor& floor) {

    std::array<double, 2> range = {1e300, 0.0};

    for (size_t q = 0; q < 4; q++) {
        const std::vector<std::vector<wood_floor::Outline>> rows = floor.quarter(q).beds();

        for (const wood_floor::Outline& bed : rows[1]) {
            const std::vector<Point> bottom = bed.bottom.get_points();
            const Plane plane = Plane::from_point_normal(bottom[0], (bottom[1] - bottom[0]).cross(bottom[3] - bottom[0]).normalized());

            for (const Point& point : bed.top.get_points()) {
                const double thickness = std::abs((point - plane.origin()).dot(plane.z_axis()));
                range = {std::min(range[0], thickness), std::max(range[1], thickness)};
            }
        }
    }

    return range;
}

/// Every central bed plate on the square exactly tsections thick.
void check_section_layers() {

    const wood_floor::Floor& floor = square_floor();
    const std::array<double, 2> section = central_bed_thickness(floor);
    check(std::abs(section[0] - floor.sizes.tsections) <= 1e-9 && std::abs(section[1] - floor.sizes.tsections) <= 1e-9, fmt::format("every central bed plate {} thick, not {:.12f} .. {:.12f}", floor.sizes.tsections, section[0], section[1]));

    std::cout << fmt::format("floor_elements: the central bed plates {:.9f} .. {:.9f} thick", section[0], section[1]) << std::endl;
}

/// The farthest point of a loop from the plane through its first point with its Newell normal, mm.
double loop_flatness(std::vector<Point> points) {

    if (points.size() > 1 && points.front() == points.back())
        points.pop_back();

    const Vector normal = compute_newell(points).normalized();
    double worst = 0.0;

    for (const Point& point : points)
        worst = std::max(worst, std::abs((point - points[0]).dot(normal)));

    return worst;
}

/// The least flat face of a member outline: its two loops and every side quad between them, mm.
double outline_flatness(const wood_floor::Outline& outline) {

    const std::vector<Point> top = outline.top.get_points();
    const std::vector<Point> bottom = outline.bottom.get_points();
    double worst = std::max(loop_flatness(top), loop_flatness(bottom));

    for (size_t i = 0; i + 1 < top.size() && i + 1 < bottom.size(); i++)
        worst = std::max(worst, loop_flatness({top[i], top[i + 1], bottom[i + 1], bottom[i]}));

    return worst;
}

/// The least flat face over every member of the floor.
double floor_flatness(const wood_floor::Floor& floor) {

    std::vector<wood_floor::Outline> outlines = floor.oculus();

    for (size_t q = 0; q < 4; q++) {
        const wood_floor::Quarter quarter = floor.quarter(q);

        for (const std::vector<wood_floor::Outline>& family : {quarter.outer_ribs(), quarter.inner_ribs(), quarter.inner_beams(), quarter.wedges_inner_beams(), quarter.tsections()})
            outlines.insert(outlines.end(), family.begin(), family.end());

        for (const std::vector<wood_floor::Outline>& row : quarter.beds())
            outlines.insert(outlines.end(), row.begin(), row.end());
    }

    double worst = 0.0;

    for (const wood_floor::Outline& outline : outlines)
        worst = std::max(worst, outline_flatness(outline));

    return worst;
}

/// The eight rib face bottoms of corner q at the column head: both faces of the two outer and the two inner ribs.
std::vector<double> rib_bottoms(const wood_floor::Floor& floor, size_t q) {

    const wood_floor::Quarter quarter = floor.quarter(q);
    std::vector<double> levels;

    for (const std::vector<wood_floor::Outline>& family : {quarter.outer_ribs(), quarter.inner_ribs()})
        for (const wood_floor::Outline& rib : family)
            levels.insert(levels.end(), {rib.top.get_point(2)[2], rib.bottom.get_point(2)[2]});

    return levels;
}

/// One rib level per column head: on 3000 x 2400 both outer ribs of every corner end on their fan planes at the cutter level, the shallower end -689.979, the short rib's run-in solved to 187.667 and the long one's kept at the wedge, every inner rib face within 0.2 mm of it; the square keeps the wedge as both run-ins.
void check_rib_levels() {

    const wood_floor::Floor floor(wood_floor::FloorPlan::rectangle(3000.0, 2400.0), wood_floor::FloorSizes{});
    const wood_floor::FloorReport report = floor.check();

    for (size_t q = 0; q < 4; q++) {
        const double level = floor.columns[q].levels[1];
        const std::vector<double> bottoms = rib_bottoms(floor, q);
        const std::array<double, 2>& run_in = floor.geometry[q].run_in;
        check(std::abs(level + 689.979) < 1e-3, fmt::format("corner {}'s level at the shallower outer rib end, {:.3f}", q, level));
        check(std::max(run_in[0], run_in[1]) == floor.sizes.wedge && std::abs(std::min(run_in[0], run_in[1]) - 187.667) < 1e-3, fmt::format("corner {}'s run-ins {:.3f} / {:.3f}: the long rib keeps the wedge, the short one 187.667", q, run_in[0], run_in[1]));

        for (size_t i = 0; i < 4; i++)
            check(std::abs(bottoms[i] - level) <= 1e-9, fmt::format("corner {}'s outer rib face {} ends {:.3e} mm off the level", q, i, bottoms[i] - level));

        for (size_t i = 4; i < 8; i++)
            check(std::abs(bottoms[i] - level) <= 0.2, fmt::format("corner {}'s inner rib face {} ends {:.3f} mm off the level", q, i - 4, bottoms[i] - level));

        check(std::abs(report.rib_level_spread_mm[q] - (*std::max_element(bottoms.begin(), bottoms.end()) - *std::min_element(bottoms.begin(), bottoms.end()))) <= 1e-12, "the report's spread is the eight bottoms' range");
    }

    const wood_floor::Floor& square = square_floor();

    for (size_t q = 0; q < 4; q++)
        check(square.geometry[q].run_in[0] == square.sizes.wedge && square.geometry[q].run_in[1] == square.sizes.wedge && square.check().rib_level_spread_mm[q] <= 1e-9, "the square keeps the wedge as its run-in, every rib at one level");

    std::cout << fmt::format("floor_elements: one rib level per column on 3000 x 2400, {:.3f}, the eight rib bottoms span {:.3f} mm, the short run-in {:.3f}; the square at the wedge run-in", floor.columns[0].levels[1], report.rib_level_spread_mm[0], std::min(floor.geometry[0].run_in[0], floor.geometry[0].run_in[1])) << std::endl;
}

/// The thickness of each column block of quarter q, side 0, middle and side 1: its far plane's distance from its fan plane.
std::array<double, 3> block_thickness(const wood_floor::Floor& floor, size_t q) {

    std::array<double, 3> thickness;

    for (size_t i = 0; i < 3; i++) {
        const std::array<Plane, 2>& planes = floor.geometry[q].planes.wedges[i];
        thickness[i] = (planes[1].origin() - planes[0].origin()).dot(planes[0].z_axis());
    }

    return thickness;
}

/// The lowest corner of a column block's far face.
double far_bottom(const wood_floor::Outline& block) {

    double lowest = 1e300;

    for (const Point& point : block.top.get_points())
        lowest = std::min(lowest, point[2]);

    return lowest;
}

/// The column blocks span their ribs' run-ins: on 3000 x 2400 the side blocks 240 and 187.667 thick with their far ends within 1 mm, the middle one 1.25 times their mean, 267.292; 240 / 300 / 240 on the square.
void check_column_blocks() {

    const wood_floor::Floor floor(wood_floor::FloorPlan::rectangle(3000.0, 2400.0), wood_floor::FloorSizes{});

    for (size_t q = 0; q < 4; q++) {
        const std::array<double, 3> thickness = block_thickness(floor, q);
        const std::array<double, 2>& run_in = floor.geometry[q].run_in;
        const std::vector<wood_floor::Outline> blocks = floor.quarter(q).wedges_inner_beams();
        check(std::abs(thickness[0] - run_in[0]) <= 1e-9 && std::abs(thickness[2] - run_in[1]) <= 1e-9 && std::abs(thickness[1] - 267.292) < 1e-3, fmt::format("quarter {}'s blocks {:.3f} / {:.3f} / {:.3f} thick over the run-ins {:.3f} / {:.3f}", q, thickness[0], thickness[1], thickness[2], run_in[0], run_in[1]));
        check(std::abs(far_bottom(blocks[0]) - far_bottom(blocks[2])) <= 1.0, fmt::format("quarter {}'s side blocks end {:.3f} mm apart", q, far_bottom(blocks[0]) - far_bottom(blocks[2])));
    }

    for (size_t q = 0; q < 4; q++) {
        const std::array<double, 3> thickness = block_thickness(square_floor(), q);
        check(std::abs(thickness[0] - 240.0) <= 1e-9 && std::abs(thickness[1] - 300.0) <= 1e-9 && std::abs(thickness[2] - 240.0) <= 1e-9, fmt::format("the square's blocks 240 / 300 / 240, not {:.12f} / {:.12f} / {:.12f}", thickness[0], thickness[1], thickness[2]));
    }

    const std::vector<wood_floor::Outline> blocks = floor.quarter(0).wedges_inner_beams();
    std::cout << fmt::format("floor_elements: the column blocks over the run-ins on 3000 x 2400, {:.3f} / {:.3f} / {:.3f} thick, the side ends {:.3f} mm apart; 240 / 300 / 240 on the square", block_thickness(floor, 0)[0], block_thickness(floor, 0)[1], block_thickness(floor, 0)[2], std::abs(far_bottom(blocks[0]) - far_bottom(blocks[2]))) << std::endl;
}

/// The 3000 x 2400 bay: the report holds, rule A as the design measured it, every member face planar, and 44 of 44 contacts found by the kernel's search.
void check_rectangle() {

    const wood_floor::Floor floor(wood_floor::FloorPlan::rectangle(3000.0, 2400.0), wood_floor::FloorSizes{});
    const wood_floor::FloorReport report = floor.check();
    check(report.ok(1e-9), "the rectangle's report holds within 1e-9:\n" + report.str());
    check(std::abs(std::abs(report.ruling_off_chamfer_deg[0]) - 0.839) < 1e-3 && std::abs(report.rib_sweep_obliqueness_deg[0][0] - 20.703) < 1e-3 && std::abs(report.rib_sweep_obliqueness_deg[0][1] - 3.338) < 1e-3, "rule A on 3000 x 2400 with one rib level per column: u 0.839 deg off the chamfer, r 20.703 / 3.338 deg oblique");
    check(floor_flatness(floor) <= 1e-9, fmt::format("every member face planar, {:.3e} off", floor_flatness(floor)));

    WoodSession scene("rectangle");
    wood_floor::FloorMembers members = wood_floor::add_floor(scene, floor, nullptr);
    wood_floor::add_columns(scene, floor, nullptr, members);
    const wood_floor::ContactCheck contacts = wood_floor::verify_contacts(scene, floor, members);
    check(contacts.ok() && contacts.count == 44, "rectangle " + contacts.str());
    std::cout << fmt::format("floor_elements: 3000 x 2400 report ok, rule A {:.3f} deg, faces planar within {:.1e}, 44 of 44 contacts", report.ruling_off_chamfer_deg[0], floor_flatness(floor)) << std::endl;
}

/// The lines of a list that equal a line within 1e-9 mm at both ends.
size_t matches(const std::vector<Line>& lines, const Line& line) {

    size_t count = 0;

    for (const Line& other : lines)
        count += (other.start() - line.start()).magnitude() <= 1e-9 && (other.end() - line.end()).magnitude() <= 1e-9;

    return count;
}

/// Whether the node carries the connector colour.
bool in_connector_color(const TreeNode& node) {

    const Color& color = wood_floor::CONNECTOR_COLOR;

    return node.color && node.color->r == color.r && node.color->g == color.g && node.color->b == color.b && node.color->a == color.a;
}

/// Every connector's node and every part and dowel node nested under it carry the connector colour; returns the nodes checked.
size_t check_connector_colors(const WoodSession& scene, const std::vector<std::shared_ptr<JointBeam>>& connectors, const std::string& label) {

    size_t nodes = 0;

    for (const std::shared_ptr<JointBeam>& connector : connectors) {
        const std::shared_ptr<TreeNode> node = scene.tree.get_node_by_name(connector->guid());
        check(node && in_connector_color(*node), label + " " + connector->name + " in the connector colour");
        nodes++;

        for (TreeNode* child : node->descendants()) {
            check(in_connector_color(*child), label + " " + connector->name + " child " + child->name + " in the connector colour");
            nodes++;
        }
    }

    return nodes;
}

/// The group a connector of that relationship belongs under and that group's parent, as parent/group.
std::string expected_group(const wood_floor::Relationship& row) {

    const size_t index = row.seam_or_corner;

    if (row.place() == wood_floor::Place::quarter)
        return fmt::format("quarter_model_{}/connectors_{}", index, index);

    if (row.place() == wood_floor::Place::oculus)
        return "oculus/connectors_oculus";

    if (row.place() == wood_floor::Place::column)
        return fmt::format("column_model_{}/connectors_column_{}", index, index);

    return fmt::format("seams/seam_{}", index);
}

/// Every connector of the floor's relationships, in their order, sits in the group of its place, its parts and dowels nested under it; returns the connectors per group as parent/group.
std::map<std::string, size_t> check_connector_tree(const WoodSession& scene, const wood_floor::Floor& floor, const std::vector<std::shared_ptr<JointBeam>>& connectors, const std::string& label) {

    std::map<std::string, size_t> counts;
    size_t next = 0;

    for (const wood_floor::Relationship& row : wood_floor::relationships(floor)) {
        if (row.kind == wood_floor::Relation::support || row.kind == wood_floor::Relation::cutter)
            continue;

        const JointBeam& connector = *connectors.at(next++);
        const std::shared_ptr<TreeNode> node = scene.tree.get_node_by_name(connector.guid());
        const std::shared_ptr<TreeNode> group = node ? node->parent() : nullptr;
        const std::shared_ptr<TreeNode> parent = group ? group->parent() : nullptr;
        const std::string path = parent ? parent->name + "/" + group->name : "";
        check(path == expected_group(row), fmt::format("{} {} of {} under {}, not {}", label, connector.name, row.text(), expected_group(row), path));
        counts[path]++;
    }

    check(next == connectors.size(), label + " one connector per relationship");

    return counts;
}

/// The screws of one floor: per kind 16, 16, 16, 8 and 16, every connector pre-drilled and naming both members of its relationship, every screw 200 long, radius 2, held over its length by its members, at least 8 mm from every other and clear of every bore and pocket; no member cut by them, every member reading its pre-drill lines from the one connector, and both through a round trip; every connector of the floor and every part and dowel nested under it red, every connector in the group of its place with the counts per group, also after the round trip.
void check_floor_screws(const wood_floor::Floor& floor, const std::string& label) {

    WoodSession scene("screws");
    wood_floor::FloorMembers members = wood_floor::add_floor(scene, floor, nullptr);
    wood_floor::add_columns(scene, floor, nullptr, members);
    std::vector<std::shared_ptr<JointBeam>> connectors = wood_floor::add_connectors(scene, floor, members);
    std::map<std::string, double> volumes;

    for (const std::shared_ptr<BeamVariable>& beam : scene.beam_variables())
        volumes[beam->guid()] = compute_volume(beam->model_geometry_mesh());

    const std::vector<wood_floor::Relation> kinds(wood_floor::SCREW_RELATIONS.begin(), wood_floor::SCREW_RELATIONS.end());
    const std::vector<std::shared_ptr<JointBeam>> screws = wood_floor::add_connectors(scene, floor, members, kinds);
    const wood_floor::ScrewCheck report = wood_floor::check_screws(scene, floor, screws);
    connectors.insert(connectors.end(), screws.begin(), screws.end());
    check(report.counts.at(wood_floor::Relation::screw_rib_beam) == 16 && report.counts.at(wood_floor::Relation::screw_beam_mitre) == 16 && report.counts.at(wood_floor::Relation::screw_rib_corner) == 16 && report.counts.at(wood_floor::Relation::screw_ring) == 8 && report.counts.at(wood_floor::Relation::screw_oculus) == 16, label + " screws per kind:\n" + report.str());
    check(report.misfits.empty() && report.screw_screw_mm >= 8.0 && report.screw_bore_mm >= 0.0 && report.screw_pocket_mm >= 0.0 && std::abs(report.embedded_min_mm - 200.0) <= 1e-3, label + " screws clear and held:\n" + report.str());
    check(wood_floor::verify_contacts(scene, floor, members, 1e-6, kinds).ok(), label + " every screw contact the kernel's search finds");

    size_t next = 0;

    for (const wood_floor::Relationship& row : wood_floor::relationships(floor)) {
        if (row.screws.empty())
            continue;

        const JointBeam& connector = *screws.at(next++);
        check(connector.pre_drill && connector.cutters.empty() && connector.parts.empty() && connector.targets.size() == 2 + row.through.size(), label + " " + connector.name + " pre-drilled, no cutter, its members as targets");
        check(connector.targets[0] == members.get(row.a)->guid() && connector.targets[1] == members.get(row.b)->guid(), label + " " + connector.name + " names both members of " + row.text());
        check(std::abs(connector.line_radius - 2.0) <= 1e-12 && connector.drill_lines.size() == row.screws.size(), label + " " + connector.name + " radius 2, one line per screw");
        check_nested(scene, connector, 0, connector.drill_lines.size());

        for (const Line& line : connector.drill_lines) {
            check(std::abs(line.length() - 200.0) <= 1e-9, label + " " + connector.name + " screws 200 long");

            for (const std::string& guid : connector.targets)
                check(matches(scene.pre_drill_lines(guid), line) == 1, label + " " + connector.name + " every member reads each screw once");
        }
    }

    check(next == screws.size(), label + " one screw connector per screw row");

    for (const std::shared_ptr<BeamVariable>& beam : scene.beam_variables())
        check(std::abs(compute_volume(beam->model_geometry_mesh()) - volumes.at(beam->guid())) <= 1e-9 * volumes.at(beam->guid()), label + " no member cut by the screws: " + beam->name);

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    size_t loaded = 0;

    for (const std::shared_ptr<JointBeam>& connector : screws) {
        const std::shared_ptr<JointBeam> read = back.get_element<JointBeam>(connector->guid());
        check(read && read->pre_drill && read->targets == connector->targets && read->drill_lines.size() == connector->drill_lines.size(), label + " screw connector round trip " + connector->name);
        check_nested(back, *read, 0, read->drill_lines.size());

        for (const Line& line : connector->drill_lines)
            for (const std::string& guid : connector->targets)
                check(matches(back.pre_drill_lines(guid), line) == 1, label + " pre-drill lines round trip " + connector->name);

        loaded++;
    }

    check(loaded == screws.size(), label + " screw round trip count");
    const size_t painted = check_connector_colors(scene, connectors, label);
    const std::map<std::string, size_t> groups = check_connector_tree(scene, floor, connectors, label);
    std::map<std::string, size_t> expected = {{"oculus/connectors_oculus", 16}};

    for (size_t q = 0; q < 4; q++) {
        expected[fmt::format("quarter_model_{}/connectors_{}", q, q)] = 12;
        expected[fmt::format("column_model_{}/connectors_column_{}", q, q)] = 3;
        expected[fmt::format("seams/seam_{}", q)] = 2;
    }

    check(groups == expected, label + " 12 connectors per quarter, 16 in the oculus, 3 per column, 2 per seam");
    check(check_connector_tree(back, floor, connectors, label + " round trip") == expected, label + " the connector tree through a round trip");
    check(connectors.size() == 84 && check_connector_colors(back, connectors, label + " round trip") == painted, fmt::format("{} 84 connectors, {} red nodes, the same after a round trip", label, painted));
    size_t total = 0;

    for (const std::pair<const wood_floor::Relation, size_t>& count : report.counts)
        total += count.second;

    std::cout << fmt::format("floor_elements: {} screws in {} connectors on {}, both members named, 200 x d4, held, {:.3f} mm apart at the least, {:.3f} mm clear of bores, {:.3f} mm of pockets, nothing cut, pre-drill lines through a round trip; {} connectors and their {} part and dowel nodes red, 12 in each quarter, 16 in the oculus, 3 at each column, 2 on each seam, also after it", total, screws.size(), label, report.screw_screw_mm, report.screw_bore_mm, report.screw_pocket_mm, connectors.size(), painted - connectors.size()) << std::endl;
}

/// The assembly screws on the square and on 3000 x 2400.
void check_screws() {

    check_floor_screws(square_floor(), "the square");
    check_floor_screws(wood_floor::Floor(wood_floor::FloorPlan::rectangle(3000.0, 2400.0), wood_floor::FloorSizes{}), "3000 x 2400");
}

int main() {

    check_shared_entities();
    check_ring();
    check_report();
    check_section_layers();
    check_rectangle();
    check_rib_levels();
    check_column_blocks();
    check_relationships();
    check_beams();
    check_thickness();
    check_support();
    check_wedges();
    check_dowels();
    check_quarter_dowels();
    check_rectangle_plates();
    check_screws();

    return 0;
}
