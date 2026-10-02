#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_brep_drill.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

const wood_floor::FloorGuide GUIDE{
    .size_column_head = 220.0,
    .size_column_head_chamfer = 120.0,
    .size_wedge = 240.0,
};

/// The four quarter guides of the square bay.
std::vector<wood_floor::FloorGuide> square_guides() {

    std::vector<wood_floor::FloorGuide> guides;

    for (int i = 0; i < 4; i++)
        guides.push_back(wood_floor::FloorGuide::rectangle(3000.0, 3000.0, i, GUIDE));

    return guides;
}

const double EXACT_SUPPORT = 500671.261678; // compas_tf SupportElement.brep volume, exact cylinders and hexagons
const double COMPAS_TF_OUTER_RIB = 99598198.606378; // compas_tf outer rib carved by its rectangle plate pocket and dowels
const double COMPAS_TF_TIED_RIB = 98812970.259836; // the same rib after the seam connector pocket too
const double COMPAS_TF_TIE = 1570456.693007; // compas_tf OuterRibConnector.obj body, the parametric tie's defaults
const double COMPAS_TF_HEAD_CUT = 211196000.0 - 176418621.638340; // compas_tf stock less its carved column: what the six head cutters take

/// Throws with the message when the condition fails.
void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
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

    const std::vector<wood_floor::Outline> outer = GUIDE.outer_ribs();
    const std::vector<wood_floor::Outline> inner = GUIDE.inner_ribs();

    for (size_t i = 0; i < 2; i++) {
        beams.push_back(wood_floor::to_rib(outer[i], "outer_rib"));
        check_beam(*beams.back(), outer[i], 11, "outer rib " + std::to_string(i));
        beams.push_back(wood_floor::to_rib(inner[i], "inner_rib"));
        check_beam(*beams.back(), inner[i], 11, "inner rib " + std::to_string(i));
    }

    for (const wood_floor::Outline& outline : GUIDE.inner_beams()) {
        beams.push_back(wood_floor::to_beam(outline, {0, 3}, {1, 2}, "inner_beam"));
        check_beam(*beams.back(), outline, 6, "inner beam");
    }

    const std::vector<wood_floor::Outline> oculus = wood_floor::FloorGuide::oculus(square_guides());

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
    const wood_floor::Quarter quarter = wood_floor::add_quarter_model(scene, wood_floor::FloorGuide::rectangle(3000.0, 3000.0, 1, GUIDE), nullptr, "_1");
    const std::vector<wood_floor::Outline> outlines = GUIDE.outer_ribs();

    check(quarter.outer_ribs[0].thickness == wood_floor::outline_thickness(outlines[0]), "a turned rib keeps its outline's thickness");
    check(std::abs(quarter.outer_ribs[0].thickness - GUIDE.size_outer_ribs) < 1e-6, "an outer rib as thick as the guide says, " + std::to_string(quarter.outer_ribs[0].thickness));
    check(quarter.inner_beams[1].thickness > GUIDE.size_inner_beams - 1e-9 && quarter.inner_beams[1].thickness < 1.5 * GUIDE.size_inner_beams, "an inner beam about as thick as the guide says, " + std::to_string(quarter.inner_beams[1].thickness));
    check(quarter.blocks[1].thickness > 1.25 * GUIDE.size_wedge - 1e-9, "the tilted middle block at least its plane offset thick, " + std::to_string(quarter.blocks[1].thickness));
    check(quarter.beds.size() == 3 && quarter.tsections.size() == 6 && quarter.inner_ribs.size() == 2, "a quarter of three bed rows, six t-sections and two inner ribs");

    std::cout << "floor_elements: every quarter member carries its outline thickness, a rib, a beam and a block checked" << std::endl;
}

/// The area of the polygon a circle of that radius is faceted into.
double faceted_area(double radius, double chord_tolerance) {

    const int n = circle_segments(radius, chord_tolerance);

    return 0.5 * n * radius * radius * std::sin(2.0 * M_PI / n);
}

