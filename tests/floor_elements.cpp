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

    const std::vector<wood_floor::Outline> oculus = GUIDE.oculus();

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

/// The wedges between the inner beams and the oculus: eight visible connectors with their parts and cutters, their dowels flush with the beams, exact cylinders in their BReps and exact bores through the wedge parts, and the carved beams, through a round trip.
void check_wedges() {

    WoodSession scene("wedges");

    for (int i = 0; i < 4; i++)
        wood_floor::add_quarter_model(scene, GUIDE, Xform::rotation_z(i * 90.0, true), nullptr, fmt::format("_{}", i));

    wood_floor::add_oculus_model(scene, GUIDE, nullptr);
    const std::vector<std::shared_ptr<JointBeam>> wedges = wood_floor::add_wedges(scene, GUIDE, nullptr);
    check(wedges.size() == 8, "eight wedges, not " + std::to_string(wedges.size()));

    for (const std::shared_ptr<JointBeam>& wedge : wedges) {
        check(wedge->is_visible, "a wedge is a visible element");
        check_flush(*wedge, scene, wedge->name, true);
        check(count_bores(wedge->part_brep(0)) == wedge->drill_lines.size(), "every wedge dowel an exact bore through the wedge's part");
        check(count_bores(wedge->model_geometry_brep()) == 2 * wedge->drill_lines.size(), "every wedge dowel an exact cylinder in the wedge's BRep beside its bore");
    }

    std::map<std::string, double> volumes;

    for (const std::shared_ptr<BeamVariable>& beam : scene.beam_variables())
        volumes[beam->guid()] = compute_volume(beam->model_geometry_mesh());

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    size_t loaded = 0;

    for (const std::shared_ptr<JointBeam>& joint : back.get_elements<JointBeam>()) {
        check(joint->parts.size() == 1 && joint->cutters.size() == 2 && !joint->drill_lines.empty() && joint->is_visible, "wedge round trip");
        loaded++;
    }

    check(loaded == wedges.size(), "wedge round trip count");

    for (const std::shared_ptr<BeamVariable>& beam : back.beam_variables())
        check(std::abs(compute_volume(beam->model_geometry_mesh()) - volumes.at(beam->guid())) <= 1e-9 * volumes.at(beam->guid()), "carved beam round trip " + beam->name);

    for (const std::shared_ptr<BeamVariable>& beam : scene.beam_variables()) {
        if (beam->solid_cuts.empty())
            continue;

        const BRep& brep = beam->model_geometry_brep();
        size_t bores = 0;

        for (const NurbsSurface& surface : brep.m_surfaces)
            if (surface.is_rational())
                bores++;

        const double volume = compute_volume(beam->model_geometry_mesh());
        check(bores > 0 && brep.is_solid() && std::abs(brep.volume() - volume) <= 1e-3 * volume, "exact dowel bores in " + beam->name);
    }

    std::cout << "floor_elements: " << wedges.size() << " wedges, visible, dowels flush and exact, joints and carved beams through a round trip, every dowel bore exact in the BReps, pass" << std::endl;
}

/// The dowels factory on two plates face to face: four Ø10 dowels 60 long at the corners of the 600 x 200 contact inset by 20, 30 deep into both 60 plates, cut as exact bores into both, through a round trip.
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
        check(std::abs(dowel.length() - 60.0) < 1e-9, "a dowel 30 deep into each plate, " + std::to_string(dowel.length()));
        check(std::abs(dowel.center()[2] - 60.0) < 1e-9, "a dowel centred on the contact");
        corners.insert({static_cast<int>(std::lround(dowel.center()[0])), static_cast<int>(std::lround(dowel.center()[1]))});
    }

    check(corners == std::set<std::pair<int, int>>{{20, 20}, {580, 20}, {580, 180}, {20, 180}}, "the dowels 20 in from the contact's corners");

    scene.add(joint);
    scene.add_joint(joint);
    const double bore = faceted_area(joint->line_radius, joint->chord_tolerance) * 30.0 * 4.0;

    for (const std::shared_ptr<Plate>& plate : {lower, upper}) {
        const double volume = compute_volume(plate->model_geometry_mesh());
        check(std::abs(600.0 * 200.0 * 60.0 - bore - volume) <= 1e-6 * volume, "four blind holes out of " + plate->name);
        check(count_bores(plate->model_geometry_brep()) == 4 && plate->model_geometry_brep().is_solid(), "four exact bores in " + plate->name);
    }

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    const std::shared_ptr<JointBeam> loaded = back.get_elements<JointBeam>().front();
    check(loaded->drill_lines.size() == 4 && loaded->cutters.size() == 2 && loaded->drill_overshoot == joint->drill_overshoot && loaded->is_visible, "dowels round trip");

    std::cout << "floor_elements: four dowels at the corners of a plate contact, 60 long, exact bores and a round trip pass" << std::endl;
}

