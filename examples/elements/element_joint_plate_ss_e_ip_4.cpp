#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_ip_4: the two crossed milled keys and four drills of two plates edge to edge in one plane, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_ip_4");

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
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_4();
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
The in-plane design ss_e_ip_4 on one pair: two 300 x 400 plates 40 thick meeting edge to edge in one plane; the joint is oriented on their face contact and passed to each plate with add_interaction, two slanted key grooves crossing each other milled into each edge, each projected from face to face, and four drills through the thickness, two in each plate; the joint owns no piece and stays hidden, each plate hosts its grooves and its two drills; the right plate is moved 200 along the seam's normal afterwards so both grooves read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_ip_4 --parallel 6 && ./build/element_joint_plate_ss_e_ip_4 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_ip_4

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
