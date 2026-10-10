#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// cr_c_ip_2 on two upright plates crossing at 45 degrees at their middles, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_cr_c_ip_angle_45");

    // two 400 x 200 plates 40 thick standing upright, the second crossing the first at 45 degrees in plan
    const std::shared_ptr<Plate> first = Plate::from_rectangle({0.0, 20.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, 400.0, 200.0, 40.0, "first");
    const std::shared_ptr<Plate> second = Plate::from_rectangle({44.437, -127.279, 0.0}, {0.707, 0.707, 0.0}, {0.0, 0.0, 1.0}, 400.0, 200.0, 40.0, "second");
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
cr_c_ip_2 on its defaults on two upright 400 x 200 plates 40 thick crossing at their middles at 45 degrees in plan, as the oracle's cr@45 fixture: each milled half-lap follows the oblique crossing, its walls on the 0.01 mm grid 2024 clipped the slots on; the second plate is lifted 250 afterwards so both half-laps read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_cr_c_ip_angle_45 --parallel 6 && ./build/element_joint_plate_cr_c_ip_angle_45 && ../bash/publish-scene.sh --target element_joint_plate_cr_c_ip_angle_45

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
