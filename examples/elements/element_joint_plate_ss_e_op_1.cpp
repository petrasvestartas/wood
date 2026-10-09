#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_op_1: the parametric finger joint of a floor and a wall plate mitred at a right angle, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_op_1");

    // a floor and a wall meeting at a right angle, their side faces mitred on one plane
    const Polyline floor_bottom({{0.0, 0.0, 0.0}, {300.0, 0.0, 0.0}, {300.0, 400.0, 0.0}, {0.0, 400.0, 0.0}, {0.0, 0.0, 0.0}});
    const Polyline floor_top({{0.0, 0.0, 40.0}, {260.0, 0.0, 40.0}, {260.0, 400.0, 40.0}, {0.0, 400.0, 40.0}, {0.0, 0.0, 40.0}});
    const Polyline wall_bottom({{300.0, 0.0, 0.0}, {300.0, 400.0, 0.0}, {300.0, 400.0, 300.0}, {300.0, 0.0, 300.0}, {300.0, 0.0, 0.0}});
    const Polyline wall_top({{260.0, 0.0, 40.0}, {260.0, 400.0, 40.0}, {260.0, 400.0, 300.0}, {260.0, 0.0, 300.0}, {260.0, 0.0, 40.0}});
    const std::shared_ptr<Plate> floor = std::make_shared<Plate>(floor_bottom, floor_top, "floor");
    const std::shared_ptr<Plate> wall = std::make_shared<Plate>(wall_bottom, wall_top, "wall");
    scene.add(floor);
    scene.add(wall);

    // the joint from their contact, added, and passed to each plate in its target order: the wall first, the male of an out-of-plane pair
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(floor, wall);
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_op_1(8);
    joint->orient(contact, {floor, wall});
    scene.add(joint);
    scene.add_interaction(joint, wall, joint->interaction(0));
    scene.add_interaction(joint, floor, joint->interaction(1));

    // drawn apart along the normal of the wall's contact face, the mitre, the direction the wall slides onto the floor
    wall->place(Xform::translation(100.0, 0.0, 100.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The out-of-plane design ss_e_op_1 on one pair: a 300 x 400 floor plate and a 300 high wall plate, both 40 thick, meeting at a right angle on a mitred side face; the joint is oriented on their face contact and passed to each plate with add_interaction, the wall its male side and the floor its female, eight divisions at the family's shift 0.64, a zigzag of fingers merged into both mitred edges, each plate's fingers in the other's notches; the joint owns no piece and stays hidden; the wall is moved 100 out and 100 up afterwards, along the mitre's normal, so both sides read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_op_1 --parallel 6 && ./build/element_joint_plate_ss_e_op_1 && ../bash/publish-scene.sh --target element_joint_plate_ss_e_op_1

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
