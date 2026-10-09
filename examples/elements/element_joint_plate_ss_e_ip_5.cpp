#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_ip_5: the reversed-tooth keys of two plates edge to edge in one plane, on its defaults, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_ip_5");

    const std::shared_ptr<Plate> left = Plate::from_rectangle(
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        300.0,
        400.0,
        40.0,
        "left"
    );
    const std::shared_ptr<Plate> right = Plate::from_rectangle(
        {300.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        300.0,
        400.0,
        40.0,
        "right"
    );
    scene.add(left);
    scene.add(right);

    // the joint from their contact, added, and passed to each plate in its target order
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_5();
    joint->orient(contact, {left, right});
    joint->is_visible = true; // the keys are the joint's own pieces
    scene.add(joint);
    scene.add_interaction(joint, left, joint->interaction(0));
    scene.add_interaction(joint, right, joint->interaction(1));

    // drawn apart along the seam's normal, in the plates' plane, so both edges read
    right->place(Xform::translation(200.0, 0.0, 0.0));

    // the keys half way between the pockets they fill
    joint->place(Xform::translation(100.0, 0.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The in-plane design ss_e_ip_5 on one pair, on its defaults: two 300 x 400 plates 40 thick meeting edge to edge in one plane; the joint is oriented on their face contact and passed to each plate with add_interaction, an eight-point tooth pocket into each edge per division, each reversed, the count geometric, one every 300 of the joint line, the tooth scaled to the male plate's thickness; the joint is drawn, its pieces the keys that fill both pockets; the right plate is moved 200 along the seam's normal and the keys 100 afterwards so the pockets and the keys read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_ip_5 --parallel 6 && ./build/element_joint_plate_ss_e_ip_5 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_ip_5

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
