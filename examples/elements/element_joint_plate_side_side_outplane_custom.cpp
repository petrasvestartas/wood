#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// side_side_outplane_custom.
int main() {

    WoodSession scene("element_joint_plate_side_side_outplane_custom");

    const std::array<double, 6> walls = {-0.375, -0.225, -0.075, 0.075, 0.225, 0.375};
    const auto fanned = [](double wall, double x, double y) {
        const double turn = 10.0 * Tolerance::TO_RADIANS * wall / 0.375;
        return wall + std::tan(turn) * 0.5 * (x + y);
    };
    std::array<std::vector<Polyline>, 2> wall_faces;
    std::array<std::vector<Polyline>, 2> floor_faces;
    for (int face = 0; face < 2; face++) {
        const double side = face == 0 ? 0.5 : -0.5;
        std::vector<Point> floor_points;
        std::vector<Point> wall_points;
        for (size_t k = 0; k < walls.size(); k++) {
            const double first = k % 2 == 0 ? 0.5 : -0.5;
            floor_points.push_back(Point(side, first, fanned(walls[k], side, first)));
            floor_points.push_back(Point(side, -first, fanned(walls[k], side, -first)));
            const double wall = walls[walls.size() - 1 - k];
            wall_points.push_back(Point(-first, side, fanned(wall, -first, side)));
            wall_points.push_back(Point(first, side, fanned(wall, first, side)));
        }
        floor_faces[face] = {
            Polyline(floor_points), Polyline({Point(side, 0.5, -0.5), Point(side, 0.5, 0.5)})
        };
        wall_faces[face] = {
            Polyline(wall_points), Polyline({Point(-0.5, side, 0.5), Point(-0.5, side, -0.5)})
        };
    }
    const std::vector<Polyline> male = {
        wall_faces[0][0], wall_faces[1][0], wall_faces[0][1], wall_faces[1][1]
    };
    const std::vector<Polyline> female = {
        floor_faces[0][0], floor_faces[1][0], floor_faces[0][1], floor_faces[1][1]
    };

    // a 300 x 400 floor and a 300 high wall, both 40 thick, mitered on the bisector: square and folded to 60 degrees, the second pair 800 along y, each in its group
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

        const std::shared_ptr<InteractionContactFace> contact =
            scene.compute_face_contact(floor, wall);
        const std::shared_ptr<JointPlate> joint =
            JointPlate::side_side_outplane_custom(male, female);
        joint->orient(contact, {floor, wall});
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, wall, joint->interaction(0));
        scene.add_interaction(joint, floor, joint->interaction(1));

        const Vector apart =
            (wall->polylines[0].center() - floor->polylines[0].center()).normalized() * 141.4;
        wall->place(Xform::translation(apart[0], apart[1], apart[2]));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
side_side_outplane_custom: Manis's sunrise, three tenons whose walls fan out like rays, in the joint's unit box of a pair mitered at a right angle, on the pairs square and folded to 60 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_side_side_outplane_custom --parallel 6 && ./build/element_joint_plate_side_side_outplane_custom && ../bash/publish-scene.sh --target element_joint_plate_side_side_outplane_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
