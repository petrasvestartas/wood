#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_op_17: the fingers with flat mitre caps of a floor and a wall plate mitred at a right angle, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_op_17");

    // a 300 x 400 floor and a 300 high wall, both 40 thick, at a right angle, mitred on the bisector
    const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_folded(90.0);
    const std::shared_ptr<Plate> floor = pair[0];
    const std::shared_ptr<Plate> wall = pair[1];
    scene.add(floor);
    scene.add(wall);

    // the joint from their contact, added, and passed to each plate in its target order: the wall first, the male of an out-of-plane pair
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(floor, wall);
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_op_17(4);
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
The out-of-plane design ss_e_op_17 on one pair: a 300 x 400 floor plate and a 300 high wall plate, both 40 thick, meeting at a right angle on a mitred side face; the joint is oriented on their face contact and passed to each plate with add_interaction, the wall its male side and the floor its female, four divisions, so two fingers: ss_e_op_0 with the finger count parametric and the mitre capped flat at both ends, merged into both mitred edges; the joint owns no piece and stays hidden; the wall is moved 100 out and 100 up afterwards, along the mitre's normal, so both sides read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_op_17 --parallel 6 && ./build/element_joint_plate_ss_e_op_17 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_op_17

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
