#include "wood_session.h"
#include "wood_element_geometry.h"
#include "oracle_polyline.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// The boundary family against the 2025 reference solver
// ═══════════════════════════════════════════════════════════════════════════
static const double MATCH = 1e-6;

/// Throws with the message when the condition fails.
static void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
}

/// The slice rectangle at x over the whole side face 2 of the 300 x 200 x 40 plate, the side on x = 0: y from -1 to 201, z from -1 to 41.
static Polyline reference_rectangle(double x) {

    return Polyline(
        {{x, -1.0, 41.0}, {x, -1.0, -1.0}, {x, 201.0, -1.0}, {x, 201.0, 41.0}, {x, -1.0, 41.0}}
    );
}

/// border on a border contact of a plate's side face.
static void check_border() {

    WoodSession scene("border");
    const Polyline bottom(
        {{0.0, 0.0, 0.0},
         {300.0, 0.0, 0.0},
         {300.0, 200.0, 0.0},
         {0.0, 200.0, 0.0},
         {0.0, 0.0, 0.0}}
    );
    const std::shared_ptr<FacePlanarElement> plate = std::make_shared<FacePlanarElement>(
        bottom, bottom.transformed(Xform::translation(0.0, 0.0, 40.0)), "plate"
    );
    scene.add(plate);

    const std::shared_ptr<InteractionContactFace> contact =
        WoodSession::compute_border_contact(*plate, 2);
    check(
        contact != nullptr && contact->type == ContactType::border,
        "a border contact on side face 2"
    );
    const std::shared_ptr<JointPlateElement> joint = JointPlateElement::border();
    joint->orient(contact, {plate}, scene.settings);
    scene.add(joint);
    scene.add_interaction(joint, plate, joint->interaction(0));

    const InteractionFeaturePlate& connection = joint->connections.at(0);
    check(
        connection.joint_type == 60 && connection.element_a == plate->guid() &&
            connection.element_b == plate->guid(),
        "a family 60 joint on the plate alone"
    );

    // the slices: the pair out of the plate, a quarter millimetre and 16 off the face, then the pair into it
    const std::array<Polyline, 4> written = {
        connection.male_outlines[0].at(0),
        connection.male_outlines[1].at(0),
        connection.male_outlines[0].at(2),
        connection.male_outlines[1].at(2)
    };
    const std::array<double, 4> levels = {-0.25, -16.0, 0.25, 16.0};
    for (size_t k = 0; k < 4; k++) {
        const double distance =
            oracle::point_set_distance(written[k], reference_rectangle(levels[k]));
        check(
            distance <= MATCH,
            fmt::format(
                "slice {} lies {:.6g} mm from the reference rectangle at x = {:g}",
                k,
                distance,
                levels[k]
            )
        );
    }

    // the slab between the inner pair, 15.75 x 200 x 40, is cut off the plate; the outer pair lies in the air
    const double stock = compute_volume(plate->element_geometry_mesh());
    const double model = compute_volume(plate->model_geometry_mesh());
    const double slab = 15.75 * 200.0 * 40.0;
    check(
        std::abs((stock - model) - slab) <= 0.02 * slab,
        fmt::format(
            "the plate loses its border slab, {:.0f} of {:.0f} mm3 cut", stock - model, slab
        )
    );

    std::cout
        << "joint_border: border on a border contact writes four slices over the whole side face "
           "and cuts the 16 mm slab off the plate"
        << std::endl;
}

/// The dataset boundary_side.
static void check_dataset() {

    config::reset_defaults();
    WoodSession scene = WoodSession::yaml_load("boundary_side");
    const std::vector<InteractionFeaturePlate> joints = scene.compute_features();
    check(
        joints.size() == 1 && joints[0].joint_type == 60 && joints[0].name == "border",
        fmt::format(
            "{} joints, the first of type {}, not one border of type 60",
            joints.size(),
            joints.empty() ? -1 : joints[0].joint_type
        )
    );

    const InteractionFeaturePlate& joint = joints[0];
    const std::array<Polyline, 4> written = {
        joint.male_outlines[0].at(0),
        joint.male_outlines[1].at(0),
        joint.male_outlines[0].at(2),
        joint.male_outlines[1].at(2)
    };
    const std::array<double, 4> levels = {-0.25, -16.0, 0.25, 16.0};
    for (size_t k = 0; k < 4; k++) {
        const double distance =
            oracle::point_set_distance(written[k], reference_rectangle(levels[k]));
        check(
            distance <= MATCH,
            fmt::format(
                "the dataset's slice {} lies {:.6g} mm from the reference rectangle at x = {:g}",
                k,
                distance,
                levels[k]
            )
        );
    }

    std::cout << "joint_border: boundary_side's self-adjacency makes the reference's border "
                 "through compute_features"
              << std::endl;
}

int main() {

    try {
        check_border();
        check_dataset();
    } catch (const std::exception& error) {
        std::cerr << "joint_border: " << error.what() << std::endl;
        return 1;
    }

    return 0;
}
