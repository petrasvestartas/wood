#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The parts of a wedge connector alone: its wedge with its bores, and its pins.
int main() {

    WoodSession scene("element_connector_part");

    const std::shared_ptr<BeamVariable> left = BeamVariable::between(
        Polyline::rectangle({0.0, -200.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 200.0, 300.0),
        Polyline::rectangle({1600.0, -200.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 200.0, 300.0),
        "left"
    );
    const std::shared_ptr<BeamVariable> right = BeamVariable::between(
        Polyline::rectangle({0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 200.0, 300.0),
        Polyline::rectangle({1600.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 200.0, 300.0),
        "right"
    );
    left->is_visible = false;
    right->is_visible = false;
    scene.add(left);
    scene.add(right);

    // the wedge from their contact, added with its parts and pins nested under it
    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
    const std::shared_ptr<JointBeam> wedge = JointBeam::wedge(
        *left,
        *right,
        *contact,
        300.0,
        133.0
    );
    wedge->name = "wedge";
    scene.add(wedge);
    scene.add_interaction(wedge, left, wedge->interaction(0));
    scene.add_interaction(wedge, right, wedge->interaction(1));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The wedge connector of element_joint_beam_wedge with its two beams hidden: JointBeam::children() gives its wedge as a ConnectorPart, with the connector's cuts and the exact bores of its pins, and each pin as a Pin; the session nests them under the connector when it is added.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_connector_part --parallel 6 && ./build/element_connector_part && ../bash/publish-scene.sh --target element_connector_part

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