/// The support under the column: closed, near the exact solid, its joint cutting exactly the head plate pocket and the screws, the head cutters still removing compas_tf's volume, and every dimension through a round trip.
void check_support() {

    WoodSession scene("support");
    const std::shared_ptr<Support> support = wood_floor::to_support(GUIDE);
    const std::shared_ptr<Column> column = wood_floor::to_column(GUIDE, *support);
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

    for (const std::shared_ptr<Joint>& cutter : wood_floor::to_column_cutters(GUIDE, *column)) {
        scene.add(cutter);
        scene.add_joint(cutter);
    }

    const double head = stock - removed - compute_volume(column->model_geometry_mesh());
    check(std::abs(head - COMPAS_TF_HEAD_CUT) <= 1e-6 * COMPAS_TF_HEAD_CUT, "head cutters remove " + std::to_string(head));
    check(column->model_geometry_mesh().is_closed(), "carved column closed");

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    const std::shared_ptr<Support> loaded = back.supports().front();
    check(loaded->plane.origin() == support->plane.origin() && loaded->height == support->height && loaded->head_plate_diameter == support->head_plate_diameter, "support round trip");
    check(loaded->screw_count == support->screw_count && loaded->screw_angle == support->screw_angle && loaded->base_plate_hole_spacing == support->base_plate_hole_spacing, "support round trip fasteners");

    std::cout << fmt::format("floor_elements: support {:.3f} mm3, joint removes {:.3f}, head cutters {:.3f}, round trip pass", compute_volume(base), removed, head) << std::endl;
}

/// The number of exact bores in a BRep: its rational surfaces.
size_t count_bores(const BRep& brep) {

    size_t bores = 0;

    for (const NurbsSurface& surface : brep.m_surfaces)
        if (surface.is_rational())
            bores++;

    return bores;
}

/// The children of a connector in the scene tree: its part and dowel elements, in order.
std::vector<std::shared_ptr<Joint>> children_of(const WoodSession& scene, const JointBeam& connector) {

    std::vector<std::shared_ptr<Joint>> children;

    for (TreeNode* child : scene.tree.get_node_by_name(connector.guid())->children())
        children.push_back(scene.get_element<Joint>(child->name));

    return children;
}

/// The connector is nested: it draws nothing itself, parts part children then dowels dowel children follow it in the tree, named after it, every one visible and exact.
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
            check(std::dynamic_pointer_cast<Dowel>(child) && child->name == fmt::format("{}_dowel_{}", connector.name, i - parts) && std::abs(child->model_geometry_brep().volume() - cylinder) < 1e-2 * cylinder, connector.name + " dowel child " + child->name);
        }
    }
}

/// No dowel of the connector protrudes: just inside every dowel's end lies in one of its two members; and when the dowels pass through, just outside every end lies in neither, the end flush with the outer face.
void check_flush(const JointBeam& joint, const WoodSession& scene, const std::string& name, bool through) {

    const std::shared_ptr<Element> a = scene.get_element<Element>(joint.targets[0]);
    const std::shared_ptr<Element> b = scene.get_element<Element>(joint.targets[1]);

    for (const Line& dowel : joint.drill_lines) {
        const Vector d = dowel.to_vector().normalized();

        for (const Point& end : {dowel.start(), dowel.end()}) {
            const Vector out = end == dowel.start() ? -d : d;
            check(is_inside(a->element_geometry_mesh(), end - out * 0.5) || is_inside(b->element_geometry_mesh(), end - out * 0.5), name + " dowel does not protrude");

            if (through)
                check(!is_inside(a->element_geometry_mesh(), end + out * 0.5) && !is_inside(b->element_geometry_mesh(), end + out * 0.5), name + " dowel ends flush with the outer face");
        }
    }
}

/// The four quarters placed by quarter turns, their inner beams collected into the ring.
std::vector<wood_floor::Quarter> add_quarters(WoodSession& scene, std::vector<wood_floor::Member>& ring) {

    std::vector<wood_floor::Quarter> quarters;

    for (int i = 0; i < 4; i++) {
        quarters.push_back(wood_floor::add_quarter_model(scene, wood_floor::FloorGuide::rectangle(3000.0, 3000.0, i, GUIDE), nullptr, fmt::format("_{}", i)));
        ring.insert(ring.end(), quarters.back().inner_beams.begin(), quarters.back().inner_beams.end());
    }

    return quarters;
}

