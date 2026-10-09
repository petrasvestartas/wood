#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_r_3: the diamond tenon pockets of two plates edge to edge, their contact read as rotated, on its defaults, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_r_3");
    scene.settings.all_treated_as_rotated = true;
    scene.settings.rotated_joint_as_average = true;

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
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_r_3();
    joint->orient(contact, {left, right}, scene.settings);
    scene.add(joint);
    scene.add_interaction(joint, left, joint->interaction(0));
    scene.add_interaction(joint, right, joint->interaction(1));

    // drawn apart along the seam's normal, in the plates' plane, so both edges read
    right->place(Xform::translation(160.0, 0.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The rotated design ss_e_r_3 on one pair, on its defaults: two 300 x 400 plates 40 thick meeting edge to edge in one plane, the scene set to read every side-to-side contact as rotated; the joint is oriented on their face contact and passed to each plate with add_interaction, a diamond pocket milled into each edge per division, the count geometric, one every 300 of the joint line, the tile scaled to the male plate's thickness; the right plate is moved 160 along the seam's normal so both pockets read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_r_3 --parallel 6 && ./build/element_joint_plate_ss_e_r_3 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_r_3

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
