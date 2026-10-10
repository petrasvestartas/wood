#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_op_custom: your own rectangles in the joint's unit box, a slot into the wall and a notch into the floor of a pair mitred at a right angle, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_op_custom");

    // a floor and a wall meeting at a right angle, their side faces mitred on one plane
    const Polyline floor_bottom({{0.0, 0.0, 0.0}, {300.0, 0.0, 0.0}, {300.0, 400.0, 0.0}, {0.0, 400.0, 0.0}, {0.0, 0.0, 0.0}});
    const Polyline floor_top({{0.0, 0.0, 40.0}, {260.0, 0.0, 40.0}, {260.0, 400.0, 40.0}, {0.0, 400.0, 40.0}, {0.0, 0.0, 40.0}});
    const Polyline wall_bottom({{300.0, 0.0, 0.0}, {300.0, 400.0, 0.0}, {300.0, 400.0, 300.0}, {300.0, 0.0, 300.0}, {300.0, 0.0, 0.0}});
    const Polyline wall_top({{260.0, 0.0, 40.0}, {260.0, 400.0, 40.0}, {260.0, 400.0, 300.0}, {260.0, 0.0, 300.0}, {260.0, 0.0, 40.0}});
    const std::shared_ptr<Plate> floor = std::make_shared<Plate>(floor_bottom, floor_top, "floor");
    const std::shared_ptr<Plate> wall = std::make_shared<Plate>(wall_bottom, wall_top, "wall");
    scene.add(floor);
    scene.add(wall);

    // the outlines in the joint's unit box, mapped onto the corner the plates share: the floor's thickness along x, the wall's
    // along y, z along the joint line; a pair per side, the wall's at y = 0.5 (face 0) and y = -0.5 (face 1), the floor's at
    // x = 0.5 (face 0) and x = -0.5 (face 1); as 2024 merged a custom pair, a closed rectangle of five points is clipped into
    // the plate: here a slot into the wall's bottom edge, 20 past the floor's top, and a notch into the floor's mitred edge,
    // 40 past the wall's inner face, each over its own stretch of the joint line
    const std::vector<Polyline> male = {
        Polyline({{-1.0, 0.5, 0.4}, {1.0, 0.5, 0.4}, {1.0, 0.5, 0.1}, {-1.0, 0.5, 0.1}, {-1.0, 0.5, 0.4}}),
        Polyline({{-1.0, -0.5, 0.4}, {1.0, -0.5, 0.4}, {1.0, -0.5, 0.1}, {-1.0, -0.5, 0.1}, {-1.0, -0.5, 0.4}}),
    };
    const std::vector<Polyline> female = {
        Polyline({{0.5, -1.0, -0.1}, {0.5, 1.0, -0.1}, {0.5, 1.0, -0.4}, {0.5, -1.0, -0.4}, {0.5, -1.0, -0.1}}),
        Polyline({{-0.5, -1.0, -0.1}, {-0.5, 1.0, -0.1}, {-0.5, 1.0, -0.4}, {-0.5, -1.0, -0.4}, {-0.5, -1.0, -0.1}}),
    };

    // the joint from their contact, added, and passed to each plate in its target order: the wall first, the male of an out-of-plane pair
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(floor, wall);
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_op_custom(male, female);
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
The out-of-plane design ss_e_op_custom on one pair: a 300 x 400 floor plate and a 300 high wall plate, both 40 thick, meeting at a right angle on a mitred side face; the joint is oriented on their face contact and passed to each plate with add_interaction, the wall its male side and the floor its female, your own outlines in its unit box, the box mapped onto the corner the plates share; as the 2024 library kept a custom pair, the outlines carry the fabrication type nothing and only a closed rectangle of five points, or a line of two, is merged into the plate: here a rectangle on each face of the wall cuts a slot into its bottom edge, 20 past the floor's top, and one on each face of the floor cuts a notch into its mitred edge, 40 past the wall's inner face, each over its own stretch of the joint line; the joint owns no piece and stays hidden; the wall is moved 100 out and 100 up afterwards, along the mitre's normal, so both sides read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_op_custom --parallel 6 && ./build/element_joint_plate_ss_e_op_custom && ../bash/publish-scene.sh --target element_joint_plate_ss_e_op_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