/// The outer ribs of every quarter, in quarter order.
std::vector<wood_floor::Member> outer_ribs(const std::vector<wood_floor::Quarter>& quarters) {

    std::vector<wood_floor::Member> ribs;

    for (const wood_floor::Quarter& quarter : quarters)
        ribs.insert(ribs.end(), quarter.outer_ribs.begin(), quarter.outer_ribs.end());

    return ribs;
}

/// The wedges between the inner beams and the oculus: eight visible connectors with their parts and cutters, their dowels flush with the beams, exact cylinders in their BReps and exact bores through the wedge parts, and the carved beams, through a round trip.
void check_wedges() {

    WoodSession scene("wedges");
    std::vector<wood_floor::Member> ring;
    add_quarters(scene, ring);
    const std::vector<wood_floor::Member> oculus = wood_floor::add_oculus_model(scene, square_guides(), nullptr);
    ring.insert(ring.end(), oculus.begin(), oculus.end());
    const std::vector<std::shared_ptr<JointBeam>> wedges = wood_floor::add_wedges(scene, ring, nullptr);
    check(wedges.size() == 8, "eight wedges, not " + std::to_string(wedges.size()));

    for (const std::shared_ptr<JointBeam>& wedge : wedges) {
        check_flush(*wedge, scene, wedge->name, true);
        check(count_bores(wedge->part_brep(0)) == wedge->drill_lines.size(), "every wedge dowel an exact bore through the wedge's part");
        check_nested(scene, *wedge, 1, wedge->drill_lines.size());
        check(count_bores(children_of(scene, *wedge).front()->model_geometry_brep()) == wedge->drill_lines.size(), "the wedge child carries the bores");
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
        check(count_bores(brep) > 0 && brep.is_solid() && std::abs(brep.volume() - volume) <= 1e-3 * volume, "exact dowel bores in " + beam->name);
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
        check(count_bores(plate->model_geometry_brep()) == 4 && plate->model_geometry_brep().is_solid(), "four exact bores in " + plate->name);
    }

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    const std::shared_ptr<JointBeam> loaded = back.get_elements<JointBeam>().front();
    check(loaded->drill_lines.size() == 4 && loaded->cutters.size() == 2 && loaded->drill_overshoot == joint->drill_overshoot, "dowels round trip");
    check_nested(back, *loaded, 0, 4);

    std::cout << "floor_elements: four dowels at the corners of a plate contact, 30 long, exact bores, nested as four dowel children and a round trip pass" << std::endl;
}

/// Every inner rib is exact, bored once per dowel of the sets that join it.
void check_inner_rib_bores(const std::vector<wood_floor::Quarter>& quarters, const std::vector<std::shared_ptr<JointBeam>>& sets) {

    for (const wood_floor::Quarter& quarter : quarters)
        for (const wood_floor::Member& rib : quarter.inner_ribs) {
            size_t crossing = 0;

            for (const std::shared_ptr<JointBeam>& set : sets)
                if (std::find(set->targets.begin(), set->targets.end(), rib.element->guid()) != set->targets.end())
                    crossing += set->drill_lines.size();

            const BRep& brep = rib.element->model_geometry_brep();
            check(brep.is_solid() && count_bores(brep) == crossing, fmt::format("an inner rib bored once per dowel, one exact cylinder each: {} dowels, {} bores in {}", crossing, count_bores(brep), rib.element->name));
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
        check(element->model_geometry_brep().is_solid() && count_bores(element->model_geometry_brep()) > 0, fmt::format("exact dowel bores in {}: solid {}, {} bores", element->name, element->model_geometry_brep().is_solid(), count_bores(element->model_geometry_brep())));
    }

    check(drilled == 28, "seven drilled members per quarter, the ribs and the blocks, not " + std::to_string(drilled));
}

