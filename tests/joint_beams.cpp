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
                "cross_half_lap",
                "cross_half_lap",
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

/// The area of a plan polygon.
static double plan_area(const std::vector<std::array<double, 2>>& plan) {

    double area = 0.0;
    for (size_t i = 0; i < plan.size(); i++) {
        const std::array<double, 2>& a = plan[i];
        const std::array<double, 2>& b = plan[(i + 1) % plan.size()];
        area += a[0] * b[1] - b[0] * a[1];
    }

    return 0.5 * std::abs(area);
}

/// The skew T-bridle top_side_bridle (id 26) at the photographed 57 degrees, either side of it, square and mirrored: no fallback to another design, both beams closed, no overlap
/// seated, free only along the slide, square to the first beam out of its near face, and at 57 degrees the wood each beam keeps as computed by hand.
static void check_bridles() {

    const double width = 100.0;
    for (const double angle : {57.0, 45.0, 75.0, 90.0, 123.0}) {
        WoodSession scene(fmt::format("bridle_{:g}", angle));
        scene.settings.joint_parameters[7] = 0.3;
        scene.settings.joint_parameters[8] = JointPlate::library_id("top_side_bridle");
        const double turn = angle * std::numbers::pi / 180.0;
        const Vector along(std::cos(turn), std::sin(turn), 0.0);
        const std::shared_ptr<Beam> a = std::make_shared<Beam>(
            Polyline({Point(-400.0, 0.0, 0.0), Point(400.0, 0.0, 0.0)}), 0.5 * width, "beam_a"
        );
        const std::shared_ptr<Beam> b = std::make_shared<Beam>(
            Polyline({Point(0.0, 0.0, 0.0), Point(0.0, 0.0, 0.0) + along * 300.0}),
            0.5 * width,
            "beam_b"
        );
        b->extension = 1.5 * width;
        scene.add(a);
        scene.add(b);
        const InteractionContactAxis contact(
            Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}), 0.5, 0.0, 0, 0, 0, 0
        );
        const std::shared_ptr<JointBeam> joint =
            JointBeam::from_contact(*a, *b, contact, 500.0, 0.91, 1, scene.settings);
        check(joint != nullptr, fmt::format("the bridle at {:g} degrees is joined", angle));
        check(
            joint->joinery.name == "top_side_bridle",
            fmt::format("the bridle at {:g} degrees became {}", angle, joint->joinery.name)
        );
        scene.add(joint);
        scene.add_interaction(joint, a, joint->interaction(0));
        scene.add_interaction(joint, b, joint->interaction(1));

        const Mesh first = a->model_geometry_mesh();
        const Mesh second = b->model_geometry_mesh();
        check(first.is_closed(), fmt::format("the first beam is closed at {:g}", angle));
        check(second.is_closed(), fmt::format("the second beam is closed at {:g}", angle));

        const double seated = boolean_volume(first, second, SolidOperation::intersect);
        check(seated < 1.0, fmt::format("seated at {:g} they overlap {:.3f} mm3", angle, seated));

        // out of the near face in steps it stays free; any other way it hits at once
        double slid = 0.0;
        for (double step = 5.0; step <= 200.0; step += 5.0)
            slid = std::max(
                slid,
                boolean_volume(
                    first,
                    second.transformed(Xform::translation(0.0, step, 0.0)),
                    SolidOperation::intersect
                )
            );
        check(slid < 1.0, fmt::format("sliding out at {:g} they overlap {:.3f} mm3", angle, slid));
        const std::array<Xform, 4> blocked = {
            Xform::translation(0.0, -2.0, 0.0),
            Xform::translation(2.0, 0.0, 0.0),
            Xform::translation(-2.0, 0.0, 0.0),
            Xform::translation(0.0, 0.0, 2.0)
        };
        double least = std::numeric_limits<double>::max();
        for (const Xform& move : blocked)
            least = std::min(
                least,
                boolean_volume(first, second.transformed(move), SolidOperation::intersect)
            );
        const double lost = compute_volume(a->element_geometry_mesh()) - compute_volume(first);
        std::cout << fmt::format(
                         "joint_beams: bridle at {:g} degrees: seated overlap {:.3f}, sliding out "
                         "{:.3f}, least blocked move {:.0f} mm3, the first beam lost {:.0f}",
                         angle,
                         seated,
                         slid,
                         least,
                         lost
                     )
                  << std::endl;
        check(least > 100.0, fmt::format("at {:g} a blocked move overlaps {:.1f}", angle, least));

        if (angle != 57.0)
            continue;

        // by hand, across the first beam from its near face and along it from the second's acute edge
        const double run = width / std::tan(turn);
        const double span = width / std::sin(turn);
        const double shoulder = 0.3 * width;
        const double wall = 0.3 * run;
        const std::vector<std::array<double, 2>> pocket = {
            {0.0, wall}, {shoulder, wall}, {width, run}, {width, span}, {0.0, span}
        };
        const std::vector<std::array<double, 2>> sawn = {
            {0.0, wall}, {shoulder, wall}, {0.0, span}
        };
        const std::vector<std::array<double, 2>> behind = {
            {0.0, wall}, {shoulder, wall}, {0.0, wall + shoulder * std::tan(turn)}
        };
        const double layer = width / 3.0;
        const double expected = 2.0 * layer * plan_area(pocket) + layer * plan_area(sawn);
        check(
            std::abs(lost - expected) < 1e-3 * expected,
            fmt::format("the first beam lost {:.0f}, by hand {:.0f}", lost, expected)
        );
        const double inside =
            boolean_volume(second, a->element_geometry_mesh(), SolidOperation::intersect);
        const double fork = 2.0 * layer * plan_area(pocket) + layer * plan_area(behind);
        check(
            std::abs(inside - fork) < 1e-3 * fork,
            fmt::format("the fork inside the first beam is {:.0f}, by hand {:.0f}", inside, fork)
        );
        std::cout << fmt::format(
                         "joint_beams: bridle by hand: the first beam lost {:.0f} of {:.0f} mm3, "
                         "the fork fills {:.0f} of {:.0f}",
                         lost,
                         expected,
                         inside,
                         fork
                     )
                  << std::endl;
    }
}

