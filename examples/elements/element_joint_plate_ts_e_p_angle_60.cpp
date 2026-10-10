#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ts_e_p_3 on an upright standing on a base, its foot skewed 60 degrees to the base's edges in plan, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ts_e_p_angle_60");

    // a 400 x 400 base and a 250 x 250 upright, both 40 thick, the upright's foot on the base's middle at 60 degrees to its x axis
    const std::shared_ptr<Plate> base = Plate::from_rectangle({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 400.0, 400.0, 40.0, "base");
    const std::shared_ptr<Plate> upright = Plate::from_rectangle({120.179, 101.747, 40.0}, {0.5, 0.866, 0.0}, {0.0, 0.0, 1.0}, 250.0, 250.0, 40.0, "upright");
    scene.add(base);
    scene.add(upright);

    // the joint from their contact, added, and passed to each plate in its target order: the upright first, its tenons, then the base, its mortises
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
    const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_3();
    joint->orient(contact, {base, upright});
    scene.add(joint);
    scene.add_interaction(joint, upright, joint->interaction(0));
    scene.add_interaction(joint, base, joint->interaction(1));

    // drawn apart: the upright lifted 150, the direction it drops into the base
    upright->place(Xform::translation(0.0, 0.0, 150.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
ts_e_p_3 on its defaults on a 400 x 400 base and a 250 x 250 upright, both 40 thick, the upright standing on the base's middle with its foot at 60 degrees to the base's x axis, as the oracle's ts@skew60 fixture: the tenons and the mortises follow the skewed contact; the upright is lifted 150 afterwards so the mortises read.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ts_e_p_angle_60 --parallel 6 && ./build/element_joint_plate_ts_e_p_angle_60 && ../bash/publish-scene.sh --target element_joint_plate_ts_e_p_angle_60

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
