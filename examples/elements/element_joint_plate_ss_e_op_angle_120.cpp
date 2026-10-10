#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_op_1 on a floor and a wall folded 120 degrees, their side faces mitred on the bisector, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_op_angle_120");

    // a 300 x 400 floor and a 300 high wall, both 40 thick, meeting at 120 degrees on the 400 seam, mitred on the bisector
    const std::shared_ptr<Plate> floor = std::make_shared<Plate>(Polyline({{0.0, 0.0, 0.0}, {300.0, 0.0, 0.0}, {300.0, 400.0, 0.0}, {0.0, 400.0, 0.0}, {0.0, 0.0, 0.0}}), Polyline({{0.0, 0.0, 40.0}, {276.906, 0.0, 40.0}, {276.906, 400.0, 40.0}, {0.0, 400.0, 40.0}, {0.0, 0.0, 40.0}}), "floor");
    const std::shared_ptr<Plate> wall = std::make_shared<Plate>(Polyline({{300.0, 0.0, 0.0}, {300.0, 400.0, 0.0}, {450.0, 400.0, 259.808}, {450.0, 0.0, 259.808}, {300.0, 0.0, 0.0}}), Polyline({{276.906, 0.0, 40.0}, {276.906, 400.0, 40.0}, {415.359, 400.0, 279.808}, {415.359, 0.0, 279.808}, {276.906, 0.0, 40.0}}), "wall");
    scene.add(floor);
    scene.add(wall);

    // the joint from their contact, added, and passed to each plate in its target order: the wall first, the male of an out-of-plane pair
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(floor, wall);
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_op_1(8);
    joint->orient(contact, {floor, wall});
    scene.add(joint);
    scene.add_interaction(joint, wall, joint->interaction(0));
    scene.add_interaction(joint, floor, joint->interaction(1));

    // drawn apart 150 along the mitre's normal, the direction the wall slides onto the floor
    wall->place(Xform::translation(129.904, 0.0, 75.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
ss_e_op_1 with eight divisions on a 300 x 400 floor and a 300 high wall, both 40 thick, folded 120 degrees on their 400 seam, their side faces mitred on the bisector through the outer and the inner corner, as the oracle's op@120 fixture; the fingers keep the family's shape at the angle, merged into both mitred edges; the wall is moved 150 along the mitre's normal afterwards so both edges read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_op_angle_120 --parallel 6 && ./build/element_joint_plate_ss_e_op_angle_120 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_op_angle_120

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
