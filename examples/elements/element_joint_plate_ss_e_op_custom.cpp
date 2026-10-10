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

    // the user's tile in the joint's unit box, mapped onto the corner the plates share: the floor's thickness along x, the wall's along y,
    // z along the 400 long joint line; three fingers on the floor, wide, narrow, wide, and the sockets they fill in the wall, at the user's own
    // stations along the joint line
    // (z = +-0.42, +-0.18, +-0.06: fingers of 96, 48 and 96 on the 400 joint line), drawn as the library's own ss_e_op_0 draws its fingers: per face a zigzag
    // across the thickness and the two-point line of the joint's ends, the wall's on y = 0.5 (face 0) and y = -0.5 (face 1), the floor's
    // on x = 0.5 (face 0) and x = -0.5 (face 1)
    const double a = 0.42;
    const double b = 0.18;
    const double c = 0.06;
    std::array<std::vector<Polyline>, 2> wall_faces;
    std::array<std::vector<Polyline>, 2> floor_faces;
    for (int face = 0; face < 2; face++) {
        const double side = face == 0 ? 0.5 : -0.5;
        floor_faces[face] = {
            Polyline({
                Point(side, 0.5, -a), Point(side, -0.5, -a), Point(side, -0.5, -b), Point(side, 0.5, -b),
                Point(side, 0.5, -c), Point(side, -0.5, -c), Point(side, -0.5, c), Point(side, 0.5, c),
                Point(side, 0.5, b), Point(side, -0.5, b), Point(side, -0.5, a), Point(side, 0.5, a),
            }),
            Polyline({Point(side, 0.5, -0.5), Point(side, 0.5, 0.5)}),
        };
        wall_faces[face] = {
            Polyline({
                Point(-0.5, side, a), Point(0.5, side, a), Point(0.5, side, b), Point(-0.5, side, b),
                Point(-0.5, side, c), Point(0.5, side, c), Point(0.5, side, -c), Point(-0.5, side, -c),
                Point(-0.5, side, -b), Point(0.5, side, -b), Point(0.5, side, -a), Point(-0.5, side, -a),
            }),
            Polyline({Point(-0.5, side, 0.5), Point(-0.5, side, -0.5)}),
        };
    }
    const std::vector<Polyline> male = {wall_faces[0][0], wall_faces[1][0], wall_faces[0][1], wall_faces[1][1]};
    const std::vector<Polyline> female = {floor_faces[0][0], floor_faces[1][0], floor_faces[0][1], floor_faces[1][1]};

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
The out-of-plane design ss_e_op_custom on one pair: a floor and a wall meeting at a right angle on a mitre, joined by a tile the user draws in the joint's unit box: three fingers on the floor, wide, narrow, wide (96, 48 and 96 of the 400 joint line), and the sockets they fill in the wall, at the user's own stations z = +-0.42, +-0.18, +-0.06. The tile is drawn as the library's own ss_e_op_0 draws its fingers, per face a zigzag across the thickness and the two-point line of the joint's ends; ss_e_op_custom merges an open profile into the edge as the library's fingers are merged (2024 left it uncut), a closed rectangle is clipped as 2024 clipped it. The joint is oriented on the face contact and passed to each plate with add_interaction, the wall first; the wall is moved off along the mitre's normal so the fingers and the sockets read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_op_custom --parallel 6 && ./build/element_joint_plate_ss_e_op_custom && ../bash/publish-scene.sh --target element_joint_plate_ss_e_op_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
