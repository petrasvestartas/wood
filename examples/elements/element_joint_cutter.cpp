#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Joints that cut by a solid: a box taken away from a beam, and a profile it is trimmed to.
int main() {

    WoodSession scene("element_joint_cutter");

    const std::shared_ptr<BeamVariable> beam = BeamVariable::between(
        Polyline::rectangle({0.0, -100.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 200.0, 300.0),
        Polyline::rectangle({1200.0, -100.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, 200.0, 300.0),
        "beam"
    );
    scene.add(beam);

    // a notch: a closed box subtracted from the beam
    const Polyline notch_loop = Polyline::rectangle({500.0, -150.0, 200.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 200.0, 300.0);
    const Mesh box = Mesh::loft({notch_loop}, {notch_loop.translated({0.0, 0.0, 200.0})});
    const std::shared_ptr<Joint> notch = std::make_shared<Joint>(box, SolidOperation::subtract);
    notch->name = "notch";
    notch->is_visible = false;
    scene.add(notch);
    scene.add_interaction(notch, beam, notch->interaction(0));

    // a trim: a closed profile in the xz plane, extruded across the beam, keeps its inside
    const Polyline arch({{-10.0, -150.0, 300.0}, {-10.0, -150.0, 0.0}, {300.0, -150.0, 0.0}, {600.0, -150.0, 120.0}, {900.0, -150.0, 0.0}, {1210.0, -150.0, 0.0}, {1210.0, -150.0, 300.0}, {-10.0, -150.0, 300.0}});
    const std::shared_ptr<Joint> trim = std::make_shared<Joint>(std::vector<Polyline>{arch}, Vector(0.0, 300.0, 0.0));
    trim->name = "trim";
    trim->is_visible = false;
    scene.add(trim);
    scene.add_interaction(trim, beam, trim->interaction(0));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Joints that cut by a solid: a box taken away from a beam, and a profile it is trimmed to.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_cutter --parallel 6 && ./build/element_joint_cutter && ../bash/publish-scene.sh --target element_joint_cutter

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
