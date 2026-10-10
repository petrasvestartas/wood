#include "wood_session.h"
#include "wood_element_geometry.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Measurements
// ═══════════════════════════════════════════════════════════════════════════

static const double KEPT_OVERLAP = 0.05; // the share of a pair's overlap its joint may leave: the logs overlap beyond 2024's volumes, where no joint acts, and inside them the boxes' tenons and mortises do not nest yet (open)

/// Throws with the message when the condition fails.
static void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
}

/// The volume of a boolean of two solids, zero for an empty result.
static double boolean_volume(const Mesh& a, const Mesh& b, SolidOperation operation) {

    const Mesh result = solid_boolean(a, b, operation);
    return result.number_of_faces() == 0 ? 0.0 : compute_volume(result);
}

// ═══════════════════════════════════════════════════════════════════════════
// The beam dataset: every pair joined as 2024 joined it, and cut
// ═══════════════════════════════════════════════════════════════════════════

/// phanomema_node as the dataset runner builds it: five pairs of round logs at a node, each joined by the top-to-side joint 2024 put on the
/// boxes of its volumes; every beam on a joint is cut and stays a closed solid, and each pair keeps at most KEPT_OVERLAP of its overlap.
static void check_phanomema_node() {

    const Settings settings = config::load_yaml("phanomema_node");
    const std::vector<double>& beams = settings.beams;
    WoodSession scene("phanomema_node");
    scene.settings = settings;
    std::vector<std::shared_ptr<Beam>> members;
    for (const Polyline& axis : io::load_obj("phanomema_node")) {
        members.push_back(std::make_shared<Beam>(axis, std::vector<double>(axis.segment_count(), beams[0]), std::vector<Vector>{}, static_cast<int>(beams[1])));
        members.back()->name = fmt::format("beam_{}", members.size() - 1);
        scene.add(members.back());
    }
    scene.compute_axis_contacts(beams[2]);
    scene.compute_beam_features(beams[3], beams[4], static_cast<int>(beams[5]));

    size_t joints = 0;
    for (const std::shared_ptr<Element>& element : *scene.objects.elements) {
        const std::shared_ptr<JointBeam> joint = std::dynamic_pointer_cast<JointBeam>(element);
        if (!joint || joint->is_connector())
            continue;
        joints++;
        check(joint->joinery.joint_type == 20, fmt::format("{} is joined by the top-to-side family 2024 chose, not {}", joint->name, joint->joinery.joint_type));

        const std::shared_ptr<Beam> a = scene.get_element<Beam>(joint->targets[0]);
        const std::shared_ptr<Beam> b = scene.get_element<Beam>(joint->targets[1]);
        const double before = boolean_volume(a->element_geometry_mesh(), b->element_geometry_mesh(), SolidOperation::intersect);
        const double after = boolean_volume(a->model_geometry_mesh(), b->model_geometry_mesh(), SolidOperation::intersect);
        // the overlap left inside the zone of the two boxes, where the joint acts; outside it the logs overlap beyond 2024's volumes
        const Mesh rest = solid_boolean(a->model_geometry_mesh(), b->model_geometry_mesh(), SolidOperation::intersect);
        const Mesh zone = solid_boolean(Mesh::loft({joint->feature.volumes[0]}, {joint->feature.volumes[1]}, true), Mesh::loft({joint->feature.volumes[2]}, {joint->feature.volumes[3]}, true), SolidOperation::add);
        const double in_zone = rest.number_of_faces() == 0 ? 0.0 : boolean_volume(rest, zone, SolidOperation::intersect);
        check(before > 0.0, fmt::format("{} and {} overlapped before their joint", a->name, b->name));
        // the two beams come apart in opposite directions, each away from the other's box
        const Vector first = joint->insertion(0);
        const Vector second = joint->insertion(1);
        check(std::abs(first.magnitude() - 1.0) <= 1e-9 && std::abs(first.dot(second) + 1.0) <= 1e-9, fmt::format("{} and {} come apart in opposite unit directions, their dot {:.9f}", a->name, b->name, first.dot(second)));

        check(after <= KEPT_OVERLAP * before, fmt::format("{} and {} still overlap by {:.1f} of their {:.1f} mm3 after the joint", a->name, b->name, after, before));
        std::cout << fmt::format("joint_beams: {} and {} overlapped {:.0f} mm3, {:.0f} after the joint, {:.0f} of it inside the joint's zone", a->name, b->name, before, after, in_zone) << std::endl;
    }
    check(joints == 5, fmt::format("{} beam joints, not the reference's 5", joints));

    // the axes' normal and directions where they meet travel with the joint through the file
    const WoodSession restored = WoodSession::pb_loads(scene.pb_dumps());
    for (const std::shared_ptr<Element>& element : *scene.objects.elements) {
        const std::shared_ptr<JointBeam> joint = std::dynamic_pointer_cast<JointBeam>(element);
        if (!joint || joint->is_connector())
            continue;
        const std::shared_ptr<JointBeam> twin = restored.get_element<JointBeam>(joint->guid());
        check(twin && (twin->feature.normal - joint->feature.normal).magnitude() <= 1e-9 && (twin->feature.axes[1] - joint->feature.axes[1]).magnitude() <= 1e-9 && (twin->insertion(0) - joint->insertion(0)).magnitude() <= 1e-9,
              joint->name + " keeps its normal, its axes and its insertions through the file");
    }

    for (const std::shared_ptr<Beam>& member : members) {
        const Mesh& model = member->model_geometry_mesh();
        const double lost = compute_volume(member->element_geometry_mesh()) - compute_volume(model);
        check(model.is_closed(), member->name + " is no closed solid after its joints");
        check(lost > 0.0, fmt::format("{} lost {:.1f} mm3 to its joints", member->name, lost));
        std::cout << fmt::format("joint_beams: {} lost {:.0f} mm3 to its joints", member->name, lost) << std::endl;
    }

    std::cout << "joint_beams: phanomema_node's five pairs joined and cut, each pair's overlap at least 95 % gone" << std::endl;
}

int main() {

    check_phanomema_node();
    return 0;
}
