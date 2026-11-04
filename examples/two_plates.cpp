#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Two plates, one joint: an upright plate stands on a base plate and is tenoned into it.
int main() {

    WoodSession scene("two_plates");

    // an upright plate on the middle of a base plate
    const std::array<std::shared_ptr<Plate>, 2> pair = Plate::pair_top_side(90.0);
    const std::shared_ptr<Plate> base = pair[0];
    const std::shared_ptr<Plate> upright = pair[1];
    scene.add(base);
    scene.add(upright);

    // where they touch, and the tenon joint placed on that contact
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(base, upright);
    scene.add_interaction(base, upright, contact);
    const std::shared_ptr<JointPlate> joint = JointPlate::top_side_tenons();
    joint->orient(contact, {base, upright});

    // the joint cuts tenons into the upright and mortises into the base
    scene.add(joint);
    scene.add_interaction(joint, upright, joint->interaction(0));
    scene.add_interaction(joint, base, joint->interaction(1));

    // the upright lifted off its base, so the tenons and mortises show
    const Vector apart = (upright->polylines[0].center() - base->polylines[0].center()).normalized() * 120.0;
    upright->place(Xform::translation(apart[0], apart[1], apart[2]));

    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Two plates and one joint: an upright plate on a base plate, the contact between them, and a tenon joint cut into both, drawn apart.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target two_plates --parallel 6 && ./build/two_plates && ../bash/publish-scene.sh --target two_plates

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
