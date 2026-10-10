#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_ip_1 on two plates edge to edge in one plane, their seam slanted 75 degrees to their edges, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_ip_angle_75");

    // two 40 thick plates in one plane sharing a seam that runs at 75 degrees to the plates' bottom edge
    const Polyline left({{0.0, 0.0, 0.0}, {300.0, 0.0, 0.0}, {192.82, 400.0, 0.0}, {0.0, 400.0, 0.0}, {0.0, 0.0, 0.0}});
    const Polyline right({{300.0, 0.0, 0.0}, {600.0, 0.0, 0.0}, {600.0, 400.0, 0.0}, {192.82, 400.0, 0.0}, {300.0, 0.0, 0.0}});
    const std::shared_ptr<Plate> left_plate = std::make_shared<Plate>(left, left.transformed(Xform::translation(0.0, 0.0, 40.0)), "left");
    const std::shared_ptr<Plate> right_plate = std::make_shared<Plate>(right, right.transformed(Xform::translation(0.0, 0.0, 40.0)), "right");
    scene.add(left_plate);
    scene.add(right_plate);

    // the joint from their contact, added, and passed to each plate in the contact's order
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left_plate, right_plate);
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_1(8);
    joint->orient(contact, {left_plate, right_plate});
    scene.add(joint);
    scene.add_interaction(joint, left_plate, joint->interaction(0));
    scene.add_interaction(joint, right_plate, joint->interaction(1));

    // drawn apart 150 along the seam's normal in the plates' plane
    right_plate->place(Xform::translation(144.889, 38.823, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
ss_e_ip_1 with eight divisions on two 40 thick plates edge to edge in one plane whose seam runs at 75 degrees to their bottom edge, 414 long: the fingers keep their shape along the slanted seam; the right plate is moved 150 along the seam's normal in the plates' plane afterwards so both edges read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_ip_angle_75 --parallel 6 && ./build/element_joint_plate_ss_e_ip_angle_75 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_ip_angle_75

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
