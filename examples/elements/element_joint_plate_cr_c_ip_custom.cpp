#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// cr_c_ip_custom: your own rectangles in the joint's unit box, an unequal slot into each of two upright plates crossing at their middles, on the pairs square and crossing at 60 degrees, each drawn apart.
int main() {

    WoodSession scene("element_joint_plate_cr_c_ip_custom");

    // the outlines in the joint's unit box, mapped onto the cross: one plate's thickness along x, the other's along y and the
    // cross's depth along z, its middle at z = 0, as 2024's library drew the family; a pair per side, the male's on the faces at
    // y = 0.5 and y = -0.5, the first plate's on this pair, the male of a cross, and the female's on the faces at x = -0.5 and
    // x = 0.5, the second's; as 2024 merged a custom pair, a closed rectangle of five points is clipped into the plate's outline:
    // here the slots split the depth unequally, the male's from the first plate's top edge, where cr_c_ip_0 starts its slot, to
    // z = -0.4, 60 deep, and the female's from the second plate's bottom edge to the same z = -0.4, 140 deep
    const std::vector<Polyline> male = {
        Polyline({{0.5, 0.5, -1.0}, {-0.5, 0.5, -1.0}, {-0.5, 0.5, -0.4}, {0.5, 0.5, -0.4}, {0.5, 0.5, -1.0}}),
        Polyline({{0.5, -0.5, -1.0}, {-0.5, -0.5, -1.0}, {-0.5, -0.5, -0.4}, {0.5, -0.5, -0.4}, {0.5, -0.5, -1.0}}),
    };
    const std::vector<Polyline> female = {
        Polyline({{-0.5, 0.5, 1.0}, {-0.5, -0.5, 1.0}, {-0.5, -0.5, -0.4}, {-0.5, 0.5, -0.4}, {-0.5, 0.5, 1.0}}),
        Polyline({{0.5, 0.5, 1.0}, {0.5, -0.5, 1.0}, {0.5, -0.5, -0.4}, {0.5, 0.5, -0.4}, {0.5, 0.5, 1.0}}),
    };

    // two upright 400 x 200 plates 40 thick crossing at their middles: square and crossing at 60 degrees, the second pair 800 along y, each in its group
    for (const double angle : {90.0, 60.0}) {
        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("angle_{:g}", angle));
        const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_crossing(angle);
        const std::shared_ptr<Plate> first = pair[0];
        const std::shared_ptr<Plate> second = pair[1];
        for (const std::shared_ptr<Plate>& plate : pair) {
            plate->place(Xform::translation(0.0, angle == 90.0 ? 0.0 : 800.0, 0.0));
            plate->name = fmt::format("{}_{:g}", plate->name, angle);
            scene.add(plate, group);
        }

        // the joint from their contact, added, and passed to each plate in its target order
        const std::shared_ptr<InteractionContactCross> contact = scene.compute_cross_contact(first, second);
        const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_custom(male, female);
        joint->orient(contact, {first, second});
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, first, joint->interaction(0));
        scene.add_interaction(joint, second, joint->interaction(1));

        // drawn apart 250 up, so both sides read
        const Vector apart(0.0, 0.0, 250.0);
        second->place(Xform::translation(apart[0], apart[1], apart[2]));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
cr_c_ip_custom: your own rectangles in the joint's unit box, an unequal slot into each of two upright plates crossing at their middles, on the pairs square and crossing at 60 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_cr_c_ip_custom --parallel 6 && ./build/element_joint_plate_cr_c_ip_custom && ../bash/publish-scene.sh --target element_joint_plate_cr_c_ip_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
