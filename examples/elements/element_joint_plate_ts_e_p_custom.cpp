#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ts_e_p_custom: your own two tenons on an upright plate and their mortises in the base, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ts_e_p_custom");

    // a 400 x 400 base and a 250 x 250 upright standing on it, both 40 thick
    const std::shared_ptr<Plate> base = Plate::from_rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 400.0, 400.0, 40.0, "base");
    const std::shared_ptr<Plate> upright = Plate::from_rectangle({180.0, 75.0, 40.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 250.0, 250.0, 40.0, "upright");
    scene.add(base);
    scene.add(upright);

    // the unit box: x the upright's thickness, y the base's, z along the joint line; per upright face the tenon profile and its end
    // line, per base face one rectangle per mortise
    const auto profile = [](double x) {
        return Polyline({{x, -0.5, -0.35}, {x, 0.5, -0.35}, {x, 0.5, -0.1}, {x, -0.5, -0.1}, {x, -0.5, 0.1}, {x, 0.5, 0.1}, {x, 0.5, 0.35}, {x, -0.5, 0.35}});
    };
    const auto end_line = [](double x) {
        return Polyline({{x, -0.5, -0.35}, {x, -0.5, 0.35}});
    };
    const auto mortise = [](double y, double z0, double z1) {
        return Polyline({{-0.5, y, z1}, {0.5, y, z1}, {0.5, y, z0}, {-0.5, y, z0}, {-0.5, y, z1}});
    };
    const std::vector<Polyline> male = {profile(0.5), profile(-0.5), end_line(0.5), end_line(-0.5)};
    const std::vector<Polyline> female = {mortise(-0.5, 0.1, 0.35), mortise(0.5, 0.1, 0.35), mortise(-0.5, -0.35, -0.1), mortise(0.5, -0.35, -0.1)};

    // the joint, added, and passed to each plate in its target order: the upright first
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
    const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_custom(male, female);
    joint->orient(contact, {base, upright});
    scene.add(joint);
    scene.add_interaction(joint, upright, joint->interaction(0));
    scene.add_interaction(joint, base, joint->interaction(1));

    // drawn apart: the upright lifted
    upright->place(Xform::translation(0.0, 0.0, 120.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
ts_e_p_custom on an upright standing on a base: your own two tenons, written as the library writes ts_e_p_0, cut into the upright's edge, and their mortises cut through the base.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ts_e_p_custom --parallel 6 && ./build/element_joint_plate_ts_e_p_custom && ../bash/publish-scene.sh --target element_joint_plate_ts_e_p_custom

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
