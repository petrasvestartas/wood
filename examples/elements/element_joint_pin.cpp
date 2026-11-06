#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A pin: one cylinder along its axis.
int main() {

    WoodSession scene("element_joint_pin");

    scene.add(
        std::make_shared<JointPinElement>(Line::from_points({0.0, 0.0, 0.0}, {0.0, 0.0, 200.0}), 10.0, 0.05)
    );

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A pin 10 in radius along a 200 long axis, meshed so no facet strays more than 0.05 from the circle; a connector made by JointBeamElement::centred_pins or headed_pins nests its pins under it like this one.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_pin --parallel 6 && ./build/element_joint_pin && ../bash/publish-scene.sh --target element_joint_pin

|||||||| TIME ||||||||
0.44 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
