#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_ip_1: the parametric finger joint of two plates edge to edge in one plane, eight divisions, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_ip_1");

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
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_1(8, 0.5);
    joint->orient(contact, {left, right});
    scene.add(joint);
    scene.add_interaction(joint, left, joint->interaction(0));
    scene.add_interaction(joint, right, joint->interaction(1));

    // drawn apart along the seam's normal, in the plates' plane, so both edges read
    right->place(Xform::translation(120.0, 0.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The in-plane design ss_e_ip_1 on one pair, eight divisions and the shift 0.5: two 300 x 400 plates 40 thick meeting edge to edge in one plane; the joint is oriented on their face contact and passed to each plate with add_interaction, its zigzag of square fingers, 20 deep, merged into both edges, four on each plate; on its defaults the count is geometric, one division every 300 of the joint line, at least two and even, which on this 400 seam is a single tongue; the right plate is moved 120 along the seam's normal afterwards so both edges read. The joint owns no piece and stays hidden.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_ip_1 --parallel 6 && ./build/element_joint_plate_ss_e_ip_1 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_ip_1

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
