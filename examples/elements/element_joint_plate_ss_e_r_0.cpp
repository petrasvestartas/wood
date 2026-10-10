#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_r_0: the four slices of two plates folded along a shared edge, their contact read as rotated, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_r_0");
    scene.settings.rotated_joint_as_average = true;

    // two 300 x 400 blocks 100 thick end to end, their side faces on one plane, the right one twisted 20 degrees about the seam's normal
    const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_rotated(20.0);
    const std::shared_ptr<Plate> left = pair[0];
    const std::shared_ptr<Plate> right = pair[1];
    scene.add(left);
    scene.add(right);

    // the joint from their contact, added, and passed to each plate in its target order
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_r_0();
    joint->orient(contact, {left, right}, scene.settings);
    scene.add(joint);
    scene.add_interaction(joint, left, joint->interaction(0));
    scene.add_interaction(joint, right, joint->interaction(1));

    // drawn apart along the mitre's normal, the direction the plates slide together, so both sides read
    right->place(Xform::translation(150.0, 0.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The rotated design ss_e_r_0 on one pair: two 300 x 400 plates 40 thick folded 120 degrees along their shared edge, their side faces mitred on the bisector, the scene set to read every side-to-side contact as rotated; the joint is oriented on their face contact and passed to each plate with add_interaction, each joint volume split in half through the thickness and the halves offset along the joint line, four slices on each plate taken in world space without orienting; the right plate is moved 150 along the mitre's normal so both sides read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_r_0 --parallel 6 && ./build/element_joint_plate_ss_e_r_0 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_r_0

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
