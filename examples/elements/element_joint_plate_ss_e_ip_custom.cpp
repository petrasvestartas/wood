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

    // the user's tile in the joint's unit box: x across the seam, about a plate thickness (40), y through the thickness (face 0 at y = -0.5,
    // face 1 at y = 0.5), z along the 400 long joint line; three jigsaw tabs, each a round head of radius 10 on a neck 9 wide, the middle
    // one into the left plate and the outer two into the right, drawn once per face as an open profile from one end of the joint line to
    // the other, the seam itself as the closing two-point line, as the library's own in-plane designs draw it; male and female share the
    // profile, each plate keeps its side of it
    const double head_x = 10.0 / 40.0;
    const double head_z = 10.0 / 400.0;
    const double neck_angle = std::acos((0.03 - head_x) / head_x);
    std::array<std::vector<Polyline>, 2> faces;
    for (int face = 0; face < 2; face++) {
        const double y = face == 0 ? -0.5 : 0.5;
        std::vector<Point> profile = {Point(0.0, y, 0.5)};
        for (const auto& [centre, side] : std::array<std::pair<double, double>, 3>{{{0.3, 1.0}, {0.0, -1.0}, {-0.3, 1.0}}}) {
            profile.push_back(Point(0.0, y, centre + head_z * std::sin(neck_angle)));
            for (int k = 0; k <= 16; k++) {
                const double angle = neck_angle - 2.0 * neck_angle * k / 16.0;
                profile.push_back(Point(side * head_x * (1.0 + std::cos(angle)), y, centre + head_z * std::sin(angle)));
            }
            profile.push_back(Point(0.0, y, centre - head_z * std::sin(neck_angle)));
        }
        profile.push_back(Point(0.0, y, -0.5));
        faces[face] = {Polyline(profile), Polyline({Point(0.0, y, 0.5), Point(0.0, y, -0.5)})};
    }
    const std::vector<Polyline> male = {faces[0][0], faces[1][0], faces[0][1], faces[1][1]};
    const std::vector<Polyline> female = male;

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
The in-plane design ss_e_ip_custom on one pair: two 300 x 400 plates 40 thick meeting edge to edge in one plane, joined by a tile the user draws in the joint's unit box: three jigsaw tabs, each a round head of radius 10 on a neck 9 wide, the middle one into the left plate and the outer two into the right. The tile is drawn as the library's own in-plane designs draw theirs: per face an open profile from one end of the joint line to the other and the seam as a two-point line, sized in millimetres against the box, about a plate thickness (40) across the seam and the 400 long joint line along it. The joint is oriented on the plates' face contact and passed to each plate with add_interaction; each plate merges its side of the profile into its outline, the right plate is moved 120 off along the seam's normal so both edges read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ss_e_ip_custom --parallel 6 && ./build/element_joint_plate_ss_e_ip_custom && ../bash/publish-scene.sh --target element_joint_plate_ss_e_ip_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
