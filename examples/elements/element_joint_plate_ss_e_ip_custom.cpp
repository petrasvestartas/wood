#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ss_e_ip_custom: your own feather joint, sharp V teeth, between two plates edge to edge in one plane, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ss_e_ip_custom");

    // two 300 x 400 plates 40 thick, edge to edge
    const std::shared_ptr<Plate> left = Plate::from_rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 300.0, 400.0, 40.0, "left");
    const std::shared_ptr<Plate> right = Plate::from_rectangle({300.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 300.0, 400.0, 40.0, "right");
    scene.add(left);
    scene.add(right);

    // the unit box: x across the seam, y the thickness, z along the joint line; per face a feather profile, eight sharp teeth
    // alternating across the seam, and its end line, shared by both plates, as the library writes ss_e_ip_0
    const auto profile = [](double y) {
        std::vector<Point> points = {Point(0.0, y, 0.45)};
        for (int i = 0; i <= 8; i++)
            points.push_back(Point(i % 2 == 0 ? -0.5 : 0.5, y, 0.45 - i * 0.1125));
        points.push_back(Point(0.0, y, -0.45));
        return Polyline(points);
    };
    const auto end_line = [](double y) {
        return Polyline({{0.0, y, 0.5}, {0.0, y, -0.5}});
    };
    const std::vector<Polyline> outlines = {profile(-0.5), profile(0.5), end_line(-0.5), end_line(0.5)};

    // the joint, added, and passed to each plate in its target order
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
    const std::shared_ptr<JointPlate> joint = JointPlate::ss_e_ip_custom(outlines, outlines);
    joint->orient(contact, {left, right});
    scene.add(joint);
    scene.add_interaction(joint, left, joint->interaction(0));
    scene.add_interaction(joint, right, joint->interaction(1));

    // drawn apart: the right plate moved along the seam's normal
    right->place(Xform::translation(100.0, 0.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
ss_e_ip_custom on two plates edge to edge: your own feather joint, eight sharp V teeth written as the library writes ss_e_ip_0, merged into both edges.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_ip_custom --parallel 6 && ./build/element_joint_plate_ss_e_ip_custom && ../bash/publish-scene.sh --target element_joint_plate_ss_e_ip_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