/// The five-member node JointBeam::column_head as rebuilt from photographs, at its measured size and at other proportions and a turn in plan: closed solids and no overlap seated, the head of the
/// column filled with no void, the first tongue as computed by hand, every beam free to be lowered past every member, so any order of the four goes in, every other move blocked, and the
/// column's walls what stops the scarf sliding along its seam.
static void check_column_head() {

    struct Proportions {
        double width;
        double height;
        double radius;
        double turn;
    };
    for (const Proportions& node : std::vector<Proportions>{
             {100.0, 185.0, 100.0, 0.0},
             {100.0, 185.0, 100.0, 30.0},
             {80.0, 200.0, 120.0, 0.0},
             {120.0, 150.0, 100.0, 45.0},
             {100.0, 185.0, 300.0, 0.0}
         }) {
        const double width = node.width;
        const double height = node.height;
        const Xform spin = Xform::rotation_z(node.turn, true);
        const double level = -0.5 * height;
        const double reach = 0.8 * width;
        const double tail = (0.35 - 0.146) * width;
        const std::vector<Polyline> section = {Polyline::rectangle(
            {-0.5 * width, -0.5 * height, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, width, height
        )};
        const auto beam = [&](double x0, double y0, double x1, double y1, const std::string& name) {
            return std::make_shared<Beam>(
                Polyline({Point(x0, y0, level).transformed(spin), Point(x1, y1, level).transformed(spin)}),
                section,
                std::vector<Vector>{Vector(0.0, 0.0, 1.0)},
                name
            );
        };

        WoodSession scene("column_head");
        const std::shared_ptr<Column> column = Column::round(
            Line::from_points({0.0, 0.0, -4.25 * width}, {0.0, 0.0, 0.0}), node.radius, 0.02, "column"
        );
        const double main_arm = std::max(3.85 * width, 1.5 * node.radius);
        const double secondary_arm = std::max(4.65 * width, 1.5 * node.radius);
        const std::shared_ptr<Beam> first = beam(-main_arm, 0.0, reach, 0.0, "first");
        const std::shared_ptr<Beam> second = beam(main_arm, 0.0, -reach, 0.0, "second");
        const std::shared_ptr<Beam> third = beam(0.0, -secondary_arm, 0.0, -tail, "third");
        const std::shared_ptr<Beam> fourth = beam(0.0, secondary_arm, 0.0, tail, "fourth");
        scene.add(column);
        scene.add(first);
        scene.add(second);
        scene.add(third);
        scene.add(fourth);
        const std::shared_ptr<JointBeam> joint =
            JointBeam::column_head(*column, *first, *second, *third, *fourth);
        scene.add(joint);
        scene.add_interaction(joint, column, joint->interaction(0));
        scene.add_interaction(joint, first, joint->interaction(1));
        scene.add_interaction(joint, second, joint->interaction(2));
        scene.add_interaction(joint, third, joint->interaction(3));
        scene.add_interaction(joint, fourth, joint->interaction(4));

        const std::array<std::string, 5> names = {"column", "first", "second", "third", "fourth"};
        const std::array<Mesh, 5> solids = {
            column->model_geometry_mesh(),
            first->model_geometry_mesh(),
            second->model_geometry_mesh(),
            third->model_geometry_mesh(),
            fourth->model_geometry_mesh()
        };
        for (size_t i = 0; i < 5; i++)
            check(solids[i].is_closed(), fmt::format("the {} is a closed solid", names[i]));

        // seated: no two members overlap
        double seated = 0.0;
        for (size_t i = 0; i < 5; i++)
            for (size_t j = i + 1; j < 5; j++)
                seated += boolean_volume(solids[i], solids[j], SolidOperation::intersect);
        check(seated < 1.0, fmt::format("seated the members overlap {:.3f} mm3", seated));

        // the head of the column, as tall as the beams, is filled with no void
        const Mesh head = Column::round(
                              Line::from_points({0.0, 0.0, -height}, {0.0, 0.0, 0.0}), node.radius, 0.02
        )
                              ->element_geometry_mesh();
        double filled = 0.0;
        for (const Mesh& solid : solids)
            filled += boolean_volume(solid, head, SolidOperation::intersect);
        check(
            std::abs(filled - compute_volume(head)) < 1e-3 * compute_volume(head),
            fmt::format("the head holds {:.0f} of {:.0f} mm3", filled, compute_volume(head))
        );

        // by hand: the first beam inside the shoulders is its tongue less the dovetail's pocket
        const Polyline zone = Polyline::rectangle(
            {-reach, -0.6 * width, -height}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 2.0 * reach, 1.2 * width
        );
        const Mesh tongue_zone =
            Mesh::loft({zone}, {zone.transformed(Xform::translation(0.0, 0.0, height))}, true)
                .transformed(spin);
        const double root = 0.143 * width;
        const double depth = 0.146 * width;
        const double tip = root + 0.176 * depth;
        const double tongue = (2.0 * reach * 0.35 * width - (root + tip) * depth) * height;
        const double kept = boolean_volume(solids[1], tongue_zone, SolidOperation::intersect);
        check(
            std::abs(kept - tongue) < 1e-3 * tongue,
            fmt::format("the first tongue is {:.0f}, by hand {:.0f}", kept, tongue)
        );

        // assembly: from above every pair stays clear all the way, so any order of the four beams goes in
        double dropped = 0.0;
        for (size_t i = 1; i < 5; i++)
            for (size_t j = 0; j < 5; j++)
                if (i != j)
                    for (double lift = 10.0; lift <= height + 100.0; lift += 10.0)
                        dropped = std::max(
                            dropped,
                            boolean_volume(
                                solids[i].transformed(Xform::translation(0.0, 0.0, lift)),
                                solids[j],
                                SolidOperation::intersect
                            )
                        );
        check(dropped < 1.0, fmt::format("lowering a beam overlaps {:.3f} mm3", dropped));

        // seated, every beam blocked by the others 2 mm any way but up
        const auto move = [&](double x, double y, double z) {
            const Vector turned = Vector(x, y, z).transformed(spin);
            return Xform::translation(turned[0], turned[1], turned[2]);
        };
        const std::array<Xform, 5> moves = {
            move(2.0, 0.0, 0.0),
            move(-2.0, 0.0, 0.0),
            move(0.0, 2.0, 0.0),
            move(0.0, -2.0, 0.0),
            move(0.0, 0.0, -2.0)
        };
        double least = std::numeric_limits<double>::max();
        for (size_t i = 1; i < 5; i++)
            for (const Xform& step : moves) {
                const Mesh moved = solids[i].transformed(step);
                double blocked = 0.0;
                for (size_t j = 0; j < 5; j++)
                    if (i != j)
                        blocked += boolean_volume(moved, solids[j], SolidOperation::intersect);
                least = std::min(least, blocked);
            }
        check(least > 100.0, fmt::format("a blocked move overlaps only {:.1f} mm3", least));

        // the scarf: pulling apart along the beam meets the seam, sliding along the seam does not; only the column's walls stop that
        const double pulled = boolean_volume(
            solids[1].transformed(move(-2.0, 0.0, 0.0)), solids[2], SolidOperation::intersect
        );
        const double along_seam = boolean_volume(
            solids[1].transformed(move(-2.0, 0.2, 0.0)), solids[2], SolidOperation::intersect
        );
        const double walled = boolean_volume(
            solids[1].transformed(move(-2.0, 0.2, 0.0)), solids[0], SolidOperation::intersect
        );
        check(pulled > 100.0, fmt::format("pulling the scarf apart overlaps {:.1f} mm3", pulled));
        check(along_seam < 1.0, fmt::format("along the seam it overlaps {:.3f} mm3", along_seam));
        check(walled > 100.0, fmt::format("the column stops the seam slide with {:.1f} mm3", walled));

        std::cout << fmt::format(
                         "joint_beams: column head {:g} x {:g}, radius {:g}, turned {:g}: seated "
                         "overlap {:.3f} mm3, head holds {:.0f} of {:.0f}, first tongue {:.0f} of "
                         "{:.0f} by hand, lowered clear {:.3f}, least blocked move {:.0f}, scarf "
                         "pulled {:.0f}, along the seam {:.3f}, stopped by the column {:.0f}, exact "
                         "faces {}",
                         width,
                         height,
                         node.radius,
                         node.turn,
                         seated,
                         filled,
                         compute_volume(head),
                         kept,
                         tongue,
                         dropped,
                         least,
                         pulled,
                         along_seam,
                         walled,
                         column->model_geometry_brep().face_count()
                     )
                  << std::endl;
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
    check_bridles();
    check_column_head();
    check_end_to_end();
    return 0;
}
