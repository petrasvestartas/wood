#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// cr_c_ip_2 on two upright plates crossing at 45 degrees at their middles, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_cr_c_ip_angle_45");

    // two upright 400 x 200 plates 40 thick, the second crossing the first at 45 degrees in plan
    const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_crossing(45.0);
    const std::shared_ptr<Plate> first = pair[0];
    const std::shared_ptr<Plate> second = pair[1];
    scene.add(first);
    scene.add(second);

    // the joint from their cross contact, added, and passed to each plate in the contact's order
    const std::shared_ptr<InteractionContactCross> contact = scene.compute_cross_contact(first, second);
    const std::shared_ptr<JointPlate> joint = JointPlate::cr_c_ip_2();
    joint->orient(contact, {first, second});
    scene.add(joint);
    scene.add_interaction(joint, first, joint->interaction(0));
    scene.add_interaction(joint, second, joint->interaction(1));

    // drawn apart: the second plate lifted 250, the direction it slides down onto the first
    second->place(Xform::translation(0.0, 0.0, 250.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
cr_c_ip_2 on two upright plates crossing at 45 degrees at their middles, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_cr_c_ip_angle_45 --parallel 6 && ./build/element_joint_plate_cr_c_ip_angle_45 && ../bash/publish-scene.sh --target element_joint_plate_cr_c_ip_angle_45

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
