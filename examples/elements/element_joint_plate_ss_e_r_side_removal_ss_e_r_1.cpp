#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// side_removal_ss_e_r_1: the trait de Jupiter of two plates in line, their key in the slot at the redan, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_r_side_removal_ss_e_r_1");

    // two 300 x 400 plates 40 thick, edge to edge
    const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_in_plane();
    const std::shared_ptr<Plate> left = pair[0];
    const std::shared_ptr<Plate> right = pair[1];
    scene.add(left);
    scene.add(right);

    // the joint from their contact, added, and passed to each plate in its target order
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
    const std::shared_ptr<JointPlate> joint = JointPlate::side_removal_ss_e_r_1();
    joint->orient(contact, {left, right});
    scene.add(joint);
    scene.add_interaction(joint, left, joint->interaction(0));
    scene.add_interaction(joint, right, joint->interaction(1));

    // drawn apart along the seam's normal, in the plates' plane, so both edges read
    right->place(Xform::translation(200.0, 0.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
side_removal_ss_e_r_1: the trait de Jupiter of two plates in line, their key in the slot at the redan, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_r_side_removal_ss_e_r_1 --parallel 6 && ./build/element_joint_plate_ss_e_r_side_removal_ss_e_r_1 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_r_side_removal_ss_e_r_1

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
