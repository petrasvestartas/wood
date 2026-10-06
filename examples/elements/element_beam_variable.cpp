#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// The variable beam element: an axis with a section at each end, and two sections alone.
int main() {

    WoodSession scene("element_beam_variable");

    const Polyline start = Polyline::rectangle(Point(0.0, -60.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 120.0, 300.0);
    const Polyline end = Polyline::rectangle(Point(2000.0, -60.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0), 120.0, 120.0);
    scene.add(std::make_shared<BeamVariable>(Line::from_points(Point(0.0, 0.0, 0.0), Point(2000.0, 0.0, 0.0)), std::vector<Polyline>{start, end}, "axis_and_sections"));
    scene.add(BeamVariable::between(start.translated(Vector(0.0, 600.0, 0.0)), end.translated(Vector(0.0, 600.0, 0.0)), "between"));

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The variable beam element, two ways: an axis with a 120 x 300 section at its start tapering to 120 x 120 at its end, and BeamVariable::between, the same two sections without an axis.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_beam_variable --parallel 6 && ./build/element_beam_variable && ../bash/publish-scene.sh --target element_beam_variable

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
