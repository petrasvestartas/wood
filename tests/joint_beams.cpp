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
    std::vector<std::shared_ptr<EdgeStraightElement>> members;
    for (const Polyline& axis : io::load_obj("phanomema_node")) {
        members.push_back(std::make_shared<EdgeStraightElement>(
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

        const std::shared_ptr<EdgeStraightElement> a = scene.get_element<EdgeStraightElement>(joint->targets[0]);
        const std::shared_ptr<EdgeStraightElement> b = scene.get_element<EdgeStraightElement>(joint->targets[1]);
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

    for (const std::shared_ptr<EdgeStraightElement>& member : members) {
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
            const std::shared_ptr<EdgeStraightElement> a = std::make_shared<EdgeStraightElement>(
                Polyline({Point(-600.0, 0.0, 0.0), Point(600.0, 0.0, 0.0)}), 75.0, "beam"
            );
            const std::shared_ptr<EdgeStraightElement> b = std::make_shared<EdgeStraightElement>(
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
            for (const std::shared_ptr<EdgeStraightElement>& member : {a, b}) {
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
            for (const std::shared_ptr<EdgeStraightElement>& member : {a, b})
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
            const std::shared_ptr<EdgeStraightElement> a = std::make_shared<EdgeStraightElement>(
                Polyline({Point(-600.0, 0.0, 0.0), Point(600.0, 0.0, 0.0)}), 75.0, "beam"
            );
            const std::shared_ptr<EdgeStraightElement> b = std::make_shared<EdgeStraightElement>(
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
            for (const std::shared_ptr<EdgeStraightElement>& member : {a, b}) {
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
        const std::shared_ptr<EdgeStraightElement> a = std::make_shared<EdgeStraightElement>(
            Polyline({Point(-400.0, 0.0, 0.0), Point(400.0, 0.0, 0.0)}), 0.5 * width, "beam_a"
        );
        const std::shared_ptr<EdgeStraightElement> b = std::make_shared<EdgeStraightElement>(
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

/// A round column and four beams at any angles in plan, their axes simply ending on the column's: `turn` the main line, the secondary beams' directions out from the column.
struct HeadNode {
    double width = 100.0;
    double height = 185.0;
    double radius = 100.0;
    double turn = 0.0;
    double secondary_0 = 90.0;
    double secondary_1 = 270.0;
    double secondary_width = 0.0;
    double near_end = 0.0; // Where every beam's axis stops, along it from the column's axis: past it when negative.
    bool reversed = false; // Axes drawn from the column out, the column's from its head down.
    double min_post = 0.05; // The smallest post allowed, in square beam widths.
    int main_beam = -1; // Which member is named as of the main pair, 1 to 4; none by default.
};

/// The node's column and beams, the beams in the order given by `order`.
static std::array<std::shared_ptr<Element>, 5> head_members(const HeadNode& node, const std::array<size_t, 4>& order) {

    const double level = -0.5 * node.height;
    const Point foot(0.0, 0.0, -4.25 * node.width);
    const Point top(0.0, 0.0, 0.0);
    const std::shared_ptr<Column> column = Column::round(
        node.reversed ? Line::from_points(top, foot) : Line::from_points(foot, top), node.radius, 0.02, "column"
    );
    const std::array<double, 4> angles = {
        node.turn + 180.0, node.turn, node.secondary_0 + node.turn, node.secondary_1 + node.turn
    };
    const std::array<double, 4> arms = {
        std::max(3.85 * node.width, 1.5 * node.radius),
        std::max(3.85 * node.width, 1.5 * node.radius),
        std::max(4.65 * node.width, 1.5 * node.radius),
        std::max(4.65 * node.width, 1.5 * node.radius)
    };
    const std::array<std::string, 4> names = {"first", "second", "third", "fourth"};
    std::array<std::shared_ptr<Element>, 5> members = {column, nullptr, nullptr, nullptr, nullptr};
    for (size_t i = 0; i < 4; i++) {
        const size_t k = order[i];
        const double width = k >= 2 && node.secondary_width > 0.0 ? node.secondary_width : node.width;
        const std::vector<Polyline> section = {Polyline::rectangle(
            {-0.5 * width, -0.5 * node.height, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, width, node.height
        )};
        const double angle = angles[k] * std::numbers::pi / 180.0;
        const Vector way(std::cos(angle), std::sin(angle), 0.0);
        const Point far_end = Point(0.0, 0.0, level) + way * arms[k];
        const Point near_end = Point(0.0, 0.0, level) + way * node.near_end;
        members[i + 1] = std::make_shared<EdgeStraightElement>(
            node.reversed ? Polyline({near_end, far_end}) : Polyline({far_end, near_end}),
            section,
            std::vector<Vector>{Vector(0.0, 0.0, 1.0)},
            names[k]
        );
    }

    return members;
}

/// The node joined in a scene of its own.
static std::shared_ptr<JointBeam> head_joined(
    WoodSession& scene, const std::array<std::shared_ptr<Element>, 5>& members, const HeadNode& node = HeadNode()
) {

    for (const std::shared_ptr<Element>& member : members)
        scene.add(member);
    const std::shared_ptr<JointBeam> joint = JointBeam::column_head(
        *std::dynamic_pointer_cast<Column>(members[0]),
        *std::dynamic_pointer_cast<EdgeStraightElement>(members[1]),
        *std::dynamic_pointer_cast<EdgeStraightElement>(members[2]),
        *std::dynamic_pointer_cast<EdgeStraightElement>(members[3]),
        *std::dynamic_pointer_cast<EdgeStraightElement>(members[4]),
        node.main_beam < 0 ? nullptr : std::dynamic_pointer_cast<EdgeStraightElement>(members[node.main_beam]).get(),
        0.7, 0.8, 0.1, 0.145, 0.286, 0.146, 0.176, 0.08,
        node.min_post
    );
    scene.add(joint);
    for (size_t i = 0; i < 5; i++)
        scene.add_interaction(joint, members[i], joint->interaction(i));

    return joint;
}

/// What a joined node measures: pieces closed, seated clear, lowered clear, every other move blocked.
struct HeadMeasure {
    double seated = 0.0;
    double filled = 0.0;
    double head = 0.0;
    double lowered = 0.0;
    double least = std::numeric_limits<double>::max();
};

static HeadMeasure measure_head(const HeadNode& node, const std::array<std::shared_ptr<Element>, 5>& members) {

    const std::array<Mesh, 5> solids = {
        members[0]->model_geometry_mesh(),
        members[1]->model_geometry_mesh(),
        members[2]->model_geometry_mesh(),
        members[3]->model_geometry_mesh(),
        members[4]->model_geometry_mesh()
    };
    for (size_t i = 0; i < 5; i++)
        check(solids[i].is_closed(), fmt::format("the {} is a closed solid", members[i]->name));

    HeadMeasure measure;
    for (size_t i = 0; i < 5; i++)
        for (size_t j = i + 1; j < 5; j++)
            measure.seated += boolean_volume(solids[i], solids[j], SolidOperation::intersect);

    // the head, as tall as the beams, filled with no void
    const Mesh head = Column::round(
                          Line::from_points({0.0, 0.0, -node.height}, {0.0, 0.0, 0.0}), node.radius, 0.02
    )
                          ->element_geometry_mesh();
    measure.head = compute_volume(head);
    for (const Mesh& solid : solids)
        measure.filled += boolean_volume(solid, head, SolidOperation::intersect);

    // from above every pair stays clear all the way, so any order of the four beams goes in
    for (size_t i = 1; i < 5; i++)
        for (size_t j = 0; j < 5; j++)
            if (i != j)
                for (double lift = 20.0; lift <= node.height + 60.0; lift += 20.0)
                    measure.lowered = std::max(
                        measure.lowered,
                        boolean_volume(
                            solids[i].transformed(Xform::translation(0.0, 0.0, lift)),
                            solids[j],
                            SolidOperation::intersect
                        )
                    );

    // seated, every beam stopped by the others 2 mm any way but up: along and across the main line and its own, and down
    std::vector<Vector> ways = {Vector(0.0, 0.0, -1.0)};
    for (const double angle : {node.turn, node.turn + 90.0, node.turn + node.secondary_0, node.turn + node.secondary_1}) {
        const double turn = angle * std::numbers::pi / 180.0;
        const Vector way(std::cos(turn), std::sin(turn), 0.0);
        ways.push_back(way);
        ways.push_back(way * -1.0);
    }
    for (size_t i = 1; i < 5; i++)
        for (const Vector& way : ways) {
            const Mesh moved =
                solids[i].transformed(Xform::translation(way[0] * 2.0, way[1] * 2.0, way[2] * 2.0));
            double blocked = 0.0;
            for (size_t j = 0; j < 5; j++)
                if (i != j)
                    blocked += boolean_volume(moved, solids[j], SolidOperation::intersect);
            measure.least = std::min(measure.least, blocked);
        }

    return measure;
}

/// Any group of the five pieces moved 2 mm any way, the rest still: the least overlap of a blocked move, and how many moves were free. Only the beams lifting off the column, or
/// the column dropping away from them, may be free: the 15 groups of beams up and the 15 groups holding the column down.
static std::pair<double, size_t> head_group_moves(const HeadNode& node, const std::array<std::shared_ptr<Element>, 5>& members) {

    const std::array<Mesh, 5> solids = {
        members[0]->model_geometry_mesh(),
        members[1]->model_geometry_mesh(),
        members[2]->model_geometry_mesh(),
        members[3]->model_geometry_mesh(),
        members[4]->model_geometry_mesh()
    };
    std::vector<Vector> ways = {Vector(0.0, 0.0, 1.0), Vector(0.0, 0.0, -1.0)};
    for (const double angle : {node.turn, node.turn + 90.0, node.turn + node.secondary_0, node.turn + node.secondary_1}) {
        const double turn = angle * std::numbers::pi / 180.0;
        const Vector way(std::cos(turn), std::sin(turn), 0.0);
        ways.push_back(way);
        ways.push_back(way * -1.0);
    }

    double least = std::numeric_limits<double>::max();
    size_t free_moves = 0;
    for (unsigned group = 1; group < 31; group++)
        for (size_t w = 0; w < ways.size(); w++) {
            double blocked = 0.0;
            for (size_t i = 0; i < 5; i++)
                for (size_t j = 0; j < 5; j++)
                    if ((group >> i & 1u) && !(group >> j & 1u))
                        blocked += boolean_volume(
                            solids[i].transformed(Xform::translation(ways[w][0] * 2.0, ways[w][1] * 2.0, ways[w][2] * 2.0)),
                            solids[j],
                            SolidOperation::intersect
                        );
            const bool holds_column = group & 1u;
            const bool may_leave = (!holds_column && w == 0) || (holds_column && w == 1);
            if (may_leave) {
                check(blocked < 1.0, fmt::format("group {} can not leave: {:.3f} mm3", group, blocked));
                free_moves++;
                continue;
            }
            check(blocked > 1.0, fmt::format("group {} moves freely along way {}: {:.3f} mm3", group, w, blocked));
            least = std::min(least, blocked);
        }

    return {least, free_moves};
}

/// The smallest post of a joined node, in mm2.
static double smallest_post(const HeadNode& node, const Mesh& column) {

    const Polyline square = Polyline::rectangle(
        {-2.0 * node.radius, -2.0 * node.radius, -node.height + 1.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 4.0 * node.radius, 4.0 * node.radius
    );
    const Mesh slab =
        Mesh::loft({square}, {square.transformed(Xform::translation(0.0, 0.0, node.height + 10.0))}, true);
    double least = std::numeric_limits<double>::max();
    for (const Mesh& post : solid_pieces(solid_boolean(column, slab, SolidOperation::intersect)))
        least = std::min(least, compute_volume(post) / (node.height - 1.0));

    return least;
}

/// The node from one column and four beams at its measured size and at others, the seat, roles and cuts all read from the members: seated clear, the head filled, every beam lowered
/// clear and stopped every other way, the first tongue as computed by hand, and the scarf stopped sliding along its seam only by the column's walls.
static void check_column_head() {

    for (const HeadNode& node : std::vector<HeadNode>{
             {100.0, 185.0, 100.0, 0.0},
             {100.0, 185.0, 100.0, 30.0},
             {80.0, 200.0, 120.0, 0.0},
             {120.0, 150.0, 100.0, 45.0},
             {100.0, 185.0, 300.0, 0.0}
         }) {
        const std::array<std::shared_ptr<Element>, 5> members = head_members(node, {0, 1, 2, 3});
        WoodSession scene("column_head");
        const std::shared_ptr<JointBeam> joint = head_joined(scene, members);
        const HeadMeasure measure = measure_head(node, members);
        check(measure.seated < 1.0, fmt::format("seated the members overlap {:.3f} mm3", measure.seated));
        check(
            std::abs(measure.filled - measure.head) < 1e-3 * measure.head,
            fmt::format("the head holds {:.0f} of {:.0f} mm3", measure.filled, measure.head)
        );
        check(measure.lowered < 1.0, fmt::format("lowering a beam overlaps {:.3f} mm3", measure.lowered));
        check(measure.least > 100.0, fmt::format("a blocked move overlaps only {:.1f} mm3", measure.least));

        // by hand: the first beam inside the shoulders is its tongue less the dovetail's pocket
        const Xform spin = Xform::rotation_z(node.turn, true);
        const double reach = 0.8 * node.radius;
        const Polyline zone = Polyline::rectangle(
            {-reach, -0.6 * node.width, -node.height}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 2.0 * reach, 1.2 * node.width
        );
        const Mesh tongue_zone =
            Mesh::loft({zone}, {zone.transformed(Xform::translation(0.0, 0.0, node.height))}, true).transformed(spin);
        const double root = 0.143 * node.width;
        const double depth = 0.146 * node.width;
        const double tip = root + 0.176 * depth;
        const double tongue = (2.0 * reach * 0.35 * node.width - (root + tip) * depth) * node.height;
        const double kept = boolean_volume(members[1]->model_geometry_mesh(), tongue_zone, SolidOperation::intersect);
        check(
            std::abs(kept - tongue) < 1e-3 * tongue,
            fmt::format("the first tongue is {:.0f}, by hand {:.0f}", kept, tongue)
        );

        // the scarf: pulling apart along the beam meets the seam, sliding along the seam does not; only the column's walls stop that
        const auto move = [&](double x, double y) {
            const Vector turned = Vector(x, y, 0.0).transformed(spin);
            return Xform::translation(turned[0], turned[1], 0.0);
        };
        const Mesh first = members[1]->model_geometry_mesh();
        const Mesh second = members[2]->model_geometry_mesh();
        const Mesh post = members[0]->model_geometry_mesh();
        const double pulled = boolean_volume(first.transformed(move(-2.0, 0.0)), second, SolidOperation::intersect);
        const double along_seam = boolean_volume(first.transformed(move(-2.0, 0.2)), second, SolidOperation::intersect);
        const double walled = boolean_volume(first.transformed(move(-2.0, 0.2)), post, SolidOperation::intersect);
        check(pulled > 100.0, fmt::format("pulling the scarf apart overlaps {:.1f} mm3", pulled));
        check(along_seam < 1.0, fmt::format("along the seam it overlaps {:.3f} mm3", along_seam));
        check(walled > 100.0, fmt::format("the column stops the seam slide with {:.1f} mm3", walled));

        std::cout << fmt::format(
                         "joint_beams: column head {:g} x {:g}, radius {:g}, turned {:g}: seated overlap {:.3f} mm3, head holds {:.0f} "
                         "of {:.0f}, first tongue {:.0f} of {:.0f} by hand, lowered clear {:.3f}, least blocked move {:.0f}, "
                         "scarf pulled {:.0f}, along the seam {:.3f}, stopped by the column {:.0f}",
                         node.width, node.height, node.radius, node.turn, measure.seated, measure.filled, measure.head, kept,
                         tongue, measure.lowered, measure.least, pulled, along_seam, walled
                     )
                  << std::endl;
    }
}

/// The joint is the same in every order of the four beams: all 24 cut each beam to the same solid.
static void check_column_head_orders() {

    const HeadNode node = {100.0, 185.0, 100.0, 0.0, 70.0, 250.0};
    std::array<size_t, 4> order = {0, 1, 2, 3};
    std::map<std::string, Mesh> first_solids;
    size_t orders = 0;
    double worst = 0.0;
    do {
        const std::array<std::shared_ptr<Element>, 5> members = head_members(node, order);
        WoodSession scene("column_head");
        head_joined(scene, members);
        for (const std::shared_ptr<Element>& member : members) {
            const Mesh solid = member->model_geometry_mesh();
            const auto known = first_solids.find(member->name);
            if (known == first_solids.end()) {
                first_solids[member->name] = solid;
                continue;
            }
            worst = std::max(worst, compute_volume(solid) - boolean_volume(solid, known->second, SolidOperation::intersect));
        }
        orders++;
    } while (std::next_permutation(order.begin(), order.end()));

    check(orders == 24, fmt::format("{} orders of the four beams", orders));
    check(worst < 1.0, fmt::format("an order changes a beam by {:.3f} mm3", worst));
    std::cout << fmt::format("joint_beams: column head: all {} orders of the beams agree within {:.3f} mm3", orders, worst) << std::endl;
}

/// Beams too short, too long or drawn the other way, and a column drawn from its head down, all give the same solids: the joint finds each near end and trims or extends to suit.
static void check_column_head_ends() {

    const HeadNode base = {100.0, 185.0, 100.0, 0.0, 75.0, 280.0};
    const std::array<std::shared_ptr<Element>, 5> reference = head_members(base, {0, 1, 2, 3});
    WoodSession reference_scene("column_head");
    head_joined(reference_scene, reference);

    double worst = 0.0;
    for (const std::array<double, 2>& variant : std::vector<std::array<double, 2>>{
             {90.0, 0.0}, {-60.0, 0.0}, {0.0, 1.0}, {-60.0, 1.0}
         }) {
        HeadNode node = base;
        node.near_end = variant[0];
        node.reversed = variant[1] > 0.5;
        const std::array<std::shared_ptr<Element>, 5> members = head_members(node, {0, 1, 2, 3});
        WoodSession scene("column_head");
        head_joined(scene, members);
        for (size_t i = 0; i < 5; i++) {
            const Mesh solid = members[i]->model_geometry_mesh();
            const Mesh same = reference[i]->model_geometry_mesh();
            check(solid.is_closed(), fmt::format("the {} is a closed solid", members[i]->name));
            worst = std::max(
                {worst,
                 compute_volume(solid) - boolean_volume(solid, same, SolidOperation::intersect),
                 compute_volume(same) - boolean_volume(solid, same, SolidOperation::intersect)}
            );
        }
    }
    check(worst < 1.0, fmt::format("a beam's end changes the joint by {:.3f} mm3", worst));
    std::cout << fmt::format(
                     "joint_beams: column head: beams stopping 90 short of the axis, running 60 past it, or drawn the other way, and the column drawn head down, agree within {:.3f} mm3",
                     worst
                 )
              << std::endl;
}

/// The secondary beams swept through their angles in plan, together and each on its own: at every angle the joint builds it is sound, and past the limits it is refused naming the beam.
static void check_column_head_angles() {

    const auto joins = [](const HeadNode& node, std::string& refused) {
        try {
            const std::array<std::shared_ptr<Element>, 5> members = head_members(node, {0, 1, 2, 3});
            WoodSession scene("column_head");
            head_joined(scene, members);
            const HeadMeasure measure = measure_head(node, members);
            check(measure.seated < 1.0, fmt::format("at {:g} and {:g} seated {:.3f} mm3", node.secondary_0, node.secondary_1, measure.seated));
            check(
                std::abs(measure.filled - measure.head) < 2e-3 * measure.head,
                fmt::format("at {:g} and {:g} the head holds {:.0f} of {:.0f}", node.secondary_0, node.secondary_1, measure.filled, measure.head)
            );
            check(measure.lowered < 1.0, fmt::format("at {:g} and {:g} lowering overlaps {:.3f}", node.secondary_0, node.secondary_1, measure.lowered));
            check(measure.least > 10.0, fmt::format("at {:g} and {:g} a move is free, {:.1f}", node.secondary_0, node.secondary_1, measure.least));
            return true;
        } catch (const std::invalid_argument& error) {
            refused = error.what();
            return false;
        }
    };

    // both turned the same way, the node keeping its half-turn symmetry
    double low = 180.0;
    double high = 0.0;
    size_t valid = 0;
    std::string below;
    std::string above;
    for (double angle = 5.0; angle <= 175.0; angle += 1.0) {
        std::string refused;
        if (joins({100.0, 185.0, 100.0, 0.0, angle, 180.0 + angle}, refused)) {
            low = std::min(low, angle);
            high = std::max(high, angle);
            valid++;
        } else {
            (angle < 90.0 ? below : above) = refused;
        }
    }
    check(valid > 0, "no angle joins");
    std::cout << fmt::format("joint_beams: column head: secondary beams valid from {:g} to {:g} degrees in 1 degree steps, {} angles; below: {}; above: {}", low, high, valid, below, above) << std::endl;

    // each on its own, the two sides independent
    size_t asymmetric = 0;
    for (const std::array<double, 2>& pair : std::vector<std::array<double, 2>>{
             {90.0, 240.0}, {70.0, 290.0}, {60.0, 300.0}, {110.0, 250.0}, {75.0, 255.0}, {100.0, 280.0}
         }) {
        std::string refused;
        if (joins({100.0, 185.0, 100.0, 0.0, pair[0], pair[1]}, refused))
            asymmetric++;
        else
            std::cout << fmt::format("joint_beams: column head at {:g} and {:g} refused: {}", pair[0], pair[1], refused) << std::endl;
    }
    check(asymmetric > 0, "no asymmetric node joins");

    // a narrower secondary beam
    std::string refused;
    check(joins({100.0, 185.0, 100.0, 0.0, 80.0, 260.0, 70.0}, refused), "a narrower secondary beam joins: " + refused);
    std::cout << fmt::format("joint_beams: column head: {} of 6 asymmetric nodes join, and a narrower secondary beam", asymmetric) << std::endl;
}

/// Any group of pieces is held at the square node and across the valid angles, and the caller can choose the main pair at a plain cross.
static void check_column_head_groups() {

    for (const HeadNode& node : std::vector<HeadNode>{
             {100.0, 185.0, 100.0, 0.0, 90.0, 270.0},
             {100.0, 185.0, 100.0, 0.0, 67.0, 247.0},
             {100.0, 185.0, 100.0, 0.0, 113.0, 293.0},
             {100.0, 185.0, 100.0, 0.0, 75.0, 280.0}
         }) {
        const std::array<std::shared_ptr<Element>, 5> members = head_members(node, {0, 1, 2, 3});
        WoodSession scene("column_head");
        head_joined(scene, members, node);
        const std::pair<double, size_t> moves = head_group_moves(node, members);
        check(moves.second == 30, fmt::format("{} free moves, not 30", moves.second));
        std::cout << fmt::format(
                         "joint_beams: column head at {:g} and {:g}: every group of pieces, 300 moves, 270 blocked, least {:.0f} mm3, 30 free, all a group of beams lifting off",
                         node.secondary_0, node.secondary_1, moves.first
                     )
                  << std::endl;
    }

    // at a plain cross with equal beams the default takes the pair along world x; naming a beam takes its pair
    HeadNode cross = {100.0, 185.0, 100.0, 0.0, 90.0, 270.0};
    cross.main_beam = -1;
    const auto main_is_second_pair = [](const HeadNode& node) {
        std::array<std::shared_ptr<Element>, 5> members = head_members(node, {0, 1, 2, 3});
        WoodSession scene("column_head");
        head_joined(scene, members, node);
        const HeadMeasure measure = measure_head(node, members);
        check(measure.seated < 1.0, fmt::format("named, seated {:.3f}", measure.seated));
        check(measure.lowered < 1.0, fmt::format("named, lowered {:.3f}", measure.lowered));
        check(measure.least > 100.0, fmt::format("named, a move is free, {:.1f}", measure.least));

        // a main beam's tongue runs past the column's axis, a secondary's stops short of it
        double reach = std::numeric_limits<double>::max();
        for (const Point& point : members[3]->model_geometry_mesh().to_vertices_and_faces().first)
            reach = std::min(reach, point[1]);
        return reach < 0.0;
    };
    check(!main_is_second_pair(cross), "by default the shorter pair is the main one");
    cross.main_beam = 3;
    check(main_is_second_pair(cross), "naming the third beam makes its pair the main one");
    std::cout << "joint_beams: column head: at a plain cross the shorter pair by default, a named beam's pair when asked" << std::endl;
}

/// The real limit of the turn: with no floor on a post's size, how far from square the secondary beams turn, going out from 90 degrees until the joint first fails. At the narrow side a post
/// necks and splits; further on one piece of it is gone and a speck is left, which counts as a post again, so the range is what holds without a break, not the widest angle that builds.
static void check_column_head_limit() {

    std::map<int, double> posts;
    std::map<int, std::string> refusals;
    for (int angle = 5; angle <= 175; angle++) {
        HeadNode node = {100.0, 185.0, 100.0, 0.0, double(angle), 180.0 + angle};
        node.min_post = 0.0;
        try {
            const std::array<std::shared_ptr<Element>, 5> members = head_members(node, {0, 1, 2, 3});
            WoodSession scene("column_head");
            head_joined(scene, members, node);
            const HeadMeasure measure = measure_head(node, members);
            check(measure.seated < 1.0, fmt::format("at {} seated {:.3f} mm3", angle, measure.seated));
            check(measure.lowered < 1.0, fmt::format("at {} lowering overlaps {:.3f}", angle, measure.lowered));
            check(measure.least > 10.0, fmt::format("at {} a move is free, {:.1f}", angle, measure.least));
            posts[angle] = smallest_post(node, members[0]->model_geometry_mesh());
        } catch (const std::invalid_argument& error) {
            refusals[angle] = error.what();
        }
    }

    int low = 90;
    int high = 90;
    while (posts.count(low - 1))
        low--;
    while (posts.count(high + 1))
        high++;
    check(low == 65 && high == 115, fmt::format("the sound range moved to {} and {}", low, high));
    std::cout << fmt::format(
                     "joint_beams: column head: with no floor on a post, sound without a break from {} to {} degrees, the post falling from {:.0f} to {:.0f} mm2; at {}: {}; at {}: {}",
                     low, high, posts[90], posts[low], low - 1, refusals[low - 1], high + 1, refusals[high + 1]
                 )
              << std::endl;
    for (const std::pair<const int, double>& post : posts)
        if (post.first < low || post.first > high)
            std::cout << fmt::format("joint_beams: column head turn {}: a speck of {:.0f} mm2 left of the post", post.first, post.second) << std::endl;
}

/// The column cut by the kernel's boolean on the exact cylinder, no mesh behind it, at the measured node, across the valid turn and on asymmetric nodes: a closed solid, a closed mesh and the volume
/// of the cut mesh. Every case runs; those that fail are named together at the end.
static void check_column_head_brep() {

    std::vector<HeadNode> nodes = {
        {100.0, 185.0, 100.0, 0.0, 90.0, 270.0},
        {100.0, 185.0, 100.0, 30.0, 90.0, 270.0},
        {80.0, 200.0, 120.0, 0.0, 90.0, 270.0},
        {120.0, 150.0, 100.0, 45.0, 90.0, 270.0},
        {100.0, 185.0, 300.0, 0.0, 90.0, 270.0},
        {100.0, 185.0, 100.0, 0.0, 75.0, 280.0},
        {100.0, 185.0, 100.0, 0.0, 70.0, 290.0},
        {100.0, 185.0, 100.0, 0.0, 110.0, 250.0}
    };
    for (double angle = 67.0; angle <= 113.0; angle += 2.0)
        nodes.push_back({100.0, 185.0, 100.0, 0.0, angle, 180.0 + angle});

    size_t sound = 0;
    double worst = 0.0;
    std::string failures;
    for (const HeadNode& node : nodes) {
        const std::string name = fmt::format("{:g} x {:g} radius {:g} turned {:g} at {:g} and {:g}", node.width, node.height, node.radius, node.turn, node.secondary_0, node.secondary_1);
        try {
            const std::array<std::shared_ptr<Element>, 5> members = head_members(node, {0, 1, 2, 3});
            WoodSession scene("column_head");
            head_joined(scene, members, node);
            const BRep& brep = members[0]->model_geometry_brep();
            const double mesh_volume = compute_volume(members[0]->model_geometry_mesh());
            const double exact = brep.volume_exact();
            const Mesh shown = brep.mesh();
            const double off = std::abs(exact - mesh_volume) / mesh_volume;
            worst = std::max(worst, off);
            if (!brep.is_solid() || !shown.is_closed() || off > 1e-3)
                failures += fmt::format(
                    "\n  {}: solid {}, mesh closed {}, exact volume {:.0f} against {:.0f}",
                    name, brep.is_solid(), shown.is_closed(), exact, mesh_volume
                );
            else
                sound++;
        } catch (const std::exception& error) {
            failures += fmt::format("\n  {}: {}", name, error.what());
        }
    }
    std::cout << fmt::format(
                     "joint_beams: column head: {} of {} columns cut by the exact boolean, closed solids with closed meshes, volume within {:.4f} percent of the cut mesh{}",
                     sound, nodes.size(), 100.0 * worst, failures
                 )
              << std::endl;
    check(sound == nodes.size(), "a column did not come through the exact boolean");
}

/// What no joint exists for is refused, naming the beam and the reason.
static void check_column_head_refusals() {

    const std::vector<Polyline> section = {
        Polyline::rectangle({-50.0, -92.5, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 100.0, 185.0)
    };
    const auto at = [&](double angle, double start, double end, double level, const std::string& name) {
        const double turn = angle * std::numbers::pi / 180.0;
        const Vector way(std::cos(turn), std::sin(turn), 0.0);
        return std::make_shared<EdgeStraightElement>(
            Polyline({Point(0.0, 0.0, level) + way * end, Point(0.0, 0.0, level) + way * start}),
            section,
            std::vector<Vector>{Vector(0.0, 0.0, 1.0)},
            name
        );
    };
    const std::shared_ptr<Column> column =
        Column::round(Line::from_points({0.0, 0.0, -425.0}, {0.0, 0.0, 0.0}), 100.0, 0.02, "column");
    const auto refused = [&](const std::array<std::shared_ptr<EdgeStraightElement>, 4>& beams, const std::string& expected) {
        std::string message;
        try {
            JointBeam::column_head(*column, *beams[0], *beams[1], *beams[2], *beams[3]);
        } catch (const std::invalid_argument& error) {
            message = error.what();
        }
        check(message.find(expected) != std::string::npos, fmt::format("expected '{}', got '{}'", expected, message));
        std::cout << fmt::format("joint_beams: column head refused: {}", message) << std::endl;
    };
    const double level = -92.5;
    const auto fine = [&](double angle, const std::string& name) { return at(angle, 0.0, 400.0, level, name); };

    refused({fine(0.0, "a"), fine(170.0, "b"), fine(80.0, "c"), fine(250.0, "d")}, "no two beams are in line");
    refused({fine(0.0, "a"), fine(180.0, "b"), fine(60.0, "c"), fine(120.0, "d")}, "on the same side");
    refused({fine(0.0, "a"), fine(180.0, "b"), fine(90.0, "c"), at(270.0, 250.0, 500.0, level, "d")}, "beam d stops");
    refused({fine(0.0, "a"), fine(180.0, "b"), fine(90.0, "c"), at(270.0, 0.0, 400.0, level + 30.0, "d")}, "has its top");
    refused({fine(0.0, "a"), fine(180.0, "b"), fine(10.0, "c"), fine(190.0, "d")}, "beam c at 10.0 degrees");

    const std::shared_ptr<EdgeStraightElement> tilted = std::make_shared<EdgeStraightElement>(
        Polyline({Point(0.0, -400.0, level + 60.0), Point(0.0, 0.0, level)}),
        section,
        std::vector<Vector>{Vector(0.0, 0.0, 1.0)},
        "tilted"
    );
    refused({fine(0.0, "a"), fine(180.0, "b"), fine(90.0, "c"), tilted}, "beam tilted is tilted");
    const std::shared_ptr<EdgeStraightElement> aside = std::make_shared<EdgeStraightElement>(
        Polyline({Point(30.0, -400.0, level), Point(30.0, 0.0, level)}),
        section,
        std::vector<Vector>{Vector(0.0, 0.0, 1.0)},
        "aside"
    );
    refused({fine(0.0, "a"), fine(180.0, "b"), fine(90.0, "c"), aside}, "beam aside misses the column's axis");

    // a named main beam with nothing in line with it
    const std::array<std::shared_ptr<EdgeStraightElement>, 4> turned = {fine(0.0, "a"), fine(180.0, "b"), fine(75.0, "c"), fine(280.0, "d")};
    std::string message;
    try {
        JointBeam::column_head(*column, *turned[0], *turned[1], *turned[2], *turned[3], turned[2].get());
    } catch (const std::invalid_argument& error) {
        message = error.what();
    }
    check(message.find("no two beams are in line") != std::string::npos, "named c, got: " + message);
    std::cout << fmt::format("joint_beams: column head refused: {}", message) << std::endl;
    message.clear();
    const std::shared_ptr<EdgeStraightElement> stranger = fine(30.0, "stranger");
    try {
        JointBeam::column_head(*column, *turned[0], *turned[1], *turned[2], *turned[3], stranger.get());
    } catch (const std::invalid_argument& error) {
        message = error.what();
    }
    check(message.find("not one of the four") != std::string::npos, "stranger, got: " + message);
    std::cout << fmt::format("joint_beams: column head refused: {}", message) << std::endl;
}

/// The keyed box JointBeam::keyed_box at its default sizes, on other plates and other sizes, and on plates lying flat and turned: each piece goes in in the sequence the mechanism needs, and once the keys are in
/// every piece is stopped every way but a key straight back out. The solids are read from the joint: plates, box, first key, second key. A wedge raised h leaves h times the taper of play.
static void check_keyed_boxes() {

    struct Keyed {
        double first;
        double second;
        double box_width;
        double box_depth;
        double mortise;
        double key_width;
        double key_taper;
        bool standing;
    };
    for (const Keyed& node : std::vector<Keyed>{
             {40.0, 40.0, 2.4, 1.6, 0.6, 1.5, 0.125, true},
             {24.0, 24.0, 2.4, 1.6, 0.6, 1.5, 0.125, true},
             {30.0, 50.0, 2.4, 1.6, 0.6, 1.5, 0.125, true},
             {40.0, 40.0, 3.0, 2.0, 0.9, 2.0, 0.2, true},
             {40.0, 40.0, 2.4, 1.6, 0.6, 1.5, 0.125, false}
         }) {
        WoodSession scene("keyed_box");
        const double size = 300.0;
        const double tall = 400.0;
        const Vector across = node.standing ? Vector(0.0, 1.0, 0.0) : Vector(1.0, 0.0, 0.0);
        const Vector up = node.standing ? Vector(0.0, 0.0, 1.0) : Vector(0.0, 1.0, 0.0);
        const Vector through = across.cross(up);
        const Point centre(0.0, 0.0, 0.0);
        const Point corner = centre - across * (0.5 * size) - up * (0.5 * tall);
        const std::shared_ptr<FacePlanarElement> a = FacePlanarElement::from_rectangle(
            corner - through * node.first, across, up, size, tall, node.first, "a"
        );
        const std::shared_ptr<FacePlanarElement> b =
            FacePlanarElement::from_rectangle(corner, across, up, size, tall, node.second, "b");
        scene.add(a);
        scene.add(b);
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(a, b);
        check(contact != nullptr, "two plates face to face touch");
        check(
            contact->type == ContactType::top_top,
            fmt::format("their contact is {}", to_string(contact->type))
        );
        check(
            JointBeam::keyed_box(*a, *b, *contact, 2.4, 1.6, 0.6, 1.5, 0.0) == nullptr &&
                JointBeam::keyed_box(*a, *b, *contact, 2.4, 1.6, 1.7) == nullptr &&
                JointBeam::keyed_box(*a, *b, *contact, 20.0) == nullptr,
            "straight keys, a mortise wider than the box and a box past the plates are refused"
        );
        const std::shared_ptr<JointBeam> joint = JointBeam::keyed_box(
            *a,
            *b,
            *contact,
            node.box_width,
            node.box_depth,
            node.mortise,
            node.key_width,
            node.key_taper
        );
        check(joint != nullptr, "the keyed box is built");
        scene.add(joint);
        scene.add_interaction(joint, a, joint->interaction(0));
        scene.add_interaction(joint, b, joint->interaction(1));

        const std::vector<std::shared_ptr<Joint>> parts = joint->children();
        check(parts.size() == 3, fmt::format("the connector has {} parts", parts.size()));
        const Mesh plate_a = a->model_geometry_mesh();
        const Mesh plate_b = b->model_geometry_mesh();
        const Mesh box = parts[0]->element_geometry_mesh();
        const Mesh key_first = parts[1]->element_geometry_mesh();
        const Mesh key_second = parts[2]->element_geometry_mesh();
        const std::array<Mesh, 5> solids = {plate_a, plate_b, box, key_first, key_second};
        const std::array<std::string, 5> names = {
            "first plate", "second plate", "box", "first key", "second key"
        };
        for (size_t i = 0; i < 5; i++) {
            check(solids[i].is_closed(), fmt::format("the {} is a closed solid", names[i]));
            check(compute_volume(solids[i]) > 0.0, fmt::format("the {} is empty", names[i]));
        }

        const Vector along = joint->insertion(1);
        const Vector out = joint->insertion(2);
        const Vector side = along.cross(out);
        if (node.standing)
            check(out[2] > 1.0 - 1e-9, "on standing plates the keys come out straight up");
        const auto shift = [](const Vector& way, double distance) {
            return Xform::translation(way[0] * distance, way[1] * distance, way[2] * distance);
        };
        const auto overlap = [](const Mesh& moved, const Mesh& other) {
            return boolean_volume(moved, other, SolidOperation::intersect);
        };
        const auto moved = [&](const Mesh& solid, const Vector& way, double distance) {
            return solid.transformed(shift(way, distance));
        };

        // seated: no two pieces overlap
        double seated = 0.0;
        for (size_t i = 0; i < 5; i++)
            for (size_t j = i + 1; j < 5; j++)
                seated += overlap(solids[i], solids[j]);
        check(seated < 1.0, fmt::format("seated the pieces overlap {:.3f} mm3", seated));

        // the sizes by hand
        const double unit = node.first;
        const double width = node.box_width * unit;
        const double depth = node.box_depth * unit;
        const double slot = node.mortise * unit;
        const double key = node.key_width * unit;
        const double taper = node.key_taper;
        const double head = 1.2 * unit;
        const double proud = 0.5 * unit;
        const double plates = node.first + node.second;
        const double length = plates + 2.0 * (key + head);
        const double mean_mortise = plates + 2.0 * (key - 0.5 * taper * width);
        const double box_by_hand = (length * depth - mean_mortise * slot) * width;
        const double key_by_hand = (key - 0.5 * taper * width) * slot * (width + 2.0 * proud);
        const double plate_by_hand = (size * tall - width * depth) * node.first;
        check(
            std::abs(compute_volume(box) - box_by_hand) < 1e-3 * box_by_hand,
            fmt::format("the box is {:.0f}, by hand {:.0f}", compute_volume(box), box_by_hand)
        );
        check(
            std::abs(compute_volume(key_first) - key_by_hand) < 1e-3 * key_by_hand,
            fmt::format("a key is {:.0f}, by hand {:.0f}", compute_volume(key_first), key_by_hand)
        );
        check(
            std::abs(compute_volume(plate_a) - plate_by_hand) < 1e-3 * plate_by_hand,
            fmt::format("the plate is {:.0f}, by hand {:.0f}", compute_volume(plate_a), plate_by_hand)
        );

        // stage 1: the first key drops into the box, and no further than home
        const double key_length = width + 2.0 * proud;
        double stage = 0.0;
        for (double lift = 2.0 * key_length; lift > 0.0; lift -= 5.0)
            stage = std::max(stage, overlap(moved(key_first, out, lift), box));
        check(stage < 1.0, fmt::format("lowering the first key overlaps {:.3f} mm3", stage));
        const double key_home = overlap(moved(key_first, out, -2.0), box);
        check(key_home > 10.0, fmt::format("past home the key overlaps {:.1f}", key_home));

        // stage 2: box and first key slide in, plain end first, with the plates apart, and the key stops on the second plate
        const double gap = 2.0 * unit;
        const Mesh parted = moved(plate_a, along, -gap);
        for (double travel = length + 2.0 * key_length; travel > 0.0; travel -= 5.0) {
            stage = std::max(stage, overlap(moved(box, along, travel), parted));
            stage = std::max(stage, overlap(moved(box, along, travel), plate_b));
            stage = std::max(stage, overlap(moved(key_first, along, travel), parted));
            stage = std::max(stage, overlap(moved(key_first, along, travel), plate_b));
        }
        check(stage < 1.0, fmt::format("sliding the box in overlaps {:.3f} mm3", stage));
        const double box_home = overlap(moved(key_first, along, -2.0), plate_b);
        check(box_home > 10.0, fmt::format("the key stops on the plate with {:.1f}", box_home));

        // stage 3: the first plate closes on the second, the box still
        for (double travel = gap; travel > 0.0; travel -= 5.0) {
            stage = std::max(stage, overlap(moved(plate_a, along, -travel), box));
            stage = std::max(stage, overlap(moved(plate_a, along, -travel), plate_b));
        }
        check(stage < 1.0, fmt::format("closing the plates overlaps {:.3f} mm3", stage));

        // stage 4: the second key drops in once the plates have met, and not before
        for (double lift = 2.0 * key_length; lift > 0.0; lift -= 5.0)
            for (const Mesh& other : {box, plate_a, plate_b, key_first})
                stage = std::max(stage, overlap(moved(key_second, out, lift), other));
        check(stage < 1.0, fmt::format("lowering the second key overlaps {:.3f} mm3", stage));
        const double too_soon = overlap(key_second, parted);
        check(too_soon > 10.0, fmt::format("before the plates met it overlaps {:.1f}", too_soon));

        // and the box with both keys in can not enter at all: on its way the second key meets the second plate
        double both_keys = 0.0;
        for (double travel = length + 2.0 * key_length; travel > 0.0; travel -= 5.0)
            both_keys = std::max(both_keys, overlap(moved(key_second, along, travel), plate_b));
        check(both_keys > 10.0, fmt::format("the second key first overlaps {:.1f}", both_keys));

        // locked: any group of pieces moved 2 mm any way, the rest still. The only freedom is a key, or both, straight out
        const std::array<Vector, 6> ways = {along, along * -1.0, out, out * -1.0, side, side * -1.0};
        double least = std::numeric_limits<double>::max();
        size_t free_ways = 0;
        for (unsigned group = 1; group < 31; group++)
            for (size_t w = 0; w < 6; w++) {
                double blocked = 0.0;
                for (size_t i = 0; i < 5; i++)
                    for (size_t j = 0; j < 5; j++)
                        if ((group >> i & 1u) && !(group >> j & 1u))
                            blocked += overlap(moved(solids[i], ways[w], 2.0), solids[j]);
                const bool only_keys = (group & 7u) == 0u;
                const bool only_rest = (group & 7u) == 7u;
                const bool key_out = (only_keys && w == 2) || (only_rest && w == 3);
                if (key_out) {
                    check(blocked < 1.0, fmt::format("group {} out overlaps {:.3f}", group, blocked));
                    free_ways++;
                    continue;
                }
                check(blocked > 1.0, fmt::format("group {} moves along way {}: {:.3f}", group, w, blocked));
                least = std::min(least, blocked);
            }
        check(free_ways == 6, fmt::format("{} free moves, not 6", free_ways));

        // the second key is what locks it: without it the first plate leaves and the box backs out
        const double plate_leaves = overlap(moved(plate_a, along, -3.0 * unit), box) +
                                    overlap(moved(plate_a, along, -3.0 * unit), key_first);
        const double box_backs = overlap(moved(box, along, 3.0 * unit), plate_a) +
                                 overlap(moved(box, along, 3.0 * unit), plate_b);
        check(plate_leaves < 1.0, fmt::format("the plate still held {:.3f}", plate_leaves));
        check(box_backs < 1.0, fmt::format("the box still held {:.3f}", box_backs));

        // the wedge: raised h, plate and key share h times the taper of play, and no more
        const double raised = 8.0;
        const double play = raised * taper;
        const Mesh key_up = moved(key_second, out, raised);
        const double within = overlap(moved(key_up, along, -0.9 * play), box);
        const double beyond = overlap(moved(key_up, along, -1.5 * play), box);
        check(within < 1e-3, fmt::format("within its play the key overlaps {:.4f}", within));
        check(beyond > 1e-3, fmt::format("past its play the key overlaps {:.4f}", beyond));

        std::cout << fmt::format(
                         "joint_beams: keyed box plates {:g} and {:g}, {}: seated overlap {:.3f} mm3, "
                         "sequence clear {:.3f}, past home {:.0f}, the key stops on the plate {:.0f}, "
                         "key before the plates meet {:.0f}, second key first {:.0f}, least blocked move {:.0f} of 180, "
                         "{} free, box {:.0f} of {:.0f} by hand, a key {:.0f} of {:.0f}, raised {:g} "
                         "leaves {:.2f} of play",
                         node.first,
                         node.second,
                         node.standing ? "standing" : "flat",
                         seated,
                         stage,
                         key_home,
                         box_home,
                         too_soon,
                         both_keys,
                         least,
                         free_ways,
                         compute_volume(box),
                         box_by_hand,
                         compute_volume(key_first),
                         key_by_hand,
                         raised,
                         play
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
        const std::array<std::shared_ptr<EdgeStraightElement>, 2> pair = EdgeStraightElement::pair_end_to_end(end_case.first);
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
        for (const std::shared_ptr<EdgeStraightElement>& member : pair) {
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
    check_column_head_orders();
    check_column_head_ends();
    check_column_head_angles();
    check_column_head_groups();
    check_column_head_limit();
    check_column_head_brep();
    check_column_head_refusals();
    check_keyed_boxes();
    check_end_to_end();
    return 0;
}