/// The assembly dowels of the quarters: a dowel set on every rib-to-wedge-block contact, the six per quarter compas_tf finds, and on the uncut inner beams' mitres and ends, never across quarters, 60 mm dowels at the inset corners; in every 60 inner rib the pairs from its two blocks meet end to end and bore it through, one exact cylinder each, so the rib carries eight bores; the misfits at the blocks' apexes are listed and the crossing ones dropped, so every drilled member is exact; through a round trip.
void check_quarter_dowels() {

    WoodSession scene("quarter_dowels");

    for (int i = 0; i < 4; i++)
        wood_floor::add_quarter_model(scene, GUIDE, Xform::rotation_z(i * 90.0, true), nullptr, fmt::format("_{}", i));

    std::vector<std::string> misfits;
    const std::vector<std::shared_ptr<JointBeam>> sets = wood_floor::add_quarter_dowels(scene, GUIDE, nullptr, 5.0, 60.0, 20.0, 20.0, &misfits);
    size_t blocks = 0;
    size_t dowels = 0;

    for (const std::shared_ptr<JointBeam>& set : sets) {
        const std::shared_ptr<Element> first = scene.get_element<Element>(set->targets[0]);
        const std::shared_ptr<Element> second = scene.get_element<Element>(set->targets[1]);
        check(first->name.substr(first->name.find_last_of('_')) == second->name.substr(second->name.find_last_of('_')), "a dowel set stays within one quarter, not " + first->name + " to " + second->name);
        blocks += first->name.find("ribs_") != std::string::npos && second->name.starts_with("wedges_");
        check(set->drill_lines.size() <= 4 && !set->drill_lines.empty(), "up to four dowels per contact");

        for (const Line& dowel : set->drill_lines) {
            check(std::abs(dowel.length() - 60.0) < 1e-9, "a dowel 60 long");
            check(is_inside(first->element_geometry_mesh(), dowel.center() - dowel.to_vector().normalized() * 0.5) && is_inside(second->element_geometry_mesh(), dowel.center() + dowel.to_vector().normalized() * 0.5), "a dowel crosses the contact at its middle, half in each member");
            dowels++;
        }
    }

    check(blocks == 24, "six rib-to-block dowel sets per quarter, not " + std::to_string(blocks));
    size_t through = 0;

    for (const std::shared_ptr<BeamVariable>& rib : scene.beam_variables()) {
        if (!rib->name.starts_with("inner_ribs_"))
            continue;

        std::vector<Line> crossing;

        for (const std::shared_ptr<JointBeam>& set : sets)
            if (std::find(set->targets.begin(), set->targets.end(), rib->guid()) != set->targets.end())
                crossing.insert(crossing.end(), set->drill_lines.begin(), set->drill_lines.end());

        size_t pairs = 0;

        for (size_t i = 0; i < crossing.size(); i++)
            for (size_t j = i + 1; j < crossing.size(); j++) {
                const Vector direction = crossing[i].to_vector().normalized();
                const Vector offset = crossing[j].center() - crossing[i].center();

                if (std::abs(offset.dot(direction)) > 59.0 && (offset - direction * offset.dot(direction)).magnitude() < 1e-6)
                    pairs++;
            }

        const BRep& brep = rib->model_geometry_brep();
        check(pairs >= 3 && brep.is_solid() && count_bores(brep) == crossing.size() - pairs, fmt::format("an inner rib bored through by its block pairs, one exact cylinder each: {} dowels, {} pairs, {} bores in {}", crossing.size(), pairs, count_bores(brep), rib->name));
        through += pairs;
    }

    size_t drilled = 0;

    for (const std::shared_ptr<Element>& element : scene.world_elements()) {
        const std::vector<SolidCut>* cuts = solid_cuts_of(*element);

        if (!cuts || cuts->empty())
            continue;

        drilled++;
        check(element->model_geometry_brep().is_solid() && count_bores(element->model_geometry_brep()) > 0, fmt::format("exact dowel bores in {}: solid {}, {} bores", element->name, element->model_geometry_brep().is_solid(), count_bores(element->model_geometry_brep())));
    }

    check(drilled == 40, "ten drilled members per quarter, not " + std::to_string(drilled));
    check(!misfits.empty(), "the apex corners of the blocks are listed as misfits");
    size_t dropped = 0;

    for (const std::string& misfit : misfits) {
        check(misfit.find("wedges_inner_beams_") != std::string::npos || misfit.find("inner_beams_") != std::string::npos, "a misfit only at a block or an inner beam: " + misfit);
        dropped += misfit.find("dropped") != std::string::npos;
    }

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    size_t loaded = 0;

    for (const std::shared_ptr<JointBeam>& set : back.get_elements<JointBeam>())
        loaded += set->drill_lines.size();

    check(loaded == dowels, "quarter dowels round trip");

    std::cout << "floor_elements: " << sets.size() << " dowel sets of " << dowels << " dowels in the quarters, " << blocks << " on the wedge blocks, " << through << " through bores in the inner ribs, every drilled member exact, " << misfits.size() << " misfits listed of which " << dropped << " dropped, round trip pass" << std::endl;
}

