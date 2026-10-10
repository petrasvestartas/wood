#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_r_0 on two blocks end to end, the second twisted 40 degrees about the seam, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_r_angle_40");
    scene.settings.rotated_joint_as_average = true;

    // two 300 x 400 blocks 100 thick end to end, the second twisted 40 degrees about the seam's normal
    const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_rotated(40.0);
    const std::shared_ptr<Plate> first = pair[0];
    const std::shared_ptr<Plate> second = pair[1];
    scene.add(first);
    scene.add(second);

    // the joint from their contact, added, and passed to each plate in the contact's order
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(first, second);
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_r_0();
    joint->orient(contact, {first, second});
    scene.add(joint);
    scene.add_interaction(joint, first, joint->interaction(0));
    scene.add_interaction(joint, second, joint->interaction(1));

    // drawn apart 150 along the mitre's normal
    second->place(Xform::translation(150.0, 0.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
ss_e_r_0 on two blocks end to end, the second twisted 40 degrees about the seam, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_r_angle_40 --parallel 6 && ./build/element_joint_plate_ss_e_r_angle_40 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_r_angle_40

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
