#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_r_3: the diamond pockets of two plates folded along a shared edge, their contact read as rotated, and the key that fills both, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_r_3");
    scene.settings.rotated_joint_as_average = true;

    // two 300 x 400 blocks 100 thick end to end, their side faces on one plane, the right one twisted 20 degrees about the seam's normal
    const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_rotated(20.0);
    const std::shared_ptr<Plate> left = pair[0];
    const std::shared_ptr<Plate> right = pair[1];
    scene.add(left);
    scene.add(right);

    // the joint from their contact, added, and passed to each plate in its target order
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_r_3();
    joint->orient(contact, {left, right}, scene.settings);
    joint->is_visible = true; // the key is the joint's own piece
    scene.add(joint);
    scene.add_interaction(joint, left, joint->interaction(0));
    scene.add_interaction(joint, right, joint->interaction(1));

    // drawn apart along the mitre's normal, the direction the plates slide together, so both pockets read, the key half way
    right->place(Xform::translation(150.0, 0.0, 0.0));
    joint->place(Xform::translation(75.0, 0.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
ss_e_r_3: the diamond pockets of two plates folded along a shared edge, their contact read as rotated, and the key that fills both, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_r_3 --parallel 6 && ./build/element_joint_plate_ss_e_r_3 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_r_3

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
