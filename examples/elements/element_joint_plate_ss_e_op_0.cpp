#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_op_0: the fixed three-finger joint of a floor and a wall plate mitred at a right angle, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_op_0");

    // a 300 x 400 floor and a 300 high wall, both 40 thick, at a right angle, mitred on the bisector
    const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_folded(90.0);
    const std::shared_ptr<Plate> floor = pair[0];
    const std::shared_ptr<Plate> wall = pair[1];
    scene.add(floor);
    scene.add(wall);

    // the joint from their contact, added, and passed to each plate in its target order: the wall first, the male of an out-of-plane pair
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(floor, wall);
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_op_0();
    joint->orient(contact, {floor, wall});
    scene.add(joint);
    scene.add_interaction(joint, wall, joint->interaction(0));
    scene.add_interaction(joint, floor, joint->interaction(1));

    // drawn apart along the normal of the wall's contact face, the mitre, the direction the wall slides onto the floor
    wall->place(Xform::translation(100.0, 0.0, 100.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
ss_e_op_0: the fixed three-finger joint of a floor and a wall plate mitred at a right angle, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_op_0 --parallel 6 && ./build/element_joint_plate_ss_e_op_0 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_op_0

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
