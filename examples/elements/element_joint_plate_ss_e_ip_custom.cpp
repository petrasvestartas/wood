#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_ip_custom: your own rectangles in the joint's unit box, a notch into each of two plates edge to edge in one plane, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_ip_custom");

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

    // the outlines in the joint's unit box: x across the seam, y through the thickness, z along the joint line; one pair per
    // side, its face 0 at y = -0.5 and its face 1 at y = 0.5; as 2024 merged a custom pair, a closed rectangle of five points
    // is clipped into the plate: here a notch half a thickness deep into the male plate's edge over one stretch of the joint
    // line and one into the female plate's edge over another
    const std::vector<Polyline> male = {
        Polyline({{-0.5, -0.5, 0.4}, {0.5, -0.5, 0.4}, {0.5, -0.5, 0.1}, {-0.5, -0.5, 0.1}, {-0.5, -0.5, 0.4}}),
        Polyline({{-0.5, 0.5, 0.4}, {0.5, 0.5, 0.4}, {0.5, 0.5, 0.1}, {-0.5, 0.5, 0.1}, {-0.5, 0.5, 0.4}}),
    };
    const std::vector<Polyline> female = {
        Polyline({{-0.5, -0.5, -0.1}, {0.5, -0.5, -0.1}, {0.5, -0.5, -0.4}, {-0.5, -0.5, -0.4}, {-0.5, -0.5, -0.1}}),
        Polyline({{-0.5, 0.5, -0.1}, {0.5, 0.5, -0.1}, {0.5, 0.5, -0.4}, {-0.5, 0.5, -0.4}, {-0.5, 0.5, -0.1}}),
    };

    // the joint from their contact, added, and passed to each plate in its target order
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_custom(male, female);
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
The in-plane design ss_e_ip_custom on one pair: two 300 x 400 plates 40 thick meeting edge to edge in one plane; the joint takes your own outlines in its unit box, is oriented on their face contact and passed to each plate with add_interaction, the box mapped onto the whole contact, its x across the seam scaled to the thickness; as the 2024 library kept a custom pair, the outlines carry the fabrication type nothing and only a closed rectangle of five points, or a line of two, is merged into the plate: here a rectangle on each face of the male plate cuts a notch half a thickness deep into its edge over one stretch of the seam and one on each face of the female cuts the same notch into hers over another; the joint stays hidden; the right plate is moved 120 along the seam's normal afterwards.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_ip_custom --parallel 6 && ./build/element_joint_plate_ss_e_ip_custom && ../bash/publish-scene.sh --target element_joint_plate_ss_e_ip_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
