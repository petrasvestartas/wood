#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// side_removal_ss_e_r_1 merged: the ss_e_r_1 arc tile at the middle of the joint line of two plates folded along a shared edge, the side removal 2024 swapped off the stock, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_r_side_removal_ss_e_r_1");
    scene.settings.all_treated_as_rotated = true;
    scene.settings.rotated_joint_as_average = true;

    // two 300 x 400 plates 40 thick folded 120 degrees along their shared edge, the side faces mitred on the bisector
    const Polyline left_bottom({{0.0, 0.0, 0.0}, {300.0, 0.0, 0.0}, {300.0, 400.0, 0.0}, {0.0, 400.0, 0.0}, {0.0, 0.0, 0.0}});
    const Polyline left_top({{0.0, 0.0, 40.0}, {276.906, 0.0, 40.0}, {276.906, 400.0, 40.0}, {0.0, 400.0, 40.0}, {0.0, 0.0, 40.0}});
    const Polyline right_bottom({{300.0, 0.0, 0.0}, {450.0, 0.0, 259.808}, {450.0, 400.0, 259.808}, {300.0, 400.0, 0.0}, {300.0, 0.0, 0.0}});
    const Polyline right_top({{276.906, 0.0, 40.0}, {415.359, 0.0, 279.808}, {415.359, 400.0, 279.808}, {276.906, 400.0, 40.0}, {276.906, 0.0, 40.0}});
    const std::shared_ptr<Plate> left = std::make_shared<Plate>(left_bottom, left_top, "left");
    const std::shared_ptr<Plate> right = std::make_shared<Plate>(right_bottom, right_top, "right");
    scene.add(left);
    scene.add(right);

    // the joint from their contact, added, and passed to each plate in its target order: interaction(0) to the male the solver designated, interaction(1) to the female
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
    const std::shared_ptr<JointPlate> joint = JointPlate::side_removal_ss_e_r_1(true);
    joint->orient(contact, {left, right}, scene.settings);
    scene.add(joint);
    scene.add_interaction(joint, left, joint->interaction(0));
    scene.add_interaction(joint, right, joint->interaction(1));

    // drawn apart along the mitre's normal, the direction the plates slide together, so both sides read
    right->place(Xform::translation(129.904, 0.0, 75.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The rotated family's side_removal_ss_e_r_1 under merge_with_joint on one pair: two 300 x 400 plates 40 thick folded 120 degrees along their shared edge, their side faces mitred on the bisector, the scene set to read every side-to-side contact as rotated; the joint is oriented on their face contact and passed to each plate with add_interaction, the side removal outlines of both faces and the ss_e_r_1 arc tile oriented on two 20 x 20 rectangles at the middle of the joint line, offset by the conic allowance, cut out of the male's third outline and appended as conic, mill and reverse conic cuts; kept as 2024 wrote it, the merged form swaps the sides last and hands each plate its own side slab outside its stock, so no side is removed and only the tile's conic slivers cut; the right plate is moved 150 along the mitre's normal so both sides read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_r_side_removal_ss_e_r_1 --parallel 6 && ./build/element_joint_plate_ss_e_r_side_removal_ss_e_r_1 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_r_side_removal_ss_e_r_1

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
