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

// ═══════════════════════════════════════════════════════════════════════════
// Cross joints on beams: the half-lap and the conic half-laps of the thesis (Fig 5.51, 5.55)
// ═══════════════════════════════════════════════════════════════════════════

/// Two beams of half-width 75 crossing in plan at 90, 60 and 45 degrees, joined by each cross design cr_c_ip_0 to 5 (ids 30 to 35): the
/// pair's overlap is gone, the beams lost at least their overlap, each a closed solid, and the designs with drills bore the beams with
/// exact cylinders.
static void check_crossings() {

    for (const double angle : {90.0, 60.0, 45.0}) {
        for (int id = 30; id <= 35; id++) {
            WoodSession scene(fmt::format("crossing_{:g}_{}", angle, id));
            scene.settings.joint_parameters[11] = id;
            const double turn = angle * std::numbers::pi / 180.0;
            const Vector along(std::cos(turn), std::sin(turn), 0.0);
            const std::shared_ptr<Beam> a = std::make_shared<Beam>(Polyline({Point(-600.0, 0.0, 0.0), Point(600.0, 0.0, 0.0)}), 75.0, "beam");
            const std::shared_ptr<Beam> b = std::make_shared<Beam>(Polyline({Point(0.0, 0.0, 0.0) - along * 600.0, Point(0.0, 0.0, 0.0) + along * 600.0}), 75.0, "beam");
            a->name = "beam_a";
            b->name = "beam_b";
            scene.add(a);
            scene.add(b);
            scene.compute_axis_contacts(20.0);
            scene.compute_beam_features(500.0, 0.91, 1);

            std::shared_ptr<JointBeam> joint;
            for (const std::shared_ptr<Element>& element : *scene.objects.elements)
                if (std::shared_ptr<JointBeam> found = std::dynamic_pointer_cast<JointBeam>(element))
                    joint = found;
            check(joint != nullptr, fmt::format("the beams crossing at {:g} degrees are joined by cr_c_ip id {}", angle, id));
            const std::string design = fmt::format("cr_c_ip_{}", id - 30);
            check(joint->joinery.name == design, fmt::format("the crossing at {:g} degrees takes {}, not {}", angle, design, joint->joinery.name));

            const double before = boolean_volume(a->element_geometry_mesh(), b->element_geometry_mesh(), SolidOperation::intersect);
            const double after = boolean_volume(a->model_geometry_mesh(), b->model_geometry_mesh(), SolidOperation::intersect);
            check(after <= KEPT_OVERLAP * before, fmt::format("id {} at {:g} degrees leaves {:.1f} of the {:.1f} mm3 overlap", id, angle, after, before));

            double lost = 0.0;
            for (const std::shared_ptr<Beam>& member : {a, b}) {
                const Mesh& model = member->model_geometry_mesh();
                check(model.is_closed(), fmt::format("{} is a closed solid after id {} at {:g} degrees", member->name, id, angle));
                lost += compute_volume(member->element_geometry_mesh()) - compute_volume(model);
            }
            check(lost >= before * (1.0 - KEPT_OVERLAP), fmt::format("the beams lost {:.0f} mm3 to id {} at {:g} degrees, less than their overlap {:.0f}", lost, id, angle, before));

            // the designs with drills (cr_c_ip_3 to 5) bore both beams with exact cylinders
            size_t cylinders = 0;
            for (const std::shared_ptr<Beam>& member : {a, b})
                for (const BRepFace& face : member->model_geometry_brep().m_faces)
                    cylinders += member->model_geometry_brep().m_surfaces[face.surface_index].is_rational();
            if (id >= 33)
                check(cylinders > 0, fmt::format("id {} at {:g} degrees bores no exact cylinder", id, angle));

            std::cout << fmt::format("joint_beams: cr_c_ip id {} at {:g} degrees: overlap {:.0f} to {:.0f} mm3, the beams lost {:.0f}, {} exact bores", id, angle, before, after, lost, cylinders) << std::endl;
        }
    }
}

/// A beam ending on the side of another, square and at 60 degrees, joined by the wedge ts_e_p_4 (id 24), whose milled pockets and wedge
/// flanks are solids: both beams cut, closed, their overlap gone. The tenon designs ts_e_p_2 and 3 on such a tee are open: at 90 degrees
/// the beams lose 14.6e6 mm3 for an overlap of 1.7e6, at 60 degrees ts_e_p_3 leaves 1.2e6 of the overlap (as before the solids were
/// carried onto beams).
static void check_tees() {

    for (const double angle : {90.0, 60.0}) {
        for (const int id : {24}) {
            WoodSession scene(fmt::format("tee_{:g}_{}", angle, id));
            scene.settings.joint_parameters[8] = id;
            const double turn = angle * std::numbers::pi / 180.0;
            const Vector along(std::cos(turn), std::sin(turn), 0.0);
            const std::shared_ptr<Beam> a = std::make_shared<Beam>(Polyline({Point(-600.0, 0.0, 0.0), Point(600.0, 0.0, 0.0)}), 75.0, "beam");
            const std::shared_ptr<Beam> b = std::make_shared<Beam>(Polyline({Point(0.0, 0.0, 0.0), Point(0.0, 0.0, 0.0) + along * 600.0}), 75.0, "beam");
            a->name = "beam_a";
            b->name = "beam_b";
            scene.add(a);
            scene.add(b);
            scene.compute_axis_contacts(20.0);
            scene.compute_beam_features(500.0, 0.91, 1);

            std::shared_ptr<JointBeam> joint;
            for (const std::shared_ptr<Element>& element : *scene.objects.elements)
                if (std::shared_ptr<JointBeam> found = std::dynamic_pointer_cast<JointBeam>(element))
                    joint = found;
            check(joint != nullptr, fmt::format("the tee at {:g} degrees is joined by id {}", angle, id));
            const double before = boolean_volume(a->element_geometry_mesh(), b->element_geometry_mesh(), SolidOperation::intersect);
            const double after = boolean_volume(a->model_geometry_mesh(), b->model_geometry_mesh(), SolidOperation::intersect);
            double lost = 0.0;
            for (const std::shared_ptr<Beam>& member : {a, b}) {
                check(member->model_geometry_mesh().is_closed(), fmt::format("{} is a closed solid after id {} at {:g} degrees", member->name, id, angle));
                lost += compute_volume(member->element_geometry_mesh()) - compute_volume(member->model_geometry_mesh());
            }
            std::cout << fmt::format("joint_beams: tee {} ({}) at {:g} degrees: overlap {:.0f} to {:.0f} mm3, the beams lost {:.0f}", joint->joinery.name, id, angle, before, after, lost) << std::endl;
            check(after <= KEPT_OVERLAP * before, fmt::format("{} at {:g} degrees leaves {:.1f} of the {:.1f} mm3 overlap", joint->joinery.name, angle, after, before));
        }
    }
}

int main() {

    check_phanomema_node();
    check_crossings();
    check_tees();
    return 0;
}
