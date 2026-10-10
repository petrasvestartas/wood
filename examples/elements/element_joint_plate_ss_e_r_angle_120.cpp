#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_r_0 on two plates folded 120 degrees along a mitred seam, the scene reading the contact as rotated, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_r_angle_120");
    scene.settings.all_treated_as_rotated = true;
    scene.settings.rotated_joint_as_average = true;

    // a 300 x 400 plate and a 300 wide one, both 40 thick, folded 120 degrees on the 400 seam, mitred on the bisector
    const std::shared_ptr<Plate> first = std::make_shared<Plate>(Polyline({{0.0, 0.0, 0.0}, {300.0, 0.0, 0.0}, {300.0, 400.0, 0.0}, {0.0, 400.0, 0.0}, {0.0, 0.0, 0.0}}), Polyline({{0.0, 0.0, 40.0}, {276.906, 0.0, 40.0}, {276.906, 400.0, 40.0}, {0.0, 400.0, 40.0}, {0.0, 0.0, 40.0}}), "first");
    const std::shared_ptr<Plate> second = std::make_shared<Plate>(Polyline({{300.0, 0.0, 0.0}, {300.0, 400.0, 0.0}, {450.0, 400.0, 259.808}, {450.0, 0.0, 259.808}, {300.0, 0.0, 0.0}}), Polyline({{276.906, 0.0, 40.0}, {276.906, 400.0, 40.0}, {415.359, 400.0, 279.808}, {415.359, 0.0, 279.808}, {276.906, 0.0, 40.0}}), "second");
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
    second->place(Xform::translation(129.904, 0.0, 75.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
ss_e_r_0 on two 40 thick plates folded 120 degrees on their 400 seam, mitred on the bisector, the scene treating every side-to-side contact as rotated, as the oracle's r@120 fixture: the four slices of the design follow the seam at the angle; the second plate is moved 150 along the mitre's normal afterwards so both edges read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_r_angle_120 --parallel 6 && ./build/element_joint_plate_ss_e_r_angle_120 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_r_angle_120

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
