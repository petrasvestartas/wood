#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// blind_mortise_tenon on beams: a beam ending square on the side of another in a tenon, drawn out of its mortise.
int main() {

    WoodSession scene("element_joint_beam_blind_mortise_tenon");

    const std::shared_ptr<EdgeStraightElement> carrier = EdgeStraightElement::square(Polyline({Point(-500.0, 0.0, 0.0), Point(500.0, 0.0, 0.0)}), 70.0, "carrier");
    const std::shared_ptr<EdgeStraightElement> member = EdgeStraightElement::square(Polyline({Point(0.0, 0.0, 0.0), Point(0.0, 600.0, 0.0)}), 60.0, "member");
    scene.add(carrier);
    scene.add(member);

    const std::shared_ptr<JointBeamElement> joint = JointBeamElement::blind_mortise_tenon(*member, *carrier);
    scene.add(joint);
    scene.add_interaction(joint, member, joint->interaction(0));
    scene.add_interaction(joint, carrier, joint->interaction(1));

    member->place(Xform::translation(0.0, 200.0, 0.0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
blind_mortise_tenon on beams, the verdeckter Zapfen of the Joinery Compendium (p. 496): the member's end crosscut to length (A1), its cheeks ripped and crosscut (A2) and its edge shoulders sawn (A3) into a tenon a third of its width, set in by a sixth along the carrier; the mortise chopped half the carrier's depth (B1). The member is drawn 200 mm out along its axis, the one way it comes out.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_blind_mortise_tenon --parallel 6 && ./build/element_joint_beam_blind_mortise_tenon && ../bash/publish-scene.sh --target element_joint_beam_blind_mortise_tenon

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
