#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// corner_custom: your own rectangles in the joint's unit box, a slot into the wall and a notch into the floor of a pair mitred at a right angle, on the pairs square and folded to 60 degrees, each drawn apart.
int main() {

    WoodSession scene("element_joint_plate_corner_custom");

    // the user's tile in the joint's unit box, mapped onto the corner the plates share: the floor's thickness along x, the wall's along y,
    // z along the 400 long joint line; three fingers on the floor, wide, narrow, wide, and the sockets they fill in the wall, at the user's own
    // stations along the joint line
    // (z = +-0.42, +-0.18, +-0.06: fingers of 96, 48 and 96 on the 400 joint line), drawn as the library's own corner_three_fingers draws its fingers: per face a zigzag
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

    // a 300 x 400 floor and a 300 high wall, both 40 thick, mitred on the bisector: square and folded to 60 degrees, the second pair 800 along y, each in its group
    for (const double angle : {90.0, 60.0}) {
        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("angle_{:g}", angle));
        const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_folded(angle);
        const std::shared_ptr<Plate> floor = pair[0];
        const std::shared_ptr<Plate> wall = pair[1];
        for (const std::shared_ptr<Plate>& plate : pair) {
            plate->place(Xform::translation(0.0, angle == 90.0 ? 0.0 : 800.0, 0.0));
            plate->name = fmt::format("{}_{:g}", plate->name, angle);
            scene.add(plate, group);
        }

        // the joint from their contact, added, and passed to each plate in its target order
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(floor, wall);
        const std::shared_ptr<JointPlate> joint = JointPlate::corner_custom(male, female);
        joint->orient(contact, {floor, wall});
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, wall, joint->interaction(0));
        scene.add_interaction(joint, floor, joint->interaction(1));

        // drawn apart 141 along the line between the two plates' centres, so both sides read
        const Vector apart = (wall->polylines[0].center() - floor->polylines[0].center()).normalized() * 141.4;
        wall->place(Xform::translation(apart[0], apart[1], apart[2]));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
corner_custom: your own rectangles in the joint's unit box, a slot into the wall and a notch into the floor of a pair mitred at a right angle, on the pairs square and folded to 60 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_corner_custom --parallel 6 && ./build/element_joint_plate_corner_custom && ../bash/publish-scene.sh --target element_joint_plate_corner_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
