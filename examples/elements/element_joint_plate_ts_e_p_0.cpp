#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// ts_e_p_0: the three fixed tenons of an upright plate standing on a base plate, through three mortises in the base, drawn apart.
int main() {

    WoodSession scene("element_joint_plate_ts_e_p_0");

    // a 400 x 400 base and a 250 x 250 upright on its middle, both 40 thick
    const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_top_side(90.0);
    const std::shared_ptr<Plate> base = pair[0];
    const std::shared_ptr<Plate> upright = pair[1];
    scene.add(base);
    scene.add(upright);

    // the joint from their contact, added, and passed to each plate in its target order: the upright first, the male of a top-side pair
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
    const std::shared_ptr<JointPlate> joint = JointPlate::ts_e_p_0();
    joint->orient(contact, {base, upright});
    scene.add(joint);
    scene.add_interaction(joint, upright, joint->interaction(0));
    scene.add_interaction(joint, base, joint->interaction(1));

    // drawn apart along the base's normal, the direction the upright drops onto the base, so the tenons and the mortises both read
    upright->place(Xform::translation(0.0, 0.0, 120.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
ts_e_p_0: the three fixed tenons of an upright plate standing on a base plate, through three mortises in the base, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_plate_ts_e_p_0 --parallel 6 && ./build/element_joint_plate_ts_e_p_0 && ../bash/publish-scene.sh --target element_joint_plate_ts_e_p_0

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
