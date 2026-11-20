#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// notched_oblique_thrust on beams: a strut leaning at 40 degrees into a notch in the side of a carrier, drawn out along its insertion.
int main() {

    WoodSession scene("element_joint_beam_notched_oblique_thrust");

    const double turn = 140.0 * Tolerance::PI / 180.0;
    const std::shared_ptr<EdgeStraightElement> carrier = EdgeStraightElement::square(Polyline({Point(-500.0, 0.0, 0.0), Point(500.0, 0.0, 0.0)}), 60.0, "carrier");
    const std::shared_ptr<EdgeStraightElement> strut = EdgeStraightElement::square(Polyline({Point(0.0, 0.0, 0.0), Point(700.0 * std::cos(turn), 700.0 * std::sin(turn), 0.0)}), 60.0, "strut");
    scene.add(carrier);
    scene.add(strut);

    const std::shared_ptr<JointBeamElement> joint = JointBeamElement::notched_oblique_thrust(*strut, *carrier);
    scene.add(joint);
    scene.add_interaction(joint, strut, joint->interaction(0));
    scene.add_interaction(joint, carrier, joint->interaction(1));

    const Vector out = joint->insertion_directions(0)[0] * -150.0;
    strut->place(Xform::translation(out[0], out[1], out[2]));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
notched_oblique_thrust on beams, the Stirnversatz of the Joinery Compendium (p. 584): the strut's end, sawn along the front face and the seat (A1, A2), sits in a triangular notch sawn into the side of the carrier (B1, B2), the front face bisecting the obtuse angle between them. The strut is drawn 150 mm out along its insertion, the bisector of the notch's two faces.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_notched_oblique_thrust --parallel 6 && ./build/element_joint_beam_notched_oblique_thrust && ../bash/publish-scene.sh --target element_joint_beam_notched_oblique_thrust

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