/// The rectangle plates between the columns and the outer ribs and the ties on the rib seams: eight and four, every carved outer rib and every tie at compas_tf's volume; the two plates of every column half-lapped by a cross lap, each slotted part an exact solid with its four dowel bores, the two touching without overlap, and the column still exact.
void check_rectangle_plates() {

    WoodSession scene("rectangle_plates");

    for (int i = 0; i < 4; i++) {
        wood_floor::add_quarter_model(scene, GUIDE, Xform::rotation_z(i * 90.0, true), nullptr, fmt::format("_{}", i));
        wood_floor::add_column_model(scene, GUIDE, Xform::rotation_z(i * 90.0, true), nullptr, fmt::format("_{}", i));
    }

    const std::vector<std::shared_ptr<JointBeam>> plates = wood_floor::add_rectangle_plates(scene, GUIDE, nullptr);
    check(plates.size() == 8, "eight rectangle plates, not " + std::to_string(plates.size()));

    for (const std::shared_ptr<BeamVariable>& beam : scene.beam_variables())
        if (beam->name.starts_with("outer_ribs_"))
            check(std::abs(compute_volume(beam->model_geometry_mesh()) - COMPAS_TF_OUTER_RIB) <= 1e-9 * COMPAS_TF_OUTER_RIB, "carved outer rib " + beam->name);

    const std::vector<std::shared_ptr<JointBeam>> laps = wood_floor::add_cross_laps(scene, plates, nullptr);
    check(laps.size() == 4, "four cross laps, not " + std::to_string(laps.size()));

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
            check(count_bores(plate->model_geometry_brep()) == 8, "the connector's BRep carries the bores and the dowels");
        }
    }

    for (const std::shared_ptr<Column>& column : scene.columns())
        check(column->model_geometry_brep().is_solid() && count_bores(column->model_geometry_brep()) == 11, "the column exact with its eight dowel and three screw bores");

    const std::vector<std::shared_ptr<JointBeam>> ties = wood_floor::add_ties(scene, nullptr);
    check(ties.size() == 4, "four ties, not " + std::to_string(ties.size()));

    for (const std::shared_ptr<BeamVariable>& beam : scene.beam_variables())
        if (beam->name.starts_with("outer_ribs_"))
            check(std::abs(compute_volume(beam->model_geometry_mesh()) - COMPAS_TF_TIED_RIB) <= 1e-9 * COMPAS_TF_TIED_RIB, "tied outer rib " + beam->name);

    for (const std::shared_ptr<JointBeam>& tie : ties) {
        Mesh key;

        for (const std::array<Polyline, 2>& part : tie->parts)
            append_mesh(key, Mesh::loft({part[0]}, {part[1]}, true));

        check(std::abs(compute_volume(key) - COMPAS_TF_TIE) <= 1e-9 * COMPAS_TF_TIE, "tie volume " + std::to_string(compute_volume(key)));
    }

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    size_t slotted = 0;

    for (const std::shared_ptr<JointBeam>& joint : back.get_elements<JointBeam>())
        slotted += joint->solid_cuts.size();

    check(slotted == 8, "the slots round trip on the plates");

    std::cout << "floor_elements: " << plates.size() << " rectangle plates half-lapped by " << laps.size() << " cross laps, drilled and exact, and " << ties.size() << " ties, carved outer ribs and ties at compas_tf's volume, round trip pass" << std::endl;
}

int main() {

    check_beams();
    check_support();
    check_wedges();
    check_dowels();
    check_quarter_dowels();
    check_rectangle_plates();

    return 0;
}
