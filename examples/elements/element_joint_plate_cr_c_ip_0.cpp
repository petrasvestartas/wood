#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// cr_c_ip_0: the plain slot of two upright plates crossing at their middles, a slot into each over half the cross's depth, on the pairs square and crossing at 60 degrees, each drawn apart.
int main() {

    WoodSession scene("element_joint_plate_cr_c_ip_0");

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
        const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_0();
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
cr_c_ip_0: the plain slot of two upright plates crossing at their middles, a slot into each over half the cross's depth, on the pairs square and crossing at 60 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_cr_c_ip_0 --parallel 6 && ./build/element_joint_plate_cr_c_ip_0 && ../bash/publish-scene.sh --target element_joint_plate_cr_c_ip_0

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
