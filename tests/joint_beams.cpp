#include "wood_session.h"
#include "wood_element_geometry.h"
#include <numbers>

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Measurements
// ═══════════════════════════════════════════════════════════════════════════
static const double KEPT_OVERLAP = 0.05;

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

/// phanomema_node as the dataset runner builds it.
static void check_phanomema_node() {

    const Settings settings = config::load_yaml("phanomema_node");
    const std::vector<double>& beams = settings.beams;
    WoodSession scene("phanomema_node");
    scene.settings = settings;
    std::vector<std::shared_ptr<Beam>> members;
    for (const Polyline& axis : io::load_obj("phanomema_node")) {
        members.push_back(std::make_shared<Beam>(
            axis,
            std::vector<double>(axis.segment_count(), beams[0]),
            std::vector<Vector>{},
            static_cast<int>(beams[1])
        ));
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
        check(
            joint->joinery.joint_type == 20,
            fmt::format(
                "{} is joined by the top-to-side family 2024 chose, not {}",
                joint->name,
                joint->joinery.joint_type
            )
        );

        const std::shared_ptr<Beam> a = scene.get_element<Beam>(joint->targets[0]);
        const std::shared_ptr<Beam> b = scene.get_element<Beam>(joint->targets[1]);
        const double before = boolean_volume(
            a->element_geometry_mesh(), b->element_geometry_mesh(), SolidOperation::intersect
        );
        const double after = boolean_volume(
            a->model_geometry_mesh(), b->model_geometry_mesh(), SolidOperation::intersect
        );
        const Mesh rest = solid_boolean(
            a->model_geometry_mesh(), b->model_geometry_mesh(), SolidOperation::intersect
        );
        const Mesh zone = solid_boolean(
            Mesh::loft({joint->feature.volumes[0]}, {joint->feature.volumes[1]}, true),
            Mesh::loft({joint->feature.volumes[2]}, {joint->feature.volumes[3]}, true),
            SolidOperation::add
        );
        const double in_zone = rest.number_of_faces() == 0
                                   ? 0.0
                                   : boolean_volume(rest, zone, SolidOperation::intersect);
        check(
            before > 0.0, fmt::format("{} and {} overlapped before their joint", a->name, b->name)
        );
        const Vector first = joint->insertion(0);
        const Vector second = joint->insertion(1);
        check(
            std::abs(first.magnitude() - 1.0) <= 1e-9 && std::abs(first.dot(second) + 1.0) <= 1e-9,
            fmt::format(
                "{} and {} come apart in opposite unit directions, their dot {:.9f}",
                a->name,
                b->name,
                first.dot(second)
            )
        );

        check(
            after <= KEPT_OVERLAP * before,
            fmt::format(
                "{} and {} still overlap by {:.1f} of their {:.1f} mm3 after the joint",
                a->name,
                b->name,
                after,
                before
            )
        );
        std::cout << fmt::format(
                         "joint_beams: {} and {} overlapped {:.0f} mm3, {:.0f} after the joint, "
                         "{:.0f} of it inside the joint's zone",
                         a->name,
                         b->name,
                         before,
                         after,
                         in_zone
                     )
                  << std::endl;
    }
    check(joints == 5, fmt::format("{} beam joints, not the reference's 5", joints));

    const WoodSession restored = WoodSession::pb_loads(scene.pb_dumps());
    for (const std::shared_ptr<Element>& element : *scene.objects.elements) {
        const std::shared_ptr<JointBeam> joint = std::dynamic_pointer_cast<JointBeam>(element);
        if (!joint || joint->is_connector())
            continue;
        const std::shared_ptr<JointBeam> twin = restored.get_element<JointBeam>(joint->guid());
        check(
            twin && (twin->feature.normal - joint->feature.normal).magnitude() <= 1e-9 &&
                (twin->feature.axes[1] - joint->feature.axes[1]).magnitude() <= 1e-9 &&
                (twin->insertion(0) - joint->insertion(0)).magnitude() <= 1e-9,
            joint->name + " keeps its normal, its axes and its insertions through the file"
        );
    }

    for (const std::shared_ptr<Beam>& member : members) {
        const Mesh& model = member->model_geometry_mesh();
        const double lost = compute_volume(member->element_geometry_mesh()) - compute_volume(model);
        check(model.is_closed(), member->name + " is no closed solid after its joints");
        check(lost > 0.0, fmt::format("{} lost {:.1f} mm3 to its joints", member->name, lost));
        std::cout << fmt::format(
                         "joint_beams: {} lost {:.0f} mm3 to its joints", member->name, lost
                     )
                  << std::endl;
    }

    std::cout << "joint_beams: phanomema_node's five pairs joined and cut, each pair's overlap at "
                 "least 95 % gone"
              << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// Cross joints on beams: the half-lap and the conic half-laps of the thesis (Fig 5.51, 5.55)
// ═══════════════════════════════════════════════════════════════════════════

/// Two beams of half-width 75 crossing in plan at 90, 60 and 45 degrees, joined by each cross design cross_slot to 5 (ids 30 to 35): the pair's overlap is gone, the beams lost at least their overlap, each a closed solid, and the designs with drills bore the beams with exact cylinders.
static void check_crossings() {

    for (const double angle : {90.0, 60.0, 45.0}) {
        for (int id = 30; id <= 35; id++) {
            WoodSession scene(fmt::format("crossing_{:g}_{}", angle, id));
            scene.settings.joint_parameters[11] = id;
            const double turn = angle * std::numbers::pi / 180.0;
            const Vector along(std::cos(turn), std::sin(turn), 0.0);
            const std::shared_ptr<Beam> a = std::make_shared<Beam>(
                Polyline({Point(-600.0, 0.0, 0.0), Point(600.0, 0.0, 0.0)}), 75.0, "beam"
            );
            const std::shared_ptr<Beam> b = std::make_shared<Beam>(
                Polyline(
                    {Point(0.0, 0.0, 0.0) - along * 600.0, Point(0.0, 0.0, 0.0) + along * 600.0}
                ),
                75.0,
                "beam"
            );
            a->name = "beam_a";
            b->name = "beam_b";
            scene.add(a);
            scene.add(b);
            scene.compute_axis_contacts(20.0);
            scene.compute_beam_features(500.0, 0.91, 1);

            std::shared_ptr<JointBeam> joint;
            for (const std::shared_ptr<Element>& element : *scene.objects.elements)
                if (std::shared_ptr<JointBeam> found =
                        std::dynamic_pointer_cast<JointBeam>(element))
                    joint = found;
            check(
                joint != nullptr,
                fmt::format(
                    "the beams crossing at {:g} degrees are joined by cross id {}", angle, id
                )
            );
            const std::array<std::string, 6> designs = {
                "cross_slot",
                "cross_sliced_half_lap",
                "cross_half_lap",
                "cross_half_lap_diagonal_drills",
                "cross_half_lap_drill",
                "cross_brussels_half_lap"
            };
            const std::string design = designs[id - 30];
            check(
                joint->joinery.name == design,
                fmt::format(
                    "the crossing at {:g} degrees takes {}, not {}",
                    angle,
                    design,
                    joint->joinery.name
                )
            );

            const double before = boolean_volume(
                a->element_geometry_mesh(), b->element_geometry_mesh(), SolidOperation::intersect
            );
            const double after = boolean_volume(
                a->model_geometry_mesh(), b->model_geometry_mesh(), SolidOperation::intersect
            );
            check(
                after <= KEPT_OVERLAP * before,
                fmt::format(
                    "id {} at {:g} degrees leaves {:.1f} of the {:.1f} mm3 overlap",
                    id,
                    angle,
                    after,
                    before
                )
            );

            double lost = 0.0;
            for (const std::shared_ptr<Beam>& member : {a, b}) {
                const Mesh& model = member->model_geometry_mesh();
                check(
                    model.is_closed(),
                    fmt::format(
                        "{} is a closed solid after id {} at {:g} degrees", member->name, id, angle
                    )
                );
                lost += compute_volume(member->element_geometry_mesh()) - compute_volume(model);
            }
            check(
                lost >= before * (1.0 - KEPT_OVERLAP),
                fmt::format(
                    "the beams lost {:.0f} mm3 to id {} at {:g} degrees, less than their overlap "
                    "{:.0f}",
                    lost,
                    id,
                    angle,
                    before
                )
            );

            size_t cylinders = 0;
            for (const std::shared_ptr<Beam>& member : {a, b})
                for (const BRepFace& face : member->model_geometry_brep().m_faces)
                    cylinders +=
                        member->model_geometry_brep().m_surfaces[face.surface_index].is_rational();
            if (id >= 33)
                check(
                    cylinders > 0,
                    fmt::format("id {} at {:g} degrees bores no exact cylinder", id, angle)
                );

            std::cout << fmt::format(
                             "joint_beams: cross id {} at {:g} degrees: overlap {:.0f} to {:.0f} "
                             "mm3, the beams lost {:.0f}, {} exact bores",
                             id,
                             angle,
                             before,
                             after,
                             lost,
                             cylinders
                         )
                      << std::endl;
        }
    }
}

/// A beam ending on the side of another, square and at 60 degrees, joined by the wedge top_side_milled_wedge (id 24), whose milled pockets and wedge flanks are solids: both beams cut, closed, their overlap gone.
static void check_tees() {

    for (const double angle : {90.0, 60.0}) {
        for (const int id : {20, 21, 24}) {
            WoodSession scene(fmt::format("tee_{:g}_{}", angle, id));
            scene.settings.joint_parameters[8] = id;
            const double turn = angle * std::numbers::pi / 180.0;
            const Vector along(std::cos(turn), std::sin(turn), 0.0);
            const std::shared_ptr<Beam> a = std::make_shared<Beam>(
                Polyline({Point(-600.0, 0.0, 0.0), Point(600.0, 0.0, 0.0)}), 75.0, "beam"
            );
            const std::shared_ptr<Beam> b = std::make_shared<Beam>(
                Polyline({Point(0.0, 0.0, 0.0), Point(0.0, 0.0, 0.0) + along * 600.0}), 75.0, "beam"
            );
            a->name = "beam_a";
            b->name = "beam_b";
            scene.add(a);
            scene.add(b);
            scene.compute_axis_contacts(20.0);
            scene.compute_beam_features(500.0, 0.91, 1);

            std::shared_ptr<JointBeam> joint;
            for (const std::shared_ptr<Element>& element : *scene.objects.elements)
                if (std::shared_ptr<JointBeam> found =
                        std::dynamic_pointer_cast<JointBeam>(element))
                    joint = found;
            check(
                joint != nullptr,
                fmt::format("the tee at {:g} degrees is joined by id {}", angle, id)
            );
            const double before = boolean_volume(
                a->element_geometry_mesh(), b->element_geometry_mesh(), SolidOperation::intersect
            );
            const double after = boolean_volume(
                a->model_geometry_mesh(), b->model_geometry_mesh(), SolidOperation::intersect
            );
            double lost = 0.0;
            for (const std::shared_ptr<Beam>& member : {a, b}) {
                check(
                    member->model_geometry_mesh().is_closed(),
                    fmt::format(
                        "{} is a closed solid after id {} at {:g} degrees", member->name, id, angle
                    )
                );
                lost += compute_volume(member->element_geometry_mesh()) -
                        compute_volume(member->model_geometry_mesh());
            }
            std::cout << fmt::format(
                             "joint_beams: tee {} ({}) at {:g} degrees: overlap {:.0f} to {:.0f} "
                             "mm3, the beams lost {:.0f}",
                             joint->joinery.name,
                             id,
                             angle,
                             before,
                             after,
                             lost
                         )
                      << std::endl;
            check(
                after <= KEPT_OVERLAP * before,
                fmt::format(
                    "{} at {:g} degrees leaves {:.1f} of the {:.1f} mm3 overlap",
                    joint->joinery.name,
                    angle,
                    after,
                    before
                )
            );
        }
    }
}

/// Two extended beams of half-width 75 meeting end to end.
static void check_end_to_end() {

    const std::array<std::pair<double, std::string>, 4> cases = {
        {{0.0, "side_side_inplane_splayed_scarf"},
         {0.0, "side_side_inplane_cross_splayed_scarf"},
         {90.0, "side_side_rotated_miter"},
         {60.0, "side_side_rotated_miter"}}
    };
    for (const std::pair<double, std::string>& end_case : cases) {
        WoodSession scene(fmt::format("end_to_end_{:g}_{}", end_case.first, end_case.second));
        scene.settings.joint_parameters[2] = JointPlate::library_id(end_case.second);
        const std::array<std::shared_ptr<Beam>, 2> pair = Beam::pair_end_to_end(end_case.first);
        scene.add(pair[0]);
        scene.add(pair[1]);
        const InteractionContactAxis contact(
            Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}), 1.0, 0.0, 0, 0, 0, 0
        );
        const std::shared_ptr<JointBeam> joint =
            JointBeam::from_contact(*pair[0], *pair[1], contact, 600.0, 0.91, 0, scene.settings);
        check(
            joint != nullptr && joint->joinery.name == end_case.second,
            fmt::format("the beams at {:g} degrees take {}", end_case.first, end_case.second)
        );
        scene.add(joint);
        scene.add_interaction(joint, pair[0], joint->interaction(0));
        scene.add_interaction(joint, pair[1], joint->interaction(1));

        const double before = boolean_volume(
            pair[0]->element_geometry_mesh(),
            pair[1]->element_geometry_mesh(),
            SolidOperation::intersect
        );
        const double after = boolean_volume(
            pair[0]->model_geometry_mesh(),
            pair[1]->model_geometry_mesh(),
            SolidOperation::intersect
        );
        double kept = 0.0;
        for (const std::shared_ptr<Beam>& member : pair) {
            check(
                member->model_geometry_mesh().is_closed(),
                fmt::format(
                    "{} is a closed solid after {} at {:g} degrees",
                    member->name,
                    end_case.second,
                    end_case.first
                )
            );
            kept += compute_volume(member->model_geometry_mesh());
        }
        const double axes = 2.0 * (1200.0 + 150.0) * 150.0 * 150.0;
        std::cout << fmt::format(
                         "joint_beams: {} at {:g} degrees: overlap {:.0f} to {:.0f} mm3, the pair "
                         "keeps {:.0f} of its axes' and free ends' {:.0f}",
                         end_case.second,
                         end_case.first,
                         before,
                         after,
                         kept,
                         axes
                     )
                  << std::endl;
        check(
            before > 0.0 && after <= 1e-6 * before,
            fmt::format(
                "{} at {:g} degrees leaves {:.1f} of the {:.1f} mm3 overlap",
                end_case.second,
                end_case.first,
                after,
                before
            )
        );
        check(
            std::abs(kept - axes) <= 0.01 * axes,
            fmt::format(
                "{} at {:g} degrees keeps {:.0f} mm3, not its axes' {:.0f}",
                end_case.second,
                end_case.first,
                kept,
                axes
            )
        );
    }
}

int main() {

    check_phanomema_node();
    check_crossings();
    check_tees();
    check_end_to_end();
    return 0;
}
