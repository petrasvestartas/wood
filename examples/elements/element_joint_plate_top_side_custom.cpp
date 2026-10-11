#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// top_side_custom: Manis's chamfered tenons, two through tenons narrowing to their tips on an upright plate and their tapered mortises in the base, on the pairs upright and leaning to 60 degrees, each drawn apart.
int main() {

    WoodSession scene("element_joint_plate_top_side_custom");

    // Manis's chamfered tenons (IBOIS, add_chamfered_tenons) in the unit box: x the upright's thickness, y the base's from its top (-0.5)
    // to its bottom (0.5), z along the joint line; two through tenons 0.2 of the joint line wide at the base's top narrowing to 0.12 at its
    // bottom, centred at z = -0.22 and 0.22, per upright face the profile and its end line, per base face each mortise at both widths
    const auto profile = [](double x) {
        return Polyline({
            {x, -0.5, -0.45}, {x, -0.5, -0.32}, {x, 0.5, -0.28}, {x, 0.5, -0.16}, {x, -0.5, -0.12},
            {x, -0.5, 0.12}, {x, 0.5, 0.16}, {x, 0.5, 0.28}, {x, -0.5, 0.32}, {x, -0.5, 0.45},
        });
    };
    const auto end_line = [](double x) {
        return Polyline({{x, -0.5, -0.45}, {x, -0.5, 0.45}});
    };
    const auto mortise = [](double centre, double y, double half) {
        return Polyline({{-0.5, y, centre + half}, {0.5, y, centre + half}, {0.5, y, centre - half}, {-0.5, y, centre - half}, {-0.5, y, centre + half}});
    };
    const std::vector<Polyline> male = {profile(0.5), profile(-0.5), end_line(0.5), end_line(-0.5)};
    const std::vector<Polyline> female = {mortise(-0.22, -0.5, 0.1), mortise(-0.22, 0.5, 0.06), mortise(0.22, -0.5, 0.1), mortise(0.22, 0.5, 0.06)};

    // a 400 x 400 base and a 250 x 250 upright on its middle, both 40 thick: upright and leaning to 60 degrees, the second pair 800 along y, each in its group
    for (const double angle : {90.0, 60.0}) {
        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("angle_{:g}", angle));
        const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_top_side(angle);
        const std::shared_ptr<Plate> base = pair[0];
        const std::shared_ptr<Plate> upright = pair[1];
        for (const std::shared_ptr<Plate>& plate : pair) {
            plate->place(Xform::translation(0.0, angle == 90.0 ? 0.0 : 800.0, 0.0));
            plate->name = fmt::format("{}_{:g}", plate->name, angle);
            scene.add(plate, group);
        }

        // the joint from their contact, added, and passed to each plate in its target order
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
        const std::shared_ptr<JointPlate> joint = JointPlate::top_side_custom(male, female);
        joint->orient(contact, {base, upright});
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, upright, joint->interaction(0));
        scene.add_interaction(joint, base, joint->interaction(1));

        // drawn apart 120 along the line between the two plates' centres, so both sides read
        const Vector apart = (upright->polylines[0].center() - base->polylines[0].center()).normalized() * 120.0;
        upright->place(Xform::translation(apart[0], apart[1], apart[2]));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
top_side_custom: Manis's chamfered tenons, two through tenons narrowing to their tips on an upright plate and their tapered mortises in the base, on the pairs upright and leaning to 60 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_top_side_custom --parallel 6 && ./build/element_joint_plate_top_side_custom && ../bash/publish-scene.sh --target element_joint_plate_top_side_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
