#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

const wood_floor::FloorGuide GUIDE{
    .size_column_head = 220.0,
    .size_column_head_chamfer = 120.0,
    .size_wedge = 240.0,
};

const double EXACT_SUPPORT = 500671.261678; // compas_tf SupportElement.brep volume, exact cylinders and hexagons
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

/// The wedges between the inner beams and the oculus: eight, and the joints with their parts and cutters, and the carved beams, through a round trip.
void check_wedges() {

    WoodSession scene("wedges");

    for (int i = 0; i < 4; i++)
        wood_floor::add_quarter_model(scene, GUIDE, Xform::rotation_z(i * 90.0, true), nullptr, fmt::format("_{}", i));

    wood_floor::add_oculus_model(scene, GUIDE, nullptr);
    const std::vector<std::shared_ptr<JointBeam>> wedges = wood_floor::add_wedges(scene, GUIDE, nullptr);
    check(wedges.size() == 8, "eight wedges, not " + std::to_string(wedges.size()));

    std::map<std::string, double> volumes;

    for (const std::shared_ptr<BeamVariable>& beam : scene.beam_variables())
        volumes[beam->guid()] = compute_volume(beam->model_geometry_mesh());

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    size_t loaded = 0;

    for (const std::shared_ptr<JointBeam>& joint : back.get_elements<JointBeam>()) {
        check(joint->parts.size() == 1 && joint->cutters.size() == 2 && !joint->drill_lines.empty(), "wedge round trip");
        loaded++;
    }

    check(loaded == wedges.size(), "wedge round trip count");

    for (const std::shared_ptr<BeamVariable>& beam : back.beam_variables())
        check(std::abs(compute_volume(beam->model_geometry_mesh()) - volumes.at(beam->guid())) <= 1e-9 * volumes.at(beam->guid()), "carved beam round trip " + beam->name);

    std::cout << "floor_elements: " << wedges.size() << " wedges, joints and carved beams through a round trip pass" << std::endl;
}

int main() {

    check_beams();
    check_support();
    check_wedges();

    return 0;
}
