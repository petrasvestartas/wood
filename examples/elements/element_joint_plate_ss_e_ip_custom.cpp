#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_ip_custom: your own feather joint, sharp V teeth, between two plates edge to edge in one plane, on the pairs in line and bent to 150 degrees, each drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_ip_custom");

    // the unit box: x across the seam, y the thickness, z along the joint line; per face a feather profile, eight sharp teeth
    // alternating across the seam, and its end line, shared by both plates, as the library writes ss_e_ip_0
    const auto profile = [](double y) {
        std::vector<Point> points = {Point(0.0, y, 0.5)};
        for (int i = 0; i <= 8; i++)
            points.push_back(Point(i % 2 == 0 ? -0.5 : 0.5, y, 0.45 - i * 0.1125));
        points.push_back(Point(0.0, y, -0.5));
        return Polyline(points);
    };
    const auto end_line = [](double y) {
        return Polyline({{0.0, y, 0.5}, {0.0, y, -0.5}});
    };
    const std::vector<Polyline> outlines = {profile(-0.5), profile(0.5), end_line(-0.5), end_line(0.5)};

    // two 300 x 400 plates 40 thick edge to edge in one plane, their seam the mitre: in line and bent to 150 degrees, the second pair 800 along y, each in its group
    for (const double angle : {180.0, 150.0}) {
        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("angle_{:g}", angle));
        const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_in_plane(angle);
        const std::shared_ptr<Plate> left = pair[0];
        const std::shared_ptr<Plate> right = pair[1];
        for (const std::shared_ptr<Plate>& plate : pair) {
            plate->place(Xform::translation(0.0, angle == 180.0 ? 0.0 : 800.0, 0.0));
            plate->name = fmt::format("{}_{:g}", plate->name, angle);
            scene.add(plate, group);
        }

        // the joint from their contact, added, and passed to each plate in its target order
        const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
        const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_custom(outlines, outlines);
        joint->orient(contact, {left, right});
        joint->name = fmt::format("{}_{:g}", joint->name, angle);
        scene.add(joint, group);
        scene.add_interaction(joint, left, joint->interaction(0));
        scene.add_interaction(joint, right, joint->interaction(1));

        // drawn apart 100 along the line between the two plates' centres, so both sides read
        const Vector apart = (right->polylines[0].center() - left->polylines[0].center()).normalized() * 100.0;
        right->place(Xform::translation(apart[0], apart[1], apart[2]));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
ss_e_ip_custom: your own feather joint, sharp V teeth, between two plates edge to edge in one plane, on the pairs in line and bent to 150 degrees, each drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_ip_custom --parallel 6 && ./build/element_joint_plate_ss_e_ip_custom && ../bash/publish-scene.sh --target element_joint_plate_ss_e_ip_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
