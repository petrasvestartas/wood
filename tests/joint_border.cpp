#include "wood_session.h"
#include "wood_element_geometry.h"
#include "oracle_polyline.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// The boundary family against the 2025 reference solver
// ═══════════════════════════════════════════════════════════════════════════

static const double MATCH = 1e-6; // mm, a rectangle against the reference's

/// Throws with the message when the condition fails.
static void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
}

/// The rectangle at y across x from 0 to -6 and z from -1 to 41: what the 2025 solver wrote for b_0 on side face 2 of a 300 x 200 x 40 plate
/// whose adjacency row is [0, 0, 2, 2] (compas_wood 2.4.0, get_connection_zones, output type 3).
static Polyline reference_rectangle(double y) {

    return Polyline({{0.0, y, -1.0}, {-6.0, y, -1.0}, {-6.0, y, 41.0}, {0.0, y, 41.0}, {0.0, y, -1.0}});
}

/// b_0 on a border contact of a plate's side face: oriented on the plate alone, its four slice rectangles are the reference's, 0.25 and 16
/// either side of the face's middle, 6 out of the face and a millimetre past the plate's faces, and the plate keeps its stock, as 2024 merged
/// nothing of a boundary joint into its plate.
static void check_b_0() {

    WoodSession scene("border");
    const Polyline bottom({{0.0, 0.0, 0.0}, {300.0, 0.0, 0.0}, {300.0, 200.0, 0.0}, {0.0, 200.0, 0.0}, {0.0, 0.0, 0.0}});
    const std::shared_ptr<Plate> plate = std::make_shared<Plate>(bottom, bottom.transformed(Xform::translation(0.0, 0.0, 40.0)), "plate");
    scene.add(plate);

    const std::shared_ptr<InteractionContactFace> contact = WoodSession::compute_border_contact(*plate, 2);
    check(contact != nullptr && contact->type == ContactType::border, "a border contact on side face 2");
    const std::shared_ptr<JointPlate> joint = JointPlate::b_0();
    joint->orient(contact, {plate}, scene.settings);
    scene.add(joint);
    scene.add_interaction(joint, plate, joint->interaction(0));

    const InteractionFeaturePlate& connection = joint->connections.at(0);
    check(connection.joint_type == 60 && connection.element_a == plate->guid() && connection.element_b == plate->guid(), "a family 60 joint on the plate alone");

    // the slices as 2024 wrote them: rect0 and rect1 the first pair, rect2 and rect3 the second
    const std::array<Polyline, 4> written = {connection.male_outlines[0].at(0), connection.male_outlines[1].at(0), connection.male_outlines[0].at(2), connection.male_outlines[1].at(2)};
    const std::array<double, 4> levels = {100.25, 116.0, 99.75, 84.0};
    for (size_t k = 0; k < 4; k++) {
        const double distance = oracle::point_set_distance(written[k], reference_rectangle(levels[k]));
        check(distance <= MATCH, fmt::format("slice {} lies {:.6g} mm from the reference rectangle at y = {:g}", k, distance, levels[k]));
    }

    const double stock = compute_volume(plate->element_geometry_mesh());
    const double model = compute_volume(plate->model_geometry_mesh());
    check(std::abs(model - stock) <= 1e-9 * stock, fmt::format("the plate keeps its stock, {:.3f} of {:.3f} mm3", model, stock));

    std::cout << "joint_border: b_0 on a border contact writes the reference's four slices, 6 out of the side face, and leaves the plate whole" << std::endl;
}

/// The dataset boundary_side, one such plate whose adjacency row 0 0 2 2 pairs it with itself on side face 2: compute_features makes the
/// family 60 joint 2024's border_to_face made, b_0 by the row 6 default, with the same four slices.
static void check_dataset() {

    config::reset_defaults();
    WoodSession scene = WoodSession::yaml_load("boundary_side");
    const std::vector<InteractionFeaturePlate> joints = scene.compute_features();
    check(joints.size() == 1 && joints[0].joint_type == 60 && joints[0].name == "b_0", fmt::format("{} joints, the first of type {}, not one b_0 of type 60", joints.size(), joints.empty() ? -1 : joints[0].joint_type));

    const InteractionFeaturePlate& joint = joints[0];
    const std::array<Polyline, 4> written = {joint.male_outlines[0].at(0), joint.male_outlines[1].at(0), joint.male_outlines[0].at(2), joint.male_outlines[1].at(2)};
    const std::array<double, 4> levels = {100.25, 116.0, 99.75, 84.0};
    for (size_t k = 0; k < 4; k++) {
        const double distance = oracle::point_set_distance(written[k], reference_rectangle(levels[k]));
        check(distance <= MATCH, fmt::format("the dataset's slice {} lies {:.6g} mm from the reference rectangle at y = {:g}", k, distance, levels[k]));
    }

    std::cout << "joint_border: boundary_side's self-adjacency makes the reference's b_0 through compute_features" << std::endl;
}

int main() {

    check_b_0();
    check_dataset();
    return 0;
}