/// The assembly dowels of the quarters: a dowel set on every rib-to-wedge-block contact, the six per quarter compas_tf finds, never across quarters, four 30 mm dowels exactly at the corners of each contact inset 50, the contacts found on the uncut members; every dowel half in each member, every drilled member exact; through a round trip.
void check_quarter_dowels() {

    WoodSession scene("quarter_dowels");
    std::vector<wood_floor::Member> ring;
    const std::vector<wood_floor::Quarter> quarters = add_quarters(scene, ring);
    const std::vector<std::shared_ptr<JointBeam>> sets = wood_floor::add_quarter_dowels(scene, quarters, nullptr);
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
            check(is_inside(rib->element_geometry_mesh(), dowel.center() - dowel.to_vector().normalized() * 0.5) && is_inside(block->element_geometry_mesh(), dowel.center() + dowel.to_vector().normalized() * 0.5), "a dowel crosses the contact at its middle, half in each member");
            dowels++;
        }
    }

    check(sets.size() == 24, "six rib-to-block dowel sets per quarter, not " + std::to_string(sets.size()));
    check_inner_rib_bores(quarters, sets);
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
            check(part.is_solid() && count_bores(part) == 4, "a slotted plate exact with four dowel bores");
            check_nested(scene, *plate, 1, 4);
            const std::shared_ptr<Joint> child = children_of(scene, *plate).front();
            check(child->model_geometry_brep().is_solid() && count_bores(child->model_geometry_brep()) == 4 && std::abs(child->model_geometry_brep().volume() - part.volume()) < 1e-6 * part.volume(), "the plate child carries the slot and the four bores");
        }
    }
}

