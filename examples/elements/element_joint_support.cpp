#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A column on its support: the seat is a joint.
int main() {

    WoodSession scene("element_joint_support");

    const std::shared_ptr<NodeSupportElement> support =
        std::make_shared<NodeSupportElement>(Plane::xy_plane(), "support");
    const Line axis = support->column_axis(600.0);
    const std::shared_ptr<EdgeStraightElement> column = EdgeStraightElement::corner_square(
        axis,
        Plane(axis.start() - Vector(110.0, 110.0, 0.0), {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}),
        220.0
    );
    scene.add(support);
    scene.add(column);

    const std::shared_ptr<JointElement> seat = JointElement::support(*support, *column);
    scene.add(seat);
    scene.add_interaction(seat, column, seat->interaction(0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A column on its support: JointElement::support recesses the head plate and bores the pins into the column end.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_support --parallel 6 && ./build/element_joint_support && ../bash/publish-scene.sh --target element_joint_support

|||||||| TIME ||||||||
2.71 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
