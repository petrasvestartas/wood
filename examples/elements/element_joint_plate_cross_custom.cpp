#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// cross_custom: Manis's half-lap with a funnel mouth, a slot chamfered open at its mouth in each of two upright plates crossing at their middles, on the pairs square and crossing at 60 degrees, each drawn apart.
int main() {

    WoodSession scene("element_joint_plate_cross_custom");

    // Manis's half-lap with a funnel mouth (IBOIS, add_halflap) in the joint's unit box, mapped onto the cross: one plate's thickness along
    // x, the other's along y and the cross's depth along z, its middle at z = 0; a closed outline per face clipped into each plate, the
    // male's on the faces at y = 0.5 and y = -0.5, the female's on x = -0.5 and x = 0.5: each slot straight from its bottom at z = -0.4, then
    // chamfered out at 45 degrees by a quarter of the thickness on each side, so the other plate slides in
    const auto male_slot = [](double y) {
        return Polyline({{0.75, y, -1.0}, {-0.75, y, -1.0}, {-0.5, y, -0.75}, {-0.5, y, -0.4}, {0.5, y, -0.4}, {0.5, y, -0.75}, {0.75, y, -1.0}});
    };
    const auto female_slot = [](double x) {
        return Polyline({{x, -0.75, 1.0}, {x, 0.75, 1.0}, {x, 0.5, 0.75}, {x, 0.5, -0.4}, {x, -0.5, -0.4}, {x, -0.5, 0.75}, {x, -0.75, 1.0}});
    };
    const std::vector<Polyline> male = {male_slot(0.5), male_slot(-0.5)};
    const std::vector<Polyline> female = {female_slot(-0.5), female_slot(0.5)};

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
        const std::shared_ptr<JointPlate> joint = JointPlate::cross_custom(male, female);
        joint->orient(contact, {first, second});
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, first, joint->interaction(0));
        scene.add_interaction(joint, second, joint->interaction(1));

        // drawn apart 250 down, the way the second plate comes out of its slot
        const Vector apart(0.0, 0.0, -250.0);
        second->place(Xform::translation(apart[0], apart[1], apart[2]));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
cross_custom: Manis's half-lap with a funnel mouth, a slot chamfered open at its mouth in each of two upright plates crossing at their middles, on the pairs square and crossing at 60 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_cross_custom --parallel 6 && ./build/element_joint_plate_cross_custom && ../bash/publish-scene.sh --target element_joint_plate_cross_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