/// The ties on the rib seams: four, nested as four key parts, every tied outer rib and every key at compas_tf's volume.
void check_ties(const WoodSession& scene, const std::vector<wood_floor::Member>& ribs, const std::vector<std::shared_ptr<JointBeam>>& ties) {

    check(ties.size() == 4, "four ties, not " + std::to_string(ties.size()));

    for (const std::shared_ptr<JointBeam>& tie : ties)
        check_nested(scene, *tie, 4, 0);

    for (const wood_floor::Member& rib : ribs)
        check(std::abs(compute_volume(rib.element->model_geometry_mesh()) - COMPAS_TF_TIED_RIB) <= 1e-9 * COMPAS_TF_TIED_RIB, "tied outer rib " + rib.element->name);

    for (const std::shared_ptr<JointBeam>& tie : ties) {
        Mesh key;

        for (const std::array<Polyline, 2>& part : tie->parts)
            append_mesh(key, Mesh::loft({part[0]}, {part[1]}, true));

        check(std::abs(compute_volume(key) - COMPAS_TF_TIE) <= 1e-9 * COMPAS_TF_TIE, "tie volume " + std::to_string(compute_volume(key)));
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

/// The rectangle plates between the columns and the outer ribs and the ties on the rib seams: eight and four, every carved outer rib at compas_tf's volume, the plates half-lapped and the column still exact, through a round trip.
void check_rectangle_plates() {

    WoodSession scene("rectangle_plates");
    std::vector<wood_floor::Member> ring;
    const std::vector<wood_floor::Quarter> quarters = add_quarters(scene, ring);
    std::vector<std::shared_ptr<Column>> columns;

    for (int i = 0; i < 4; i++)
        columns.push_back(wood_floor::add_column_model(scene, wood_floor::FloorGuide::rectangle(3000.0, 3000.0, i, GUIDE), nullptr, fmt::format("_{}", i)));

    const std::vector<wood_floor::Member> ribs = outer_ribs(quarters);
    const std::vector<std::shared_ptr<JointBeam>> plates = wood_floor::add_rectangle_plates(scene, columns, ribs, nullptr);
    check(plates.size() == 8, "eight rectangle plates, not " + std::to_string(plates.size()));

    for (const wood_floor::Member& rib : ribs)
        check(std::abs(compute_volume(rib.element->model_geometry_mesh()) - COMPAS_TF_OUTER_RIB) <= 1e-9 * COMPAS_TF_OUTER_RIB, "carved outer rib " + rib.element->name);

    const std::vector<std::shared_ptr<JointBeam>> laps = wood_floor::add_cross_laps(scene, plates, nullptr);
    check(laps.size() == 4, "four cross laps, not " + std::to_string(laps.size()));
    check_cross_laps(scene, laps);

    for (const std::shared_ptr<Column>& column : columns)
        check(column->model_geometry_brep().is_solid() && count_bores(column->model_geometry_brep()) == 11, "the column exact with its eight dowel and three screw bores");

    const std::vector<std::shared_ptr<JointBeam>> ties = wood_floor::add_ties(scene, ribs, nullptr);
    check_ties(scene, ribs, ties);

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    size_t slotted = 0;

    for (const std::shared_ptr<JointBeam>& joint : back.get_elements<JointBeam>())
        if (!std::dynamic_pointer_cast<ConnectorPart>(joint))
            slotted += joint->solid_cuts.size();

    check(slotted == 8, "the slots round trip on the plates");
    check(back.get_elements<ConnectorPart>().size() == 8 + 16 && back.get_elements<Dowel>().size() == 32, "the plate and key parts and the dowels round trip as children");

    std::cout << "floor_elements: " << plates.size() << " rectangle plates half-lapped by " << laps.size() << " cross laps, drilled and exact, and " << ties.size() << " ties, carved outer ribs and ties at compas_tf's volume, round trip pass" << std::endl;
    check_loaded_tie_cuts(scene, ties);
}

/// The 6000 x 4800 bay: four quarters built in place, their outer ribs along the long and the short edge, the oculus ringed by them, and the same connectors as on the square: eight rectangle plates, four ties and twenty-four dowel sets of four.
void check_rectangle_bay() {

    WoodSession scene("rectangle_bay");
    std::vector<wood_floor::FloorGuide> guides;
    std::vector<wood_floor::Quarter> quarters;
    std::vector<std::shared_ptr<Column>> columns;
    std::vector<wood_floor::Member> ring;

    for (int i = 0; i < 4; i++) {
        guides.push_back(wood_floor::FloorGuide::rectangle(3000.0, 2400.0, i, GUIDE));
        quarters.push_back(wood_floor::add_quarter_model(scene, guides.back(), nullptr, fmt::format("_{}", i)));
        columns.push_back(wood_floor::add_column_model(scene, guides.back(), nullptr, fmt::format("_{}", i)));
        ring.insert(ring.end(), quarters.back().inner_beams.begin(), quarters.back().inner_beams.end());
    }

    const std::vector<wood_floor::Member> oculus = wood_floor::add_oculus_model(scene, guides, nullptr);
    ring.insert(ring.end(), oculus.begin(), oculus.end());
    const std::vector<wood_floor::Member> ribs = outer_ribs(quarters);
    const double long_rib = compute_volume(ribs[0].element->element_geometry_mesh());
    const double short_rib = compute_volume(ribs[1].element->element_geometry_mesh());
    check(long_rib > short_rib && std::abs(compute_volume(ribs[3].element->element_geometry_mesh()) - long_rib) <= 1e-9 * long_rib, "the outer ribs follow the long and the short edge in every quarter");

    for (const std::shared_ptr<Element>& element : scene.world_elements())
        check(element->element_geometry_mesh().is_closed(), element->name + " closed");

    check(wood_floor::add_wedges(scene, ring, nullptr).size() == 8, "eight wedges");
    check(wood_floor::add_rectangle_plates(scene, columns, ribs, nullptr).size() == 8, "eight rectangle plates");
    check(wood_floor::add_ties(scene, ribs, nullptr).size() == 4, "four ties");
    const std::vector<std::shared_ptr<JointBeam>> sets = wood_floor::add_quarter_dowels(scene, quarters, nullptr);
    check(sets.size() == 24, "twenty-four dowel sets, not " + std::to_string(sets.size()));

    for (const std::shared_ptr<JointBeam>& set : sets)
        check(set->drill_lines.size() == 4, "four dowels per contact");

    std::cout << "floor_elements: the 6000 x 4800 bay closed with 8 wedges, 8 rectangle plates, 4 ties and 24 dowel sets of four" << std::endl;
}

int main() {

    check_beams();
    check_thickness();
    check_support();
    check_wedges();
    check_dowels();
    check_quarter_dowels();
    check_rectangle_plates();
    check_rectangle_bay();

    return 0;
}
